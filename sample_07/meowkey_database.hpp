

#ifndef MEOWKEY_DATABASE_HPP
#define MEOWKEY_DATABASE_HPP

#include "db_structures.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <chrono>
#include <stack>
#include <mutex>
#include <string>
#include <cstring>
#include <errno.h>
#include <algorithm>

using namespace std;

struct QueryResult
{
    vector<KeystrokeEvent> keystrokes;
    vector<ClipboardEvent> clipboard_events;
    vector<WindowEvent> window_events;

    size_t totalEvents() const
    {
        return keystrokes.size() + clipboard_events.size() + window_events.size();
    }
};

class MeowKeyDatabase
{
private:
    string filename_;
    int fd_;
    DatabaseHeader header_;

    static const size_t HASH_TABLE_SIZE = 1024;
    uint64_t hash_table_offset_;

    unordered_map<string, ClientRecord> client_cache_;
    stack<uint64_t> free_data_blocks_;
    stack<uint64_t> free_bplus_nodes_;

    mutable std::recursive_mutex db_mutex_;
    mutable mutex cache_mutex_;



  void reclaimBPlusTreeNodesOnlyLocked(uint64_t root_offset)
    {
        if (root_offset == 0)
            return;

        // Use iterative approach with a stack to avoid stack overflow
        vector<uint64_t> node_stack;
        node_stack.push_back(root_offset);

        int nodes_freed = 0;
        
        while (!node_stack.empty())
        {
            uint64_t current_offset = node_stack.back();
            node_stack.pop_back();

            if (current_offset == 0)
                continue;

            BPlusTreeNode node;
            if (!readNodeLocked(current_offset, node))
            {
                // Skip corrupted/invalid nodes
                continue;
            }

            // Add children to stack (internal nodes)
            if (!node.is_leaf)
            {
                for (uint32_t i = 0; i <= node.num_keys; ++i)
                {
                    if (node.children[i] != 0)
                    {
                        node_stack.push_back(node.children[i]);
                    }
                }
            }
            else
            {
                // For leaf nodes, add next leaf in chain
                if (node.next_leaf != 0)
                {
                    node_stack.push_back(node.next_leaf);
                }
            }

            // Mark this node as free
            free_bplus_nodes_.push(current_offset);
            nodes_freed++;
            
            // Periodically log progress for large deletions
            if (nodes_freed % 100 == 0)
            {
                cout << "    Freed " << nodes_freed << " nodes..." << endl;
            }
        }

        if (nodes_freed > 0)
        {
            cout << "    Total nodes freed: " << nodes_freed << endl;
        }
    }


//  void reclaimBPlusTreeNodesOnlyLocked(uint64_t node_offset)
//     {
//         if (node_offset == 0)
//             return;
            
//         BPlusTreeNode node;
//         if (!readNodeLocked(node_offset, node))
//             return;

//         // If it's not a leaf, recursively free child nodes
//         if (!node.is_leaf)
//         {
//             for (uint32_t i = 0; i <= node.num_keys; ++i)
//             {
//                 if (node.children[i] != 0)
//                     reclaimBPlusTreeNodesOnlyLocked(node.children[i]);
//             }
//         }
//         else
//         {
//             // For leaf nodes, also free the next leaf in the chain
//             if (node.next_leaf != 0)
//                 reclaimBPlusTreeNodesOnlyLocked(node.next_leaf);
//         }

//         // Mark this node as free
//         free_bplus_nodes_.push(node_offset);
//     }


public:
    MeowKeyDatabase(const string &filename)
        : filename_(filename), fd_(-1) {}

    ~MeowKeyDatabase()
    {
        close();
    }

    static uint64_t getCurrentTimestamp()
    {
        return chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
    }

    bool open()
    {
        lock_guard<recursive_mutex> lock(db_mutex_);

        fd_ = ::open(filename_.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd_ < 0)
        {
            cerr << "Failed to open database: " << filename_ << " errno=" << errno << endl;
            return false;
        }

        struct stat st;
        if (fstat(fd_, &st) != 0)
        {
            cerr << "fstat failed errno=" << errno << endl;
            ::close(fd_);
            fd_ = -1;
            return false;
        }

        if (st.st_size == 0)
        {
            if (!initializeDatabase())
            {
                ::close(fd_);
                fd_ = -1;
                return false;
            }
        }
        else
        {
            if (!readHeader())
            {
                cerr << "Failed to read database header" << endl;
                ::close(fd_);
                fd_ = -1;
                return false;
            }

            if (header_.magic != 0x4D454F57)
            {
                cerr << "Invalid magic number" << endl;
                ::close(fd_);
                fd_ = -1;
                return false;
            }
        }

        hash_table_offset_ = header_.client_table_offset;
        return true;
    }

    void close()
    {
        lock_guard<recursive_mutex> lock(db_mutex_);

        if (fd_ >= 0)
        {
            writeHeader();
            fsync(fd_);
            ::close(fd_);
            fd_ = -1;
        }
    }

    // void syncToDisk()
    // {
    //     lock_guard<recursive_mutex> lock(db_mutex_);
    //     writeHeader();
    //     if (fd_ >= 0)
    //     {
    //         fsync(fd_);
    //     }
    // }

    // In MeowKeyDatabase::syncToDisk()

    //     // void syncToDisk()
    //     // {
    //     //     lock_guard<recursive_mutex> lock(db_mutex_);
    //     //     cout << "🔍 [DEBUG] Writing header to offset 0, fd=" << fd_ << endl;
    //     //     writeHeader();
    //     //     if (fd_ >= 0)
    //     //     {
    //     //         fsync(fd_);
    //     //         // Check file size
    //     //         struct stat st;
    //     //         if (fstat(fd_, &st) == 0)
    //     //         {
    //     //             cout << "💾 [DEBUG] Database file size: " << st.st_size << " bytes" << endl;
    //     //         }
    //     //     }
    //     // }

    // void syncToDisk()
    // {
    //     lock_guard<recursive_mutex> lock(db_mutex_);

    //     if (fd_ < 0) {
    //         cerr << "⚠️ [WARN] Cannot sync - database file not open" << endl;
    //         return;
    //     }

