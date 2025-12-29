#ifndef WINDOW_PROTECTOR_HPP
#define WINDOW_PROTECTOR_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <iostream>
#include <atomic>
#include <thread>
#include <unordered_set>
#include "password_dialog_windows.hpp"
using namespace std; 

class WindowProtector {
private: 
    DWORD targetPID;
      atomic<bool> protecting_;
      thread protectionThread_;
      unordered_set<HWND> hookedWindows_;
    HHOOK keyboardHook_;
    static WindowProtector* instance_;
    static   atomic<bool>* exit_authorized_ptr_;
    
    // Store original window procedures
    static   unordered_map<HWND, WNDPROC> originalProcs_;
    
public:
    WindowProtector(DWORD pid,   atomic<bool>& exit_auth) 
        : targetPID(pid), protecting_(false), keyboardHook_(NULL) {
        instance_ = this;
        exit_authorized_ptr_ = &exit_auth;
    }
    
    ~WindowProtector() {
        stop();
        instance_ = nullptr;
    }
    
    bool start() {
        if (protecting_) return false;
        
        protecting_ = true;
        
        // Install global keyboard hook for Alt+F4
        keyboardHook_ = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardHookProc, 
                                          GetModuleHandleA(NULL), 0);
        
        if (!keyboardHook_) {
              cerr << "⚠️  Failed to install keyboard hook" <<   endl;
        }
        
        // Start window monitoring thread
        protectionThread_ =   thread(&WindowProtector::protectionLoop, this);
        
          cout << "🛡️  Window protection active - monitoring PID " << targetPID <<   endl;
        return true;
    }
    
    void stop() {
        protecting_ = false;
        
        if (keyboardHook_) {
            UnhookWindowsHookEx(keyboardHook_);
            keyboardHook_ = NULL;
        }
        
        // Restore original window procedures
        for (auto& pair : originalProcs_) {
            SetWindowLongPtrA(pair.first, GWLP_WNDPROC, (LONG_PTR)pair.second);
        }
        originalProcs_.clear();
        
        if (protectionThread_.joinable()) {
            protectionThread_.join();
        }
    }
    
private:
    void protectionLoop() {
        while (protecting_) {
            // Find all windows belonging to target process
            findAndHookWindows();
              this_thread::sleep_for(  chrono::milliseconds(500));
        }
    }
    
    void findAndHookWindows() {
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            WindowProtector* self = (WindowProtector*)lParam;
            
            DWORD windowPID;
            GetWindowThreadProcessId(hwnd, &windowPID);
            
            if (windowPID == self->targetPID) {
                // Check if this window is already hooked
                if (self->hookedWindows_.find(hwnd) == self->hookedWindows_.end()) {
                    self->hookWindow(hwnd);
                }
            }
            
            return TRUE;
        }, (LPARAM)this);
    }
    
    void hookWindow(HWND hwnd) {
        // Only hook top-level visible windows
        if (!IsWindowVisible(hwnd)) return;
        if (GetWindow(hwnd, GW_OWNER) != NULL) return;
        
        char className[256];
        GetClassNameA(hwnd, className, sizeof(className));
        
        // Skip system windows
        if (strcmp(className, "IME") == 0) return;
        if (strcmp(className, "MSCTFIME UI") == 0) return;
        
        // Subclass the window
        WNDPROC originalProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
        
        if (originalProc && originalProc != ProtectedWindowProc) {
            originalProcs_[hwnd] = originalProc;
            SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)ProtectedWindowProc);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)this);
            
            hookedWindows_.insert(hwnd);
            
            char title[256];
            GetWindowTextA(hwnd, title, sizeof(title));
              cout << "🔒 Protected window: " << (title[0] ? title : className) 
                      << " (HWND: " << hwnd << ")" <<   endl;
        }
    }
    
    static LRESULT CALLBACK ProtectedWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Get original procedure
        WNDPROC originalProc = nullptr;
        auto it = originalProcs_.find(hwnd);
        if (it != originalProcs_.end()) {
            originalProc = it->second;
        }
        
        // Intercept close messages
        if (msg == WM_CLOSE) {
              cout << "\n🚨 Window close attempt intercepted!" <<   endl;
            
            if (instance_ && exit_authorized_ptr_) {
                if (instance_->handleCloseAttempt(hwnd)) {
                    // Password correct - allow close
                    if (originalProc) {
                        return CallWindowProcA(originalProc, hwnd, msg, wParam, lParam);
                    }
                } else {
                    // Password incorrect - block close
                    return 0;
                }
            }
            return 0;
        }
        
        // Intercept system commands (Alt+F4, etc.)
        if (msg == WM_SYSCOMMAND) {
            if (wParam == SC_CLOSE) {
                  cout << "\n🚨 System close command intercepted!" <<   endl;
                
                if (instance_ && exit_authorized_ptr_) {
                    if (!instance_->handleCloseAttempt(hwnd)) {
                        return 0; // Block
                    }
                }
            }
        }
        
        // Call original window procedure
        if (originalProc) {
            return CallWindowProcA(originalProc, hwnd, msg, wParam, lParam);
        }
        
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    
    bool handleCloseAttempt(HWND hwnd) {
        char title[256];
        GetWindowTextA(hwnd, title, sizeof(title));
        
          cout << "🔐 Password required to close: " << title <<   endl;
        
        if (PasswordDialogWindows::showPasswordDialog()) {
              cout << "✅ Password correct - Close authorized" <<   endl;
            
            if (exit_authorized_ptr_) {
                *exit_authorized_ptr_ = true;
            }
            
            return true;
        } else {
              cout << "❌ Incorrect password - Close blocked\n" <<   endl;
            return false;
        }
    }
    
    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && instance_) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;
                
                // Check for Alt+F4
                if (kbd->vkCode == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
                    // Check if the foreground window belongs to our process
                    HWND fgWindow = GetForegroundWindow();
                    DWORD fgPID;
                    GetWindowThreadProcessId(fgWindow, &fgPID);
                    
                    if (fgPID == instance_->targetPID) {
                          cout << "\n🚨 Alt+F4 intercepted!" <<   endl;
                        
                        if (!instance_->handleCloseAttempt(fgWindow)) {
                            return 1; // Block Alt+F4
                        }
                    }
                }
            }
        }
        
        return CallNextHookEx(instance_->keyboardHook_, nCode, wParam, lParam);
    }
};

WindowProtector* WindowProtector::instance_ = nullptr;
  atomic<bool>* WindowProtector::exit_authorized_ptr_ = nullptr;
  unordered_map<HWND, WNDPROC> WindowProtector::originalProcs_;

#endif // PLATFORM_WINDOWS
#endif