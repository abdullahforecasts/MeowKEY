#include "server_cross.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

void printUsage() {
    cout << "Usage: ./server <port> [storage_file]" << endl;
    cout << "Example: ./server 8080" << endl;
    cout << "         ./server 8080 meowkey_server.db" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        printUsage();
        return 1;
    }

    int port = stoi(argv[1]);
    string storage_file = (argc == 3) ? argv[2] : "meowkey_server.db";

    try {
        Server server(port, storage_file);
        
        if (!server.start()) {
            cerr << "❌ Failed to start server" << endl;     
            return 1; 
        }
        
        server.runCommandLoop();
        
    } catch (const exception& e) {
        cerr << "❌ Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}