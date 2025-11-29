#include "server.hpp"
#include "client.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

void printUsage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  Server: ./keyboard_monitor server <port>" << std::endl;
    std::cout << "  Client: ./keyboard_monitor client <server_ip> <server_port>" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  Server: ./keyboard_monitor server 8080" << std::endl;
    std::cout << "  Client: ./keyboard_monitor client 192.168.1.100 8080" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string mode = argv[1];

    try {
        if (mode == "server") {
            if (argc != 3) {
                printUsage();
                return 1;
            }
            
            int port = std::stoi(argv[2]);
            Server server(port);
            
            if (!server.start()) {
                std::cerr << "Failed to start server" << std::endl;
                return 1;
            }
            
            std::cout << "Server running. Press Enter to stop..." << std::endl;
            std::cin.get();
            server.stop();
            
        } else if (mode == "client") {
            if (argc != 4) {
                printUsage();
                return 1;
            }
            
            std::string server_ip = argv[2];
            int port = std::stoi(argv[3]);
            Client client(server_ip, port);
            
            if (!client.start()) {
                std::cerr << "Failed to start client" << std::endl;
                return 1;
            }
            
            std::cout << "Client running. Press Enter to stop..." << std::endl;
            std::cin.get();
            client.stop();
            
        } else {
            printUsage();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}