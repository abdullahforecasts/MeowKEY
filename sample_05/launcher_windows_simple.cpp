// launcher_windows_simple.cpp
// Simple background monitoring - No window protection, just process monitoring

#include "platform_detector.hpp"

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <conio.h>
#include "launcher_common.hpp"
#include "password_dialog_windows.hpp"

std::atomic<bool> exit_authorized(false);
std::atomic<bool> monitoring(true);
PROCESS_INFORMATION clientProcess = {0};

bool launchClientHidden(const std::string& exe, const std::string& args) {
    std::string cmdLine = exe + " " + args;
    char* cmdBuffer = new char[cmdLine.length() + 1];
    strcpy_s(cmdBuffer, cmdLine.length() + 1, cmdLine.c_str());
    
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // HIDE the client console
    
    BOOL success = CreateProcessA(
        NULL, cmdBuffer,
        NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE | CREATE_NO_WINDOW,  // Hidden console
        NULL, NULL, &si, &clientProcess
    );
    
    delete[] cmdBuffer;
    return success == TRUE;
}

void inputThread() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "COMMANDS:" << std::endl;
    std::cout << "  exit   - Stop monitoring (requires password)" << std::endl;
    std::cout << "  status - Check client status" << std::endl;
    std::cout << "  show   - Show client console (if hidden)" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    std::string input;
    while (monitoring) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "exit" || input == "quit") {
            std::cout << "\n🔐 Password authentication required..." << std::endl;
            
            if (PasswordDialogWindows::showPasswordDialog()) {
                std::cout << "✅ Password correct - Stopping monitoring..." << std::endl;
                exit_authorized = true;
                monitoring = false;
                break;
            } else {
                std::cout << "❌ INCORRECT PASSWORD!" << std::endl;
                std::cout << "   Monitoring continues...\n" << std::endl;
            }
        } else if (input == "status") {
            if (clientProcess.hProcess) {
                DWORD exitCode;
                GetExitCodeProcess(clientProcess.hProcess, &exitCode);
                if (exitCode == STILL_ACTIVE) {
                    std::cout << "✅ Client is running (PID: " << clientProcess.dwProcessId << ")" << std::endl;
                    std::cout << "🔒 Background monitoring: ACTIVE" << std::endl;
                } else {
                    std::cout << "⚠️  Client is not running" << std::endl;
                }
            }
        } else if (input == "show") {
            std::cout << "ℹ️  Client runs in background - no console to show" << std::endl;
        } else if (!input.empty()) {
            std::cout << "Unknown command. Type 'exit', 'status', or 'show'." << std::endl;
        }
    }
}

void monitorClient(const std::string& server_ip, const std::string& port, 
                   const std::string& client_id) {
    int restartCount = 0;
    const int MAX_RESTARTS = 100;  // High number = keep trying
    
    std::string client_exe = "client.exe";
    std::string client_args = server_ip + " " + port + " " + client_id + " --guardian";
    
    while (monitoring && !exit_authorized) {
        DWORD waitResult = WaitForSingleObject(clientProcess.hProcess, 2000);
        
        if (waitResult == WAIT_OBJECT_0) {
            // Process exited
            DWORD exitCode;
            GetExitCodeProcess(clientProcess.hProcess, &exitCode);
            
            CloseHandle(clientProcess.hProcess);
            CloseHandle(clientProcess.hThread);
            ZeroMemory(&clientProcess, sizeof(clientProcess));
            
            if (!exit_authorized) {
                std::cout << "\n⚠️  CLIENT TERMINATED! (Exit code: " << exitCode << ")" << std::endl;
                
                if (restartCount < MAX_RESTARTS) {
                    std::cout << "🔄 Restarting in 3 seconds... (attempt " 
                              << (restartCount + 1) << "/" << MAX_RESTARTS << ")" << std::endl;
                    
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                    
                    if (launchClientHidden(client_exe, client_args)) {
                        std::cout << "✅ Client restarted (PID: " << clientProcess.dwProcessId << ")" << std::endl;
                        restartCount++;
                    } else {
                        std::cout << "❌ Failed to restart client" << std::endl;
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    }
                } else {
                    std::cout << "❌ Maximum restart attempts reached" << std::endl;
                    monitoring = false;
                    break;
                }
            } else {
                std::cout << "✅ Authorized exit" << std::endl;
                break;
            }
        }
    }
    
    // Final cleanup
    if (clientProcess.hProcess) {
        std::cout << "🛑 Terminating client..." << std::endl;
        TerminateProcess(clientProcess.hProcess, 0);
        WaitForSingleObject(clientProcess.hProcess, 2000);
        CloseHandle(clientProcess.hProcess);
        CloseHandle(clientProcess.hThread);
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    
    if (argc != 4) {
        std::cout << "Usage: launcher.exe <server_ip> <port> <client_id>" << std::endl;
        std::cout << "Example: launcher.exe 192.168.100.8 8080 bscs24009" << std::endl;
        return 1;
    }
    
    std::string server_ip = argv[1];
    std::string port = argv[2];
    std::string client_id = argv[3];
    
    std::cout << "\n🛡️  PROTECTED CLIENT LAUNCHER (Background Mode)" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    std::cout << "Server: " << server_ip << ":" << port << std::endl;
    std::cout << "Client ID: " << client_id << std::endl;
    
    std::string client_exe = "client.exe";
    std::string client_args = server_ip + " " + port + " " + client_id + " --guardian";
    
    std::cout << "\n⏳ Launching client in background..." << std::endl;
    
    if (!launchClientHidden(client_exe, client_args)) {
        std::cerr << "❌ Failed to launch client!" << std::endl;
        std::cout << "Press any key to exit...";
        _getch();
        return 1;
    }
    
    std::cout << "✅ Client launched (PID: " << clientProcess.dwProcessId << ")" << std::endl;
    std::cout << "✅ Running in background (hidden console)" << std::endl;
    
    std::cout << "\n🔒 PROTECTION ACTIVE:" << std::endl;
    std::cout << "   ✓ Client runs in background (no visible window)" << std::endl;
    std::cout << "   ✓ Auto-restarts on crash (up to 100 times)" << std::endl;
    std::cout << "   ✓ Password required to stop: 'exit' command" << std::endl;
    std::cout << "   ✓ Alternative: Run guardian_exit.exe" << std::endl;
    
    // Start input handler
    std::thread inputHandler(inputThread);
    
    // Monitor the client
    monitorClient(server_ip, port, client_id);
    
    // Cleanup
    monitoring = false;
    if (inputHandler.joinable()) {
        inputHandler.join();
    }
    
    std::cout << "\n✅ Guardian shutdown complete" << std::endl;
    return 0;
}

#endif // PLATFORM_WINDOWS