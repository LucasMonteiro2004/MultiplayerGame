#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <algorithm>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "2323"
#define MAX_CLIENTS 2
#define DEFAULT_BUFLEN 256

class Server {
public:
    Server();
    ~Server();

    bool initialize();
    void start();
    void shutdown();

private:
    struct ClientInfo {
        SOCKET socket;
        int clientId;
        int pos_x;
        int pos_y;
    };

    void handleClient(SOCKET clientSocket, int clientId);
    void broadcastMessage(const std::string& message, int senderClientId);

    WSADATA wsaData;
    struct addrinfo *resultList, hints;
    SOCKET listenSocket;
    std::vector<ClientInfo> clientSockets; // Armazena informações de clientes
    std::mutex clientMutex;
};

#endif // SERVER_H
