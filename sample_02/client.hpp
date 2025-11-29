// #ifndef CLIENT_HPP
// #define CLIENT_HPP

// #include "keyboard_capture.hpp"
// #include "socket_utils.hpp"
// #include "thread_safe_queue.hpp"
// #include <thread>
// #include <atomic>
// #include <iostream>

// class Client {
// private:
//     std::string server_host_;
//     int server_port_;
//     std::atomic<bool> running_;
//     std::thread network_thread_;
//     ThreadSafeQueue<std::string> keystroke_queue_;
//     std::unique_ptr<KeyboardCapture> keyboard_capture_;

// public:
//     Client(const std::string& host, int port) 
//         : server_host_(host), server_port_(port), running_(false) {}

//     ~Client() {
//         stop();
//     }

//     bool start() {
//         if (running_) {
//             return false;
//         }

//         running_ = true;
        
//         // Initialize keyboard capture
//         keyboard_capture_ = std::make_unique<KeyboardCapture>(
//             [this](const std::string& key) {
//                 keystroke_queue_.push(key);
//             }
//         );

//         // Get user consent
//         if (!getUserConsent()) {
//             std::cout << "Consent not granted. Exiting." << std::endl;
//             return false;
//         }

//         // Start keyboard capture
//         if (!keyboard_capture_->start()) {
//             std::cerr << "Failed to start keyboard capture" << std::endl;
//             return false;
//         }

//         // Start network thread
//         network_thread_ = std::thread(&Client::networkLoop, this);

//         std::cout << "Client started. Capturing global keystrokes..." << std::endl;
//         return true;
//     }

//     void stop() {
//         running_ = false;
//         if (keyboard_capture_) {
//             keyboard_capture_->stop();
//         }
//         if (network_thread_.joinable()) {
//             network_thread_.join();
//         }
//     }

// private:
//     bool getUserConsent() {
//         std::cout << "=== KEYBOARD MONITORING CLIENT ===" << std::endl;
//         std::cout << "This application will capture ALL keystrokes on your system," << std::endl;
//         std::cout << "including those typed in other applications, and send them" << std::endl;
//         std::cout << "to the server at " << server_host_ << ":" << server_port_ << std::endl;
//         std::cout << std::endl;
//         std::cout << "Do you give explicit permission for this monitoring? (yes/no): ";
        
//         std::string response;
//         std::getline(std::cin, response);
        
//         return (response == "yes" || response == "y" || response == "YES" || response == "Y");
//     }

//     void networkLoop() {
//         while (running_) {
//             try {
//                 int sockfd = SocketUtils::createSocket();
//                 SocketUtils::connectToServer(sockfd, server_host_, server_port_);
                
//                 std::cout << "Connected to server" << std::endl;

//                 while (running_) {
//                     auto key_opt = keystroke_queue_.pop();
//                     if (key_opt) {
//                         std::string message = *key_opt + "\n";
//                         if (!SocketUtils::sendData(sockfd, message)) {
//                             std::cerr << "Failed to send data. Reconnecting..." << std::endl;
//                             break;
//                         }
//                     }
                    
//                     // Small delay to prevent busy waiting
//                     std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                 }

//                 close(sockfd);
//             } catch (const std::exception& e) {
//                 std::cerr << "Connection error: " << e.what() << ". Retrying in 5 seconds..." << std::endl;
//                 std::this_thread::sleep_for(std::chrono::seconds(5));
//             }
//         }
//     }
// };

// #endif
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "keyboard_capture.hpp"
#include "clipboard_monitor.hpp"  // Include clipboard monitor
#include "socket_utils.hpp"
#include "thread_safe_queue.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>

class Client {
private:
    std::string server_host_;
    int server_port_;
    std::atomic<bool> running_;
    std::thread network_thread_;
    ThreadSafeQueue<std::string> event_queue_;  // Renamed from keystroke_queue_
    std::unique_ptr<KeyboardCapture> keyboard_capture_;
    std::unique_ptr<ClipboardMonitor> clipboard_monitor_;  // Clipboard monitor
    bool enable_clipboard_monitoring_;  // Track clipboard consent

public:
    Client(const std::string& host, int port) 
        : server_host_(host), server_port_(port), running_(false), 
          enable_clipboard_monitoring_(false) {}

