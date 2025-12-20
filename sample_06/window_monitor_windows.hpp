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
using namespace std;
#pragma comment(lib, "psapi.lib")

class WindowMonitorWindows
{
private:
    atomic<bool> monitoring_;
    thread monitor_thread_;
    function<void(const string &)> callback_;
    string last_window_title_;
    static constexpr int CHECK_INTERVAL_MS = 1000;

public:
    WindowMonitorWindows(function<void(const string &)> callback)
        : monitoring_(false), callback_(callback) {}

    ~WindowMonitorWindows()
    {
        stop();
    }

    bool start()
    {
        if (monitoring_)
        {
            return false;
        }

        monitoring_ = true;
        monitor_thread_ = thread(&WindowMonitorWindows::monitorLoop, this);

        cout << "Window monitoring started" << endl;
        return true;
    }

    void stop()
    {
        monitoring_ = false;
        if (monitor_thread_.joinable())
        {
            monitor_thread_.join();
        }
        cout << "Window monitoring stopped" << endl;
    }

    bool isMonitoring() const
    {
        return monitoring_;
    }

private:
    void monitorLoop()
    {
        string initial_window = getActiveWindowTitle();
        if (!initial_window.empty())
        {
            last_window_title_ = initial_window;
            if (callback_)
            {
                callback_("WINDOW: " + initial_window);
            }
        }

        while (monitoring_)
        {
            string current_window = getActiveWindowTitle();

            if (!current_window.empty() && current_window != last_window_title_)
            {
                cout << "Window changed to: " << current_window << endl;

                if (callback_)
                {
                    callback_("WINDOW: " + current_window);
                }

                last_window_title_ = current_window;
            }

            this_thread::sleep_for(chrono::milliseconds(CHECK_INTERVAL_MS));
        }
    }

    string getActiveWindowTitle()
    {
        HWND hwnd = GetForegroundWindow();
        if (hwnd == nullptr)
        {
            return "Unknown Window";
        }

        // Get window title
        wchar_t title[256];
        int len = GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));

        string window_title;
        if (len > 0)
        {
            int size = WideCharToMultiByte(CP_UTF8, 0, title, -1, nullptr, 0, nullptr, nullptr);
            if (size > 0)
            {
                char *buffer = new char[size];
                WideCharToMultiByte(CP_UTF8, 0, title, -1, buffer, size, nullptr, nullptr);
                window_title = string(buffer);
                delete[] buffer;
            }
        }

        // Get process name
        DWORD processId;
        GetWindowThreadProcessId(hwnd, &processId);

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        string process_name;

        if (hProcess)
        {
            wchar_t processPath[MAX_PATH];
            if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH))
            {
                // Extract just the filename
                wchar_t *fileName = wcsrchr(processPath, L'\\');
                if (fileName)
                {
                    fileName++;
                    int size = WideCharToMultiByte(CP_UTF8, 0, fileName, -1, nullptr, 0, nullptr, nullptr);
                    if (size > 0)
                    {
                        char *buffer = new char[size];
                        WideCharToMultiByte(CP_UTF8, 0, fileName, -1, buffer, size, nullptr, nullptr);
                        process_name = string(buffer);
                        delete[] buffer;
                    }
                }
            }
            CloseHandle(hProcess);
        }

        string result;
        if (!window_title.empty())
        {
            result = window_title;
            if (!process_name.empty())
            {
                result += " [" + process_name + "]";
            }
        }
        else if (!process_name.empty())
        {
            result = process_name;
        }
        else
        {
            result = "Window " + to_string(reinterpret_cast<uintptr_t>(hwnd));
        }

        return result;
    }
};

#endif // PLATFORM_WINDOWS
#endif