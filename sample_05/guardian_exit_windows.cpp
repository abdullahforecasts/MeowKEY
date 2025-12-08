#ifdef _WIN32

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <conio.h>

std::string getPasswordInput() {
    std::string password;
    char ch;
    
    while (true) {
        ch = _getch();
        
        if (ch == '\r' || ch == '\n') { // Enter key
            break;
        } else if (ch == '\b' || ch == 127) { // Backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b"; // Erase character from display
            }
        } else if (ch >= 32 && ch <= 126) { // Printable characters only
            password += ch;
            std::cout << '*'; // Show asterisk
        }
    }
    
    return password;
}

std::vector<DWORD> findAllProcesses() {
    std::vector<DWORD> pids;
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return pids;
    }
    
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &processEntry)) {
        do {
            std::string exeName = processEntry.szExeFile;
            
            // Find launcher.exe and client.exe processes
            if (exeName == "launcher.exe" || exeName == "client.exe") {
                pids.push_back(processEntry.th32ProcessID);
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    
    CloseHandle(snapshot);
    return pids;
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║     GUARDIAN EXIT TOOL (Windows)      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝\n" << std::endl;
    
    // Find all related processes
    std::vector<DWORD> pids = findAllProcesses();
    
    if (pids.empty()) {
        std::cerr << "❌ No launcher or client processes found!" << std::endl;
        std::cerr << "   Make sure the protected client is running." << std::endl;
        std::cout << "\nPress any key to exit...";
        _getch();
        return 1;
    }
    
    std::cout << "✓ Found " << pids.size() << " process(es) to terminate:" << std::endl;
    for (DWORD pid : pids) {
        std::cout << "  - PID: " << pid << std::endl;
    }
    
    std::cout << "\n🔐 Enter password to stop monitoring: ";
    
    std::string password = getPasswordInput();
    std::cout << std::endl;
    
    // Hardcoded password check (CHANGE THIS to match launcher_common.hpp!)
    if (password == "abd101") {
        std::cout << "\n✅ Password correct - Sending termination signal..." << std::endl;
        
        int killed_count = 0;
        int failed_count = 0;
        
        for (DWORD pid : pids) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProcess != NULL) {
                if (TerminateProcess(hProcess, 0)) {
                    std::cout << "✅ Terminated process PID: " << pid << std::endl;
                    killed_count++;
                } else {
                    std::cerr << "⚠️  Failed to terminate PID " << pid << ": Error " << GetLastError() << std::endl;
                    failed_count++;
                }
                CloseHandle(hProcess);
            } else {
                std::cerr << "⚠️  Failed to open PID " << pid << ": Error " << GetLastError() << std::endl;
                failed_count++;
            }
        }
        
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "✅ Successfully killed: " << killed_count << " process(es)" << std::endl;
        if (failed_count > 0) {
            std::cout << "⚠️  Failed to kill: " << failed_count << " process(es)" << std::endl;
            std::cout << "   Try running as Administrator" << std::endl;
        }
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
        
        std::cout << "Press any key to exit...";
        _getch();
        return (failed_count > 0) ? 1 : 0;
    } else {
        std::cout << "\n❌ INCORRECT PASSWORD!" << std::endl;
        std::cout << "   Exit denied. Monitoring continues..." << std::endl;
        std::cout << "\nPress any key to exit...";
        _getch();
        return 1;
    }
}

#endif // _WIN32