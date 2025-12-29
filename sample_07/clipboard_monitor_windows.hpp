#ifndef CLIPBOARD_MONITOR_WINDOWS_HPP
#define CLIPBOARD_MONITOR_WINDOWS_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <iostream>
using namespace std; 
class ClipboardMonitorWindows {
private:
      atomic<bool> monitoring_;
      thread monitor_thread_;
      function<void(const   string&)> callback_;
      string last_clipboard_content_;
    static const size_t MAX_CLIPBOARD_SIZE = 10240;

public:
    ClipboardMonitorWindows(  function<void(const   string&)> callback) 
        : monitoring_(false), callback_(callback) {}

    ~ClipboardMonitorWindows() {
        stop();
    }

    bool start() {
        if (monitoring_) {
            return false;
        }

        monitoring_ = true;
        monitor_thread_ =   thread(&ClipboardMonitorWindows::monitorLoop, this);
        
          cout << "Clipboard monitoring started" <<   endl;
        return true;
    }

    void stop() {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
          cout << "Clipboard monitoring stopped" <<   endl;
    }

    bool isMonitoring() const {
        return monitoring_;
    }

private:
    void monitorLoop() {
          string initial_content = getClipboardText();
        if (!initial_content.empty()) {
            last_clipboard_content_ = initial_content;
        }

        while (monitoring_) {
              string current_content = getClipboardText();
            
            if (!current_content.empty() && current_content != last_clipboard_content_) {
                  string display_content = current_content;
                if (display_content.length() > 100) {
                    display_content = display_content.substr(0, 100) + "...";
                }
                
                  cout << "Clipboard changed: " << display_content <<   endl;
                
                if (callback_) {
                    if (current_content.length() > MAX_CLIPBOARD_SIZE) {
                          string truncated = current_content.substr(0, MAX_CLIPBOARD_SIZE) + "...[TRUNCATED]";
                        callback_("CLIPBOARD: " + truncated);
                    } else {
                        callback_("CLIPBOARD: " + current_content);
                    }
                }
                
                last_clipboard_content_ = current_content;
            }
            
              this_thread::sleep_for(  chrono::milliseconds(500));
        }
    }

      string getClipboardText() {
        if (!OpenClipboard(nullptr)) {
            return "";
        }

          string result;
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData != nullptr) {
            wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
            if (pszText != nullptr) {
                // Convert wide string to narrow string
                int size = WideCharToMultiByte(CP_UTF8, 0, pszText, -1, nullptr, 0, nullptr, nullptr);
                if (size > 0) {
                    char* buffer = new char[size];
                    WideCharToMultiByte(CP_UTF8, 0, pszText, -1, buffer, size, nullptr, nullptr);
                    result =   string(buffer);
                    delete[] buffer;
                }
                GlobalUnlock(hData);
            }
        }

        CloseClipboard();
        return result;
    }
};

#endif // PLATFORM_WINDOWS
#endif