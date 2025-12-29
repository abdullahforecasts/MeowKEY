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
using namespace std;
class WindowMonitor
{
private:
    atomic<bool> monitoring_;
    thread monitor_thread_;
    function<void(const string &)> callback_;
    Display *display_;
    Window root_window_;
    string last_window_title_;
    static constexpr int CHECK_INTERVAL_MS = 1000; // Check every second

public:
    WindowMonitor(function<void(const string &)> callback)
        : monitoring_(false), callback_(callback), display_(nullptr), root_window_(0) {}

    ~WindowMonitor()
    {
        stop();
    }

    bool start()
    {
        if (monitoring_)
        {
            return false;
        }

        // Initialize X11 display
        display_ = XOpenDisplay(nullptr);
        if (!display_)
        {
            cerr << "Failed to open X11 display for window monitoring." << endl;
            return false;
        }

        root_window_ = DefaultRootWindow(display_);
        monitoring_ = true;
        monitor_thread_ = thread(&WindowMonitor::monitorLoop, this);

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
        if (display_)
        {
            XCloseDisplay(display_);
            display_ = nullptr;
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
        // Get initial window
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
        if (!display_)
        {
            return "";
        }

        Window focused_window;
        int revert_to;

        // Get currently focused window
        if (XGetInputFocus(display_, &focused_window, &revert_to) == 0)
        {
            return "";
        }

        // If we get the root window, try another method
        if (focused_window == root_window_)
        {
            // Use _NET_ACTIVE_WINDOW property
            Atom active_window_atom = XInternAtom(display_, "_NET_ACTIVE_WINDOW", False);
            Atom type;
            int format;
            unsigned long nitems, bytes_after;
            unsigned char *data = nullptr;

            if (XGetWindowProperty(display_, root_window_, active_window_atom,
                                   0, 1024, False, XA_WINDOW,
                                   &type, &format, &nitems, &bytes_after,
                                   &data) == Success &&
                data)
            {
                if (nitems > 0)
                {
                    focused_window = *((Window *)data);
                }
                XFree(data);
            }
        }

        if (focused_window == None || focused_window == root_window_)
        {
            return "Unknown Window";
        }

        // Get window title using _NET_WM_NAME (UTF-8)
        string title = getWindowProperty(focused_window, "_NET_WM_NAME");
        if (title.empty())
        {
            // Fallback to WM_NAME (legacy)
            title = getWindowProperty(focused_window, "WM_NAME");
        }

        // Get process name if available
        string process_name = getWindowProcessName(focused_window);

        string result;
        if (!title.empty())
        {
            result = title;
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
            result = "Window " + to_string((unsigned long)focused_window);
        }

        // Clean up the string
        result.erase(remove_if(result.begin(), result.end(),
                               [](char c)
                               {
                                   return (c < 32 && c != '\n' && c != '\t') || c == 127;
                               }),
                     result.end());

        return result;
    }

    string getWindowProperty(Window window, const char *property_name)
    {
        Atom property_atom = XInternAtom(display_, property_name, False);
        Atom type;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data = nullptr;

        string result;

        if (XGetWindowProperty(display_, window, property_atom,
                               0, 1024, False, AnyPropertyType,
                               &type, &format, &nitems, &bytes_after,
                               &data) == Success &&
            data && nitems > 0)
        {

            if (format == 8)
            { // String format
                result = string(reinterpret_cast<char *>(data), nitems);
            }

            XFree(data);
        }

        return result;
    }

    string getWindowProcessName(Window window)
    {
        // Try to get process ID and name using _NET_WM_PID
        Atom pid_atom = XInternAtom(display_, "_NET_WM_PID", False);
        Atom type;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data = nullptr;

        string process_name;

        if (XGetWindowProperty(display_, window, pid_atom,
                               0, 1, False, XA_CARDINAL,
                               &type, &format, &nitems, &bytes_after,
                               &data) == Success &&
            data && nitems > 0)
        {

            if (format == 32)
            {
                pid_t pid = *((pid_t *)data);

                // Try to get process name from /proc
                string proc_path = "/proc/" + to_string(pid) + "/comm";
                ifstream proc_file(proc_path);
                if (proc_file)
                {
                    getline(proc_file, process_name);
                }
            }

            XFree(data);
        }

        return process_name;
    }
};

#endif