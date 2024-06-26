#include <iostream> //input/ output streams
#include <winsock2.h> //Network communications on Windows
#include <ws2tcpip.h> // Additional structures for network applications
#include <string>
#include <fstream> // file input-output
#include <thread> // multi-threading 
#include <mutex> // mutual exclusions
#include <sstream> // string stream operations
#include <cctype> // character handling

#pragma comment(lib, "Ws2_32.lib") //Link with Windows socket library
#define PORT "8080" // defines the server port
#define MAX_CLIENTS 100 //maximum number of clients

std::mutex connections_mutex; //mutex to synchronize access to shared data

struct Client { //group several structures under one name
    SOCKET socket;
    std::string name;
    bool active;
};

Client clients[MAX_CLIENTS] = {};

std::string trim(const std::string& str) { // trims whitespaces from both end of the string, cleans up strings before processing used within different functions
    size_t first = str.find_first_not_of(" \n\r\t\f\v");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \n\r\t\f\v");
    return str.substr(first, (last - first + 1));
}

void broadcast_message(const std::string& message, int exclude_index) { // enable public message distribution within the network by sending the message to all active clients except for the sender through mutual exclusion.
    std::lock_guard<std::mutex> lock(connections_mutex); 
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && i != exclude_index) {
            send(clients[i].socket, message.c_str(), message.length(), 0); 
        }
    }
}

std::string caesarEncrypt(const std::string& text, int shift) { // caesar cipher encryption
    std::string encrypted = "";
    for (char c : text) {
        if (isalpha(c)) {
            char base = islower(c) ? 'a' : 'A';
            encrypted += static_cast<char>(((c - base + shift) % 26) + base);
        } else {
            encrypted += c;
        }
    }
    return encrypted;
}

std::string caesarDecrypt(const std::string& text, int shift) { // caesar cipher decryption
    return caesarEncrypt(text, 26 - shift); 
}

bool register_user(const std::string& name, const std::string& email, const std::string& password) { //matching the entered password to the one stored on the database by decrypting the registered value and comparing it to the inputted value. Performing proper authentication of users and welcoming the user by their saved name, they are requested to enter their name, email address & password 
    std::ifstream check("users.txt");
    std::string line;
    while (getline(check, line)) {
        std::istringstream iss(line);
        std::string storedName, storedEmail, storedPassword;
        getline(iss, storedName, '|');
        getline(iss, storedEmail, '|');
        if (storedEmail == email) { //checks if the user already exists
            check.close();
            return false; 
        }
    }
    check.close();

    std::ofstream file("users.txt", std::ios::app);
    if (!file.is_open()) return false;

    std::string encryptedPassword = caesarEncrypt(password, 3); //encrypts the password upon storage 
    file << name << "|" << email << "|" << encryptedPassword << "\n";
    file.close();
    return true;
}

bool validate_credentials(const std::string& email, const std::string& password, std::string& name) { // matching the entered password to the one stored on the database by decrypting the registered value and comparing it to the inputted value. Performing proper authentication of users and welcoming the user by their saved name. 
    std::ifstream file("users.txt");
    std::string line;
    while (getline(file, line)) {
        std::istringstream iss(line);
        std::string storedName, storedEmail, storedPassword;
        getline(iss, storedName, '|');
        getline(iss, storedEmail, '|');
        getline(iss, storedPassword);
        if (storedEmail == email && caesarDecrypt(storedPassword, 3) == password) { 
            name = storedName;
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

void handle_client(int clientIndex) { //Merging both the user creation and user login into a singular function, this is where this function comes into play, after authenticating the user it maintains an active session and processes message reception and broadcasting.
    SOCKET client_socket = clients[clientIndex].socket;
    std::cout << "Client connected: " << clientIndex << std::endl;
    std::cout << clients[clientIndex].name << " is connected " << std::endl;
    char buffer[1024] = {0};

    send(client_socket, "Welcome to Rita's ChatAPP! Enter 1 for NEW user, 2 for RETURNING user:", 70, 0);
    recv(client_socket, buffer, sizeof(buffer), 0);
    int choice = buffer[0] - '0';

    std::string name, email, password;
    switch (choice) {
        case 1:  // New user registration
            send(client_socket, "Enter your Name:", 17, 0);
            memset(buffer, 0, sizeof(buffer));  
            recv(client_socket, buffer, sizeof(buffer), 0);
            name = trim(buffer);

            send(client_socket, "Enter your Email:", 18, 0);
            memset(buffer, 0, sizeof(buffer));  
            recv(client_socket, buffer, sizeof(buffer), 0);
            email = trim(buffer);

            send(client_socket, "Enter your Password:", 20, 0);
            memset(buffer, 0, sizeof(buffer));  
            recv(client_socket, buffer, sizeof(buffer), 0);
            password = trim(buffer);

            if (register_user(name, email, password)) {
                send(client_socket, ("Welcome " + name).c_str(), ("Welcome " + name).length(), 0);
            } else {
                send(client_socket, "User already exists or error in registration.", 46, 0);
                closesocket(client_socket);
                return;
            }
            break;
        case 2:  // User login
            send(client_socket, "Enter your Email:", 18, 0);
            memset(buffer, 0, sizeof(buffer));  
            recv(client_socket, buffer, sizeof(buffer), 0);
            email = trim(buffer);

            send(client_socket, "Enter your Password:", 20, 0);
            memset(buffer, 0, sizeof(buffer));  
            recv(client_socket, buffer, sizeof(buffer), 0);
            password = trim(buffer);

            if (validate_credentials(email, password, name)) {
                send(client_socket, ("Login successful. Welcome " + name + "!").c_str(), 35 + name.length(), 0);
            } else {
                send(client_socket, "Authentication failed. Please try again.", 41, 0);
                closesocket(client_socket);
                return;
            }
            break;
    }

    clients[clientIndex].name = name;
    clients[clientIndex].active = true;

    while (true) { // message reception and broadcasting
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cout << "Client disconnected: " << name << std::endl;
            break;
        }
        // Logging encrypted data  
        std::cout << "Encrypted: ";
        for (int i = 0; i < bytes_received; ++i) {
            printf("%02X", (unsigned char)buffer[i]);  // Print each byte as a hex value
        }
        std::cout << std::endl;

        std::string message = clients[clientIndex].name + ": " + std::string(buffer, bytes_received);

        broadcast_message(message, clientIndex);
    }

    // Mark the client as inactive and close the socket
    clients[clientIndex].active = false;
    closesocket(client_socket);
    std::cout << "Closed connection with client: " << clients[clientIndex].name << std::endl;
}

int main() {     // initializes the server, sets up the connections through sockets and enters a loop to accept client connections.
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