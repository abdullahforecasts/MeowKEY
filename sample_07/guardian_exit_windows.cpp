#ifdef _WIN32

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
using namespace std; 
  string getPasswordInput() {
      string password;
    char ch;
    
    while (true) {
        ch = _getch();
        
        if (ch == '\r' || ch == '\n') { // Enter key
            break;
        } else if (ch == '\b' || ch == 127) { // Backspace
            if (!password.empty()) {
                password.pop_back();
                  cout << "\b \b"; // Erase character from display
            }
        } else if (ch >= 32 && ch <= 126) { // Printable characters only
            password += ch;
              cout << '*'; // Show asterisk
        }
    }
    
    return password;
}

  vector<DWORD> findAllProcesses() {
      vector<DWORD> pids;
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return pids;
    }
    
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &processEntry)) {
        do {
              string exeName = processEntry.szExeFile;
            
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
      cout << "\n╔════════════════════════════════════════╗" <<   endl;
      cout << "║     GUARDIAN EXIT TOOL (Windows)      ║" <<   endl;
      cout << "╚════════════════════════════════════════╝\n" <<   endl;
    
    // Find all related processes
      vector<DWORD> pids = findAllProcesses();
    
    if (pids.empty()) {
          cerr << "❌ No launcher or client processes found!" <<   endl;
          cerr << "   Make sure the protected client is running." <<   endl;
          cout << "\nPress any key to exit...";
        _getch();
        return 1;
    }
    
      cout << "✓ Found " << pids.size() << " process(es) to terminate:" <<   endl;
    for (DWORD pid : pids) {
          cout << "  - PID: " << pid <<   endl;
    }
    
      cout << "\n🔐 Enter password to stop monitoring: ";
    
      string password = getPasswordInput();
      cout <<   endl;
    
    // Hardcoded password check (CHANGE THIS to match launcher_common.hpp!)
    if (password == "abd101") {
          cout << "\n✅ Password correct - Sending termination signal..." <<   endl;
        
        int killed_count = 0;
        int failed_count = 0;
        
        for (DWORD pid : pids) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProcess != NULL) {
                if (TerminateProcess(hProcess, 0)) {
                      cout << "✅ Terminated process PID: " << pid <<   endl;
                    killed_count++;
                } else {
                      cerr << "⚠️  Failed to terminate PID " << pid << ": Error " << GetLastError() <<   endl;
                    failed_count++;
                }
                CloseHandle(hProcess);
            } else {
                  cerr << "⚠️  Failed to open PID " << pid << ": Error " << GetLastError() <<   endl;
                failed_count++;
            }
        }
        
          cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" <<   endl;
          cout << "✅ Successfully killed: " << killed_count << " process(es)" <<   endl;
        if (failed_count > 0) {
              cout << "⚠️  Failed to kill: " << failed_count << " process(es)" <<   endl;
              cout << "   Try running as Administrator" <<   endl;
        }
          cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" <<   endl;
        
          cout << "Press any key to exit...";
        _getch();
        return (failed_count > 0) ? 1 : 0;
    } else {
          cout << "\n❌ INCORRECT PASSWORD!" <<   endl;
          cout << "   Exit denied. Monitoring continues..." <<   endl;
          cout << "\nPress any key to exit...";
        _getch();
        return 1;
    }
}

#endif // _WIN32