    //     cout << "🔍 [DEBUG] Writing header to offset 0, fd=" << fd_ << endl;

    //     if (!writeHeader()) {
    //         cerr << "❌ Failed to write header in syncToDisk()" << endl;
    //     }

    //     if (fd_ >= 0) {
    //         fsync(fd_);
    //         // Check file size
    //         struct stat st;
    //         if (fstat(fd_, &st) == 0) {
    //             cout << "💾 [DEBUG] Database file size: " << st.st_size << " bytes" << endl;
    //         }
    //     }
    // }

    void syncToDisk()
    {
        lock_guard<recursive_mutex> lock(db_mutex_);

        if (fd_ < 0)
        {
            // Quietly skip if DB already closed
            return;
        }

        cout << "🔍 [DEBUG] Writing header to offset 0, fd=" << fd_ << endl;

        if (!writeHeader())
        {
            cerr << "❌ Failed to write header in syncToDisk()" << endl;
            return;
        }

        fsync(fd_);
        struct stat st;
        if (fstat(fd_, &st) == 0)
        {
            cout << "💾 [DEBUG] Database file size: " << st.st_size << " bytes" << endl;
        }
    }

    bool registerClient(const string &client_id, ClientRecord &record)
    {
        lock_guard<recursive_mutex> lock(db_mutex_);

        // Try cache first
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            auto it = client_cache_.find(client_id);
            if (it != client_cache_.end() && !it->second.isEmpty())
            {
                record = it->second;
                record.is_active = 1;
                record.session_count++;
                record.last_seen = getCurrentTimestamp();
                saveClientRecordNoLock(record);
                client_cache_[client_id] = record;
                cout << "✓ Loaded existing client: " << client_id << " (sessions: " << record.session_count << ")" << endl;
                return true;
            }
        }

        // Try disk
        ClientRecord disk_record;
        if (loadClientRecordNoLock(client_id, disk_record))
        {
            // Found on disk - check if it's marked as deleted
            if (!disk_record.isEmpty())
            {
                record = disk_record;
                record.is_active = 1;
                record.session_count++;
                record.last_seen = getCurrentTimestamp();
                saveClientRecordNoLock(record);
                {
                    lock_guard<mutex> c_lock(cache_mutex_);
                    client_cache_[client_id] = record;
                }
                cout << "✓ Loaded existing client (disk): " << client_id << " (sessions: " << record.session_count << ")" << endl;
                return true;
            }
        }

        // Create new client (only if not found anywhere)
        record = ClientRecord();
        record.setClientId(client_id);
        record.is_active = 1;
        record.session_count = 1;
        record.first_seen = getCurrentTimestamp();
        record.last_seen = record.first_seen;

        // Allocate B+ tree roots
        record.keystroke_tree_root = allocateNodeLocked();
        record.clipboard_tree_root = allocateNodeLocked();
        record.window_tree_root = allocateNodeLocked();

        initializeBPlusTreeRootLocked(record.keystroke_tree_root);
        initializeBPlusTreeRootLocked(record.clipboard_tree_root);
        initializeBPlusTreeRootLocked(record.window_tree_root);

        saveClientRecordNoLock(record);
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            client_cache_[client_id] = record;
        }

        header_.num_clients++;
        writeHeader();

        cout << "✓ New client registered: " << client_id << endl;
        return false;
    }

    void deactivateClient(const string &client_id)
    {
        lock_guard<recursive_mutex> lock(db_mutex_);

        ClientRecord record;
        if (!loadClientRecordNoLock(client_id, record))
        {
            cerr << "Client " << client_id << " not found for deactivation" << endl;
            return;
        }

        if (record.isEmpty())
        {
            cerr << "Client " << client_id << " is already marked as deleted" << endl;
            return;
        }

        record.is_active = 0;
        record.last_seen = getCurrentTimestamp();

        saveClientRecordNoLock(record);

        {
            lock_guard<mutex> c_lock(cache_mutex_);
            client_cache_[client_id] = record;
        }

        writeHeader();
        cout << "✓ Client deactivated: " << client_id << endl;
    }


bool deleteClient(const string &client_id)
{
    lock_guard<recursive_mutex> lock(db_mutex_);

    ClientRecord record;
    if (!loadClientRecordNoLock(client_id, record))
    {
        cerr << "Client " << client_id << " not found for deletion" << endl;
        return false;
    }

    if (record.isEmpty())
    {
        cerr << "Client " << client_id << " is already marked as deleted" << endl;
        return false;
    }

    cout << "🗑️  Deleting client: " << client_id << endl;
    cout << "  Data: KS=" << record.total_keystrokes 
         << ", CB=" << record.total_clipboard 
         << ", W=" << record.total_windows << endl;

    // CRITICAL: Remove from hash table and cache FIRST
    // This prevents anyone from trying to access the client while we're deleting
    cout << "  Removing from hash table and cache..." << endl;
    removeClientFromHashTableLocked(client_id);
    
    {
        lock_guard<mutex> c_lock(cache_mutex_);
        client_cache_.erase(client_id);
    }

    // Update header BEFORE reclaiming space
    if (header_.num_clients > 0)
        header_.num_clients--;
    
    writeHeader();
    fsync(fd_);

    // NOW it's safe to reclaim the tree space
    cout << "  Reclaiming keystroke tree..." << endl;
    reclaimBPlusTreeNodesOnlyLocked(record.keystroke_tree_root);
    
    cout << "  Reclaiming clipboard tree..." << endl;
    reclaimBPlusTreeNodesOnlyLocked(record.clipboard_tree_root);
    
    cout << "  Reclaiming window tree..." << endl;
    reclaimBPlusTreeNodesOnlyLocked(record.window_tree_root);

    cout << "✅ Client deleted and space reclaimed: " << client_id << endl;
    return true;
}

// bool deleteClient(const string &client_id)
// {
//     lock_guard<recursive_mutex> lock(db_mutex_);

//     ClientRecord record;
//     if (!loadClientRecordNoLock(client_id, record))
//     {
//         cerr << "Client " << client_id << " not found for deletion" << endl;
//         return false;
//     }