    ~Client() {
        stop();
    }

    bool start() {
        if (running_) {
            return false;
        }

        running_ = true;
        
        // Get user consent
        if (!getUserConsent()) {
            std::cout << "Consent not granted. Exiting." << std::endl;
            return false;
        }

        // Initialize keyboard capture
        try {
            keyboard_capture_ = std::make_unique<KeyboardCapture>(
                [this](const std::string& key) {
                    event_queue_.push("KEY: " + key);
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize keyboard capture: " << e.what() << std::endl;
            return false;
        }

        // Initialize clipboard monitor if consented
        if (enable_clipboard_monitoring_) {
            try {
                clipboard_monitor_ = std::make_unique<ClipboardMonitor>(
                    [this](const std::string& content) {
                        event_queue_.push(content);
                    }
                );
            } catch (const std::exception& e) {
                std::cerr << "Failed to initialize clipboard monitor: " << e.what() << std::endl;
                // Continue without clipboard monitoring
            }
        }

        // Start keyboard capture
        if (!keyboard_capture_->start()) {
            std::cerr << "Failed to start keyboard capture" << std::endl;
            return false;
        }

        // Start clipboard monitoring if enabled
        if (clipboard_monitor_ && !clipboard_monitor_->start()) {
            std::cerr << "Failed to start clipboard monitoring" << std::endl;
            // Continue without clipboard monitoring
        }

        std::cout << "Keyboard device: " << keyboard_capture_->getKeyboardDevice() << std::endl;
        if (enable_clipboard_monitoring_) {
            std::cout << "Clipboard monitoring: ENABLED" << std::endl;
        }

        // Start network thread
        network_thread_ = std::thread(&Client::networkLoop, this);

        std::cout << "Client started. Capturing input events..." << std::endl;
        return true;
    }

    void stop() {
        running_ = false;
        if (keyboard_capture_) {
            keyboard_capture_->stop();
        }
        if (clipboard_monitor_) {
            clipboard_monitor_->stop();
        }
        if (network_thread_.joinable()) {
            network_thread_.join();
        }
    }

private:
    bool getUserConsent() {
        std::cout << "=== INPUT MONITORING CLIENT ===" << std::endl;
        std::cout << "This application will capture:" << std::endl;
        std::cout << "1. ALL keystrokes on your system" << std::endl;
        std::cout << "2. Clipboard content when it changes" << std::endl;
        std::cout << "and send them to the server at " << server_host_ << ":" << server_port_ << std::endl;
        std::cout << std::endl;
        std::cout << "WARNING: Clipboard monitoring may capture sensitive data like passwords!" << std::endl;
        std::cout << std::endl;
        
        std::string response;
        std::cout << "Do you give permission for keyboard monitoring? (yes/no): ";
        std::getline(std::cin, response);
        bool keyboard_consent = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        
        if (!keyboard_consent) {
            return false;
        }
        
        std::cout << "Do you also give permission for clipboard monitoring? (yes/no): ";
        std::getline(std::cin, response);
        enable_clipboard_monitoring_ = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        
        if (enable_clipboard_monitoring_) {
            std::cout << "CLIPBOARD WARNING: This will capture ALL copied text including passwords and sensitive data!" << std::endl;
            std::cout << "Are you sure? (yes/no): ";
            std::getline(std::cin, response);
            enable_clipboard_monitoring_ = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        }
        
        return true;
    }

    void networkLoop() {
        while (running_) {
            try {
                int sockfd = SocketUtils::createSocket();
                SocketUtils::connectToServer(sockfd, server_host_, server_port_);
                
                std::cout << "Connected to server successfully!" << std::endl;

                while (running_) {
                    auto event_opt = event_queue_.pop();
                    if (event_opt) {
                        std::string message = *event_opt + "\n";
                        if (!SocketUtils::sendData(sockfd, message)) {
                            std::cerr << "Failed to send data. Reconnecting..." << std::endl;
                            break;
                        }
                    }
                    
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