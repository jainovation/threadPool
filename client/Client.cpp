#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/select.h>

#define THREAD_POOL_SIZE 8

Client::Client() : m_dataClientSocket(0), m_cmdClientSocket(0)
{
    // 데이터 채널 소켓 생성
    m_dataClientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 명령 채널 소켓 생성
    m_cmdClientSocket = socket(AF_INET, SOCK_STREAM, 0);

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

    connectToServer();
}

Client::~Client()
{
    close(m_dataClientSocket);
    close(m_cmdClientSocket);
}

void Client::connectToServer()
{
    // 데이터 채널 서버에 연결
    connect(m_dataClientSocket, (struct sockaddr *)&m_dataServerAddr, sizeof(m_dataServerAddr));
    std::cout << "데이터 서버에 연결되었습니다.\n";
    // 명령 채널 서버에 연결
    connect(m_cmdClientSocket, (struct sockaddr *)&m_cmdServerAddr, sizeof(m_cmdServerAddr));
    std::cout << "명령 서버에 연결되었습니다.\n";
}

void Client::receiveFile()
{
    std::lock_guard<std::mutex> lock(m_dataMutex);

    // 파일 이름 수신
    char filenameBuffer[256];
    recv(m_dataClientSocket, filenameBuffer, sizeof(filenameBuffer), 0);
    std::string filename(filenameBuffer);

    // 파일 수신
    char buffer[1024];
    std::ofstream receivedFile(filename, std::ios::binary);
    int bytesRead;

    while (true)
    {
        bytesRead = recv(m_dataClientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead > 0)
        {
            // 파일에 데이터 쓰기
            receivedFile.write(buffer, bytesRead);
        }
        else
        {
            // 파일 수신이 완료되면 반복문 종료
            std::cout << "파일 수신 완료.\n";
            break;
        }
    }

    receivedFile.close();
}

void Client::handleCommandChannel()
{
    // char commandBuffer[256];
    // while (true)
    // {
    //     memset(commandBuffer, 0, sizeof(commandBuffer));

    //     // 명령 채널 소켓을 감시
    //     FD_ZERO(&m_fds);
    //     FD_SET(m_cmdClientSocket, &m_fds);

    //     struct timeval timeout;
    //     timeout.tv_sec = 1;
    //     timeout.tv_usec = 0;

    //     int result = select(m_cmdClientSocket + 1, &m_fds, nullptr, nullptr, &timeout);

    //     if (result > 0 && FD_ISSET(m_cmdClientSocket, &m_fds))
    //     {
    //         // 명령 수신
    //         recv(m_cmdClientSocket, commandBuffer, sizeof(commandBuffer), 0);
    //         std::string command(commandBuffer);

    //         // 명령어 처리 로직 추가
    //         std::cout << "수신된 명령: " << command << "\n";
    //     }
    // }
}

void Client::handleSendChannel()
{
    std::string command;

    while (true)
    {
        // 명령 전송 (예: start)
        std::cout << "전송할 명령 입력: ";
        std::cin >> command;
        sendCommand(command);
    }
}

void Client::startCommandThread()
{
    // 명령 채널을 처리할 별도의 쓰레드 시작
    // m_commandThread = std::thread(&Client::handleCommandChannel, this);
}

void Client::startSendThread()
{
    // 데이터 채널에 입력을 보낼 별도의 쓰레드 시작
    m_sendThread = std::thread(&Client::handleSendChannel, this);
}

void Client::requestFile(const std::string &filename)
{
    // 파일 요청을 데이터 채널 서버에게 전송
    send(m_dataClientSocket, filename.c_str(), filename.size(), 0);
}

void Client::sendCommand(const std::string &command)
{
    // 명령을 명령 채널 서버에게 전송
    send(m_cmdClientSocket, command.c_str(), command.size(), 0);
}

void Client::handleReceiveChannel()
{
    while(true)
    {
        receiveFile();
    }
}

void Client::startReceiveThread()
{
    // 여러 개의 수신 쓰레드를 시작
    for (int i = 0; i < THREAD_POOL_SIZE; ++i)
    {
        m_receiveThreadPool.emplace_back(&Client::handleReceiveChannel, this);
    }
}

void Client::start()
{
    // startCommandThread();

    startSendThread();

    startReceiveThread();

    // 파일 수신
    // receiveFile();

    // m_commandThread.join();
    m_sendThread.join();
}

void Client::stop()
{
    // 종료 처리
    for (auto &thread : m_receiveThreadPool)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}
