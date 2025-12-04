#ifndef CLIENT_CROSS_HPP
#define CLIENT_CROSS_HPP

#include "platform_detector.hpp"
#include "socket_utils_cross.hpp"
#include "thread_safe_queue.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>

#ifdef PLATFORM_WINDOWS
    #include "keyboard_capture_windows.hpp"
    #include "clipboard_monitor_windows.hpp"
    #include "window_monitor_windows.hpp"
    typedef KeyboardCaptureWindows KeyboardCapture;
    typedef ClipboardMonitorWindows ClipboardMonitor;
    typedef WindowMonitorWindows WindowMonitor;
#else
    #include "keyboard_capture.hpp"
    #include "clipboard_monitor.hpp"
    #include "window_monitor.hpp"
#endif

class Client {
private:
    std::string server_host_;
    int server_port_;
    std::string client_id_;
    std::atomic<bool> running_;
    std::thread network_thread_;
    ThreadSafeQueue<std::string> event_queue_;
    std::unique_ptr<KeyboardCapture> keyboard_capture_;
    std::unique_ptr<ClipboardMonitor> clipboard_monitor_;
    std::unique_ptr<WindowMonitor> window_monitor_;
    bool enable_clipboard_monitoring_;
    bool enable_window_monitoring_;

public:
    Client(const std::string& host, int port, const std::string& client_id) 
        : server_host_(host), server_port_(port), client_id_(client_id),
          running_(false), enable_clipboard_monitoring_(false), 
          enable_window_monitoring_(false) {}

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
            }
        }

        // Initialize window monitor if consented
        if (enable_window_monitoring_) {
            try {
                window_monitor_ = std::make_unique<WindowMonitor>(
                    [this](const std::string& window_info) {
                        event_queue_.push(window_info);
                    }
                );
            } catch (const std::exception& e) {
                std::cerr << "Failed to initialize window monitor: " << e.what() << std::endl;
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
        }

        // Start window monitoring if enabled
        if (window_monitor_ && !window_monitor_->start()) {
            std::cerr << "Failed to start window monitoring" << std::endl;
        }

        std::cout << "Client ID: " << client_id_ << std::endl;
        std::cout << "Keyboard device: " << keyboard_capture_->getKeyboardDevice() << std::endl;
        if (enable_clipboard_monitoring_) {
            std::cout << "Clipboard monitoring: ENABLED" << std::endl;
        }
        if (enable_window_monitoring_) {
            std::cout << "Window monitoring: ENABLED" << std::endl;
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
        if (window_monitor_) {
            window_monitor_->stop();
        }
        if (network_thread_.joinable()) {
            network_thread_.join();
        }
        SocketUtils::cleanupWSA();
    }

private:
    bool getUserConsent() {
        std::cout << "=== INPUT MONITORING CLIENT ===" << std::endl;
        std::cout << "Client ID: " << client_id_ << std::endl;
        std::cout << "This application will capture:" << std::endl;
        std::cout << "1. ALL keystrokes on your system" << std::endl;
        std::cout << "2. Clipboard content when it changes" << std::endl;
        std::cout << "3. Active window/tab information when you switch" << std::endl;
        std::cout << "and send them to the server at " << server_host_ << ":" << server_port_ << std::endl;
        std::cout << std::endl;
        std::cout << "WARNING: This may capture sensitive information!" << std::endl;
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
            std::cout << "CLIPBOARD WARNING: This will capture ALL copied text including passwords!" << std::endl;
            std::cout << "Are you sure? (yes/no): ";
            std::getline(std::cin, response);
            enable_clipboard_monitoring_ = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        }
        
        std::cout << "Do you also give permission for window/tab monitoring? (yes/no): ";
        std::getline(std::cin, response);
        enable_window_monitoring_ = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        
        return true;
    }

    void networkLoop() {
        while (running_) {
            try {
                int sockfd = SocketUtils::createSocket();
                SocketUtils::connectToServer(sockfd, server_host_, server_port_);
                
                // Send client ID first
                std::string hello_msg = "CLIENT_ID: " + client_id_ + "\n";
                SocketUtils::sendData(sockfd, hello_msg);
                
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

//