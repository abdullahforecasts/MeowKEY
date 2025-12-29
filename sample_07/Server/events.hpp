// #ifndef EVENTS_HPP
// #define EVENTS_HPP

// #include <cstdint>
// #include <cstring>
// #include <string>
// #include <chrono>
// #include <openssl/sha.h>

// using namespace std;

// inline uint64_t getCurrentTimestamp() {
//     return chrono::duration_cast<chrono::microseconds>(
//         chrono::system_clock::now().time_since_epoch()
//     ).count();
// }

// enum class EventType : uint8_t {
//     KEYSTROKE = 1,
//     CLIPBOARD = 2,
//     WINDOW = 3
// };

// // Keystroke event (32 bytes fixed)
// struct KeystrokeEvent {
//     uint64_t timestamp;
//     char key[16];
//     uint32_t sequence;
//     uint32_t padding;
    
//     KeystrokeEvent() : timestamp(0), sequence(0), padding(0) {
//         memset(key, 0, sizeof(key));
//     }
    
//     KeystrokeEvent(const string& key_str, uint32_t seq) 
//         : timestamp(getCurrentTimestamp()), sequence(seq), padding(0) {
//         memset(key, 0, sizeof(key));
//         strncpy(key, key_str.c_str(), sizeof(key) - 1);
//     }
    
//     string getKey() const {
//         return string(key);
//     }
// };

// // Clipboard event (10,304 bytes fixed)
// struct ClipboardEvent {
//     uint64_t timestamp;
//     uint32_t content_length;
//     uint8_t hash[32];
//     char content[10240];
//     uint32_t padding;
    
//     ClipboardEvent() : timestamp(0), content_length(0), padding(0) {
//         memset(hash, 0, sizeof(hash));
//         memset(content, 0, sizeof(content));
//     }
    
//     ClipboardEvent(const string& text) 
//         : timestamp(getCurrentTimestamp()), padding(0) {
//         content_length = min((size_t)10240, text.size());
//         memset(content, 0, sizeof(content));
//         memcpy(content, text.c_str(), content_length);
        
//         SHA256_CTX sha256;
//         SHA256_Init(&sha256);
//         SHA256_Update(&sha256, text.c_str(), text.size());
//         SHA256_Final(hash, &sha256);
//     }
    
//     string getContent() const {
//         return string(content, content_length);
//     }
// };

// // Window event (328 bytes fixed)
// struct WindowEvent {
//     uint64_t timestamp;
//     char title[256];
//     char process[64];
//     uint32_t padding;
    
//     WindowEvent() : timestamp(0), padding(0) {
//         memset(title, 0, sizeof(title));
//         memset(process, 0, sizeof(process));
//     }
    
//     WindowEvent(const string& win_title, const string& proc_name) 
//         : timestamp(getCurrentTimestamp()), padding(0) {
//         memset(title, 0, sizeof(title));
//         memset(process, 0, sizeof(process));
//         strncpy(title, win_title.c_str(), sizeof(title) - 1);
//         strncpy(process, proc_name.c_str(), sizeof(process) - 1);
//     }
    
//     string getTitle() const {
//         return string(title);
//     }
    
//     string getProcess() const {
//         return string(process);
//     }
// };

// inline EventType parseEventType(const string& message) {
//     if (message.find("KEY: ") == 0) return EventType::KEYSTROKE;
//     else if (message.find("CLIPBOARD: ") == 0) return EventType::CLIPBOARD;
//     else if (message.find("WINDOW: ") == 0) return EventType::WINDOW;
//     return EventType::KEYSTROKE;
// }

// inline string extractEventData(const string& message) {
//     size_t pos = message.find(": ");
//     if (pos != string::npos) {
//         return message.substr(pos + 2);
//     }
//     return message;
// }

