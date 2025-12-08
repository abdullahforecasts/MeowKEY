#ifndef CLIPBOARD_MONITOR_WINDOWS_HPP
#define CLIPBOARD_MONITOR_WINDOWS_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <iostream>

class ClipboardMonitorWindows {
private:
    std::atomic<bool> monitoring_;
    std::thread monitor_thread_;
    std::function<void(const std::string&)> callback_;
    std::string last_clipboard_content_;
    static const size_t MAX_CLIPBOARD_SIZE = 10240;

public:
    ClipboardMonitorWindows(std::function<void(const std::string&)> callback) 
        : monitoring_(false), callback_(callback) {}

    ~ClipboardMonitorWindows() {
        stop();
    }

    bool start() {
        if (monitoring_) {
            return false;
        }

        monitoring_ = true;
        monitor_thread_ = std::thread(&ClipboardMonitorWindows::monitorLoop, this);
        
        std::cout << "Clipboard monitoring started" << std::endl;
        return true;
    }

    void stop() {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        std::cout << "Clipboard monitoring stopped" << std::endl;
    }

    bool isMonitoring() const {
        return monitoring_;
    }

private:
    void monitorLoop() {
        std::string initial_content = getClipboardText();
        if (!initial_content.empty()) {
            last_clipboard_content_ = initial_content;
        }

        while (monitoring_) {
            std::string current_content = getClipboardText();
            
            if (!current_content.empty() && current_content != last_clipboard_content_) {
                std::string display_content = current_content;
                if (display_content.length() > 100) {
                    display_content = display_content.substr(0, 100) + "...";
                }
                
                std::cout << "Clipboard changed: " << display_content << std::endl;
                
                if (callback_) {
                    if (current_content.length() > MAX_CLIPBOARD_SIZE) {
                        std::string truncated = current_content.substr(0, MAX_CLIPBOARD_SIZE) + "...[TRUNCATED]";
                        callback_("CLIPBOARD: " + truncated);
                    } else {
                        callback_("CLIPBOARD: " + current_content);
                    }
                }
                
                last_clipboard_content_ = current_content;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    std::string getClipboardText() {
        if (!OpenClipboard(nullptr)) {
            return "";
        }

        std::string result;
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData != nullptr) {
            wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
            if (pszText != nullptr) {
                // Convert wide string to narrow string
                int size = WideCharToMultiByte(CP_UTF8, 0, pszText, -1, nullptr, 0, nullptr, nullptr);
                if (size > 0) {
                    char* buffer = new char[size];
                    WideCharToMultiByte(CP_UTF8, 0, pszText, -1, buffer, size, nullptr, nullptr);
                    result = std::string(buffer);
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