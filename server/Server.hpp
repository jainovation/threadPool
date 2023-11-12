#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>

#define THREAD_POOL_SIZE 8

class Server
{
public:
    Server();
    ~Server();

    void start();
    void stop();
    void enqueueFileTransfer(const std::string& filename);


private:
    int m_dataServerSocket;
    int m_cmdServerSocket;
    int m_dataClientSocket;
    int m_cmdClientSocket;
    sockaddr_in m_dataServerAddr;
    sockaddr_in m_cmdServerAddr;

    std::mutex m_queueMutex;
    std::mutex m_reserveMutex;
    std::mutex m_dataMutex;
    std::condition_variable m_conditionVariable;
    std::queue<std::string> m_fileTransferReservation;
    std::queue<std::string> m_fileTransferQueue;
    std::vector<std::thread> m_dataThreadpool;

    bool m_stopFlag;

    void bindSocket();
    void listenForClients();
    void acceptDataClient();
    void acceptCmdClient();
    void handleCmdChannel();
    void scheduleFileTransfer();
    void reserveFileTransfer(const std::string& filename);
    void handleDataChannel();
    void sendDataFile(const std::string& filename);
    void processCommand(const std::string& command);
    void fileInputThread();
    void listReservedFiles();
};

#endif // SERVER_H