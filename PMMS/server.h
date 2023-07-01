#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <thread>
#include <semaphore.h>
#include <mutex>

class Server {
public:
    void handle_client(int client_socket);
    void start();
};

void Server::handle_client(int client_socket) {
    // Receive the request
    std::string request;
    char buffer[4096];
    int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesReceived < 0) {
        std::cerr << "Failed to read request." << std::endl;
        close(client_socket);
        return;
    }
    request.append(buffer, bytesReceived);

    int num_workers = 4;
    std::string num_workers_str = request.substr(request.find("num_workers=") + 12);
    
    // Initialize task semaphore
    sem_t task_sem;

    // Initialize mutex
    std::mutex mtx;

    // Extract the two matrices from the request
    std::string matrix1 = request.substr(request.find("matrix1=") + 8);
    std::string matrix2 = request.substr(request.find("matrix2=") + 8);

    // Multiply the matrices by worker processes
    std::string result;
    for (int i = 0; i < matrix1.size(); i += 2) {
        int row = matrix1[i] - '0';
        int col = matrix1[i + 1] - '0';
        int sum = 0;
        for (int j = 0; j < matrix2.size(); j += 2) {
            if (matrix2[j] - '0' == col) {
                sum += (matrix2[j + 1] - '0') * (matrix2[j + 1] - '0');
            }
        }
        result += std::to_string(row) + std::to_string(sum);
    }

    // Send the HTTP response to the client
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: " + std::to_string(matrix1.size() + matrix2.size()) + "\r\n";
    response += "\r\n";
    response += matrix1 + matrix2;
    if (send(client_socket, response.c_str(), response.size(), 0) < 0) {
        std::cerr << "Failed to send response." << std::endl;
        close(client_socket);
        return;
    }
    
    // Close the client socket
    close(client_socket);

    // Print the request
    std::cout << "Request:\n" << request << std::endl;
}

void Server::start() {
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    // Set up server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080); // Change the port if needed
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind the socket." << std::endl;
        close(serverSocket);
        return;
    }

    // Start listening for connections
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Failed to listen for connections." << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Server started. Listening on port 8080..." << std::endl;

    while (true) {
        // Accept a client connection
        struct sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        int client_socket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (client_socket < 0) {
            std::cerr << "Failed to accept a client connection." << std::endl;
            continue;
        }

        // Create a new thread to handle the client
        std::thread thread(&Server::handle_client, this, client_socket);
        thread.detach();
    }

    // Close the server socket
    close(serverSocket);
}

#endif
