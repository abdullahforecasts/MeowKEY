#include "client_cross.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

void printUsage() {
    std::cout << "Usage: ./client <server_ip> <server_port> <client_id>" << std::endl;
    std::cout << "Example: ./client 192.168.1.100 8080 bscs24009" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage();
        return 1;
    }

    std::string server_ip = argv[1];
    int port = std::stoi(argv[2]);
    std::string client_id = argv[3];

    try {
        Client client(server_ip, port, client_id);
        
        if (!client.start()) {
            std::cerr << "Failed to start client" << std::endl;
            return 1;
        }
        
        std::cout << "Client running. Press Enter to stop..." << std::endl;
        std::cin.get();
        client.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}