#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>

class Client
{
public:
    Client();
    ~Client();

    void start();

private:
    int m_clientSocket;
    sockaddr_in m_serverAddr;

    void connectToServer();
    void sendFile();
    void receiveFile();
};

#endif // CLIENT_H