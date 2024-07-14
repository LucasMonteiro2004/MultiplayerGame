#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <sstream> 
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "2323"
#define DEFAULT_BUFLEN 256
#define SQUARE_SIZE 32 // Tamanho do quadrado em pixels

class NetworkClient {
public:
    struct PlayerInfo {
        int pos_x;
        int pos_y;
        int id = 0;
        float color[3];
    };

    int pos_x = 0, pos_y = 0, mapWidth = 800, mapHeight = 600;
    std::mutex sendMutex;
    NetworkClient(const char* serverAddress = "127.0.0.1", const char* port = DEFAULT_PORT);
    ~NetworkClient();

    bool initialize();
    bool connectToServer();
    void disconnect();
    bool sendMessage(const char* message);
    void receiveData();
    void updatePlayerPosition(int playerId, int x, int y);

    std::unordered_map<int, PlayerInfo> players;
    std::mutex playersMutex;

private:
    WSADATA wsaData;
    struct addrinfo *resultList, *ptr, hints;
    SOCKET connectSocket;
    const char* serverAddress;
    const char* port;
    bool connected;
    std::thread receiveThread;

    void receiveThreadFunction();
    void stopReceiveThread();
};

#endif // NETWORK_CLIENT_H
