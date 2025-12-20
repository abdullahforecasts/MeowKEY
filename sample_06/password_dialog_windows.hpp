#ifndef PASSWORD_DIALOG_WINDOWS_HPP
#define PASSWORD_DIALOG_WINDOWS_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <iostream>
#include <conio.h>
#include "launcher_common.hpp"
using namespace std; 
class PasswordDialogWindows {
public:
    static bool showPasswordDialog() {
          cout << "\n╔════════════════════════════════════════╗" <<   endl;
          cout << "║  EXIT AUTHORIZATION REQUIRED          ║" <<   endl;
          cout << "╚════════════════════════════════════════╝" <<   endl;
          cout << "Enter password to stop monitoring: ";
        
          string password = getPasswordInput();
          cout <<   endl;
        
        return verifyPassword(password);
    }
    
private:
    static   string getPasswordInput() {
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
};

#endif // PLATFORM_WINDOWS
#endif