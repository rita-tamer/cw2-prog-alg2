#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")
#define PORT "8080"

void receive_messages(SOCKET sock) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::cout << buffer << std::endl;
        } else {
            std::cout << "Server disconnected." << std::endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(std::stoi(PORT));
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Unable to connect to server!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::thread recvThread(receive_messages, sock);
    recvThread.detach();

    std::string input;
    // Get user choice and credentials
    getline(std::cin, input);
    send(sock, input.c_str(), input.length(), 0);

    while (getline(std::cin, input)) {
        send(sock, input.c_str(), input.length(), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
