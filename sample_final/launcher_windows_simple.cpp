


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
using namespace std; 
  atomic<bool> exit_authorized(false);
  atomic<bool> monitoring(true);
PROCESS_INFORMATION clientProcess = {0};

bool launchClientHidden(const   string& exe, const   string& args) {
      string cmdLine = exe + " " + args;
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
      cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" <<   endl;
      cout << "COMMANDS:" <<   endl;
      cout << "  exit   - Stop monitoring (requires password)" <<   endl;
      cout << "  status - Check client status" <<   endl;
      cout << "  show   - Show client console (if hidden)" <<   endl;
      cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" <<   endl;
    
      string input;
    while (monitoring) {
          cout << "> ";
          getline(  cin, input);
        
        if (input == "exit" || input == "quit") {
              cout << "\n🔐 Password authentication required..." <<   endl;
            
            if (PasswordDialogWindows::showPasswordDialog()) {
                  cout << "✅ Password correct - Stopping monitoring..." <<   endl;
                exit_authorized = true;
                monitoring = false;
                break;
            } else {
                  cout << "❌ INCORRECT PASSWORD!" <<   endl;
                  cout << "   Monitoring continues...\n" <<   endl;
            }
        } else if (input == "status") {
            if (clientProcess.hProcess) {
                DWORD exitCode;
                GetExitCodeProcess(clientProcess.hProcess, &exitCode);
                if (exitCode == STILL_ACTIVE) {
                      cout << "✅ Client is running (PID: " << clientProcess.dwProcessId << ")" <<   endl;
                      cout << "🔒 Background monitoring: ACTIVE" <<   endl;
                } else {
                      cout << "⚠️  Client is not running" <<   endl;
                }
            }
        } else if (input == "show") {
              cout << "ℹ️  Client runs in background - no console to show" <<   endl;
        } else if (!input.empty()) {
              cout << "Unknown command. Type 'exit', 'status', or 'show'." <<   endl;
        }
    }
}

void monitorClient(const   string& server_ip, const   string& port, 
                   const   string& client_id) {
    int restartCount = 0;
    const int MAX_RESTARTS = 100;  // High number = keep trying
    
      string client_exe = "client.exe";
      string client_args = server_ip + " " + port + " " + client_id + " --guardian";
    
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
                  cout << "\n⚠️  CLIENT TERMINATED! (Exit code: " << exitCode << ")" <<   endl;
                
                if (restartCount < MAX_RESTARTS) {
                      cout << "🔄 Restarting in 3 seconds... (attempt " 
                              << (restartCount + 1) << "/" << MAX_RESTARTS << ")" <<   endl;
                    
                      this_thread::sleep_for(  chrono::seconds(3));
                    
                    if (launchClientHidden(client_exe, client_args)) {
                          cout << "✅ Client restarted (PID: " << clientProcess.dwProcessId << ")" <<   endl;
                        restartCount++;
                    } else {
                          cout << "❌ Failed to restart client" <<   endl;
                          this_thread::sleep_for(  chrono::seconds(5));
                    }
                } else {
                      cout << "❌ Maximum restart attempts reached" <<   endl;
                    monitoring = false;
                    break;
                }
            } else {
                  cout << "✅ Authorized exit" <<   endl;
                break;
            }
        }
    }
    
    // Final cleanup
    if (clientProcess.hProcess) {
          cout << "🛑 Terminating client..." <<   endl;
        TerminateProcess(clientProcess.hProcess, 0);
        WaitForSingleObject(clientProcess.hProcess, 2000);
        CloseHandle(clientProcess.hProcess);
        CloseHandle(clientProcess.hThread);
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    
    if (argc != 4) {
          cout << "Usage: launcher.exe <server_ip> <port> <client_id>" <<   endl;
          cout << "Example: launcher.exe 192.168.100.8 8080 bscs24009" <<   endl;
        return 1;
    }
    
      string server_ip = argv[1];
      string port = argv[2];
      string client_id = argv[3];
    
      cout << "\n🛡️  PROTECTED CLIENT LAUNCHER (Background Mode)" <<   endl;
      cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" <<   endl;
      cout << "Server: " << server_ip << ":" << port <<   endl;
      cout << "Client ID: " << client_id <<   endl;
    
      string client_exe = "client.exe";
      string client_args = server_ip + " " + port + " " + client_id + " --guardian";
    
      cout << "\n⏳ Launching client in background..." <<   endl;
    
    if (!launchClientHidden(client_exe, client_args)) {
          cerr << "❌ Failed to launch client!" <<   endl;
          cout << "Press any key to exit...";
        _getch();
        return 1;
    }
    
      cout << "✅ Client launched (PID: " << clientProcess.dwProcessId << ")" <<   endl;
      cout << "✅ Running in background (hidden console)" <<   endl;
    
      cout << "\n🔒 PROTECTION ACTIVE:" <<   endl;
      cout << "   ✓ Client runs in background (no visible window)" <<   endl;
      cout << "   ✓ Auto-restarts on crash (up to 100 times)" <<   endl;
      cout << "   ✓ Password required to stop: 'exit' command" <<   endl;
      cout << "   ✓ Alternative: Run guardian_exit.exe" <<   endl;
    
    // Start input handler
      thread inputHandler(inputThread);
    
    // Monitor the client
    monitorClient(server_ip, port, client_id);
    
    // Cleanup
    monitoring = false;
    if (inputHandler.joinable()) {
        inputHandler.join();
    }
    
      cout << "\n✅ Guardian shutdown complete" <<   endl;
    return 0;
}

#endif // PLATFORM_WINDOWS