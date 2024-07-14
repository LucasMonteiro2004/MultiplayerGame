#include "Server.h"

Server::Server() : resultList(nullptr), listenSocket(INVALID_SOCKET) {
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
}

Server::~Server() {
    shutdown();
}

bool Server::initialize() {
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup falhou: " << result << std::endl;
        return false;
    }

    result = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &resultList);
    if (result != 0) {
        std::cerr << "getaddrinfo falhou: " << result << std::endl;
        WSACleanup();
        return false;
    }

    listenSocket = socket(resultList->ai_family, resultList->ai_socktype, resultList->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Erro ao criar socket: " << WSAGetLastError() << std::endl;
        freeaddrinfo(resultList);
        WSACleanup();
        return false;
    }

    result = bind(listenSocket, resultList->ai_addr, (int)resultList->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cerr << "bind falhou: " << WSAGetLastError() << std::endl;
        freeaddrinfo(resultList);
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    freeaddrinfo(resultList);

    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        std::cerr << "listen falhou: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return false;
    }

    std::cout << "Servidor esperando por conexoes na porta " << DEFAULT_PORT << "...\n";
    return true;
}

void Server::start() {
    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept falhou: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        std::lock_guard<std::mutex> lock(clientMutex);
        if (clientSockets.size() < MAX_CLIENTS) {
            int clientId = clientSockets.size() + 1;
            clientSockets.push_back({clientSocket, clientId});
            std::thread clientThread(&Server::handleClient, this, clientSocket, clientId);
            clientThread.detach();
        } else {
            std::cerr << "Máximo de clientes atingido. Cliente em espera.\n";
            send(clientSocket, "Servidor cheio. Por favor, espere.\n", DEFAULT_BUFLEN, 0);
            closesocket(clientSocket);
        }
    }
}

void Server::handleClient(SOCKET clientSocket, int clientId) {
    std::cout << "Cliente " << clientId << " conectado ao servidor.\n";
    send(clientSocket, ("Cliente " + std::to_string(clientId) + " conectado.\n").c_str(), DEFAULT_BUFLEN, 0);

    char buffer[DEFAULT_BUFLEN];
    int result;
    do {
        result = recv(clientSocket, buffer, DEFAULT_BUFLEN, 0);
        if (result > 0) {
            buffer[result] = '\0';
            //std::cout << "Mensagem do cliente " << clientId << ": " << buffer << std::endl;

            // Interpretar a mensagem recebida como novas coordenadas x e y
            int new_pos_x, new_pos_y;
            sscanf_s(buffer, "%d %d", &new_pos_x, &new_pos_y);

            // Atualizar as coordenadas do cliente no vetor clientSockets
            {
                std::lock_guard<std::mutex> lock(clientMutex);
                auto it = std::find_if(clientSockets.begin(), clientSockets.end(),
                    [clientSocket](const ClientInfo& info) { return info.socket == clientSocket; });

                if (it != clientSockets.end()) {
                    it->pos_x = new_pos_x;
                    it->pos_y = new_pos_y;
                }
            }

            // Medir o tempo de envio da mensagem
            auto start = std::chrono::steady_clock::now();

            // Broadcast da nova posição para todos os clientes conectados
            broadcastMessage(std::to_string(clientId) + " " + std::to_string(new_pos_x) + " " + std::to_string(new_pos_y), clientId);

            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << "Tempo para enviar mensagem: " << duration.count() << " microseconds" << std::endl;
        } else if (result == 0) {
            std::cout << "Cliente " << clientId << " desconectado.\n";
        } else {
            std::cerr << "Erro ao receber dados do cliente " << clientId << ".\n";
        }
    } while (result > 0);

    // Fechar o socket do cliente
    closesocket(clientSocket);

    // Remover cliente da lista de clientes conectados
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        auto it = std::remove_if(clientSockets.begin(), clientSockets.end(),
            [clientSocket](const ClientInfo& info) { return info.socket == clientSocket; });

        clientSockets.erase(it, clientSockets.end());
    }

    std::cout << "Cliente " << clientId << " desconectado.\n";
}

void Server::broadcastMessage(const std::string& message, int senderClientId) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (auto& client : clientSockets) {
        if (client.socket != INVALID_SOCKET && client.clientId != senderClientId) {
            send(client.socket, message.c_str(), static_cast<int>(message.size()), 0);
        }
    }
}

void Server::shutdown() {
    closesocket(listenSocket);

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (const auto& client : clientSockets) {
            closesocket(client.socket);
        }
        clientSockets.clear();
    }

    WSACleanup();
}

