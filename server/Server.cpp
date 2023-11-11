#include "Server.hpp"
#include <unistd.h>
#include <sys/select.h>
#include <cstdlib>
#include <cstring>

Server::Server() : m_serverSocket(0), m_clientSocket(0), m_maxSocket(0)
{
    // 서버 소켓 생성
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 서버 주소 설정
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(12345); // 포트 번호
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;

    bindSocket();
}

Server::~Server()
{
    close(m_clientSocket);
    close(m_serverSocket);
}

void Server::bindSocket()
{
    // 바인딩
    bind(m_serverSocket, (struct sockaddr *)&m_serverAddr, sizeof(m_serverAddr));
}

void Server::listenForClients()
{
    // 리스닝
    listen(m_serverSocket, 5);
    std::cout << "서버 대기 중...\n";
}

void Server::acceptClient()
{
    // 클라이언트 연결 수락
    m_clientSocket = accept(m_serverSocket, nullptr, nullptr);
    std::cout << "클라이언트가 연결되었습니다.\n";

    // 새로운 클라이언트를 파일 디스크립터 세트에 추가
    FD_SET(m_clientSocket, &m_masterSet);

    // 최대 소켓 갱신
    if (m_clientSocket > m_maxSocket)
    {
        m_maxSocket = m_clientSocket;
    }
}

void Server::sendFile(const std::string& filename)
{
    // 파일 열기
    std::ifstream fileToSend(filename, std::ios::binary);

    // 파일 전송
    char buffer[1024];
    int bytesRead;

    while (true)
    {
        bytesRead = fileToSend.readsome(buffer, sizeof(buffer));

        if (bytesRead > 0)
        {
            // 파일 데이터를 클라이언트에 전송
            send(m_clientSocket, buffer, bytesRead, 0);
        }
        else
        {
            // 파일 전송이 완료되면 반복문 종료
            break;
        }
    }

    fileToSend.close();
    std::cout << "파일 전송 완료.\n";
}

void Server::start() {
    listenForClients();

    // 파일 이름 입력 받기
    std::string filename;
    std::cout << "전송할 파일의 이름을 입력하세요: ";
    std::cin >> filename;

    acceptClient();
    sendFile(filename);
}