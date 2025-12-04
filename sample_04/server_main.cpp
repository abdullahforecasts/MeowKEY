#include "server_cross.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

void printUsage() {
    std::cout << "Usage: ./server <port>" << std::endl;
    std::cout << "Example: ./server 8080" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage();
        return 1;
    }

    int port = std::stoi(argv[1]);

    try {
        Server server(port);
        
        if (!server.start()) {
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }
        
        std::cout << "Server running. Press Enter to stop..." << std::endl;
        std::cin.get();
        server.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}