#include "platform_detector.hpp"

#ifdef PLATFORM_LINUX

#include "launcher_common.hpp"
#include "process_guard_linux.hpp"
#include <iostream>
#include <string>

void printUsage() {
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
    std::cout << "     PROTECTED CLIENT LAUNCHER (Linux)                " << std::endl;
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
    std::cout << "Usage: ./launcher <server_ip> <port> <client_id>" << std::endl;
    std::cout << "Example: ./launcher 192.168.1.50 8080 bscs24009" << std::endl;
    std::cout << "\nFeatures:" << std::endl;
    std::cout << "  ✓ Exit protection (password required)" << std::endl;
    std::cout << "  ✓ Ctrl+C interception" << std::endl;
    std::cout << "  ✓ Signal handler protection" << std::endl;
    std::cout << "  ✓ Terminal close protection" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage();
        return 1;
    }

    std::string server_ip = argv[1];
    std::string port = argv[2];
    std::string client_id = argv[3];

    std::cout << "\n🚀 PROTECTED CLIENT LAUNCHER" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    std::cout << "Target Server: " << server_ip << ":" << port << std::endl;
    std::cout << "Client ID: " << client_id << std::endl;
    std::cout << "Protection: ACTIVE 🛡️\n" << std::endl;

    // Assume client binary is in the same directory
    std::string client_exe = "./client";
    std::string client_args = server_ip + " " + port + " " + client_id + " --guardian";

    try {
        LauncherConfig config(client_exe, server_ip, std::stoi(port), client_id);
        ProcessGuardLinux guard(config.exit_authorized);

        if (!guard.launchClient(client_exe, client_args)) {
            std::cerr << "❌ Failed to launch protected client" << std::endl;
            return 1;
        }

        // Monitor the client process
        guard.monitorProcess();

        std::cout << "\n✅ Guardian shutdown complete" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

#endif // PLATFORM_LINUX