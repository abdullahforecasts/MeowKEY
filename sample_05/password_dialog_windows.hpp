#ifndef PASSWORD_DIALOG_WINDOWS_HPP
#define PASSWORD_DIALOG_WINDOWS_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <iostream>
#include <conio.h>
#include "launcher_common.hpp"

class PasswordDialogWindows {
public:
    static bool showPasswordDialog() {
        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║  EXIT AUTHORIZATION REQUIRED          ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        std::cout << "Enter password to stop monitoring: ";
        
        std::string password = getPasswordInput();
        std::cout << std::endl;
        
        return verifyPassword(password);
    }
    
private:
    static std::string getPasswordInput() {
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
};

#endif // PLATFORM_WINDOWS
#endif