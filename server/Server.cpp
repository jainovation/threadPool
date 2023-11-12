#include "Server.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/select.h>
#include <queue>
#include <condition_variable>
#include <thread>

Server::Server() : m_dataServerSocket(0), m_cmdServerSocket(0), m_dataClientSocket(0), m_cmdClientSocket(0), m_stopFlag(false)
{
    // 데이터 채널 소켓 생성
    m_dataServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 명령 채널 소켓 생성
    m_cmdServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 데이터 채널 서버 주소 설정
    memset(&m_dataServerAddr, 0, sizeof(m_dataServerAddr));
    m_dataServerAddr.sin_family = AF_INET;
    m_dataServerAddr.sin_port = htons(12345); // 데이터 채널 포트 번호
    m_dataServerAddr.sin_addr.s_addr = INADDR_ANY;

    // 명령 채널 서버 주소 설정
    memset(&m_cmdServerAddr, 0, sizeof(m_cmdServerAddr));
    m_cmdServerAddr.sin_family = AF_INET;
    m_cmdServerAddr.sin_port = htons(12346); // 명령 채널 포트 번호
    m_cmdServerAddr.sin_addr.s_addr = INADDR_ANY;

    bindSocket();
}

Server::~Server()
{
    close(m_dataServerSocket);
    close(m_cmdServerSocket);
    close(m_dataClientSocket);
    close(m_cmdClientSocket);
}

void Server::bindSocket()
{
    // 데이터 채널 소켓 바인딩
    bind(m_dataServerSocket, (struct sockaddr*)&m_dataServerAddr, sizeof(m_dataServerAddr));
    listen(m_dataServerSocket, 5);
    std::cout << "데이터 클라이언트 연결중.\n";

    // 명령 채널 소켓 바인딩
    bind(m_cmdServerSocket, (struct sockaddr*)&m_cmdServerAddr, sizeof(m_cmdServerAddr));
    listen(m_cmdServerSocket, 5);
    std::cout << "명령 클라이언트 연결중.\n";
}

void Server::acceptDataClient()
{
    // 데이터 클라이언트 연결 대기
    m_dataClientSocket = accept(m_dataServerSocket, nullptr, nullptr);
    std::cout << "데이터 클라이언트가 연결되었습니다.\n";

    // 데이터 쓰레드 풀 시작
    for (int i = 0; i < THREAD_POOL_SIZE; ++i)
    {
        m_dataThreadpool.emplace_back(&Server::handleDataChannel, this);
    }
}

void Server::acceptCmdClient()
{
    // 명령 클라이언트 연결 대기
    m_cmdClientSocket = accept(m_cmdServerSocket, nullptr, nullptr);
    std::cout << "명령 클라이언트가 연결되었습니다.\n";

    // 명령 쓰레드 시작
    std::thread cmdThread(&Server::handleCmdChannel, this);
    cmdThread.detach(); // 쓰레드를 백그라운드로 돌리기
}

void Server::fileInputThread()
{
    std::string filename;

    while(true)
    {
        std::cout << "전송할 파일의 이름을 등록하세요: ";
        std::cin >> filename;

        // 파일 전송 예약
        enqueueFileTransfer(filename);
    }
}

void Server::handleCmdChannel()
{
    char cmdBuffer[256];

    while (true)
    {
        memset(cmdBuffer, 0, sizeof(cmdBuffer));

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_cmdClientSocket, &readSet);

        // 타임아웃 설정 (여기서는 1초로 설정)
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // select 함수를 사용하여 명령 채널에서 읽을 준비가 되었는지 확인
        int result = select(m_cmdClientSocket + 1, &readSet, nullptr, nullptr, &timeout);

        if (result > 0 && FD_ISSET(m_cmdClientSocket, &readSet))
        {
            // 명령 수신
            int bytesRead = recv(m_cmdClientSocket, cmdBuffer, sizeof(cmdBuffer), 0);

            if (bytesRead > 0)
            {
                std::cout << "명령 수신: " << cmdBuffer << "\n";
                // 명령 처리
                processCommand(cmdBuffer);
            }
        }

        // 스케줄러 호출 (큐에 있는 파일 전송 확인 및 처리)
        // scheduleFileTransfer();
    }
}

void Server::scheduleFileTransfer()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    if (!m_fileTransferQueue.empty())
    {
        // 큐에 있는 파일 전송 처리
        std::string filename = m_fileTransferQueue.front();
        m_fileTransferQueue.pop();

        // 예약 파일 전송
        reserveFileTransfer(filename);
    }
    else
    {
        std::cout << "예약된 작업없음\n";
    }
}

void Server::reserveFileTransfer(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(m_reserveMutex);
    m_fileTransferReservation.push(filename);
    m_conditionVariable.notify_one();
}

void Server::handleDataChannel()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_reserveMutex);
        m_conditionVariable.wait(lock, [&]
                                 { return !m_fileTransferReservation.empty(); });

        std::string filename = m_fileTransferReservation.front();
        m_fileTransferReservation.pop();

        lock.unlock();

        // 파일 전송
        sendDataFile(filename);
    }
}

void Server::sendDataFile(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);

    // 파일 열기
    std::ifstream fileToSend(filename, std::ios::binary);

    if (!fileToSend.is_open())
    {
        std::cerr << "파일을 열 수 없습니다.\n";
        return;
    }

    // 파일 이름 전송
    send(m_dataClientSocket, filename.c_str(), filename.size(), 0);
    std::cout << filename <<"파일 전송 시작.\n";

    // 파일 전송
    char buffer[1024];
    int bytesRead;

    while (true)
    {
        bytesRead = fileToSend.readsome(buffer, sizeof(buffer));

        if (bytesRead > 0)
        {
            // 파일 데이터를 데이터 클라이언트에 전송
            send(m_dataClientSocket, buffer, bytesRead, 0);
        }
        else
        {
            // 파일 전송이 완료되면 반복문 종료
            std::cout << filename << "파일 전송 완료.\n";
            break;
        }
    }

    fileToSend.close();
}

void Server::processCommand(const std::string &command)
{
    // 명령어 분리 로직 추가
    if (command == "list")
    {
        // "list" 명령 수신 시 예약된 파일 목록 확인
        listReservedFiles();
    }
    else if (command == "start")
    {
        // "start" 명령 수신 시 파일 전송
        scheduleFileTransfer();
    }
}

void Server::enqueueFileTransfer(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_fileTransferQueue.push(filename);
    std::cout << "예약 완료\n";
}

void Server::listReservedFiles()
{
    std::lock_guard<std::mutex> lock(m_reserveMutex);
    std::cout << "예약된 파일 목록:\n";

    std::queue<std::string> tempQueue = m_fileTransferQueue;
    while (!tempQueue.empty())
    {
        std::cout << tempQueue.front() << "\n";
        tempQueue.pop();
    }
}

void Server::start()
{
    // 데이터 클라이언트 연결 수락
    acceptDataClient();

    // 명령 클라이언트 연결 수락
    acceptCmdClient();

    // 입력 쓰레드 시작
    std::thread fileInputThread(&Server::fileInputThread, this);
    fileInputThread.detach(); // 쓰레드를 백그라운드로 돌리기
}

void Server::stop()
{
    m_stopFlag = true;

    // 종료 처리
    for (auto &thread : m_dataThreadpool)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

