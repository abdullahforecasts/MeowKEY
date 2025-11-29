#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <stdexcept>

class SocketUtils {
public:
    static int createSocket() {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        
        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set socket options");
        }
        
        return sockfd;
    }

    static void bindSocket(int sockfd, int port) {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        if (bind(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }
    }

    static void listenSocket(int sockfd, int backlog = 10) {
        if (listen(sockfd, backlog) < 0) {
            throw std::runtime_error("Failed to listen on socket");
        }
    }

    static int acceptConnection(int sockfd) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_sockfd = accept(sockfd, (sockaddr*)&client_addr, &client_len);
        if (client_sockfd < 0) {
            throw std::runtime_error("Failed to accept connection");
        }
        
        return client_sockfd;
    }

    static void connectToServer(int sockfd, const std::string& host, int port) {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address");
        }
        
        if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw std::runtime_error("Failed to connect to server");
        }
    }

    static bool sendData(int sockfd, const std::string& data) {
        size_t total_sent = 0;
        while (total_sent < data.size()) {
            ssize_t sent = send(sockfd, data.c_str() + total_sent, 
                               data.size() - total_sent, 0);
            if (sent <= 0) {
                return false;
            }
            total_sent += sent;
        }
        return true;
    }

    static std::string receiveData(int sockfd, size_t max_size = 1024) {
        char buffer[max_size];
        ssize_t received = recv(sockfd, buffer, max_size - 1, 0);
        if (received <= 0) {
            return "";
        }
        buffer[received] = '\0';
        return std::string(buffer, received);
    }
};

#endif