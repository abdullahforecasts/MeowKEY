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
using namespace std; 
  string getPasswordInput() {
      string password;
    
    // Disable echo
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // Read password
      getline(  cin, password);
    
    // Restore echo
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    return password;
}

  vector<pid_t> findAllProcesses() {
      vector<pid_t> pids;
    
    // Find launcher process
      string cmd_launcher = "pgrep -f './launcher' 2>/dev/null";
    FILE* pipe = popen(cmd_launcher.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            pids.push_back(  stoi(buffer));
        }
        pclose(pipe);
    }
    
    // Find client process
      string cmd_client = "pgrep -f './client.*--guardian' 2>/dev/null";
    pipe = popen(cmd_client.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            pids.push_back(  stoi(buffer));
        }
        pclose(pipe);
    }
    
    return pids;
}

int main(int argc, char* argv[]) {
      cout << "\n╔════════════════════════════════════════╗" <<   endl;
      cout << "║     GUARDIAN EXIT TOOL                ║" <<   endl;
      cout << "╚════════════════════════════════════════╝\n" <<   endl;
    
    // Find all related processes
      vector<pid_t> pids = findAllProcesses();
    
    if (pids.empty()) {
          cerr << "❌ No launcher or client processes found!" <<   endl;
          cerr << "   Make sure the protected client is running." <<   endl;
        return 1;
    }
    
      cout << "✓ Found " << pids.size() << " process(es) to terminate:" <<   endl;
    for (pid_t pid : pids) {
          cout << "  - PID: " << pid <<   endl;
    }
    
      cout << "\n🔐 Enter password to stop monitoring: " <<   flush;
    
      string password = getPasswordInput();
      cout <<   endl;
    
    // Hardcoded password check (CHANGE THIS to match launcher_common.hpp!)
    if (password == "abd101") {
          cout << "\n✅ Password correct - Sending termination signal..." <<   endl;
        
        int killed_count = 0;
        int failed_count = 0;
        
        for (pid_t pid : pids) {
            if (kill(pid, SIGKILL) == 0) {
                  cout << "✅ Terminated process PID: " << pid <<   endl;
                killed_count++;
            } else {
                  cerr << "⚠️  Failed to terminate PID " << pid << ": " << strerror(errno) <<   endl;
                failed_count++;
            }
        }
        
          cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" <<   endl;
          cout << "✅ Successfully killed: " << killed_count << " process(es)" <<   endl;
        if (failed_count > 0) {
              cout << "⚠️  Failed to kill: " << failed_count << " process(es)" <<   endl;
              cout << "   Try running with: sudo ./guardian_exit" <<   endl;
        }
          cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" <<   endl;
        
        return (failed_count > 0) ? 1 : 0;
    } else {
          cout << "\n❌ INCORRECT PASSWORD!" <<   endl;
          cout << "   Exit denied. Monitoring continues..." <<   endl;
        return 1;
    }
}