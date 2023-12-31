#include "Client.hpp"
#include <unistd.h>
#include <sys/select.h>
#include <cstring>

Client::Client() : m_clientSocket(0)
{
    // 클라이언트 소켓 생성
    m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 서버 주소 설정
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(12345); // 서버와 동일한 포트 번호
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;

    connectToServer();
}

Client::~Client()
{
    close(m_clientSocket);
}

void Client::connectToServer()
{
    // 서버에 연결
    connect(m_clientSocket, (struct sockaddr *)&m_serverAddr, sizeof(m_serverAddr));
    std::cout << "서버에 연결되었습니다.\n";
}

void Client::receiveFile()
{
    // 파일 수신
    char buffer[1024];
    std::ofstream receivedFile("received_file.txt", std::ios::binary);
    int bytesRead;

    while (true)
    {
        bytesRead = recv(m_clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead > 0)
        {
            // 파일에 데이터 쓰기
            receivedFile.write(buffer, bytesRead);
        }
        else
        {
            // 파일 수신이 완료되면 반복문 종료
            break;
        }
    }

    receivedFile.close();
    std::cout << "파일 수신 완료.\n";
}

void Client::start()
{
    receiveFile();
    // sendFile();
}