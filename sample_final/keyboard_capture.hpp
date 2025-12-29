

#ifndef KEYBOARD_CAPTURE_HPP
#define KEYBOARD_CAPTURE_HPP

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
using namespace std; 

class KeyboardCapture {
private:
      atomic<bool> capturing_;
      thread capture_thread_;
      function<void(const   string&)> callback_;
      string keyboard_device_;

public:
    KeyboardCapture(  function<void(const   string&)> callback) 
        : capturing_(false), callback_(callback) {
        findKeyboardDevice();
    }

    ~KeyboardCapture() {
        stop();
    }

    bool start() {
        if (capturing_) {
            return false;
        }

        capturing_ = true;
        capture_thread_ =   thread(&KeyboardCapture::captureLoop, this);
        return true;
    }

    void stop() {
        capturing_ = false;
        if (capture_thread_.joinable()) {
            capture_thread_.join();
        }
    }

    bool isCapturing() const {
        return capturing_;
    }

      string getKeyboardDevice() const {
        return keyboard_device_;
    }

private:
    void findKeyboardDevice() {
          cout << "Searching for keyboard devices..." <<   endl;
        
        // Try event2 first (based on your system configuration)
          vector<  string> possible_paths = {
            "/dev/input/event2",  // Your specific keyboard device
            "/dev/input/event3", "/dev/input/event4", "/dev/input/event5",
            "/dev/input/event0", "/dev/input/event1", "/dev/input/event6",
            "/dev/input/event7", "/dev/input/event8", "/dev/input/event9"
        };

        for (const auto& path : possible_paths) {
              cout << "Testing device: " << path <<   endl;
            if (isKeyboardDevice(path)) {
                keyboard_device_ = path;
                  cout << "✓ Found keyboard device: " << path <<   endl;
                return;
            }
        }

        throw   runtime_error("Could not find keyboard device. Please check permissions.");
    }

    bool isKeyboardDevice(const   string& path) {
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
              cout << "  ✗ Cannot open: " << strerror(errno) <<   endl;
            return false;
        }

