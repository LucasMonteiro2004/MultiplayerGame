#include "NetworkClient.h"

NetworkClient::NetworkClient(const char* serverAddress, const char* port)
    : resultList(nullptr), ptr(nullptr), connectSocket(INVALID_SOCKET),
      serverAddress(serverAddress), port(port), connected(false) {
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
}

NetworkClient::~NetworkClient() {
    disconnect();
}

bool NetworkClient::initialize() {
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup falhou: " << result << std::endl;
        return false;
    }

    result = getaddrinfo(serverAddress, port, &hints, &resultList);
    if (result != 0) {
        std::cerr << "getaddrinfo falhou: " << result << std::endl;
        WSACleanup();
        return false;
    }
    return true;
}

bool NetworkClient::connectToServer() {
    for (ptr = resultList; ptr != nullptr; ptr = ptr->ai_next) {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            std::cerr << "Erro ao criar socket: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        int result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (result == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(resultList);

    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Não foi possível conectar ao servidor.\n";
        WSACleanup();
        return false;
    }

    connected = true;

    // Inicia a thread para recebimento de dados
    receiveThread = std::thread(&NetworkClient::receiveThreadFunction, this);

    return true;
}

void NetworkClient::disconnect() {
    if (connected) {
        connected = false;
        stopReceiveThread();
        closesocket(connectSocket);
        WSACleanup();
    }
}

bool NetworkClient::sendMessage(const char* message) {
    std::lock_guard<std::mutex> lock(sendMutex);
    int result = send(connectSocket, message, (int)strlen(message), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Falha ao enviar mensagem: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

void NetworkClient::receiveData() {
    // Implementação vazia porque agora a leitura é feita em uma thread separada
}

void NetworkClient::receiveThreadFunction() {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    while (connected) {
        int result = recv(connectSocket, recvbuf, recvbuflen, 0);
        if (result > 0) {
            recvbuf[result] = '\0';
            std::cout << "Mensagem do servidor: " << recvbuf << std::endl;

            std::istringstream iss(recvbuf);
            int clientId;
            int newPosX, newPosY;

            iss >> clientId >> newPosX >> newPosY;

            std::lock_guard<std::mutex> lock(playersMutex);
            if (players.find(clientId) == players.end()) {
                // Adiciona um novo jogador com uma cor aleatória
                PlayerInfo newPlayer;
                newPlayer.pos_x = newPosX;
                newPlayer.pos_y = newPosY;
                newPlayer.color[0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                newPlayer.color[1] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                newPlayer.color[2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                players[clientId] = newPlayer;
            } else {
                // Atualiza a posição do jogador existente
                players[clientId].pos_x = newPosX;
                players[clientId].pos_y = newPosY;
            }
        } else if (result == 0) {
            std::cout << "Conexão fechada pelo servidor\n";
            break;
        } else {
            std::cerr << "recv falhou: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Se sair do loop, desconecta o cliente
    disconnect();
}


void NetworkClient::stopReceiveThread() {
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

void NetworkClient::updatePlayerPosition(int playerId, int x, int y) {
    std::lock_guard<std::mutex> lock(playersMutex);
    if (players.find(playerId) != players.end()) {
        players[playerId].pos_x = x;
        players[playerId].pos_y = y;
    }
}