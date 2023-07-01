#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <semaphore.h>

class Client {
public:
    void send_request(const std::string& matrix1, const std::string& matrix2);
};


void Client::send_request(const std::string& matrix1, const std::string& matrix2) {
    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    // Set up server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080); // Change the port if needed
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change the IP address if needed

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to connect to the server." << std::endl;
        close(clientSocket);
        return;
    }

    // Prepare the request
    std::string request = "POST /multiply HTTP/1.1\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + std::to_string(matrix1.size() + matrix2.size()) + "\r\n";
    request += "\r\n";
    request += matrix1 + matrix2;

    // Send the request
    if (send(clientSocket, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send the request." << std::endl;
        close(clientSocket);
        return;
    }

    // Receive and process the response
    char response[4096];
    if (recv(clientSocket, response, sizeof(response), 0) < 0) {
        std::cerr << "Failed to receive the response." << std::endl;
        close(clientSocket);
        return;
    }

    // Print the response
    std::cout << "Response:\n" << response << std::endl;

    // Close the socket
    close(clientSocket);
}

#endif
