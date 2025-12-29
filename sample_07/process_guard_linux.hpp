

#ifndef PROCESS_GUARD_LINUX_HPP
#define PROCESS_GUARD_LINUX_HPP

#ifdef PLATFORM_LINUX

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <iostream>
#include <atomic>
#include <cstring>
#include "launcher_common.hpp"
#include "password_dialog_linux.hpp"
using namespace std; 

class ProcessGuardLinux {
private:
    pid_t child_pid;
      atomic<bool>& exit_authorized;
    static ProcessGuardLinux* instance_;
    
public:
    ProcessGuardLinux(  atomic<bool>& auth) 
        : child_pid(-1), exit_authorized(auth) {
        instance_ = this;
    }
    
    ~ProcessGuardLinux() {
        instance_ = nullptr;
    }
    
    bool launchClient(const   string& executable, const   string& args) {
          cout << "🛡️  Guardian: Launching protected client..." <<   endl;
        
        // Install signal handlers BEFORE forking
        installSignalHandlers();
        
        child_pid = fork();
        
        if (child_pid == -1) {
              cerr << "❌ Fork failed: " << strerror(errno) <<   endl;
            return false;
        }
        
        if (child_pid == 0) {
            // Child process - UNBLOCK signals so child can receive them normally
            sigset_t unblock_set;
            sigfillset(&unblock_set);
            sigprocmask(SIG_UNBLOCK, &unblock_set, NULL);
            
            // Reset signal handlers to default in child
            signal(SIGINT, SIG_DFL);
            signal(SIGTERM, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGHUP, SIG_DFL);
            
            // Create new process group to isolate from parent's signals
            setpgid(0, 0);
            
            // Execute the client
            executeClient(executable, args);
            _exit(1); // Should never reach here
        }
        
        // Parent process
          cout << "✅ Client launched successfully under protection (PID: " 
                  << child_pid << ")" <<   endl;
          cout << "🔒 Exit protection active - password required to exit" <<   endl;
        
        return true;
    }
    
