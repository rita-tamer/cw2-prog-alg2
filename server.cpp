#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")
#define PORT "8080"
#define MAX_CLIENTS 100

std::mutex connections_mutex;

struct Client {
    SOCKET socket;
    std::string name;
    bool active;
};

Client clients[MAX_CLIENTS] = {};

void broadcast_message(const std::string& message, int exclude_index) {
    std::lock_guard<std::mutex> lock(connections_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && i != exclude_index) {
            send(clients[i].socket, message.c_str(), message.length(), 0);
        }
    }
}

bool register_user(const std::string& name, const std::string& email, const std::string& password) {
    std::ifstream check("users.txt");
    std::string line;
    while (getline(check, line)) {
        std::istringstream iss(line);
        std::string storedName, storedEmail, storedPassword;
        iss >> storedName >> storedEmail >> storedPassword;
        if (storedEmail == email) {
            check.close();
            return false; // User already exists
        }
    }
    check.close();

    std::ofstream file("users.txt", std::ios::app);
    if (!file.is_open()) return false;
    file << name << " " << email << " " << password << "\n";
    file.close();
    return true;
}

bool validate_credentials(const std::string& email, const std::string& password, std::string& name) {
    std::ifstream file("users.txt");
    std::string line;
    while (getline(file, line)) {
        std::istringstream iss(line);
        std::string storedName, storedEmail, storedPassword;
        iss >> storedName >> storedEmail >> storedPassword;
        if (storedEmail == email && storedPassword == password) {
            name = storedName; // Set the name for personalized greeting
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

void handle_client(int clientIndex) {
    SOCKET client_socket = clients[clientIndex].socket;
    char buffer[1024] = {0};
    std::string name;

    send(client_socket, "Welcome to Rita's ChatAPP! Enter 1 for NEW user, 2 for RETURNING user:", 70, 0);
    recv(client_socket, buffer, sizeof(buffer), 0);
    int choice = buffer[0] - '0';

    if (choice == 1) { // New user
        send(client_socket, "Enter your Name:", 17, 0);
        recv(client_socket, buffer, sizeof(buffer), 0);
        name = buffer;

        send(client_socket, "Enter your Email:", 18, 0);
        recv(client_socket, buffer, sizeof(buffer), 0);
        std::string email(buffer);

        send(client_socket, "Enter your Password:", 20, 0);
        recv(client_socket, buffer, sizeof(buffer), 0);
        std::string password(buffer);

        if (register_user(name, email, password)) {
            send(client_socket, ("Welcome " + name).c_str(), ("Welcome " + name).length(), 0);
        } else {
            send(client_socket, "User already exists or error in registration.", 46, 0);
            closesocket(client_socket);
            return;
        }
    } else if (choice == 2) { // Returning user
        send(client_socket, "Enter your Email:", 18, 0);
        recv(client_socket, buffer, sizeof(buffer), 0);
        std::string email(buffer);

        send(client_socket, "Enter your Password:", 20, 0);
        recv(client_socket, buffer, sizeof(buffer), 0);
        std::string password(buffer);

        if (validate_credentials(email, password, name)) {
            send(client_socket, ("Login successful. Welcome " + name + "!").c_str(), 35 + name.length(), 0);
        } else {
            send(client_socket, "Authentication failed. Please try again.", 41, 0);
            closesocket(client_socket);
            return;
        }
    }

    clients[clientIndex].name = name;
    clients[clientIndex].active = true;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cout << "Client disconnected: " << name << std::endl;
            break;
        }
        std::string message = name + ": " + std::string(buffer, bytes_received);
        std::cout << message << std::endl;
        broadcast_message(message, clientIndex);
    }

    clients[clientIndex].active = false;
    closesocket(client_socket);
    std::cout << "Closed connection with client: " << name << std::endl;
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

    std::cout << "Server started. Waiting for client connections...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            continue;
        }

        bool slot_found = false;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].socket = clientSocket;
                std::thread clientThread(handle_client, i);
                clientThread.detach();
                slot_found = true;
                break;
            }
        }
        if (!slot_found) {
            std::cerr << "Server full. Connection refused.\n";
            closesocket(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
