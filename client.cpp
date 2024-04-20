#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include "Crypto.h"  // Include the Crypto class header

#pragma comment(lib, "Ws2_32.lib")
#define SERVER_IP "127.0.0.1"
#define PORT "8080"

Crypto myCrypto;  // Instance of the Crypto class for handling encryption

void receive_messages(SOCKET sock) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            // Decrypt the received message
            std::string decryptedMessage = myCrypto.aesDecrypt(reinterpret_cast<unsigned char*>(buffer), bytesReceived);
            std::cout << "Decrypted message: " << decryptedMessage << std::endl;
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
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);  // Server IP address
    serverAddr.sin_port = htons(std::stoi(PORT));  // Port
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Unable to connect to server!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::thread recvThread(receive_messages, sock);
    recvThread.detach();

    // Initialize encryption
    myCrypto.init();  // Initialize your cryptographic settings

    std::string input;
    std::cout << "Enter messages: " << std::endl;
    while (getline(std::cin, input)) {
        // Encrypt the message before sending
        std::string encryptedMessage = myCrypto.aesEncrypt(reinterpret_cast<const unsigned char*>(input.c_str()), input.length());
        send(sock, encryptedMessage.c_str(), encryptedMessage.length(), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
