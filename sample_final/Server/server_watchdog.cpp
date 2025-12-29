
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#endif

using namespace std;

// Config
string SERVER_EXECUTABLE = "server";        // Server binary name (in same directory)
string SERVER_PORT = "8080";                // Default port if not specified
const int CHECK_INTERVAL_MS = 1000;         // How often to check (1 second)
const int RESTART_DELAY_MS = 2000;          // Wait 2 seconds before restart

// Globals
atomic<bool> running(true);
atomic<int> restart_count(0);
atomic<int> server_pid(0);
atomic<bool> server_running(false);

// Get timestamp for logs
string timestamp() {
    auto now = chrono::system_clock::now();
    auto t = chrono::system_clock::to_time_t(now);
    auto ms = chrono::duration_cast<chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;
    
    stringstream ss;
    ss << put_time(localtime(&t), "%H:%M:%S") << "." 
       << setfill('0') << setw(3) << ms.count();
    return ss.str();
}

// Signal handler
void handle_signal(int sig) {
    cout << "[" << timestamp() << "] Signal " << sig << " received" << endl;
    running = false;
}

#ifdef _WIN32
// Windows process functions
bool start_server_win() {
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    string cmd = SERVER_EXECUTABLE + " " + SERVER_PORT;
    char* cmd_cstr = new char[cmd.size() + 1];
    strcpy(cmd_cstr, cmd.c_str());
    
    if (!CreateProcess(
        NULL,
        cmd_cstr,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        delete[] cmd_cstr;
        return false;
    }
    
    delete[] cmd_cstr;
    server_pid = pi.dwProcessId;
    server_running = true;
    return true;
}

bool check_server_win() {
    if (server_pid == 0) return false;
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, server_pid);
    if (!hProcess) return false;
    
    DWORD exitCode;
    GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);
    
    bool is_alive = (exitCode == STILL_ACTIVE);
    server_running = is_alive;
    return is_alive;
}

void kill_server_win() {
    if (server_pid == 0) return;
    
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, server_pid);
    if (hProcess) {
        TerminateProcess(hProcess, 0);
        WaitForSingleObject(hProcess, 5000);
        CloseHandle(hProcess);
    }
}

#else
// Linux/Unix process functions
bool start_server_unix() {
    pid_t pid = fork();
    
    if (pid < 0) {
        return false;
    }
    
    if (pid == 0) {
        // Child process: start server
        execl(
            SERVER_EXECUTABLE.c_str(),
            SERVER_EXECUTABLE.c_str(),
            SERVER_PORT.c_str(),
            (char*)NULL
        );
        // If we get here, exec failed
        cerr << "[" << timestamp() << "] exec failed: " << strerror(errno) << endl;
        exit(1);
    }
    
    server_pid = pid;
    server_running = true;
    return true;
}

bool check_server_unix() {
    if (server_pid <= 0) return false;
    
    // Use waitpid with WNOHANG to check if process is still running
    // This also detects zombies and cleans them up
    int status;
    pid_t result = waitpid(server_pid, &status, WNOHANG);
    
    if (result == 0) {
        // Process is still running
        return true;
    } else if (result == server_pid) {
        // Process has exited or is a zombie - it's dead
        return false;
    } else {
        // Process doesn't exist (result == -1, errno == ECHILD)
        return false;
    }
}

