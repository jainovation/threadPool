#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>

class Server
{
public:
    Server();
    ~Server();

    void start();

private:
    int m_serverSocket;
    int m_clientSocket;
    sockaddr_in m_serverAddr;
    fd_set m_masterSet;
    int m_maxSocket;

    void bindSocket();
    void listenForClients();
    void acceptClient();
    void receiveFile();
    void sendFile();
};

#endif // SERVER_H