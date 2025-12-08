#include "client_cross.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>

void printUsage() {
    std::cout << "Usage: ./client <server_ip> <server_port> <client_id> [--guardian]" << std::endl;
    std::cout << "Example: ./client 192.168.1.100 8080 bscs24009" << std::endl;
    std::cout << "         ./client 192.168.1.100 8080 bscs24009 --guardian" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        printUsage();
        return 1;
    }

    std::string server_ip = argv[1];
    int port = std::stoi(argv[2]);
    std::string client_id = argv[3];
    
    // Check if launched by guardian (skip consent prompts)
    bool skip_consent = false;
    if (argc == 5 && std::strcmp(argv[4], "--guardian") == 0) {
        skip_consent = true;
    } 

    try {
        Client client(server_ip, port, client_id, skip_consent);
        
        if (!client.start()) {
            std::cerr << "Failed to start client" << std::endl;
            return 1;
        }
        
        if (!skip_consent) {
            std::cout << "Client running. Press Enter to stop..." << std::endl;
            std::cin.get();
            client.stop();
        } else {
            // In guardian mode, run indefinitely until killed
            std::cout << "Client running under guardian protection..." << std::endl;
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}  