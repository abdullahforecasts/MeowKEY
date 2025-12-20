#ifndef PROCESS_GUARD_WINDOWS_HPP
#define PROCESS_GUARD_WINDOWS_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <iostream>
#include <atomic>
#include "launcher_common.hpp"
#include "password_dialog_windows.hpp"
using namespace std; 

class ProcessGuardWindows {
private:
    PROCESS_INFORMATION processInfo;
    HANDLE jobObject;
      atomic<bool>& exit_authorized;
    static ProcessGuardWindows* instance_;
    
public:
    ProcessGuardWindows(  atomic<bool>& auth) 
        : processInfo{0}, jobObject(NULL), exit_authorized(auth) {
        instance_ = this;
        ZeroMemory(&processInfo, sizeof(processInfo));
    }
    
    ~ProcessGuardWindows() {
        cleanup();
        instance_ = nullptr;
    }
    
    bool launchClient(const   string& executable, const   string& args) {
          cout << "🛡️  Guardian: Launching protected client..." <<   endl;
        
        // Install console control handler for Ctrl+C, Ctrl+Break, Close
        if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
              cerr << "⚠️  Warning: Failed to install console handler" <<   endl;
        }
        
        // Create job object to tie child process lifecycle to parent
        jobObject = CreateJobObjectA(NULL, NULL);
        if (jobObject) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
            jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            
            SetInformationJobObject(jobObject, JobObjectExtendedLimitInformation,
                                   &jeli, sizeof(jeli));
        }
        
        // Build command line
          string cmdLine = executable + " " + args;
        char* cmdLineBuffer = new char[cmdLine.length() + 1];
        strcpy_s(cmdLineBuffer, cmdLine.length() + 1, cmdLine.c_str());
        
        // Create process WITHOUT new console - inherit parent's console
        STARTUPINFOA si = {0};
        si.cb = sizeof(si);
        
        BOOL success = CreateProcessA(
            NULL,
            cmdLineBuffer,
            NULL, NULL, 
            TRUE,  // Inherit handles
            0,     // No special flags - use parent's console
            NULL, NULL,
            &si, &processInfo
        );
        
        delete[] cmdLineBuffer;
        
        if (!success) {
            DWORD error = GetLastError();
              cerr << "❌ Failed to launch client. Error code: " << error <<   endl;
            return false;
        }
        
        // Assign to job object
        if (jobObject) {
            if (!AssignProcessToJobObject(jobObject, processInfo.hProcess)) {
                  cerr << "⚠️  Warning: Failed to assign process to job object" <<   endl;
            }
        }
        
          cout << "✅ Client launched successfully under protection (PID: " 
                  << processInfo.dwProcessId << ")" <<   endl;
          cout << "🔒 Exit protection active - password required to exit" <<   endl;
        
        return true;
    }
    
    void monitorProcess() {
          cout << "\n🔍 Guardian: Monitoring client process..." <<   endl;
          cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" <<   endl;
          cout << "Press Ctrl+C or close window to test exit protection" <<   endl;
          cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" <<   endl;
        
        // Wait for process to exit or authorization
        while (true) {
            DWORD waitResult = WaitForSingleObject(processInfo.hProcess, 1000);
            
            if (waitResult == WAIT_OBJECT_0) {
                // Process exited
                DWORD exitCode;
                GetExitCodeProcess(processInfo.hProcess, &exitCode);
                
                if (exit_authorized) {
                      cout << "✅ Authorized exit - Guardian shutting down" <<   endl;
                } else {
                      cout << "⚠️  Client process terminated unexpectedly!" <<   endl;
                      cout << "Exit code: " << exitCode <<   endl;
                }
                break;
            }
            
            // Check if we should exit
            if (exit_authorized) {
                  cout << "🛑 Terminating client process..." <<   endl;
                TerminateProcess(processInfo.hProcess, 0);
                WaitForSingleObject(processInfo.hProcess, 2000);
                  cout << "✅ Client terminated" <<   endl;
                break;
            }
        }
    }
    
private:
    void cleanup() {
        if (jobObject) {
            CloseHandle(jobObject);
            jobObject = NULL;
        }
        
        if (processInfo.hProcess) {
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
            ZeroMemory(&processInfo, sizeof(processInfo));
        }
        
        SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
    }
    
    static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
        const char* eventName = "UNKNOWN";
        
        switch (ctrlType) {
            case CTRL_C_EVENT:
                eventName = "Ctrl+C";
                break;
            case CTRL_BREAK_EVENT:
                eventName = "Ctrl+Break";
                break;
            case CTRL_CLOSE_EVENT:
                eventName = "Console Close";
                break;
            case CTRL_LOGOFF_EVENT:
                eventName = "User Logoff";
                break;
            case CTRL_SHUTDOWN_EVENT:
                eventName = "System Shutdown";
                break;
        }
        
          cout << "\n\n🚨 Exit attempt detected: " << eventName <<   endl;
        
        if (instance_) {
            return instance_->handleExitAttempt() ? TRUE : FALSE;
        }
        
        return FALSE; // Allow default handling if no instance
    }
    
    bool handleExitAttempt() {
          cout << "🔐 Password authentication required..." <<   endl;
        
        if (PasswordDialogWindows::showPasswordDialog()) {
              cout << "\n✅ Password correct - Exit authorized" <<   endl;
            exit_authorized = true;
            
            // Terminate child process
            if (processInfo.hProcess) {
                  cout << "🛑 Terminating client process..." <<   endl;
                TerminateProcess(processInfo.hProcess, 0);
                WaitForSingleObject(processInfo.hProcess, 2000);
            }
            
            // Exit the guardian
              cout << "✅ Guardian shutdown complete" <<   endl;
            ExitProcess(0);
            
            return false; // Should not reach here
        } else {
            showUnauthorizedMessage();
              cout << "🔄 Continue working... (Ctrl+C again to retry exit)\n" <<   endl;
            return true; // Block exit
        }
    }
};

ProcessGuardWindows* ProcessGuardWindows::instance_ = nullptr;

#endif // PLATFORM_WINDOWS
#endif