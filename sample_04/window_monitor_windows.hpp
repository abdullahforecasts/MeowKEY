#ifndef WINDOW_MONITOR_WINDOWS_HPP
#define WINDOW_MONITOR_WINDOWS_HPP

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <iostream>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

class WindowMonitorWindows {
private:
    std::atomic<bool> monitoring_;
    std::thread monitor_thread_;
    std::function<void(const std::string&)> callback_;
    std::string last_window_title_;
    static constexpr int CHECK_INTERVAL_MS = 1000;

public:
    WindowMonitorWindows(std::function<void(const std::string&)> callback) 
        : monitoring_(false), callback_(callback) {}

    ~WindowMonitorWindows() {
        stop();
    }

    bool start() {
        if (monitoring_) {
            return false;
        }

        monitoring_ = true;
        monitor_thread_ = std::thread(&WindowMonitorWindows::monitorLoop, this);
        
        std::cout << "Window monitoring started" << std::endl;
        return true;
    }

    void stop() {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        std::cout << "Window monitoring stopped" << std::endl;
    }

    bool isMonitoring() const {
        return monitoring_;
    }

private:
    void monitorLoop() {
        std::string initial_window = getActiveWindowTitle();
        if (!initial_window.empty()) {
            last_window_title_ = initial_window;
            if (callback_) {
                callback_("WINDOW: " + initial_window);
            }
        }

        while (monitoring_) {
            std::string current_window = getActiveWindowTitle();
            
            if (!current_window.empty() && current_window != last_window_title_) {
                std::cout << "Window changed to: " << current_window << std::endl;
                
                if (callback_) {
                    callback_("WINDOW: " + current_window);
                }
                
                last_window_title_ = current_window;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
        }
    }

    std::string getActiveWindowTitle() {
        HWND hwnd = GetForegroundWindow();
        if (hwnd == nullptr) {
            return "Unknown Window";
        }

        // Get window title
        wchar_t title[256];
        int len = GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));
        
        std::string window_title;
        if (len > 0) {
            int size = WideCharToMultiByte(CP_UTF8, 0, title, -1, nullptr, 0, nullptr, nullptr);
            if (size > 0) {
                char* buffer = new char[size];
                WideCharToMultiByte(CP_UTF8, 0, title, -1, buffer, size, nullptr, nullptr);
                window_title = std::string(buffer);
                delete[] buffer;
            }
        }

        // Get process name
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        std::string process_name;
        
        if (hProcess) {
            wchar_t processPath[MAX_PATH];
            if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
                // Extract just the filename
                wchar_t* fileName = wcsrchr(processPath, L'\\');
                if (fileName) {
                    fileName++;
                    int size = WideCharToMultiByte(CP_UTF8, 0, fileName, -1, nullptr, 0, nullptr, nullptr);
                    if (size > 0) {
                        char* buffer = new char[size];
                        WideCharToMultiByte(CP_UTF8, 0, fileName, -1, buffer, size, nullptr, nullptr);
                        process_name = std::string(buffer);
                        delete[] buffer;
                    }
                }
            }
            CloseHandle(hProcess);
        }

        std::string result;
        if (!window_title.empty()) {
            result = window_title;
            if (!process_name.empty()) {
                result += " [" + process_name + "]";
            }
        } else if (!process_name.empty()) {
            result = process_name;
        } else {
            result = "Window " + std::to_string(reinterpret_cast<uintptr_t>(hwnd));
        }

        return result;
    }
};

#endif // PLATFORM_WINDOWS
#endif