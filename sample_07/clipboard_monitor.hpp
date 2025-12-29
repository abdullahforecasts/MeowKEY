#ifndef CLIPBOARD_MONITOR_HPP
#define CLIPBOARD_MONITOR_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>  // ADDED: For XA_STRING and other atom definitions
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <cstring>
#include <algorithm>
using namespace std; 

class ClipboardMonitor {
private:
      atomic<bool> monitoring_;
      thread monitor_thread_;
      function<void(const   string&)> callback_;
    Display* display_;
    Window window_;
    Atom clipboard_atom_;
    Atom utf8_string_atom_;
    Atom text_plain_atom_;
      string last_clipboard_content_;
    static const size_t MAX_CLIPBOARD_SIZE = 10240; // 10KB limit

public:
    ClipboardMonitor(  function<void(const   string&)> callback) 
        : monitoring_(false), callback_(callback), display_(nullptr), window_(0) {}

    ~ClipboardMonitor() {
        stop();
    }

    bool start() {
        if (monitoring_) {
            return false;
        }

        // Initialize X11 display
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
              cerr << "Failed to open X11 display. Make sure you're running in a graphical environment." <<   endl;
            return false;
        }

        // Create a hidden window to receive clipboard events
        int screen = DefaultScreen(display_);
        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen),
                                     0, 0, 1, 1, 0, 0, 0);
        
        clipboard_atom_ = XInternAtom(display_, "CLIPBOARD", False);
        utf8_string_atom_ = XInternAtom(display_, "UTF8_STRING", False);
        text_plain_atom_ = XInternAtom(display_, "text/plain", False);
        
        // If UTF8_STRING is not available, fall back to standard atoms
        if (utf8_string_atom_ == None) {
            utf8_string_atom_ = XInternAtom(display_, "STRING", False);
        }

        monitoring_ = true;
        monitor_thread_ =   thread(&ClipboardMonitor::monitorLoop, this);
        
          cout << "Clipboard monitoring started" <<   endl;
        return true;
    }

    void stop() {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        if (display_) {
            if (window_) {
                XDestroyWindow(display_, window_);
            }
            XCloseDisplay(display_);
            display_ = nullptr;
        }
          cout << "Clipboard monitoring stopped" <<   endl;
    }

    bool isMonitoring() const {
        return monitoring_;
    }

private:
    void monitorLoop() {
        // Get initial clipboard content
          string initial_content = getClipboardText();
        if (!initial_content.empty()) {
            last_clipboard_content_ = initial_content;
        }

        while (monitoring_) {
              string current_content = getClipboardText();
            
            if (!current_content.empty() && current_content != last_clipboard_content_) {
                // Truncate if too long for display
                  string display_content = current_content;
                if (display_content.length() > 100) {
                    display_content = display_content.substr(0, 100) + "...";
                }
                
                  cout << "Clipboard changed: " << display_content <<   endl;
                
                if (callback_) {
                    // Send truncated version if too large
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
        if (!display_) {
            return "";
        }

        XEvent event;
          string result;

        // Try multiple target types in order of preference
        Atom targets[] = {
            utf8_string_atom_,
            text_plain_atom_,
            XA_STRING
        };
        
        const int num_targets = sizeof(targets) / sizeof(targets[0]);

        for (int i = 0; i < num_targets; i++) {
            if (targets[i] == None) continue;
            
            // Request clipboard content
            XConvertSelection(display_, clipboard_atom_, targets[i], 
                             clipboard_atom_, window_, CurrentTime);
            XFlush(display_);

            // Wait for selection notify with timeout
            time_t start_time = time(nullptr);
            bool got_data = false;
            
            while (time(nullptr) - start_time < 2) { // 2 second timeout
                if (XPending(display_)) {
                    XNextEvent(display_, &event);
                    
                    if (event.type == SelectionNotify && 
                        event.xselection.selection == clipboard_atom_) {
                        
                        if (event.xselection.property != None) {
                            Atom actual_type;
                            int actual_format;
                            unsigned long nitems, bytes_after;
                            unsigned char* data = nullptr;
                            
                            if (XGetWindowProperty(display_, window_, 
                                                 event.xselection.property,
                                                 0, MAX_CLIPBOARD_SIZE, False, 
                                                 AnyPropertyType,
                                                 &actual_type, &actual_format,
                                                 &nitems, &bytes_after, &data) == Success) {
                                
                                if (data && nitems > 0 && actual_format == 8) {
                                    result =   string(reinterpret_cast<char*>(data), nitems);
                                    
                                    // Clean up: remove non-printable characters except newlines and tabs
                                    result.erase(  remove_if(result.begin(), result.end(),
                                        [](char c) { 
                                            return (c < 32 && c != '\n' && c != '\t' && c != '\r') || c == 127; 
                                        }), result.end());
                                    
                                    got_data = true;
                                }
                                
                                if (data) {
                                    XFree(data);
                                }
                                
                                // Delete the property to clean up
                                XDeleteProperty(display_, window_, event.xselection.property);
                            }
                        }
                        break;
                    }
                }
                  this_thread::sleep_for(  chrono::milliseconds(10));
            }
            
            if (got_data && !result.empty()) {
                break;
            }
        }

        return result;
    }
};

#endif