//     if (record.isEmpty())
//     {
//         cerr << "Client " << client_id << " is already marked as deleted" << endl;
//         return false;
//     }

//     // FIXED: Reclaim B+ tree space WITHOUT iterating through data
//     // Just mark the tree nodes as free, don't try to read the data
//     reclaimBPlusTreeNodesOnlyLocked(record.keystroke_tree_root);
//     reclaimBPlusTreeNodesOnlyLocked(record.clipboard_tree_root);
//     reclaimBPlusTreeNodesOnlyLocked(record.window_tree_root);

//     // Remove from hash table
//     removeClientFromHashTableLocked(client_id);

//     // Remove from cache
//     {
//         lock_guard<mutex> c_lock(cache_mutex_);
//         client_cache_.erase(client_id);
//     }

//     if (header_.num_clients > 0)
//         header_.num_clients--;

//     writeHeader();
//     fsync(fd_);
//     cout << "✓ Client deleted and space reclaimed: " << client_id << endl;
//     return true;
// }


    // bool deleteClient(const string &client_id)
    // {
    //     lock_guard<recursive_mutex> lock(db_mutex_);

    //     ClientRecord record;
    //     if (!loadClientRecordNoLock(client_id, record))
    //     {
    //         cerr << "Client " << client_id << " not found for deletion" << endl;
    //         return false;
    //     }

    //     if (record.isEmpty())
    //     {
    //         cerr << "Client " << client_id << " is already marked as deleted" << endl;
    //         return false;
    //     }

    //     // Reclaim all B+ tree space and associated data blocks
    //     reclaimBPlusTreeSpaceLocked(record.keystroke_tree_root);
    //     reclaimBPlusTreeSpaceLocked(record.clipboard_tree_root);
    //     reclaimBPlusTreeSpaceLocked(record.window_tree_root);

    //     // FIXED: Remove from hash table properly
    //     removeClientFromHashTableLocked(client_id);

    //     // Remove from cache
    //     {
    //         lock_guard<mutex> c_lock(cache_mutex_);
    //         client_cache_.erase(client_id);
    //     }

    //     if (header_.num_clients > 0)
    //         header_.num_clients--;

    //     writeHeader();
    //     fsync(fd_);
    //     cout << "✓ Client deleted and space reclaimed: " << client_id << endl;
    //     return true;
    // }


   

    void insertKeystroke(uint32_t client_hash, const KeystrokeEvent &event)
    {
        ClientRecord record;
        bool found = false;
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            for (const auto &p : client_cache_)
            {
                if (p.second.client_hash == client_hash && !p.second.isEmpty())
                {
                    record = p.second;
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (!findClientByHashNoLock(client_hash, record))
            {
                cerr << "ERROR: Client not found for hash: " << client_hash << endl;
                return;
            }
            {
                lock_guard<mutex> c_lock(cache_mutex_);
                client_cache_[string(record.client_id)] = record;
            }
        }

        if (record.isEmpty())
        {
            cerr << "ERROR: Client with hash " << client_hash << " is marked as deleted" << endl;
            return;
        }

        uint64_t data_offset;
        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (!free_data_blocks_.empty())
            {
                data_offset = free_data_blocks_.top();
                free_data_blocks_.pop();
            }
            else
            {
                data_offset = header_.next_free_offset;
                header_.next_free_offset += sizeof(event);
            }

            if (pwrite(fd_, &event, sizeof(event), (off_t)data_offset) != (ssize_t)sizeof(event))
            {
                cerr << "ERROR: Failed to write keystroke data errno=" << errno << endl;
                return;
            }
            writeHeader();
        }

        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (record.keystroke_tree_root == 0)
            {
                record.keystroke_tree_root = allocateNodeLocked();
                initializeBPlusTreeRootLocked(record.keystroke_tree_root);
            }
            insertIntoBPlusTreeLocked(record.keystroke_tree_root, event.timestamp, data_offset);
            uint64_t new_root = getCurrentRootLocked(record.keystroke_tree_root);
            if (new_root != record.keystroke_tree_root)
                record.keystroke_tree_root = new_root;

            record.total_keystrokes++;
            record.last_seen = event.timestamp;
            saveClientRecordNoLock(record);
            {
                lock_guard<mutex> c_lock(cache_mutex_);
                client_cache_[string(record.client_id)] = record;
            }
        }

        cout << "    Keystroke inserted for client: " << record.client_id
             << " at offset: " << data_offset
             << " (total: " << record.total_keystrokes << ")" << endl;
    }

    void insertClipboard(uint32_t client_hash, const ClipboardEvent &event)
    {
        ClientRecord record;
        bool found = false;
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            for (const auto &p : client_cache_)
            {
                if (p.second.client_hash == client_hash && !p.second.isEmpty())
                {
                    record = p.second;
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (!findClientByHashNoLock(client_hash, record))
            {
                cerr << "ERROR: Client not found for hash: " << client_hash << endl;
                return;
            }
            {
                lock_guard<mutex> c_lock(cache_mutex_);
                client_cache_[string(record.client_id)] = record;
            }
        }

        if (record.isEmpty())
        {
            cerr << "ERROR: Client with hash " << client_hash << " is marked as deleted" << endl;
            return;
        }

        uint64_t data_offset;
        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (!free_data_blocks_.empty())
            {
                data_offset = free_data_blocks_.top();
                free_data_blocks_.pop();
            }
            else
            {
                data_offset = header_.next_free_offset;
                header_.next_free_offset += sizeof(event);
            }

            if (pwrite(fd_, &event, sizeof(event), (off_t)data_offset) != (ssize_t)sizeof(event))
            {
                cerr << "ERROR: Failed to write clipboard data errno=" << errno << endl;
                return;
            }
            writeHeader();
        }

        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (record.clipboard_tree_root == 0)
            {
                record.clipboard_tree_root = allocateNodeLocked();
                initializeBPlusTreeRootLocked(record.clipboard_tree_root);
            }
            insertIntoBPlusTreeLocked(record.clipboard_tree_root, event.timestamp, data_offset);
            uint64_t new_root = getCurrentRootLocked(record.clipboard_tree_root);
            if (new_root != record.clipboard_tree_root)
                record.clipboard_tree_root = new_root;

            record.total_clipboard++;
            record.last_seen = event.timestamp;
            saveClientRecordNoLock(record);
            {
                lock_guard<mutex> c_lock(cache_mutex_);
                client_cache_[string(record.client_id)] = record;
            }
        }

        cout << "    Clipboard event inserted for client: " << record.client_id
             << " at offset: " << data_offset << endl;
    }

    void insertWindow(uint32_t client_hash, const WindowEvent &event)
    {
        ClientRecord record;
        bool found = false;
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            for (const auto &p : client_cache_)
            {
                if (p.second.client_hash == client_hash && !p.second.isEmpty())
                {
                    record = p.second;
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (!findClientByHashNoLock(client_hash, record))
            {
                cerr << "ERROR: Client not found for hash: " << client_hash << endl;
                return;
            }
            {
                lock_guard<mutex> c_lock(cache_mutex_);
                client_cache_[string(record.client_id)] = record;
            }
        }

        if (record.isEmpty())
        {
            cerr << "ERROR: Client with hash " << client_hash << " is marked as deleted" << endl;
            return;
        }

        uint64_t data_offset;
        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (!free_data_blocks_.empty())
            {
                data_offset = free_data_blocks_.top();
                free_data_blocks_.pop();
            }
            else
            {
                data_offset = header_.next_free_offset;
                header_.next_free_offset += sizeof(event);
            }

            if (pwrite(fd_, &event, sizeof(event), (off_t)data_offset) != (ssize_t)sizeof(event))
            {
                cerr << "ERROR: Failed to write window data errno=" << errno << endl;
                return;
            }
            writeHeader();
        }

        {
            lock_guard<recursive_mutex> lock(db_mutex_);
            if (record.window_tree_root == 0)
            {
                record.window_tree_root = allocateNodeLocked();
                initializeBPlusTreeRootLocked(record.window_tree_root);
            }
            insertIntoBPlusTreeLocked(record.window_tree_root, event.timestamp, data_offset);
            uint64_t new_root = getCurrentRootLocked(record.window_tree_root);
            if (new_root != record.window_tree_root)
                record.window_tree_root = new_root;

            record.total_windows++;
            record.last_seen = event.timestamp;
            saveClientRecordNoLock(record);
            {
                lock_guard<mutex> c_lock(cache_mutex_);
                client_cache_[string(record.client_id)] = record;
            }
        }

        cout << "    Window event inserted for client: " << record.client_id
             << " at offset: " << data_offset << endl;
    }

    bool queryClientEventsByTimeRange(const string &client_id,
                                      uint64_t start_timestamp,
                                      uint64_t end_timestamp,
                                      QueryResult &result)
    {
        lock_guard<recursive_mutex> lock(db_mutex_);

        ClientRecord record;
        if (!loadClientRecordNoLock(client_id, record))
        {
            cerr << "Client not found: " << client_id << endl;
            return false;
        }

        if (record.isEmpty())
        {
            cerr << "Client is marked as deleted: " << client_id << endl;
            return false;
        }

        queryBPlusTreeRangeLocked(record.keystroke_tree_root,
                                  start_timestamp, end_timestamp,
                                  result.keystrokes);

        queryBPlusTreeRangeLocked(record.clipboard_tree_root,
                                  start_timestamp, end_timestamp,
                                  result.clipboard_events);

        queryBPlusTreeRangeLocked(record.window_tree_root,
                                  start_timestamp, end_timestamp,
                                  result.window_events);

        cout << "✓ Retrieved " << result.totalEvents() << " events for client: " << client_id << endl;
        cout << "  - Keystrokes: " << result.keystrokes.size() << endl;
        cout << "  - Clipboard: " << result.clipboard_events.size() << endl;
        cout << "  - Windows: " << result.window_events.size() << endl;
        return true;
    }

    bool queryClientAllEvents(const string &client_id, QueryResult &result)
    {
        return queryClientEventsByTimeRange(client_id, 0, UINT64_MAX, result);
    }

    bool clientExists(const string &client_id)
    {
        // Check cache FIRST
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            auto it = client_cache_.find(client_id);
            if (it != client_cache_.end())
            {
                return !it->second.isEmpty();
            }
        }

        // If not in cache, check disk
        lock_guard<recursive_mutex> lock(db_mutex_);
        ClientRecord record;
        if (!loadClientRecordNoLock(client_id, record))
            return false;

        return !record.isEmpty();
    }

    // bool clientExists(const string &client_id)
    // {
    //     {
    //         lock_guard<mutex> c_lock(cache_mutex_);
    //         auto it = client_cache_.find(client_id);
    //         if (it != client_cache_.end())
    //         {
    //             return !it->second.isEmpty();
    //         }
    //     }

    //     lock_guard<recursive_mutex> lock(db_mutex_);
    //     ClientRecord record;
    //     if (!loadClientRecordNoLock(client_id, record))
    //         return false;

    //     return !record.isEmpty();
    // }

    vector<string> getAllClients()
    {
        vector<string> clients;

        // Get from cache first
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            for (const auto &p : client_cache_)
            {
                if (!p.second.isEmpty())
                    clients.push_back(p.first);
            }
        }

        // Also scan hash table on disk for any clients not in cache
        lock_guard<recursive_mutex> lock(db_mutex_);
        for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        {
            uint64_t offset = hash_table_offset_ + (i * sizeof(HashTableSlot));
            HashTableSlot hs;
            if (pread(fd_, &hs, sizeof(hs), (off_t)offset) != (ssize_t)sizeof(hs))
                continue;

            if (!hs.client.isEmpty())
            {
                string client_id(hs.client.client_id);
                // Check if already in our list
                if (find(clients.begin(), clients.end(), client_id) == clients.end())
                {
                    clients.push_back(client_id);
                    // Also add to cache
                    {
                        lock_guard<mutex> c_lock(cache_mutex_);
                        client_cache_[client_id] = hs.client;
                    }
                }
            }
        }

        return clients;
    }
    // vector<string> getAllClients()
    // {
    //     lock_guard<mutex> c_lock(cache_mutex_);
    //     vector<string> clients;
    //     for (const auto &p : client_cache_)
    //         if (!p.second.isEmpty())
    //             clients.push_back(p.first);
    //     return clients;
    // }

    bool getClientStats(const string &client_id, ClientRecord &record)
    {
        // Check cache first
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            auto it = client_cache_.find(client_id);
            if (it != client_cache_.end() && !it->second.isEmpty())
            {
                record = it->second;
                return true;
            }
        }

        // Then check disk
        lock_guard<recursive_mutex> lock(db_mutex_);
        if (!loadClientRecordNoLock(client_id, record))
            return false;

        return !record.isEmpty();
    }

    // bool getClientStats(const string &client_id, ClientRecord &record)
    // {
    //     lock_guard<recursive_mutex> lock(db_mutex_);
    //     if (!loadClientRecordNoLock(client_id, record))
    //         return false;
    //     return !record.isEmpty();
    // }

    void printStats()
    {
        lock_guard<recursive_mutex> lock(db_mutex_);
        cout << "\n=== DATABASE STATS ===" << endl;
        cout << "File size: " << (header_.file_size / 1024.0 / 1024.0) << " MB" << endl;
        cout << "Used space: " << (header_.next_free_offset / 1024.0 / 1024.0) << " MB" << endl;
        cout << "Usage: " << (header_.next_free_offset * 100.0 / header_.file_size) << "%" << endl;
        cout << "Total clients: " << header_.num_clients << endl;
        {
            lock_guard<mutex> c_lock(cache_mutex_);
            cout << "Cached clients: " << client_cache_.size() << endl;
        }
        cout << "Free B+ tree nodes: " << free_bplus_nodes_.size() << endl;
        cout << "Free data blocks: " << free_data_blocks_.size() << endl;
    }

