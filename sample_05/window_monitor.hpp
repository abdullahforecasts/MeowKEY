#ifndef WINDOW_MONITOR_HPP
#define WINDOW_MONITOR_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstring>
#include <fstream>
#include <algorithm>

class WindowMonitor {
private:
    std::atomic<bool> monitoring_;
    std::thread monitor_thread_;
    std::function<void(const std::string&)> callback_;
    Display* display_;
    Window root_window_;
    std::string last_window_title_;
    static constexpr int CHECK_INTERVAL_MS = 1000; // Check every second

public:
    WindowMonitor(std::function<void(const std::string&)> callback) 
        : monitoring_(false), callback_(callback), display_(nullptr), root_window_(0) {}

    ~WindowMonitor() {
        stop();
    }

    bool start() {
        if (monitoring_) {
            return false;
        }

        // Initialize X11 display
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            std::cerr << "Failed to open X11 display for window monitoring." << std::endl;
            return false;
        }

        root_window_ = DefaultRootWindow(display_);
        monitoring_ = true;
        monitor_thread_ = std::thread(&WindowMonitor::monitorLoop, this);
        
        std::cout << "Window monitoring started" << std::endl;
        return true;
    }

    void stop() {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        if (display_) {
            XCloseDisplay(display_);
            display_ = nullptr;
        }
        std::cout << "Window monitoring stopped" << std::endl;
    }

    bool isMonitoring() const {
        return monitoring_;
    }

private:
    void monitorLoop() {
        // Get initial window
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
        if (!display_) {
            return "";
        }

        Window focused_window;
        int revert_to;
        
        // Get currently focused window
        if (XGetInputFocus(display_, &focused_window, &revert_to) == 0) {
            return "";
        }

        // If we get the root window, try another method
        if (focused_window == root_window_) {
            // Use _NET_ACTIVE_WINDOW property
            Atom active_window_atom = XInternAtom(display_, "_NET_ACTIVE_WINDOW", False);
            Atom type;
            int format;
            unsigned long nitems, bytes_after;
            unsigned char* data = nullptr;
            
            if (XGetWindowProperty(display_, root_window_, active_window_atom,
                                 0, 1024, False, XA_WINDOW,
                                 &type, &format, &nitems, &bytes_after,
                                 &data) == Success && data) {
                if (nitems > 0) {
                    focused_window = *((Window*)data);
                }
                XFree(data);
            }
        }

        if (focused_window == None || focused_window == root_window_) {
            return "Unknown Window";
        }

        // Get window title using _NET_WM_NAME (UTF-8)
        std::string title = getWindowProperty(focused_window, "_NET_WM_NAME");
        if (title.empty()) {
            // Fallback to WM_NAME (legacy)
            title = getWindowProperty(focused_window, "WM_NAME");
        }

        // Get process name if available
        std::string process_name = getWindowProcessName(focused_window);
        
        std::string result;
        if (!title.empty()) {
            result = title;
            if (!process_name.empty()) {
                result += " [" + process_name + "]";
            }
        } else if (!process_name.empty()) {
            result = process_name;
        } else {
            result = "Window " + std::to_string((unsigned long)focused_window);
        }

        // Clean up the string
        result.erase(std::remove_if(result.begin(), result.end(),
            [](char c) { 
                return (c < 32 && c != '\n' && c != '\t') || c == 127; 
            }), result.end());

        return result;
    }

    std::string getWindowProperty(Window window, const char* property_name) {
        Atom property_atom = XInternAtom(display_, property_name, False);
        Atom type;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char* data = nullptr;
        
        std::string result;
        
        if (XGetWindowProperty(display_, window, property_atom,
                             0, 1024, False, AnyPropertyType,
                             &type, &format, &nitems, &bytes_after,
                             &data) == Success && data && nitems > 0) {
            
            if (format == 8) { // String format
                result = std::string(reinterpret_cast<char*>(data), nitems);
            }
            
            XFree(data);
        }
        
        return result;
    }

    std::string getWindowProcessName(Window window) {
        // Try to get process ID and name using _NET_WM_PID
        Atom pid_atom = XInternAtom(display_, "_NET_WM_PID", False);
        Atom type;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char* data = nullptr;
        
        std::string process_name;
        
        if (XGetWindowProperty(display_, window, pid_atom,
                             0, 1, False, XA_CARDINAL,
                             &type, &format, &nitems, &bytes_after,
                             &data) == Success && data && nitems > 0) {
            
            if (format == 32) {
                pid_t pid = *((pid_t*)data);
                
                // Try to get process name from /proc
                std::string proc_path = "/proc/" + std::to_string(pid) + "/comm";
                std::ifstream proc_file(proc_path);
                if (proc_file) {
                    std::getline(proc_file, process_name);
                }
            }
            
            XFree(data);
        }
        
        return process_name;
    }
};

#endif