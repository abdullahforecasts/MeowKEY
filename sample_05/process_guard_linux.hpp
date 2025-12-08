// #ifndef PROCESS_GUARD_LINUX_HPP
// #define PROCESS_GUARD_LINUX_HPP

// #ifdef PLATFORM_LINUX

// #include <unistd.h>
// #include <signal.h>
// #include <sys/wait.h>
// #include <sys/prctl.h>
// #include <string>
// #include <vector>
// #include <iostream>
// #include <atomic>
// #include <cstring>
// #include "launcher_common.hpp"
// #include "password_dialog_linux.hpp"

// class ProcessGuardLinux {
// private:
//     pid_t child_pid;
//     std::atomic<bool>& exit_authorized;
//     static ProcessGuardLinux* instance_;
    
// public:
//     ProcessGuardLinux(std::atomic<bool>& auth) 
//         : child_pid(-1), exit_authorized(auth) {
//         instance_ = this;
//     }
    
//     ~ProcessGuardLinux() {
//         instance_ = nullptr;
//     }
    
//     bool launchClient(const std::string& executable, const std::string& args) {
//         std::cout << "🛡️  Guardian: Launching protected client..." << std::endl;
        
//         // Install signal handlers BEFORE forking
//         installSignalHandlers();
        
//         child_pid = fork();
        
//         if (child_pid == -1) {
//             std::cerr << "❌ Fork failed: " << strerror(errno) << std::endl;
//             return false;
//         }
        
//         if (child_pid == 0) {
//             // Child process - UNBLOCK signals so child can receive them normally
//             sigset_t unblock_set;
//             sigfillset(&unblock_set);
//             sigprocmask(SIG_UNBLOCK, &unblock_set, NULL);
            
//             // Reset signal handlers to default in child
//             signal(SIGINT, SIG_DFL);
//             signal(SIGTERM, SIG_DFL);
//             signal(SIGQUIT, SIG_DFL);
//             signal(SIGHUP, SIG_DFL);
            
//             // Create new process group to isolate from parent's signals
//             setpgid(0, 0);
            
//             // Execute the client
//             executeClient(executable, args);
//             _exit(1); // Should never reach here
//         }
        
//         // Parent process
//         std::cout << "✅ Client launched successfully under protection (PID: " 
//                   << child_pid << ")" << std::endl;
//         std::cout << "🔒 Exit protection active - password required to exit" << std::endl;
        
//         return true;
//     }
    
//     void monitorProcess() {
//         std::cout << "\n🔍 Guardian: Monitoring client process..." << std::endl;
//         std::cout << "Press Ctrl+C to test exit protection\n" << std::endl;
        
//         int status;
//         while (true) {
//             pid_t result = waitpid(child_pid, &status, WNOHANG);
            
//             if (result == child_pid) {
//                 // Child process exited
//                 if (exit_authorized) {
//                     std::cout << "✅ Authorized exit - Guardian shutting down" << std::endl;
//                     break;
//                 } else {
//                     std::cout << "⚠️  Client process terminated unexpectedly!" << std::endl;
//                     if (WIFEXITED(status)) {
//                         std::cout << "Exit code: " << WEXITSTATUS(status) << std::endl;
//                     }
//                     break;
//                 }
//             } else if (result == -1) {
//                 std::cerr << "❌ Error waiting for child: " << strerror(errno) << std::endl;
//                 break;
//             }
            
//             // Check if we should exit
//             if (exit_authorized) {
//                 std::cout << "🛑 Terminating client process..." << std::endl;
//                 kill(child_pid, SIGTERM);
//                 sleep(2);
//                 kill(child_pid, SIGKILL);
//                 break;
//             }
            
//             usleep(100000); // 100ms
//         }
//     }
    
// private:
//     void executeClient(const std::string& executable, const std::string& args) {
//         // Parse arguments
//         std::vector<std::string> argTokens;
//         std::string current;
//         for (char c : args) {
//             if (c == ' ') {
//                 if (!current.empty()) {
//                     argTokens.push_back(current);
//                     current.clear();
//                 }
//             } else {
//                 current += c;
//             }
//         }
//         if (!current.empty()) {
//             argTokens.push_back(current);
//         }
        