private:
    bool initializeDatabase()
    {
        header_ = DatabaseHeader();

        // FIX: Set correct offsets
        header_.client_table_offset = sizeof(DatabaseHeader); // 256 bytes
        header_.keystroke_tree_offset = header_.client_table_offset +
                                        (HASH_TABLE_SIZE * sizeof(HashTableSlot)); // 256 + 1024*256 = 262,400
        header_.clipboard_tree_offset = header_.keystroke_tree_offset + sizeof(BPlusTreeNode);
        header_.window_tree_offset = header_.clipboard_tree_offset + sizeof(BPlusTreeNode);
        header_.next_free_offset = header_.window_tree_offset + sizeof(BPlusTreeNode);

        cout << "💾 Initializing database with 100MB file" << endl;
        cout << "  Hash table: " << header_.client_table_offset << endl;
        cout << "  Keystroke tree: " << header_.keystroke_tree_offset << endl;
        cout << "  Next free: " << header_.next_free_offset << endl;

        // FIX: Use proper file size
        if (ftruncate(fd_, header_.file_size) < 0)
        {
            cerr << "Failed to allocate 100MB file errno=" << errno << endl;
            return false;
        }

        if (!writeHeader())
            return false;

        // Initialize hash table with empty slots
        HashTableSlot empty_slot;
        for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        {
            uint64_t offset = header_.client_table_offset + (i * sizeof(HashTableSlot));
            if (pwrite(fd_, &empty_slot, sizeof(empty_slot), (off_t)offset) != (ssize_t)sizeof(empty_slot))
            {
                cerr << "Failed to init hash slot " << i << endl;
                return false;
            }
        }

        fsync(fd_);
        cout << "✅ Database initialized: 100MB file created" << endl;
        return true;
    }

    bool readHeader()
    {
        if (pread(fd_, &header_, sizeof(header_), 0) != (ssize_t)sizeof(header_))
            return false;
        return true;
    }

    // bool writeHeader()
    // {
    //     if (pwrite(fd_, &header_, sizeof(header_), 0) != (ssize_t)sizeof(header_))
    //         return false;
    //     fsync(fd_);
    //     return true;
    // }

    bool writeHeader()
    {
        lock_guard<recursive_mutex> lock(db_mutex_);

        cout << "💾 [WRITE HEADER] Writing header at offset 0\n";
        cout << "  Magic: 0x" << hex << header_.magic << dec << "\n";
        cout << "  File size: " << header_.file_size << " bytes\n";
        cout << "  Next free: " << header_.next_free_offset << "\n";
        cout << "  Num clients: " << header_.num_clients << "\n";

        // DEBUG: Print raw bytes
        cout << "  Raw header (first 64 bytes): ";
        char *raw = (char *)&header_;
        for (int i = 0; i < 64; i++)
        {
            printf("%02x ", (unsigned char)raw[i]);
        }
        cout << "\n";

        ssize_t result = pwrite(fd_, &header_, sizeof(header_), 0);

        if (result != (ssize_t)sizeof(header_))
        {
            cerr << "❌ HEADER WRITE FAILED! Expected: " << sizeof(header_)
                 << ", Got: " << result << ", errno=" << errno << endl;
            return false;
        }

        // Force sync
        fsync(fd_);
        return true;
    }

    uint64_t allocateNodeLocked()
    {
        uint64_t offset = header_.next_free_offset;
        header_.next_free_offset += sizeof(BPlusTreeNode);
        writeHeader();
        return offset;
    }

    bool readNodeLocked(uint64_t offset, BPlusTreeNode &node)
    {
        if (offset == 0)
            return false;
        if (offset + sizeof(BPlusTreeNode) > header_.file_size)
            return false;
        if (pread(fd_, &node, sizeof(node), (off_t)offset) != (ssize_t)sizeof(node))
            return false;
        return true;
    }

    bool writeNodeLocked(uint64_t offset, const BPlusTreeNode &node)
    {
        if (offset == 0)
            return false;
        if (pwrite(fd_, &node, sizeof(node), (off_t)offset) != (ssize_t)sizeof(node))
            return false;
        fsync(fd_);
        return true;
    }

    void initializeBPlusTreeRootLocked(uint64_t offset)
    {
        if (offset == 0)
            return;
        BPlusTreeNode root;
        memset(&root, 0, sizeof(root));
        root.is_leaf = true;
        root.num_keys = 0;
        writeNodeLocked(offset, root);
    }

    bool loadClientRecordNoLock(const string &client_id, ClientRecord &record)
    {
        uint32_t hash = ClientRecord::hashString(client_id);
        size_t slot = hash % HASH_TABLE_SIZE;

        for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        {
            size_t idx = (slot + i) % HASH_TABLE_SIZE;
            uint64_t offset = hash_table_offset_ + (idx * sizeof(HashTableSlot));
            HashTableSlot hs;
            if (pread(fd_, &hs, sizeof(hs), (off_t)offset) != (ssize_t)sizeof(hs))
                continue;

            if (hs.client.isEmpty())
                return false;

            if (string(hs.client.client_id) == client_id && !hs.client.isEmpty())
            {
                record = hs.client;
                return true;
            }
        }
        return false;
    }

    void saveClientRecordNoLock(const ClientRecord &record)
    {
        uint32_t hash = record.client_hash;
        size_t slot = hash % HASH_TABLE_SIZE;

        for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        {
            size_t idx = (slot + i) % HASH_TABLE_SIZE;
            uint64_t offset = hash_table_offset_ + (idx * sizeof(HashTableSlot));
            HashTableSlot hs;
            pread(fd_, &hs, sizeof(hs), (off_t)offset);

            if (hs.client.isEmpty() || string(hs.client.client_id) == record.client_id)
            {
                HashTableSlot newhs = hs;
                newhs.client = record;
                if (pwrite(fd_, &newhs, sizeof(newhs), (off_t)offset) != (ssize_t)sizeof(newhs))
                {
                    cerr << "Failed to save client record errno=" << errno << endl;
                }
                fsync(fd_);
                return;
            }
        }
    }

    void removeClientFromHashTableLocked(const string &client_id)
    {
        uint32_t hash = ClientRecord::hashString(client_id);
        size_t slot = hash % HASH_TABLE_SIZE;

        for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        {
            size_t idx = (slot + i) % HASH_TABLE_SIZE;
            uint64_t offset = hash_table_offset_ + (idx * sizeof(HashTableSlot));
            HashTableSlot hs;
            if (pread(fd_, &hs, sizeof(hs), (off_t)offset) != (ssize_t)sizeof(hs))
                continue;

            if (string(hs.client.client_id) == client_id && !hs.client.isEmpty())
            {
                // Clear this slot completely
                HashTableSlot empty;
                memset(&empty, 0, sizeof(empty));
                if (pwrite(fd_, &empty, sizeof(empty), (off_t)offset) != (ssize_t)sizeof(empty))
                {
                    cerr << "Failed to clear hash table slot errno=" << errno << endl;
                }
                fsync(fd_);
                cout << "✓ Client removed from hash table: " << client_id << endl;
                return;
            }
        }
    }

    // // FIXED: New function to properly remove client from hash table
    // void removeClientFromHashTableLocked(const string &client_id)
    // {
    //     uint32_t hash = ClientRecord::hashString(client_id);
    //     size_t slot = hash % HASH_TABLE_SIZE;

    //     for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
    //     {
    //         size_t idx = (slot + i) % HASH_TABLE_SIZE;
    //         uint64_t offset = hash_table_offset_ + (idx * sizeof(HashTableSlot));
    //         HashTableSlot hs;
    //         if (pread(fd_, &hs, sizeof(hs), (off_t)offset) != (ssize_t)sizeof(hs))
    //             continue;

    //         if (string(hs.client.client_id) == client_id)
    //         {
    //             // Clear this slot completely
    //             HashTableSlot empty;
    //             memset(&empty, 0, sizeof(empty));
    //             if (pwrite(fd_, &empty, sizeof(empty), (off_t)offset) != (ssize_t)sizeof(empty))
    //             {
    //                 cerr << "Failed to clear hash table slot errno=" << errno << endl;
    //             }
    //             fsync(fd_);
    //             return;
    //         }
    //     }
    // }

    bool findClientByHashNoLock(uint32_t hash, ClientRecord &out)
    {
        for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
        {
            uint64_t offset = hash_table_offset_ + (i * sizeof(HashTableSlot));
            HashTableSlot hs;
            if (pread(fd_, &hs, sizeof(hs), (off_t)offset) != (ssize_t)sizeof(hs))
                continue;

            if (!hs.client.isEmpty() && hs.client.client_hash == hash && !hs.client.isEmpty())
            {
                out = hs.client;
                return true;
            }
        }
        return false;
    }

    void insertIntoBPlusTreeLocked(uint64_t node_offset, uint64_t key, uint64_t data_offset)
    {
        if (node_offset == 0)
        {
            cerr << "ERROR: Attempting to insert into null node" << endl;
            return;
        }

        BPlusTreeNode node;
        if (!readNodeLocked(node_offset, node))
        {
            cerr << "ERROR: Failed to read B+ tree node at offset " << node_offset << endl;
            return;
        }

        if (node.num_keys > BPLUS_ORDER)
        {
            cerr << "ERROR: Corrupted node! num_keys=" << node.num_keys << " exceeds BPLUS_ORDER=" << BPLUS_ORDER << endl;
            node.num_keys = 0;
            node.is_leaf = true;
            writeNodeLocked(node_offset, node);
            return;
        }

        if (node.is_leaf)
        {
            if (node.num_keys < BPLUS_ORDER)
            {
                insertIntoLeafLocked(node, key, data_offset);
                writeNodeLocked(node_offset, node);
            }
            else
            {
                splitLeafNodeLocked(node_offset, node, key, data_offset);
            }
        }
        else
        {
            uint32_t child_idx = 0;
            while (child_idx < node.num_keys && key >= node.keys[child_idx])
                child_idx++;
            uint64_t child_off = node.children[child_idx];
            if (child_off == 0)
            {
                cerr << "ERROR: Invalid child offset during internal insert" << endl;
                return;
            }
            insertIntoBPlusTreeLocked(child_off, key, data_offset);
        }
    }

    void insertIntoLeafLocked(BPlusTreeNode &node, uint64_t key, uint64_t data_offset)
    {
        int insert_pos = (int)node.num_keys;
        while (insert_pos > 0 && node.keys[insert_pos - 1] > key)
        {
            node.keys[insert_pos] = node.keys[insert_pos - 1];
            node.children[insert_pos] = node.children[insert_pos - 1];
            insert_pos--;
        }
        node.keys[insert_pos] = key;
        node.children[insert_pos] = data_offset;
        node.num_keys++;
    }

    void splitLeafNodeLocked(uint64_t node_offset, BPlusTreeNode &node, uint64_t key, uint64_t data_offset)
    {
        vector<uint64_t> temp_keys;
        vector<uint64_t> temp_children;
        temp_keys.reserve(BPLUS_ORDER + 1);
        temp_children.reserve(BPLUS_ORDER + 1);

        bool inserted = false;
        for (uint32_t i = 0; i < node.num_keys; ++i)
        {
            if (!inserted && key < node.keys[i])
            {
                temp_keys.push_back(key);
                temp_children.push_back(data_offset);
                inserted = true;
            }
            temp_keys.push_back(node.keys[i]);
            temp_children.push_back(node.children[i]);
        }
        if (!inserted)
        {
            temp_keys.push_back(key);
            temp_children.push_back(data_offset);
        }

        uint32_t total_keys = (uint32_t)temp_keys.size();
        uint32_t split_point = (total_keys + 1) / 2;

        uint64_t new_node_offset = allocateNodeLocked();

        BPlusTreeNode new_node;
        memset(&new_node, 0, sizeof(new_node));
        new_node.is_leaf = true;
        new_node.parent_offset = node.parent_offset;
        new_node.next_leaf = node.next_leaf;

        node.num_keys = split_point;
        for (uint32_t k = 0; k < split_point; ++k)
        {
            node.keys[k] = temp_keys[k];
            node.children[k] = temp_children[k];
        }
        for (uint32_t k = split_point; k < BPLUS_ORDER; ++k)
        {
            node.keys[k] = 0;
            node.children[k] = 0;
        }

        new_node.num_keys = total_keys - split_point;
        for (uint32_t k = 0; k < new_node.num_keys; ++k)
        {
            new_node.keys[k] = temp_keys[split_point + k];
            new_node.children[k] = temp_children[split_point + k];
        }
        for (uint32_t k = new_node.num_keys; k < BPLUS_ORDER; ++k)
        {
            new_node.keys[k] = 0;
            new_node.children[k] = 0;
        }

        node.next_leaf = new_node_offset;

        writeNodeLocked(node_offset, node);
        writeNodeLocked(new_node_offset, new_node);

        uint64_t promoted_key = temp_keys[split_point];
        if (node.parent_offset == 0)
            createNewRootLocked(node_offset, new_node_offset, promoted_key);
        else
            insertKeyIntoParentLocked(node.parent_offset, promoted_key, new_node_offset);
    }

    void insertKeyIntoParentLocked(uint64_t parent_offset, uint64_t key, uint64_t child_offset)
    {
        if (parent_offset == 0)
            return;

        BPlusTreeNode parent;
        if (!readNodeLocked(parent_offset, parent))
        {
            cerr << "ERROR: Failed to read parent node at " << parent_offset << endl;
            return;
        }

        if (parent.num_keys > BPLUS_ORDER)
        {
            cerr << "ERROR: Corrupted parent node at " << parent_offset << endl;
            return;
        }

        if (parent.num_keys < BPLUS_ORDER)
        {
            insertIntoInternalNodeLocked(parent, key, child_offset);
            writeNodeLocked(parent_offset, parent);
            updateChildParentLocked(child_offset, parent_offset);
        }
        else
        {
            splitInternalNodeLocked(parent_offset, parent, key, child_offset);
        }
    }

    void insertIntoInternalNodeLocked(BPlusTreeNode &node, uint64_t key, uint64_t child_offset)
    {
        int insert_pos = (int)node.num_keys;
        while (insert_pos > 0 && node.keys[insert_pos - 1] > key)
        {
            node.keys[insert_pos] = node.keys[insert_pos - 1];
            node.children[insert_pos + 1] = node.children[insert_pos];
            insert_pos--;
        }
        node.keys[insert_pos] = key;
        node.children[insert_pos + 1] = child_offset;
        node.num_keys++;
    }

    void splitInternalNodeLocked(uint64_t node_offset, BPlusTreeNode &node, uint64_t key, uint64_t child_offset)
    {
        vector<uint64_t> temp_keys;
        vector<uint64_t> temp_children;
        temp_keys.reserve(BPLUS_ORDER + 1);
        temp_children.reserve(BPLUS_ORDER + 2);

        for (uint32_t i = 0; i <= node.num_keys; ++i)
            temp_children.push_back(node.children[i]);

        bool inserted = false;
        uint32_t i = 0;
        for (; i < node.num_keys; ++i)
        {
            if (!inserted && key < node.keys[i])
            {
                temp_keys.push_back(key);
                temp_children.insert(temp_children.begin() + i + 1, child_offset);
                inserted = true;
                break;
            }
        }
        if (!inserted)
        {
            temp_keys = vector<uint64_t>(node.keys, node.keys + node.num_keys);
            temp_keys.push_back(key);
            temp_children.push_back(child_offset);
        }
        else
        {
            for (; i < node.num_keys; ++i)
                temp_keys.push_back(node.keys[i]);
        }

        uint32_t total_keys = (uint32_t)temp_keys.size();
        uint32_t split_point = total_keys / 2;

        uint64_t new_node_offset = allocateNodeLocked();

        BPlusTreeNode new_node;
        memset(&new_node, 0, sizeof(new_node));
        new_node.is_leaf = false;
        new_node.parent_offset = node.parent_offset;

        node.num_keys = split_point;
        for (uint32_t k = 0; k < split_point; ++k)
        {
            node.keys[k] = temp_keys[k];
            node.children[k] = temp_children[k];
        }
        node.children[split_point] = temp_children[split_point];

        uint64_t promoted_key = temp_keys[split_point];

        new_node.num_keys = total_keys - split_point - 1;
        for (uint32_t k = 0; k < new_node.num_keys; ++k)
        {
            new_node.keys[k] = temp_keys[split_point + 1 + k];
            new_node.children[k] = temp_children[split_point + 1 + k];
        }
        new_node.children[new_node.num_keys] = temp_children[total_keys];

        writeNodeLocked(node_offset, node);
        writeNodeLocked(new_node_offset, new_node);

        for (uint32_t k = 0; k <= new_node.num_keys; ++k)
            updateChildParentLocked(new_node.children[k], new_node_offset);

        if (node.parent_offset == 0)
            createNewRootLocked(node_offset, new_node_offset, promoted_key);
        else
            insertKeyIntoParentLocked(node.parent_offset, promoted_key, new_node_offset);
    }

    void createNewRootLocked(uint64_t left_child, uint64_t right_child, uint64_t key)
    {
        uint64_t new_root_offset = allocateNodeLocked();

        BPlusTreeNode root;
        memset(&root, 0, sizeof(root));
        root.is_leaf = false;
        root.num_keys = 1;
        root.keys[0] = key;
        root.children[0] = left_child;
        root.children[1] = right_child;
        root.parent_offset = 0;

        writeNodeLocked(new_root_offset, root);
        updateChildParentLocked(left_child, new_root_offset);
        updateChildParentLocked(right_child, new_root_offset);
    }

    void updateChildParentLocked(uint64_t child_offset, uint64_t parent_offset)
    {
        if (child_offset == 0)
            return;
        BPlusTreeNode child;
        if (!readNodeLocked(child_offset, child))
            return;
        child.parent_offset = parent_offset;
        writeNodeLocked(child_offset, child);
    }

    uint64_t getCurrentRootLocked(uint64_t start_offset)
    {
        if (start_offset == 0)
            return 0;
        uint64_t current = start_offset;
        BPlusTreeNode node;
        if (!readNodeLocked(current, node))
            return start_offset;
        while (node.parent_offset != 0)
        {
            current = node.parent_offset;
            if (!readNodeLocked(current, node))
                break;
        }
        return current;
    }

    void queryBPlusTreeRangeLocked(uint64_t node_offset,
                                   uint64_t start_timestamp,
                                   uint64_t end_timestamp,
                                   vector<KeystrokeEvent> &results)
    {
        if (node_offset == 0)
            return;

        BPlusTreeNode node;
        if (!readNodeLocked(node_offset, node))
            return;

        if (node.is_leaf)
        {
            for (uint32_t i = 0; i < node.num_keys; ++i)
            {
                uint64_t timestamp = node.keys[i];
                uint64_t data_offset = node.children[i];

                if (timestamp >= start_timestamp && timestamp <= end_timestamp)
                {
                    KeystrokeEvent event;
                    if (pread(fd_, &event, sizeof(event), (off_t)data_offset) == (ssize_t)sizeof(event))
                    {
                        results.push_back(event);
                    }
                }
            }
        }
        else
        {
            for (uint32_t i = 0; i <= node.num_keys; ++i)
            {
                if (i < node.num_keys && start_timestamp > node.keys[i])
                    continue;
                if (i > 0 && end_timestamp < node.keys[i - 1])
                    break;

                queryBPlusTreeRangeLocked(node.children[i], start_timestamp, end_timestamp, results);
            }
        }
    }

    void queryBPlusTreeRangeLocked(uint64_t node_offset,
                                   uint64_t start_timestamp,
                                   uint64_t end_timestamp,
                                   vector<ClipboardEvent> &results)
    {
        if (node_offset == 0)
            return;

        BPlusTreeNode node;
        if (!readNodeLocked(node_offset, node))
            return;

        if (node.is_leaf)
        {
            for (uint32_t i = 0; i < node.num_keys; ++i)
            {
                uint64_t timestamp = node.keys[i];
                uint64_t data_offset = node.children[i];

                if (timestamp >= start_timestamp && timestamp <= end_timestamp)
                {
                    ClipboardEvent event;
                    if (pread(fd_, &event, sizeof(event), (off_t)data_offset) == (ssize_t)sizeof(event))
                    {
                        results.push_back(event);
                    }
                }
            }
        }
        else
        {
            for (uint32_t i = 0; i <= node.num_keys; ++i)
            {
                if (i < node.num_keys && start_timestamp > node.keys[i])
                    continue;
                if (i > 0 && end_timestamp < node.keys[i - 1])
                    break;

                queryBPlusTreeRangeLocked(node.children[i], start_timestamp, end_timestamp, results);
            }
        }
    }

    void queryBPlusTreeRangeLocked(uint64_t node_offset,
                                   uint64_t start_timestamp,
                                   uint64_t end_timestamp,
                                   vector<WindowEvent> &results)
    {
        if (node_offset == 0)
            return;

        BPlusTreeNode node;
        if (!readNodeLocked(node_offset, node))
            return;

        if (node.is_leaf)
        {
            for (uint32_t i = 0; i < node.num_keys; ++i)
            {
                uint64_t timestamp = node.keys[i];
                uint64_t data_offset = node.children[i];

                if (timestamp >= start_timestamp && timestamp <= end_timestamp)
                {
                    WindowEvent event;
                    if (pread(fd_, &event, sizeof(event), (off_t)data_offset) == (ssize_t)sizeof(event))
                    {
                        results.push_back(event);
                    }
                }
            }
        }
        else
        {
            for (uint32_t i = 0; i <= node.num_keys; ++i)
            {
                if (i < node.num_keys && start_timestamp > node.keys[i])
                    continue;
                if (i > 0 && end_timestamp < node.keys[i - 1])
                    break;

                queryBPlusTreeRangeLocked(node.children[i], start_timestamp, end_timestamp, results);
            }
        }
    }

    void reclaimBPlusTreeSpaceLocked(uint64_t node_offset)
    {
        if (node_offset == 0)
            return;
        BPlusTreeNode node;
        if (!readNodeLocked(node_offset, node))
            return;

        if (node.is_leaf)
        {
            for (uint32_t i = 0; i < node.num_keys; ++i)
                free_data_blocks_.push(node.children[i]);

            if (node.next_leaf != 0)
                reclaimBPlusTreeSpaceLocked(node.next_leaf);
        }
        else
        {
            for (uint32_t i = 0; i <= node.num_keys; ++i)
                reclaimBPlusTreeSpaceLocked(node.children[i]);
        }

        free_bplus_nodes_.push(node_offset);
    }
};

#endif
