#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")
#define PORT "8080"
#define MAX_CLIENTS 100

std::mutex connections_mutex;

struct Client {
    SOCKET socket;
    std::string name;
    bool active;
};

Client clients[MAX_CLIENTS] = {};  // Initialize all clients as inactive

void broadcast_message(const std::string& message, const std::string& senderName) {
    std::lock_guard<std::mutex> lock(connections_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && clients[i].name != senderName) {
            send(clients[i].socket, message.c_str(), message.length(), 0);
        }
    }
}

void handle_client(int clientIndex) {
    SOCKET client_socket = clients[clientIndex].socket;
    std::string clientName = clients[clientIndex].name;
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cout << "Client disconnected: " << clientName << std::endl;
            break;
        }
        std::string message = clientName + ": " + std::string(buffer, bytes_received);
        std::cout << message << std::endl;

        // Broadcast message to other clients
        broadcast_message(message, clientName);
    }

    // Clean up connection
    std::lock_guard<std::mutex> lock(connections_mutex);
    clients[clientIndex].active = false;
    closesocket(client_socket);
    std::cout << "Closed connection with client: " << clientName << std::endl;
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(std::stoi(PORT));
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started. Waiting for client connections..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::lock_guard<std::mutex> lock(connections_mutex);
        bool slot_found = false;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].socket = clientSocket;
                char buffer[1024] = {0};
                recv(clientSocket, buffer, sizeof(buffer), 0);  // Read client name
                clients[i].name = std::string(buffer);
                clients[i].active = true;
                std::thread clientThread(handle_client, i);
                clientThread.detach();
                slot_found = true;
                break;
            }
        }
        if (!slot_found) {
            std::cerr << "Server full. Connection refused." << std::endl;
            closesocket(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