        // Get device capabilities
        unsigned long evbit = 0;
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) < 0) {
            close(fd);
              cout << "  ✗ Cannot get capabilities" <<   endl;
            return false;
        }

        // Check if device has key events
        if (!(evbit & (1 << EV_KEY))) {
            close(fd);
              cout << "  ✗ Not a key device" <<   endl;
            return false;
        }

        // Get key bitmap to verify it's a keyboard
        unsigned char keybit[KEY_MAX/8 + 1];
        memset(keybit, 0, sizeof(keybit));
        
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) {
            close(fd);
              cout << "  ✗ Cannot get key bitmap" <<   endl;
            return false;
        }

        close(fd);

        // Check for common keyboard keys
        bool has_letters = (keybit[KEY_A/8] & (1 << (KEY_A % 8)));
        bool has_space = (keybit[KEY_SPACE/8] & (1 << (KEY_SPACE % 8)));
        bool has_enter = (keybit[KEY_ENTER/8] & (1 << (KEY_ENTER % 8)));

        if (has_letters && has_space && has_enter) {
              cout << "  ✓ Valid keyboard device" <<   endl;
            return true;
        }

          cout << "  ✗ Missing common keyboard keys" <<   endl;
        return false;
    }

    void captureLoop() {
          cout << "Starting capture loop on device: " << keyboard_device_ <<   endl;
        
        int fd = open(keyboard_device_.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
              cerr << "Failed to open keyboard device: " << keyboard_device_ 
                      << " Error: " << strerror(errno) <<   endl;
            return;
        }

          cout << "✓ Successfully opened keyboard device. Capturing keystrokes..." <<   endl;
          cout << "Try typing in any application (browser, terminal, etc.)" <<   endl;

        input_event ev;
        int event_count = 0;
        
        while (capturing_) {
            ssize_t bytes_read = read(fd, &ev, sizeof(ev));
            
            if (bytes_read == sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    event_count++;
                      string event_type;
                    switch (ev.value) {
                        case 0: event_type = "RELEASE"; break;
                        case 1: event_type = "PRESS"; break;
                        case 2: event_type = "REPEAT"; break;
                        default: event_type = "UNKNOWN"; break;
                    }
                    
                    // Only process key presses for now
                    if (ev.value == 1) {
                          string key_name = keyCodeToString(ev.code);
                          cout << "[" << event_count << "] Captured key: " << key_name 
                                  << " (code: " << ev.code << ", type: " << event_type << ")" <<   endl;
                        
                        if (!key_name.empty() && callback_) {
                            callback_(key_name);
                        }
                    }
                }
            } else if (bytes_read < 0) {
                if (errno != EAGAIN) {
                    // Non-recoverable error
                      cerr << "Read error: " << strerror(errno) <<   endl;
                    break;
                }
                // EAGAIN means no data available, just continue
            }
            
              this_thread::sleep_for(  chrono::milliseconds(1));
        }

        close(fd);
          cout << "Capture loop ended. Total events processed: " << event_count <<   endl;
    }

      string keyCodeToString(int key_code) {
        static const unordered_map<int, string> key_map = {
            {KEY_A, "A"}, {KEY_B, "B"}, {KEY_C, "C"}, {KEY_D, "D"}, {KEY_E, "E"},
            {KEY_F, "F"}, {KEY_G, "G"}, {KEY_H, "H"}, {KEY_I, "I"}, {KEY_J, "J"},
            {KEY_K, "K"}, {KEY_L, "L"}, {KEY_M, "M"}, {KEY_N, "N"}, {KEY_O, "O"},
            {KEY_P, "P"}, {KEY_Q, "Q"}, {KEY_R, "R"}, {KEY_S, "S"}, {KEY_T, "T"},
            {KEY_U, "U"}, {KEY_V, "V"}, {KEY_W, "W"}, {KEY_X, "X"}, {KEY_Y, "Y"},
            {KEY_Z, "Z"},
            {KEY_0, "0"}, {KEY_1, "1"}, {KEY_2, "2"}, {KEY_3, "3"}, {KEY_4, "4"},
            {KEY_5, "5"}, {KEY_6, "6"}, {KEY_7, "7"}, {KEY_8, "8"}, {KEY_9, "9"},
            {KEY_SPACE, "SPACE"}, {KEY_ENTER, "ENTER"}, {KEY_BACKSPACE, "BACKSPACE"},
            {KEY_TAB, "TAB"}, {KEY_ESC, "ESC"}, {KEY_LEFTCTRL, "CTRL"},
            {KEY_LEFTSHIFT, "SHIFT"}, {KEY_RIGHTSHIFT, "RSHIFT"}, {KEY_LEFTALT, "ALT"},
            {KEY_RIGHTALT, "RALT"}, {KEY_LEFTMETA, "META"}, {KEY_RIGHTMETA, "RMETA"},
            {KEY_COMMA, ","}, {KEY_DOT, "."}, {KEY_SLASH, "/"}, {KEY_SEMICOLON, ";"},
            {KEY_APOSTROPHE, "'"}, {KEY_LEFTBRACE, "["}, {KEY_RIGHTBRACE, "]"},
            {KEY_BACKSLASH, "\\"}, {KEY_MINUS, "-"}, {KEY_EQUAL, "="}, {KEY_GRAVE, "`"},
            {KEY_UP, "UP"}, {KEY_DOWN, "DOWN"}, {KEY_LEFT, "LEFT"}, {KEY_RIGHT, "RIGHT"},
            {KEY_F1, "F1"}, {KEY_F2, "F2"}, {KEY_F3, "F3"}, {KEY_F4, "F4"},
            {KEY_F5, "F5"}, {KEY_F6, "F6"}, {KEY_F7, "F7"}, {KEY_F8, "F8"},
            {KEY_F9, "F9"}, {KEY_F10, "F10"}, {KEY_F11, "F11"}, {KEY_F12, "F12"}
        };

        auto it = key_map.find(key_code);
        if (it != key_map.end()) {
            return it->second;
        }
        return "KEY_" +   to_string(key_code);
    }
};

#endif 