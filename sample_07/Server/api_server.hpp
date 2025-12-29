#ifndef API_SERVER_HPP
#define API_SERVER_HPP

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <sstream>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <regex>
#include<fcntl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#endif

using namespace std;

class APIServer {
private:
    int port_;
    int server_socket_;
    atomic<bool> running_;
    thread server_thread_;
    
    // Command handling
    mutex command_mutex_;
    queue<string> command_queue_;
    condition_variable command_cv_;
    
    // Response storage
    struct CommandResult {
        string output;
        bool ready;
    };
    map<string, CommandResult> command_results_;
    mutex results_mutex_;
    
    // Helper function to extract JSON value
    string extractJSONValue(const string& json, const string& key) {
        string pattern = "\"" + key + "\":\\s*\"([^\"]*)\"";
        regex reg(pattern);
        smatch match;
        
        if (regex_search(json, match, reg) && match.size() > 1) {
            return match[1].str();
        }
        
        // Try without quotes for value
        pattern = "\"" + key + "\":\\s*([^,}\\s]+)";
        regex reg2(pattern);
        if (regex_search(json, match, reg2) && match.size() > 1) {
            return match[1].str();
        }
        
        return "";
    }
    
    void handleClient(int client_socket) {
        char buffer[4096];
        string request_data;
        
        while (running_) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received <= 0) {
                break;
            }
            
            buffer[bytes_received] = '\0';
            request_data += string(buffer);
            
            // Check if we have complete request
            if (request_data.find('\n') != string::npos || 
                (request_data.find('{') != string::npos && 
                 request_data.find('}') != string::npos)) {
                
                // Log the raw request for debugging
                cout << "📥 API Request: " << request_data.substr(0, 200) << endl;
                
                string response = processJSONRequest(request_data);
                cout << "📤 API Response: " << response.substr(0, 200) << endl;
                
                send(client_socket, response.c_str(), response.size(), 0);
                
                // Clear for next request
                request_data.clear();
            }
        }
        
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
    }
    
    string processJSONRequest(const string& request) {
        // Simple JSON parser using regex
        string command = extractJSONValue(request, "command");
        string client_id = extractJSONValue(request, "client_id");
        string data_type = extractJSONValue(request, "type");
        
        cout << "🔍 Parsed JSON: command='" << command 
             << "', client_id='" << client_id 
             << "', type='" << data_type << "'" << endl;
        
        // Handle ping command
        if (command == "ping") {
            return "{\"status\":\"success\",\"message\":\"API server running\",\"timestamp\":\"" + to_string(time(nullptr)) + "\"}\n";
        }
        
        // Build command string for main server
        string cmd_str;
        if (command == "list") {
            cmd_str = "list";
        } else if (command == "clients") {
            cmd_str = "clients";
        } else if (command == "accept" && !client_id.empty()) {
            cmd_str = "accept " + client_id;
        } else if (command == "reject" && !client_id.empty()) {
            cmd_str = "reject " + client_id;
        } else if (command == "kick" && !client_id.empty()) {
            cmd_str = "kick " + client_id;
        } else if (command == "delete" && !client_id.empty()) {
            cmd_str = "delete " + client_id;
        } else if (command == "flush") {
            cmd_str = "flush";
        } else if (command == "stats") {
            cmd_str = "stats";
        } else if (command == "view" && !client_id.empty()) {
            cmd_str = "view " + client_id + " " + (data_type.empty() ? "all" : data_type);
        } else if (command == "quit") {
            cmd_str = "quit";
        } else {
            cout << "❌ Invalid command received: '" << command << "'" << endl;
            return "{\"status\":\"error\",\"message\":\"Invalid command: " + command + "\"}\n";
        }
        
        // Generate unique command ID
        string cmd_id = to_string(time(nullptr)) + "_" + to_string(rand() % 10000);
        
        // Store empty result
        {
            lock_guard<mutex> lock(results_mutex_);
            command_results_[cmd_id] = {"", false};
        }
        
        // Queue command with ID
        {
            lock_guard<mutex> lock(command_mutex_);
            command_queue_.push(cmd_id + ":" + cmd_str);
            cout << "📋 Command queued: " << cmd_id << ":" << cmd_str << endl;
        }
        command_cv_.notify_one();
        
        // Wait for result (with timeout)
        for (int i = 0; i < 100; i++) { // 10 second timeout
            this_thread::sleep_for(chrono::milliseconds(100));
            
            {
                lock_guard<mutex> lock(results_mutex_);
                if (command_results_.find(cmd_id) != command_results_.end() && 
                    command_results_[cmd_id].ready) {
                    string result = command_results_[cmd_id].output;
                    command_results_.erase(cmd_id);
                    
                    // Convert command output to JSON
                    cout << "✅ Command completed: " << cmd_str << endl;
                    return formatOutputAsJSON(command, result, cmd_str);
                }
            }
        }
        
        // Timeout
        {
            lock_guard<mutex> lock(results_mutex_);
            command_results_.erase(cmd_id);
        }
        
        cout << "⏰ Command timeout: " << cmd_str << endl;
        return "{\"status\":\"error\",\"message\":\"Command timeout: " + command + "\"}\n";
    }
    
    string formatOutputAsJSON(const string& command, const string& output, const string& full_cmd = "") {
        stringstream json;
        
        // Start JSON
        json << "{\"status\":\"success\",\"command\":\"" << command << "\"";
        
        // Add the raw output as an array of lines
        vector<string> lines;
        istringstream stream(output);
        string line;
        
        while (getline(stream, line)) {
            // Remove trailing \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        
        // Add data array
        if (!lines.empty()) {
            json << ",\"data\":[";
            for (size_t i = 0; i < lines.size(); ++i) {
                if (i > 0) json << ",";
                
                // Escape special JSON characters
                string escaped_line;
                for (char c : lines[i]) {
                    switch (c) {
                        case '"': escaped_line += "\\\""; break;
                        case '\\': escaped_line += "\\\\"; break;
                        case '\b': escaped_line += "\\b"; break;
                        case '\f': escaped_line += "\\f"; break;
                        case '\n': escaped_line += "\\n"; break;
                        case '\r': escaped_line += "\\r"; break;
                        case '\t': escaped_line += "\\t"; break;
                        default: escaped_line += c;
                    }
                }
                json << "\"" << escaped_line << "\"";
            }
            json << "]";
        } else {
            json << ",\"data\":[]";
        }
        
        // Add raw output for debugging
        json << ",\"raw\":\"";
        for (char c : output) {
            switch (c) {
                case '"': json << "\\\""; break;
                case '\\': json << "\\\\"; break;
                case '\n': json << "\\n"; break;
                case '\r': json << "\\r"; break;
                case '\t': json << "\\t"; break;
                default: json << c;
            }
        }
        json << "\"";
        
        json << "}";
        
        return json.str() + "\n";
    }
    
    void runServer() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "WSAStartup failed" << endl;
            return;
        }
