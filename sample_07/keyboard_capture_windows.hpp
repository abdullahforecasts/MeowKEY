#ifndef KEYBOARD_CAPTURE_WINDOWS_HPP
#define KEYBOARD_CAPTURE_WINDOWS_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <iostream>
using namespace std; 
class KeyboardCaptureWindows {
private:
      atomic<bool> capturing_;
      thread capture_thread_;
      function<void(const   string&)> callback_;
    static KeyboardCaptureWindows* instance_;
    static HHOOK hook_handle_;

public:
    KeyboardCaptureWindows(  function<void(const   string&)> callback) 
        : capturing_(false), callback_(callback) {
        instance_ = this;
    }

    ~KeyboardCaptureWindows() {
        stop();
        instance_ = nullptr;
    }

    bool start() {
        if (capturing_) {
            return false;
        }

        capturing_ = true;
        capture_thread_ =   thread(&KeyboardCaptureWindows::captureLoop, this);
        return true;
    }

    void stop() {
        capturing_ = false;
        if (hook_handle_) {
            UnhookWindowsHookEx(hook_handle_);
            hook_handle_ = nullptr;
        }
        if (capture_thread_.joinable()) {
            capture_thread_.join();
        }
    }

    bool isCapturing() const {
        return capturing_;
    }

      string getKeyboardDevice() const {
        return "Windows Keyboard Hook";
    }

private:
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && instance_) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
                  string key_name = instance_->vkCodeToString(pKeyboard->vkCode);
                
                if (!key_name.empty() && instance_->callback_) {
                      cout << "Captured key: " << key_name <<   endl;
                    instance_->callback_(key_name);
                }
            }
        }
        return CallNextHookEx(hook_handle_, nCode, wParam, lParam);
    }

    void captureLoop() {
          cout << "Starting Windows keyboard capture..." <<   endl;
         
        hook_handle_ = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
        if (!hook_handle_) {
              cerr << "Failed to install keyboard hook!" <<   endl;
            return;
        }

          cout << "✓ Keyboard hook installed. Capturing keystrokes..." <<   endl;

        MSG msg;
        while (capturing_ && GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

      string vkCodeToString(DWORD vkCode) {
        static const   unordered_map<DWORD,   string> key_map = {
            {'A', "A"}, {'B', "B"}, {'C', "C"}, {'D', "D"}, {'E', "E"},
            {'F', "F"}, {'G', "G"}, {'H', "H"}, {'I', "I"}, {'J', "J"},
            {'K', "K"}, {'L', "L"}, {'M', "M"}, {'N', "N"}, {'O', "O"},
            {'P', "P"}, {'Q', "Q"}, {'R', "R"}, {'S', "S"}, {'T', "T"},
            {'U', "U"}, {'V', "V"}, {'W', "W"}, {'X', "X"}, {'Y', "Y"},
            {'Z', "Z"},
            {'0', "0"}, {'1', "1"}, {'2', "2"}, {'3', "3"}, {'4', "4"},
            {'5', "5"}, {'6', "6"}, {'7', "7"}, {'8', "8"}, {'9', "9"},
            {VK_SPACE, "SPACE"}, {VK_RETURN, "ENTER"}, {VK_BACK, "BACKSPACE"},
            {VK_TAB, "TAB"}, {VK_ESCAPE, "ESC"}, {VK_CONTROL, "CTRL"},
            {VK_SHIFT, "SHIFT"}, {VK_MENU, "ALT"},
            {VK_OEM_COMMA, ","}, {VK_OEM_PERIOD, "."}, {VK_OEM_2, "/"}, 
            {VK_OEM_1, ";"}, {VK_OEM_7, "'"}, {VK_OEM_4, "["}, 
            {VK_OEM_6, "]"}, {VK_OEM_5, "\\"}, {VK_OEM_MINUS, "-"}, 
            {VK_OEM_PLUS, "="}, {VK_OEM_3, "`"},
            {VK_UP, "UP"}, {VK_DOWN, "DOWN"}, {VK_LEFT, "LEFT"}, {VK_RIGHT, "RIGHT"},
            {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"},
            {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"},
            {VK_F9, "F9"}, {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"}
        };

        auto it = key_map.find(vkCode);
        if (it != key_map.end()) {
            return it->second;
        }
        return "KEY_" +   to_string(vkCode);
    }
};

KeyboardCaptureWindows* KeyboardCaptureWindows::instance_ = nullptr;
HHOOK KeyboardCaptureWindows::hook_handle_ = nullptr;

#endif // PLATFORM_WINDOWS
#endif