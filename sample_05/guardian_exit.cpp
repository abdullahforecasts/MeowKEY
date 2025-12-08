#include <iostream>
#include <string>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <vector>

std::string getPasswordInput() {
    std::string password;
    
    // Disable echo
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // Read password
    std::getline(std::cin, password);
    
    // Restore echo
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    return password;
}

std::vector<pid_t> findAllProcesses() {
    std::vector<pid_t> pids;
    
    // Find launcher process
    std::string cmd_launcher = "pgrep -f './launcher' 2>/dev/null";
    FILE* pipe = popen(cmd_launcher.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            pids.push_back(std::stoi(buffer));
        }
        pclose(pipe);
    }
    
    // Find client process
    std::string cmd_client = "pgrep -f './client.*--guardian' 2>/dev/null";
    pipe = popen(cmd_client.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            pids.push_back(std::stoi(buffer));
        }
        pclose(pipe);
    }
    
    return pids;
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║     GUARDIAN EXIT TOOL                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝\n" << std::endl;
    
    // Find all related processes
    std::vector<pid_t> pids = findAllProcesses();
    
    if (pids.empty()) {
        std::cerr << "❌ No launcher or client processes found!" << std::endl;
        std::cerr << "   Make sure the protected client is running." << std::endl;
        return 1;
    }
    
    std::cout << "✓ Found " << pids.size() << " process(es) to terminate:" << std::endl;
    for (pid_t pid : pids) {
        std::cout << "  - PID: " << pid << std::endl;
    }
    
    std::cout << "\n🔐 Enter password to stop monitoring: " << std::flush;
    
    std::string password = getPasswordInput();
    std::cout << std::endl;
    
    // Hardcoded password check (CHANGE THIS to match launcher_common.hpp!)
    if (password == "abd101") {
        std::cout << "\n✅ Password correct - Sending termination signal..." << std::endl;
        
        int killed_count = 0;
        int failed_count = 0;
        
        for (pid_t pid : pids) {
            if (kill(pid, SIGKILL) == 0) {
                std::cout << "✅ Terminated process PID: " << pid << std::endl;
                killed_count++;
            } else {
                std::cerr << "⚠️  Failed to terminate PID " << pid << ": " << strerror(errno) << std::endl;
                failed_count++;
            }
        }
        
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "✅ Successfully killed: " << killed_count << " process(es)" << std::endl;
        if (failed_count > 0) {
            std::cout << "⚠️  Failed to kill: " << failed_count << " process(es)" << std::endl;
            std::cout << "   Try running with: sudo ./guardian_exit" << std::endl;
        }
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
        
        return (failed_count > 0) ? 1 : 0;
    } else {
        std::cout << "\n❌ INCORRECT PASSWORD!" << std::endl;
        std::cout << "   Exit denied. Monitoring continues..." << std::endl;
        return 1;
    }
}