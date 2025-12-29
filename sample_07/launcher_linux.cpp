#include "platform_detector.hpp"

#ifdef PLATFORM_LINUX

#include "launcher_common.hpp"
#include "process_guard_linux.hpp"
#include <iostream>
#include <string>
using namespace std; 
void printUsage() {
      cout << "═══════════════════════════════════════════════════════" <<   endl;
      cout << "     PROTECTED CLIENT LAUNCHER (Linux)                " <<   endl;
      cout << "═══════════════════════════════════════════════════════" <<   endl;
      cout << "Usage: ./launcher <server_ip> <port> <client_id>" <<   endl;
      cout << "Example: ./launcher 192.168.1.50 8080 bscs24009" <<   endl;
      cout << "\nFeatures:" <<   endl;
      cout << "  ✓ Exit protection (password required)" <<   endl;
      cout << "  ✓ Ctrl+C interception" <<   endl;
      cout << "  ✓ Signal handler protection" <<   endl;
      cout << "  ✓ Terminal close protection" <<   endl;
      cout << "═══════════════════════════════════════════════════════" <<   endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage();
        return 1;
    }

      string server_ip = argv[1];
      string port = argv[2];
      string client_id = argv[3];

      cout << "\n🚀 PROTECTED CLIENT LAUNCHER" <<   endl;
      cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" <<   endl;
      cout << "Target Server: " << server_ip << ":" << port <<   endl;
      cout << "Client ID: " << client_id <<   endl;
      cout << "Protection: ACTIVE 🛡️\n" <<   endl;

    // Assume client binary is in the same directory
      string client_exe = "./client";
      string client_args = server_ip + " " + port + " " + client_id + " --guardian";

    try {
        LauncherConfig config(client_exe, server_ip,   stoi(port), client_id);
        ProcessGuardLinux guard(config.exit_authorized);

        if (!guard.launchClient(client_exe, client_args)) {
              cerr << "❌ Failed to launch protected client" <<   endl;
            return 1;
        }

        // Monitor the client process
        guard.monitorProcess();

          cout << "\n✅ Guardian shutdown complete" <<   endl;

    } catch (const   exception& e) {
          cerr << "❌ Error: " << e.what() <<   endl;
        return 1;
    }

    return 0;
}

#endif // PLATFORM_LINUX