void kill_server_unix() {
    if (server_pid <= 0) return;
    
    kill(server_pid, SIGTERM);
    
    // Wait a bit for graceful shutdown
    for (int i = 0; i < 10; i++) {
        if (!check_server_unix()) break;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    
    // Force kill if still running
    if (check_server_unix()) {
        kill(server_pid, SIGKILL);
    }
    
    // Clean up zombie
    waitpid(server_pid, nullptr, 0);
}
#endif

// Platform-specific wrappers
bool start_server() {
#ifdef _WIN32
    return start_server_win();
#else
    return start_server_unix();
#endif
}

bool check_server() {
#ifdef _WIN32
    return check_server_win();
#else
    return check_server_unix();
#endif
}

void kill_server() {
#ifdef _WIN32
    kill_server_win();
#else
    kill_server_unix();
#endif
}

void print_usage(const char* prog_name) {
    cout << "Usage: " << prog_name << " [PORT]" << endl;
    cout << "  PORT: Server port number (default: 8080)" << endl;
    cout << endl;
    cout << "Examples:" << endl;
    cout << "  " << prog_name << "           # Uses port 8080" << endl;
    cout << "  " << prog_name << " 3000      # Uses port 3000" << endl;
    cout << "  " << prog_name << " 8081      # Uses port 8081" << endl;
}

void print_banner() {
    cout << "\n╔══════════════════════════════════════════════════╗" << endl;
    cout << "║       MEOWKEY SERVER WATCHDOG (v1.0)            ║" << endl;
    cout << "╠══════════════════════════════════════════════════╣" << endl;
    cout << "║ Server: " << SERVER_EXECUTABLE;
    for (int i = 0; i < 38 - SERVER_EXECUTABLE.length(); i++) cout << " ";
    cout << "║" << endl;
    cout << "║ Port: " << SERVER_PORT;
    for (int i = 0; i < 40 - SERVER_PORT.length(); i++) cout << " ";
    cout << "║" << endl;
    cout << "║                                                  ║" << endl;
    cout << "║ Press Ctrl+C to stop watchdog AND server         ║" << endl;
    cout << "╚══════════════════════════════════════════════════╝" << endl;
    cout << endl;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc > 1) {
        // Check for help
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        
        // Set port from command line
        SERVER_PORT = argv[1];
        
        // Validate port
        try {
            int port = stoi(SERVER_PORT);
            if (port < 1 || port > 65535) {
                cerr << "Error: Port must be between 1 and 65535" << endl;
                return 1;
            }
        } catch (...) {
            cerr << "Error: Invalid port number: " << SERVER_PORT << endl;
            return 1;
        }
    }
    
    // Setup signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    print_banner();
    
    cout << "[" << timestamp() << "] Watchdog starting..." << endl;
    cout << "[" << timestamp() << "] Starting server on port " << SERVER_PORT << "..." << endl;
    
    // Initial server start
    if (!start_server()) {
        cerr << "[" << timestamp() << "] ERROR: Failed to start server!" << endl;
        return 1;
    }
    
    cout << "[" << timestamp() << "] Server started (PID: " << server_pid << ")" << endl;
    cout << "[" << timestamp() << "] Monitoring..." << endl;
    
    // Main watchdog loop
    while (running) {
        // Check if server is still running
        bool alive = check_server();
        
        if (!alive && server_running) {
            // Server just died
            cout << "[" << timestamp() << "] Server crashed!" << endl;
            restart_count++;
            
            // Wait before restart
            cout << "[" << timestamp() << "] Waiting " 
                 << (RESTART_DELAY_MS / 1000.0) << " seconds..." << endl;
            this_thread::sleep_for(chrono::milliseconds(RESTART_DELAY_MS));
            
            // Restart server
            cout << "[" << timestamp() << "] Restarting server (attempt #" 
                 << restart_count << ")..." << endl;
            
            if (!start_server()) {
                cerr << "[" << timestamp() << "] ERROR: Restart failed!" << endl;
                this_thread::sleep_for(chrono::milliseconds(5000));
                continue;
            }
            
            cout << "[" << timestamp() << "] Server restarted (PID: " 
                 << server_pid << ")" << endl;
            server_running = true;  // Mark as running after successful restart
        }
        
        // Wait before next check
        this_thread::sleep_for(chrono::milliseconds(CHECK_INTERVAL_MS));
    }
    
    // Clean shutdown
    cout << "\n[" << timestamp() << "] Shutting down watchdog..." << endl;
    cout << "[" << timestamp() << "] Stopping server..." << endl;
    
    kill_server();
    
    cout << "[" << timestamp() << "] Server stopped" << endl;
    cout << "[" << timestamp() << "] Total restarts: " << restart_count << endl;
    cout << "[" << timestamp() << "] Watchdog stopped" << endl;
    
    return 0;
}