#endif

        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            cerr << "Failed to create socket" << endl;
            return;
        }
        
        // Set socket options
        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        // Set non-blocking (optional)
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(server_socket_, FIONBIO, &mode);
#else
        int flags = fcntl(server_socket_, F_GETFL, 0);
        fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
#endif
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);
        
        if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "❌ Bind failed on port " << port_ << " (error: " << errno << ")" << endl;
            return;
        }
        
        if (listen(server_socket_, 10) < 0) {
            cerr << "Listen failed" << endl;
            return;
        }
        
        cout << "✅ JSON API Server listening on port " << port_ << endl;
        
        while (running_) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_socket < 0) {
                if (running_) {
#ifdef _WIN32
                    if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
                    {
                        this_thread::sleep_for(chrono::milliseconds(100));
                        continue;
                    }
                }
                continue;
            }
            
            // Handle client in separate thread
            thread([this, client_socket]() {
                handleClient(client_socket);
            }).detach();
        }
        
#ifdef _WIN32
        closesocket(server_socket_);
        WSACleanup();
#else
        close(server_socket_);
#endif
        
        cout << "API Server stopped" << endl;
    }

public:
    APIServer(int port = 3002) : port_(port), server_socket_(-1), running_(false) {}
    
    ~APIServer() {
        stop();
    }
    
    void start() {
        if (running_) return;
        running_ = true;
        server_thread_ = thread(&APIServer::runServer, this);
        
        // Give server time to start
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    
    void stop() {
        running_ = false;
        
        if (server_socket_ >= 0) {
#ifdef _WIN32
            closesocket(server_socket_);
#else
            shutdown(server_socket_, SHUT_RDWR);
            close(server_socket_);
#endif
            server_socket_ = -1;
        }
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
    
    // Get next command from queue
    string getNextCommand(int timeout_ms = 100) {
        unique_lock<mutex> lock(command_mutex_);
        
        if (command_cv_.wait_for(lock, chrono::milliseconds(timeout_ms), [this]() {
            return !command_queue_.empty();
        })) {
            string cmd = command_queue_.front();
            command_queue_.pop();
            cout << "📨 Processing command from queue: " << cmd << endl;
            return cmd;
        }
        
        return "";
    }
    
    // Store command result
    void storeResult(const string& cmd_id, const string& output) {
        lock_guard<mutex> lock(results_mutex_);
        if (command_results_.find(cmd_id) != command_results_.end()) {
            cout << "💾 Storing result for command ID: " << cmd_id 
                 << " (length: " << output.length() << ")" << endl;
            command_results_[cmd_id] = {output, true};
        } else {
            cout << "⚠️  Command ID not found for result: " << cmd_id << endl;
        }
    }
    
    // Debug: Print queue status
    void debugPrintQueue() {
        lock_guard<mutex> lock(command_mutex_);
        cout << "📊 Command queue size: " << command_queue_.size() << endl;
    }
};

#endif