#ifndef SOCKET_UTILS_CROSS_HPP
#define SOCKET_UTILS_CROSS_HPP

#include "platform_detector.hpp"

#ifdef PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include <string>
#include <cstring>
#include <stdexcept>
#include <iostream>
using namespace std; 

class SocketUtils {
private:
    static bool wsa_initialized_;

public:
    static bool initializeWSA() {
#ifdef PLATFORM_WINDOWS
        if (!wsa_initialized_) {
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (result != 0) {
                  cerr << "WSAStartup failed: " << result <<   endl;
                return false;
            }
            wsa_initialized_ = true;
        }
#endif
        return true;
    }

    static void cleanupWSA() {
#ifdef PLATFORM_WINDOWS
        if (wsa_initialized_) {
            WSACleanup();
            wsa_initialized_ = false;
        }
#endif
    }

    static int createSocket() {
        initializeWSA();
        
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw   runtime_error("Failed to create socket");
        }
        
        int opt = 1;
#ifdef PLATFORM_WINDOWS
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
            throw   runtime_error("Failed to set socket options");
        }
        
        return sockfd;
    }

    static void bindSocket(int sockfd, int port) {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        if (bind(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw   runtime_error("Failed to bind socket");
        }
    }

    static void listenSocket(int sockfd, int backlog = 10) {
        if (listen(sockfd, backlog) < 0) {
            throw   runtime_error("Failed to listen on socket");
        }
    }

    static int acceptConnection(int sockfd) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_sockfd = accept(sockfd, (sockaddr*)&client_addr, &client_len);
        if (client_sockfd < 0) {
            throw   runtime_error("Failed to accept connection");
        }
        
        return client_sockfd;
    }

    static void connectToServer(int sockfd, const   string& host, int port) {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            throw   runtime_error("Invalid address");
        }
        
        if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw   runtime_error("Failed to connect to server");
        }
    }

    static bool sendData(int sockfd, const   string& data) {
        size_t total_sent = 0;
        while (total_sent < data.size()) {
#ifdef PLATFORM_WINDOWS
            int sent = send(sockfd, data.c_str() + total_sent, 
                           (int)(data.size() - total_sent), 0);
#else
            ssize_t sent = send(sockfd, data.c_str() + total_sent, 
                               data.size() - total_sent, 0);
#endif
            if (sent <= 0) {
                return false;
            }
            total_sent += sent;
        }
        return true;
    }

    static   string receiveData(int sockfd, size_t max_size = 1024) {
        char buffer[1024];
#ifdef PLATFORM_WINDOWS
        int received = recv(sockfd, buffer, (int)(max_size - 1), 0);
#else
        ssize_t received = recv(sockfd, buffer, max_size - 1, 0);
#endif
        if (received <= 0) {
            return "";
        }
        buffer[received] = '\0';
        return   string(buffer, received);
    }
};

bool SocketUtils::wsa_initialized_ = false;

#endif