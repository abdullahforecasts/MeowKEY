#ifndef LAUNCHER_COMMON_HPP
#define LAUNCHER_COMMON_HPP

#include <string>
#include <atomic>
#include <iostream>
#include <fstream>
using namespace std; 
// Hardcoded password - CHANGE THIS before distributing!
// For production, read from encrypted config or environment variable
const   string GUARDIAN_PASSWORD = "abd101";

class LauncherConfig {
public:
      string client_executable;
      string server_ip;
    int server_port;
      string client_id;
      atomic<bool> exit_authorized{false};
    
    LauncherConfig(const   string& exe, const   string& ip, 
                   int port, const   string& id)
        : client_executable(exe), server_ip(ip), 
          server_port(port), client_id(id) {}
};

inline bool verifyPassword(const   string& input) {
    return input == GUARDIAN_PASSWORD;
}

inline void showUnauthorizedMessage() {
      cout << "\n❌ UNAUTHORIZED EXIT ATTEMPT DENIED!" <<   endl;
      cout << "The monitoring session continues..." <<   endl;
}

#endif