//         // Build argv
//         std::vector<char*> argv;
//         argv.push_back(const_cast<char*>(executable.c_str()));
//         for (auto& arg : argTokens) {
//             argv.push_back(const_cast<char*>(arg.c_str()));
//         }
//         argv.push_back(nullptr);
        
//         // Execute
//         execvp(executable.c_str(), argv.data());
        
//         // If we reach here, exec failed
//         std::cerr << "❌ Failed to execute client: " << strerror(errno) << std::endl;
//     }
    
//     void installSignalHandlers() {
//         // Block signals in child process by setting up handler BEFORE fork
//         struct sigaction sa;
//         sa.sa_handler = signalHandler;
//         sigemptyset(&sa.sa_mask);
//         sa.sa_flags = SA_RESTART; // Automatically restart system calls
        
//         // Add all signals we want to block to the mask
//         sigaddset(&sa.sa_mask, SIGINT);
//         sigaddset(&sa.sa_mask, SIGTERM);
//         sigaddset(&sa.sa_mask, SIGQUIT);
//         sigaddset(&sa.sa_mask, SIGHUP);
        
//         sigaction(SIGINT, &sa, NULL);   // Ctrl+C
//         sigaction(SIGTERM, &sa, NULL);  // kill command
//         sigaction(SIGQUIT, &sa, NULL);  // Ctrl+\
//         sigaction(SIGHUP, &sa, NULL);   // Terminal close
        
//         // Block child from receiving these signals
//         sigset_t block_set;
//         sigemptyset(&block_set);
//         sigaddset(&block_set, SIGINT);
//         sigaddset(&block_set, SIGTERM);
//         sigprocmask(SIG_BLOCK, &block_set, NULL);
//     }
    
//     static void signalHandler(int signum) {
//         // CRITICAL: Block signal from propagating to child process
//         signal(signum, signalHandler); // Re-install handler
        
//         const char* signame = "UNKNOWN";
//         switch (signum) {
//             case SIGINT: signame = "SIGINT (Ctrl+C)"; break;
//             case SIGTERM: signame = "SIGTERM"; break;
//             case SIGQUIT: signame = "SIGQUIT (Ctrl+\\)"; break;
//             case SIGHUP: signame = "SIGHUP"; break;
//         }
        
//         // Use write() instead of cout (signal-safe)
//         const char* msg = "\n🚨 Signal intercepted: ";
//         write(STDOUT_FILENO, msg, strlen(msg));
//         write(STDOUT_FILENO, signame, strlen(signame));
//         write(STDOUT_FILENO, "\n", 1);
        
//         if (instance_) {
//             instance_->handleExitAttempt();
//         }
        
//         // DO NOT exit or terminate - just return
//     }
    
//     void handleExitAttempt() {
//         std::cout << "🔐 Password authentication required..." << std::endl;
//         std::cout << "╔════════════════════════════════════════╗" << std::endl;
//         std::cout << "║  EXIT AUTHORIZATION REQUIRED          ║" << std::endl;
//         std::cout << "╚════════════════════════════════════════╝" << std::endl;
//         std::cout << "Enter password to stop monitoring: " << std::flush;
        
//         // Read password directly (signal-safe approach)
//         std::string password;
//         std::getline(std::cin, password);
        
//         if (verifyPassword(password)) {
//             std::cout << "✅ Password correct - Exit authorized" << std::endl;
//             exit_authorized = true;
            
//             // Terminate child gracefully
//             if (child_pid > 0) {
//                 kill(child_pid, SIGTERM);
//                 sleep(1);
//                 kill(child_pid, SIGKILL);
//             }
//         } else {
//             showUnauthorizedMessage();
//             std::cout << "🔄 Continue working... (Ctrl+C again to retry exit)" << std::endl;
//             // Child continues running - do NOT terminate
//         }
//     }
// };

// ProcessGuardLinux* ProcessGuardLinux::instance_ = nullptr;

