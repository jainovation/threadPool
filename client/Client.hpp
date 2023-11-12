#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>

class Client
{
public:
    Client();
    ~Client();

    void start();
    void stop();
    void requestFile(const std::string& filename);

private:
    int m_dataClientSocket;
    int m_cmdClientSocket;
    sockaddr_in m_dataServerAddr;
    sockaddr_in m_cmdServerAddr;
    std::thread m_commandThread;
    std::thread m_sendThread;
    std::vector<std::thread> m_receiveThreadPool;
    fd_set m_fds;
    std::mutex m_dataMutex;

    void connectToServer();
    void receiveFile();
    void sendCommand(const std::string& command);
    void handleSendChannel();
    void handleCommandChannel();
    void startCommandThread();
    void startSendThread();
    void handleReceiveChannel();
    void startReceiveThread();
};

#endif // CLIENT_H