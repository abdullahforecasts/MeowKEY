#ifndef LAUNCHER_COMMON_HPP
#define LAUNCHER_COMMON_HPP

#include <string>
#include <atomic>
#include <iostream>
#include <fstream>

// Hardcoded password - CHANGE THIS before distributing!
// For production, read from encrypted config or environment variable
const std::string GUARDIAN_PASSWORD = "abd101";

class LauncherConfig {
public:
    std::string client_executable;
    std::string server_ip;
    int server_port;
    std::string client_id;
    std::atomic<bool> exit_authorized{false};
    
    LauncherConfig(const std::string& exe, const std::string& ip, 
                   int port, const std::string& id)
        : client_executable(exe), server_ip(ip), 
          server_port(port), client_id(id) {}
};

inline bool verifyPassword(const std::string& input) {
    return input == GUARDIAN_PASSWORD;
}

inline void showUnauthorizedMessage() {
    std::cout << "\n❌ UNAUTHORIZED EXIT ATTEMPT DENIED!" << std::endl;
    std::cout << "The monitoring session continues..." << std::endl;
}

#endif



