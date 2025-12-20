// decode_db_correct.cpp
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>

// Correct header structure (128 bytes total)
struct DatabaseHeader {
    uint32_t magic;                 // 4 bytes
    uint32_t version;               // 4 bytes
    uint64_t file_size;             // 8 bytes
    uint64_t next_free_offset;      // 8 bytes
    uint64_t client_table_offset;   // 8 bytes
    uint64_t keystroke_tree_offset; // 8 bytes
    uint64_t clipboard_tree_offset; // 8 bytes  
    uint64_t window_tree_offset;    // 8 bytes
    uint32_t num_clients;           // 4 bytes
    uint32_t reserved[23];          // 92 bytes (23 * 4)
};

struct KeystrokeEvent {
    uint64_t timestamp;
    uint32_t client_hash;
    uint32_t sequence;
    char key[32];
};

int main() {
    std::ifstream file("meowkey_server.db", std::ios::binary);
    
    if (!file) {
        std::cerr << "Failed to open database file!\n";
        return 1;
    }
    
    // Read the CORRECT 128-byte header
    DatabaseHeader header;
    file.read((char*)&header, 128);
    
    std::cout << "=== CORRECT DATABASE HEADER ===\n";
    std::cout << "Magic: 0x" << std::hex << header.magic << " (";
    std::cout << (char)(header.magic >> 24) << (char)(header.magic >> 16) 
              << (char)(header.magic >> 8) << (char)header.magic << ")\n";
    std::cout << "Version: " << std::dec << header.version << "\n";
    std::cout << "File size: " << header.file_size << " bytes ("
              << header.file_size / 1024.0 / 1024.0 << " MB)\n";
    std::cout << "Next free offset: " << header.next_free_offset << " bytes\n";
    std::cout << "Client table offset: " << header.client_table_offset << "\n";
    std::cout << "Keystroke tree offset: " << header.keystroke_tree_offset << "\n";
    std::cout << "Clipboard tree offset: " << header.clipboard_tree_offset << "\n";
    std::cout << "Window tree offset: " << header.window_tree_offset << "\n";
    std::cout << "Number of clients: " << header.num_clients << "\n";
    
    // Verify the file size
    file.seekg(0, std::ios::end);
    size_t actual_size = file.tellg();
    std::cout << "Actual file size: " << actual_size << " bytes\n";
    
    // Check if offsets make sense
    std::cout << "\n=== OFFSET SANITY CHECK ===\n";
    std::cout << "Client table at: " << header.client_table_offset 
              << " (within file: " << (header.client_table_offset < actual_size ? "YES" : "NO") << ")\n";
    std::cout << "Keystroke tree at: " << header.keystroke_tree_offset 
              << " (within file: " << (header.keystroke_tree_offset < actual_size ? "YES" : "NO") << ")\n";
    
    // Try to read hash table entries
    std::cout << "\n=== READING HASH TABLE ===\n";
    
    // Hash table slot structure
    struct ClientRecord {
        uint32_t client_hash;
        char client_id[32];
        uint64_t keystroke_tree_root;
        uint64_t clipboard_tree_root;
        uint64_t window_tree_root;
        uint64_t first_seen;
        uint64_t last_seen;
        uint32_t session_count;
        uint32_t total_keystrokes;
        uint32_t total_clipboard;
        uint32_t total_windows;
        uint8_t is_active;
        uint8_t padding[3];
    };
    
    file.seekg(header.client_table_offset);
    ClientRecord client;
    
    for (int i = 0; i < 10; i++) {
        file.read((char*)&client, sizeof(client));
        
        if (client.client_hash != 0) {
            std::cout << "Found client at slot " << i << ":\n";
            std::cout << "  ID: " << client.client_id << "\n";
            std::cout << "  Hash: 0x" << std::hex << client.client_hash << std::dec << "\n";
            std::cout << "  Sessions: " << client.session_count << "\n";
            std::cout << "  Keystrokes: " << client.total_keystrokes << "\n";
            std::cout << "  Clipboards: " << client.total_clipboard << "\n";
            std::cout << "  Windows: " << client.total_windows << "\n";
            std::cout << "  Active: " << (int)client.is_active << "\n";
            break;
        }
    }
    
    // Read some B+ tree nodes
    std::cout << "\n=== CHECKING B+ TREE NODES ===\n";
    
    struct BPlusTreeNode {
        uint64_t keys[31];          // BPLUS_ORDER - 1
        uint64_t children[32];      // BPLUS_ORDER
        uint32_t num_keys;
        uint64_t parent_offset;
        uint64_t next_leaf;
        uint8_t is_leaf;
        uint8_t padding[7];         // Pad to 8-byte alignment
    };
    
    // Try to read the keystroke tree root
    if (header.keystroke_tree_offset > 0 && header.keystroke_tree_offset < actual_size) {
        file.seekg(header.keystroke_tree_offset);
        BPlusTreeNode node;
        if (file.read((char*)&node, sizeof(node))) {
            std::cout << "Keystroke tree root node:\n";
            std::cout << "  Is leaf: " << (int)node.is_leaf << "\n";
            std::cout << "  Num keys: " << node.num_keys << "\n";
            if (node.num_keys > 0) {
                std::cout << "  First key (timestamp): " << node.keys[0] << "\n";
            }
        }
    }
    
    return 0;
}