    void monitorProcess() {
          cout << "\n🔍 Guardian: Monitoring client process..." <<   endl;
          cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" <<   endl;
          cout << "⚠️  IMPORTANT: To request exit, run this command in ANOTHER terminal:" <<   endl;
          cout << "    sudo kill -INT " << getpid() <<   endl;
          cout << "    (or: sudo kill -2 " << getpid() << ")" <<   endl;
          cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" <<   endl;
        
        int status;
        while (true) {
            pid_t result = waitpid(child_pid, &status, WNOHANG);
            
            if (result == child_pid) {
                // Child process exited
                if (exit_authorized) {
                      cout << "✅ Authorized exit - Guardian shutting down" <<   endl;
                    break;
                } else {
                      cout << "⚠️  Client process terminated unexpectedly!" <<   endl;
                    if (WIFEXITED(status)) {
                          cout << "Exit code: " << WEXITSTATUS(status) <<   endl;
                    }
                    break;
                }
            } else if (result == -1) {
                  cerr << "❌ Error waiting for child: " << strerror(errno) <<   endl;
                break;
            }
            
            // Check if we should exit
            if (exit_authorized) {
                  cout << "🛑 Terminating client process..." <<   endl;
                kill(child_pid, SIGTERM);
                sleep(2);
                kill(child_pid, SIGKILL);
                break;
            }
            
            usleep(100000); // 100ms
        }
    }
    
private:
    void executeClient(const   string& executable, const   string& args) {
        // Parse arguments
          vector<  string> argTokens;
          string current;
        for (char c : args) {
            if (c == ' ') {
                if (!current.empty()) {
                    argTokens.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            argTokens.push_back(current);
        }
        
        // Build argv
          vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (auto& arg : argTokens) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        // Execute
        execvp(executable.c_str(), argv.data());
        
        // If we reach here, exec failed
          cerr << "❌ Failed to execute client: " << strerror(errno) <<   endl;
    }
    
    void installSignalHandlers() {
        // Block signals in child process by setting up handler BEFORE fork
        struct sigaction sa;
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART; // Automatically restart system calls
        
        // Add all signals we want to block to the mask
        sigaddset(&sa.sa_mask, SIGINT);
        sigaddset(&sa.sa_mask, SIGTERM);
        sigaddset(&sa.sa_mask, SIGQUIT);
        sigaddset(&sa.sa_mask, SIGHUP);
        
        sigaction(SIGINT, &sa, NULL);   // Ctrl+C
        sigaction(SIGTERM, &sa, NULL);  // kill command
        sigaction(SIGQUIT, &sa, NULL);  // Ctrl+\
        sigaction(SIGHUP, &sa, NULL);   // Terminal close
        
        // Block child from receiving these signals
        sigset_t block_set;
        sigemptyset(&block_set);
        sigaddset(&block_set, SIGINT);
        sigaddset(&block_set, SIGTERM);
        sigprocmask(SIG_BLOCK, &block_set, NULL);
    }
    
    static void signalHandler(int signum) {
        // CRITICAL: Block signal from propagating to child process
        signal(signum, signalHandler); // Re-install handler
        
        const char* signame = "UNKNOWN";
        switch (signum) {
            case SIGINT: signame = "SIGINT (Ctrl+C)"; break;
            case SIGTERM: signame = "SIGTERM"; break;
            case SIGQUIT: signame = "SIGQUIT (Ctrl+\\)"; break;
            case SIGHUP: signame = "SIGHUP"; break;
        }
        
        // Use write() instead of cout (signal-safe)
        const char* msg = "\n\n🚨 Signal intercepted: ";
        write(STDOUT_FILENO, msg, strlen(msg));
        write(STDOUT_FILENO, signame, strlen(signame));
        write(STDOUT_FILENO, "\n", 1);
        
        if (instance_) {
            // Redirect stdin from controlling terminal
            int tty_fd = open("/dev/tty", O_RDWR);
            if (tty_fd >= 0) {
                dup2(tty_fd, STDIN_FILENO);
                close(tty_fd);
            }
            
            instance_->handleExitAttempt();
        }
        
        // DO NOT exit or terminate - just return
    }
    
    void handleExitAttempt() {
        // Ensure we can read from terminal
          cout << "\n🔐 Password authentication required..." <<   endl;
          cout << "╔════════════════════════════════════════╗" <<   endl;
          cout << "║  EXIT AUTHORIZATION REQUIRED          ║" <<   endl;
          cout << "╚════════════════════════════════════════╝" <<   endl;
          cout << "Enter password to stop monitoring: " <<   flush;
        
        // Make sure stdin is unbuffered and ready
          cin.clear();
          cin.sync();
        
        // Read password
          string password;
        if (  getline(  cin, password)) {
            if (verifyPassword(password)) {
                  cout << "\n✅ Password correct - Exit authorized" <<   endl;
                exit_authorized = true;
                
                // Terminate child gracefully
                if (child_pid > 0) {                   
                      cout << "🛑 Terminating client process..." <<   endl;
                    kill(child_pid, SIGTERM); 
                    sleep(1);
                     
                    // Check if still alive 
                    int status;
                    pid_t result = waitpid(child_pid, &status, WNOHANG); 
                    if (result == 0) {
                        // Still running, force kill
                        kill(child_pid, SIGKILL); 
                    }
                }
                
                // Exit the guardian
                  cout << "✅ Guardian shutdown complete" <<   endl;
                exit(0);
            } else {
                showUnauthorizedMessage();
                  cout << "🔄 Continue working... (Ctrl+C again to retry exit)\n" <<   endl;
            }
        } else {
            // Input failed, treat as wrong password
            showUnauthorizedMessage();
              cout << "🔄 Continue working... (Ctrl+C again to retry exit)\n" <<   endl;
        }
    }
};

ProcessGuardLinux* ProcessGuardLinux::instance_ = nullptr;

#endif // PLATFORM_LINUX
#endif  