// inline pair<string, string> parseWindowEvent(const string& data) {
//     size_t bracket_pos = data.rfind(" [");
//     if (bracket_pos != string::npos) {
//         string title = data.substr(0, bracket_pos);
//         string process = data.substr(bracket_pos + 2);
//         if (!process.empty() && process.back() == ']') {
//             process.pop_back();
//         }
//         return {title, process};
//     }
//     return {data, "Unknown"};
// }

// #endif


#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <chrono>
#include <openssl/sha.h>

using namespace std;

inline uint64_t getCurrentTimestamp() {
    return chrono::duration_cast<chrono::microseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Rename enum
enum class ServerEventType : uint8_t {
    KEYSTROKE = 1,
    CLIPBOARD = 2,
    WINDOW = 3
};

// Rename structs - add "Server" prefix
struct ServerKeystrokeEvent {
    uint64_t timestamp;
    char key[16];
    uint32_t sequence;
    uint32_t padding;
    
    ServerKeystrokeEvent() : timestamp(0), sequence(0), padding(0) {
        memset(key, 0, sizeof(key));
    }
    
    ServerKeystrokeEvent(const string& key_str, uint32_t seq) 
        : timestamp(getCurrentTimestamp()), sequence(seq), padding(0) {
        memset(key, 0, sizeof(key));
        strncpy(key, key_str.c_str(), sizeof(key) - 1);
    }
    
    string getKey() const {
        return string(key);
    }
};

struct ServerClipboardEvent {
    uint64_t timestamp;
    uint32_t content_length;
    uint8_t hash[32];
    char content[10240];
    uint32_t padding;
    
    ServerClipboardEvent() : timestamp(0), content_length(0), padding(0) {
        memset(hash, 0, sizeof(hash));
        memset(content, 0, sizeof(content));
    }
    
    ServerClipboardEvent(const string& text) 
        : timestamp(getCurrentTimestamp()), padding(0) {
        content_length = min((size_t)10240, text.size());
        memset(content, 0, sizeof(content));
        memcpy(content, text.c_str(), content_length);
        
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, text.c_str(), text.size());
        SHA256_Final(hash, &sha256);
    }
    
    string getContent() const {
        return string(content, content_length);
    }
};

struct ServerWindowEvent {
    uint64_t timestamp;
    char title[256];
    char process[64];
    uint32_t padding;
    
    ServerWindowEvent() : timestamp(0), padding(0) {
        memset(title, 0, sizeof(title));
        memset(process, 0, sizeof(process));
    }
    
    ServerWindowEvent(const string& win_title, const string& proc_name) 
        : timestamp(getCurrentTimestamp()), padding(0) {
        memset(title, 0, sizeof(title));
        memset(process, 0, sizeof(process));
        strncpy(title, win_title.c_str(), sizeof(title) - 1);
        strncpy(process, proc_name.c_str(), sizeof(process) - 1);
    }
    
    string getTitle() const {
        return string(title);
    }
    
    string getProcess() const {
        return string(process);
    }
};

// Update function names
inline ServerEventType parseEventType(const string& message) {
    if (message.find("KEY: ") == 0) return ServerEventType::KEYSTROKE;
    else if (message.find("CLIPBOARD: ") == 0) return ServerEventType::CLIPBOARD;
    else if (message.find("WINDOW: ") == 0) return ServerEventType::WINDOW;
    return ServerEventType::KEYSTROKE;
}

inline string extractEventData(const string& message) {
    size_t pos = message.find(": ");
    if (pos != string::npos) {
        return message.substr(pos + 2);
    }
    return message;
}

inline pair<string, string> parseWindowEvent(const string& data) {
    size_t bracket_pos = data.rfind(" [");
    if (bracket_pos != string::npos) {
        string title = data.substr(0, bracket_pos);
        string process = data.substr(bracket_pos + 2);
        if (!process.empty() && process.back() == ']') {
            process.pop_back();
        }
        return {title, process};
    }
    return {data, "Unknown"};
}

#endif