// #endif // PLATFORM_LINUX
// #endif




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

class ProcessGuardLinux {
private:
    pid_t child_pid;
    std::atomic<bool>& exit_authorized;
    static ProcessGuardLinux* instance_;
    
public:
    ProcessGuardLinux(std::atomic<bool>& auth) 
        : child_pid(-1), exit_authorized(auth) {
        instance_ = this;
    }
    
    ~ProcessGuardLinux() {
        instance_ = nullptr;
    }
    
    bool launchClient(const std::string& executable, const std::string& args) {
        std::cout << "🛡️  Guardian: Launching protected client..." << std::endl;
        
        // Install signal handlers BEFORE forking
        installSignalHandlers();
        
        child_pid = fork();
        
        if (child_pid == -1) {
            std::cerr << "❌ Fork failed: " << strerror(errno) << std::endl;
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
        std::cout << "✅ Client launched successfully under protection (PID: " 
                  << child_pid << ")" << std::endl;
        std::cout << "🔒 Exit protection active - password required to exit" << std::endl;
        
        return true;
    }
    
    void monitorProcess() {
        std::cout << "\n🔍 Guardian: Monitoring client process..." << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "⚠️  IMPORTANT: To request exit, run this command in ANOTHER terminal:" << std::endl;
        std::cout << "    sudo kill -INT " << getpid() << std::endl;
        std::cout << "    (or: sudo kill -2 " << getpid() << ")" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
        
        int status;
        while (true) {
            pid_t result = waitpid(child_pid, &status, WNOHANG);
            
            if (result == child_pid) {
                // Child process exited
                if (exit_authorized) {
                    std::cout << "✅ Authorized exit - Guardian shutting down" << std::endl;
                    break;
                } else {
                    std::cout << "⚠️  Client process terminated unexpectedly!" << std::endl;
                    if (WIFEXITED(status)) {
                        std::cout << "Exit code: " << WEXITSTATUS(status) << std::endl;
                    }
                    break;
                }
            } else if (result == -1) {
                std::cerr << "❌ Error waiting for child: " << strerror(errno) << std::endl;
                break;
            }
            
            // Check if we should exit
            if (exit_authorized) {
                std::cout << "🛑 Terminating client process..." << std::endl;
                kill(child_pid, SIGTERM);
                sleep(2);
                kill(child_pid, SIGKILL);
                break;
            }
            
            usleep(100000); // 100ms
        }
    }
    
private:
    void executeClient(const std::string& executable, const std::string& args) {
        // Parse arguments
        std::vector<std::string> argTokens;
        std::string current;
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
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (auto& arg : argTokens) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        // Execute
        execvp(executable.c_str(), argv.data());
        
        // If we reach here, exec failed
        std::cerr << "❌ Failed to execute client: " << strerror(errno) << std::endl;
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
        std::cout << "\n🔐 Password authentication required..." << std::endl;
        std::cout << "╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║  EXIT AUTHORIZATION REQUIRED          ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        std::cout << "Enter password to stop monitoring: " << std::flush;
        
        // Make sure stdin is unbuffered and ready
        std::cin.clear();
        std::cin.sync();
        
        // Read password
        std::string password;
        if (std::getline(std::cin, password)) {
            if (verifyPassword(password)) {
                std::cout << "\n✅ Password correct - Exit authorized" << std::endl;
                exit_authorized = true;
                
                // Terminate child gracefully
                if (child_pid > 0) {                   
                    std::cout << "🛑 Terminating client process..." << std::endl;
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
                std::cout << "✅ Guardian shutdown complete" << std::endl;
                exit(0);
            } else {
                showUnauthorizedMessage();
                std::cout << "🔄 Continue working... (Ctrl+C again to retry exit)\n" << std::endl;
            }
        } else {
            // Input failed, treat as wrong password
            showUnauthorizedMessage();
            std::cout << "🔄 Continue working... (Ctrl+C again to retry exit)\n" << std::endl;
        }
    }
};

ProcessGuardLinux* ProcessGuardLinux::instance_ = nullptr;

#endif // PLATFORM_LINUX
#endif  