// test_api.cpp - Test C++ API server
#include <iostream>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

using namespace std;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(3002);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection failed" << endl;
        close(sock);
        return 1;
    }
    
    // Test ping
    string ping_json = "{\"command\":\"ping\"}\n";
    cout << "Sending: " << ping_json;
    send(sock, ping_json.c_str(), ping_json.size(), 0);
    
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    cout << "Received: " << buffer << endl;
    
    // Test list
    string list_json = "{\"command\":\"list\"}\n";
    cout << "Sending: " << list_json;
    send(sock, list_json.c_str(), list_json.size(), 0);
    
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer) - 1, 0);
    cout << "Received: " << buffer << endl;
    
    close(sock);
    return 0;
}