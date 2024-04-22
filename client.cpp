#include <iostream> //input/ output streams
#include <winsock2.h> //Network communications on Windows
#include <ws2tcpip.h> // Additional structures for network applications
#include <string>
#include <thread> // multi-threading 
#include "Crypto.h" // cryptographic interface header

#pragma comment(lib, "Ws2_32.lib")
#define SERVER_IP "127.0.0.1"
#define PORT "8080"

Crypto myCrypto;  // Instance of the Crypto class for handling encryption

void receive_messages(SOCKET sock) { // Listens for and processes incoming messages from the server, ensuring that the client receives real-time updates during chat sessions, decrypts the received message using AES back into plaintext  through a calculated key based on the value of the length of the message
    char buffer[1024]; //buffer to store received data
    while (true) { // loop to continuously receive messages
        memset(buffer, 0, sizeof(buffer)); //clears the buffer
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0); //receives the data from the server
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
    WSADATA wsaData;  // Structure to store Windows Socket implementation details
    WSAStartup(MAKEWORD(2, 2), &wsaData);  // Initializes Winsock DLL
    // Creates a new socket for the IPv4 protocol and stream-based communication
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;  // Structure to hold server address information
    serverAddr.sin_family = AF_INET;  // Sets the address family to IPv4
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);  // Server IP address
    serverAddr.sin_port = htons(std::stoi(PORT));  // Port
    // Attempts to connect to the specified server
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Unable to connect to server!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::thread recvThread(receive_messages, sock); // Starts a new thread to receive messages
    recvThread.detach(); // Detaches the thread

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
