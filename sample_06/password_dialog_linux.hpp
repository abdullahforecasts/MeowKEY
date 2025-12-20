#ifndef PASSWORD_DIALOG_LINUX_HPP
#define PASSWORD_DIALOG_LINUX_HPP

#ifdef PLATFORM_LINUX

#include <string>
#include <iostream>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include "launcher_common.hpp"
using namespace std; 
class PasswordDialogLinux {
public:
    static bool showPasswordDialog() {
        // Try zenity first (GUI dialog)
        if (system("which zenity > /dev/null 2>&1") == 0) {
            return showZenityDialog();
        }
        
        // Fallback to terminal prompt
        return showTerminalPrompt();
    }
    
private:
    static bool showZenityDialog() {
          string command = "zenity --password --title=\"Exit Authorization\" "
                             "--text=\"Enter password to stop monitoring:\" 2>/dev/null";
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return showTerminalPrompt();
        }
        
        char buffer[256];
          string password;
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            password = buffer;
            // Remove newline
            if (!password.empty() && password[password.length()-1] == '\n') {
                password.erase(password.length()-1);
            }
        }
        
        pclose(pipe);
        return verifyPassword(password);
    }
    
    static bool showTerminalPrompt() {
          cout << "\n╔════════════════════════════════════════╗" <<   endl;
          cout << "║  EXIT AUTHORIZATION REQUIRED          ║" <<   endl;
          cout << "╚════════════════════════════════════════╝" <<   endl;
          cout << "Enter password to stop monitoring: ";
        
          string password = getPasswordInput();
          cout <<   endl;
        
        return verifyPassword(password);
    }
    
    static   string getPasswordInput() {
          string password;
        
        // Disable echo
        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        
        // Read password
          getline(  cin, password);
        
        // Restore echo
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        
        return password;
    }
};

#endif // PLATFORM_LINUX
#endif