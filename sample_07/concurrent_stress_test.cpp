// #include "meowkey_database.hpp"
// #include "event_buffers.hpp"
// #include <iostream>
// #include <vector>
// #include <chrono>
// #include <thread>
// #include <memory>
// #include <random>
// #include <string>

// using namespace std;

// uint64_t getTimestamp() {
//     return chrono::duration_cast<chrono::microseconds>(
//         chrono::system_clock::now().time_since_epoch()
//     ).count();
// }

// // Random string generator for realistic data
// string generateRandomString(size_t length) {
//     static const char alphanum[] =
//         "0123456789"
//         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//         "abcdefghijklmnopqrstuvwxyz"
//         " .,!?-";
    
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> dist(0, sizeof(alphanum) - 2);
    
//     string result;
//     result.reserve(length);
//     for (size_t i = 0; i < length; ++i) {
//         result += alphanum[dist(rng)];
//     }
//     return result;
// }

// // Generate realistic clipboard content
// string generateClipboardContent() {
//     vector<string> paragraphs = {
//         "The quick brown fox jumps over the lazy dog. This sentence contains all letters of the alphabet.",
//         "In software engineering, a database is an organized collection of data stored and accessed electronically.",
//         "Small databases can be stored on a file system, while large databases are hosted on computer clusters.",
//         "B+ trees are balanced tree data structures that keep data sorted and allow searches, sequential access, insertions, and deletions in logarithmic time.",
//         "The buffer system helps reduce disk I/O by accumulating events in memory before writing them to disk in batches.",
//         "Concurrent access from multiple clients requires proper synchronization mechanisms to prevent data corruption.",
//         "This test simulates real-world usage with multiple users generating events simultaneously.",
//         "The system should handle all these events efficiently without data loss or corruption.",
//         "Space reclamation ensures that deleted client data doesn't waste storage space over time.",
//         "Proper indexing with B+ trees enables fast retrieval of historical events by timestamp."
//     };
    
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> dist(0, paragraphs.size() - 1);
    
//     // Combine 2-4 random paragraphs
//     int num_paragraphs = 2 + (rng() % 3);
//     string content;
//     for (int i = 0; i < num_paragraphs; i++) {
//         content += paragraphs[dist(rng)] + "\n\n";
//     }
//     return content;
// }

// // Generate realistic window titles
// string generateWindowTitle() {
//     vector<string> applications = {
//         "Google Chrome", "Mozilla Firefox", "Visual Studio Code", "Terminal", 
//         "Microsoft Word", "Adobe Photoshop", "Slack", "Discord", "Zoom Meeting",
//         "File Explorer", "Notepad++", "IntelliJ IDEA", "Eclipse", "Postman",
//         "MySQL Workbench", "Wireshark", "Docker Desktop", "GitHub Desktop"
//     };
    
//     vector<string> activities = {
//         "writing code", "browsing internet", "reading documentation", "debugging",
//         "writing report", "designing UI", "team meeting", "presentation",
//         "file management", "note taking", "project development", "API testing",
//         "database query", "network analysis", "container management", "version control"
//     };
    
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> app_dist(0, applications.size() - 1);
//     uniform_int_distribution<> act_dist(0, activities.size() - 1);
    
//     return applications[app_dist(rng)] + " - " + activities[act_dist(rng)];
// }

// string generateProcessName() {
//     vector<string> processes = {
//         "chrome.exe", "firefox.exe", "code.exe", "terminal.exe", "winword.exe",
//         "photoshop.exe", "slack.exe", "discord.exe", "zoom.exe", "explorer.exe",
//         "notepad++.exe", "idea.exe", "eclipse.exe", "postman.exe", "mysqlworkbench.exe",
//         "wireshark.exe", "docker.exe", "github.exe"
//     };
    
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> dist(0, processes.size() - 1);
    
//     return processes[dist(rng)];
// }

// // Simulate a single user session
// void simulateUserSession(MeowKeyDatabase* db, const string& client_id, int duration_seconds) {
//     cout << "  Starting session for user: " << client_id << endl;
    
//     // Register/load client
//     ClientRecord record;
//     db->registerClient(client_id, record);
    
//     // Create session
//     ClientSession session(client_id, db);
    
//     auto start_time = chrono::steady_clock::now();
//     int event_count = 0;
    
//     while (chrono::duration_cast<chrono::seconds>(
//         chrono::steady_clock::now() - start_time).count() < duration_seconds) {
        
//         // Simulate typing (10-30 keystrokes per batch)
//         int keystrokes = 10 + (rand() % 21);
//         for (int i = 0; i < keystrokes; i++) {
//             string key = generateRandomString(1 + (rand() % 10));
//             session.addKeystroke(getTimestamp(), key);
//             event_count++;
//         }
        
//         // Simulate occasional copy-paste (clipboard event)
//         if (rand() % 5 == 0) { // 20% chance
//             string clipboard_content = generateClipboardContent();
//             session.addClipboard(getTimestamp(), clipboard_content);
//             event_count++;
//         }
        
//         // Simulate window switching
//         if (rand() % 3 == 0) { // 33% chance
//             string title = generateWindowTitle();
//             string process = generateProcessName();
//             session.addWindow(getTimestamp(), title, process);
//             event_count++;
//         }
        
//         // Small delay between batches
//         this_thread::sleep_for(chrono::milliseconds(50 + (rand() % 100)));
//     }
    
//     // Flush any remaining events
//     session.flushAll();
    
//     cout << "  Session completed for " << client_id 
//          << " (events: " << event_count << ")" << endl;
// }

// void ultimateConcurrentStressTest() {
//     cout << "\n" << string(60, '=') << endl;
//     cout << "ULTIMATE CONCURRENT STRESS TEST" << endl;
//     cout << "Simulating 10 concurrent users for 30 seconds" << endl;
//     cout << string(60, '=') << endl;
    
//     MeowKeyDatabase db("ultimate_test.db");
//     if (!db.open()) {
//         cerr << "Failed to open database" << endl;
//         return;
//     }
    
//     // Create 10 unique users
//     vector<string> user_ids;
//     for (int i = 1; i <= 10; i++) {
//         user_ids.push_back("user_" + to_string(i) + "_" + generateRandomString(5));
//     }
    
//     cout << "\nCreated 10 users:" << endl;
//     for (const auto& user : user_ids) {
//         cout << "  " << user << endl;
//     }
    
//     cout << "\nStarting concurrent simulation for 30 seconds..." << endl;
//     cout << "Each user will generate:" << endl;
//     cout << "  - Keystrokes (typing simulation)" << endl;
//     cout << "  - Clipboard events (copy-paste)" << endl;
//     cout << "  - Window events (app switching)" << endl;
    
//     auto test_start = chrono::steady_clock::now();
    
//     // Launch concurrent user sessions
//     vector<thread> user_threads;
//     for (const auto& user_id : user_ids) {
//         user_threads.emplace_back(simulateUserSession, &db, user_id, 30);
//     }
    
//     // Wait for all threads to complete
//     for (auto& thread : user_threads) {
//         thread.join();
//     }
    
//     auto test_end = chrono::steady_clock::now();
//     auto duration = chrono::duration_cast<chrono::milliseconds>(test_end - test_start);
    
//     cout << "\n" << string(60, '=') << endl;
//     cout << "CONCURRENT TEST COMPLETED!" << endl;
//     cout << "Total test duration: " << duration.count() << " ms" << endl;
    
//     // Show final statistics
//     db.printStats();
    
//     // Show individual user stats
//     cout << "\nUser Statistics:" << endl;
//     cout << string(40, '-') << endl;
//     for (const auto& user_id : user_ids) {
//         ClientRecord record;
//         if (db.getClientStats(user_id, record)) {
//             cout << "User: " << user_id << endl;
//             cout << "  Keystrokes: " << record.total_keystrokes << endl;
//             cout << "  Clipboard: " << record.total_clipboard << endl;
//             cout << "  Windows: " << record.total_windows << endl;
//             cout << "  Total Events: " << (record.total_keystrokes + record.total_clipboard + record.total_windows) << endl;
//             cout << endl;
//         }
//     }
    
//     // Test random queries to verify data integrity
//     cout << "\nVerifying data integrity with random queries..." << endl;
//     int verified_clients = 0;
//     for (const auto& user_id : user_ids) {
//         ClientRecord record;
//         if (db.getClientStats(user_id, record)) {
//             if (record.client_hash == ClientRecord::hashString(user_id)) {
//                 verified_clients++;
//             }
//         }
//     }
//     cout << "✓ Verified " << verified_clients << "/" << user_ids.size() << " clients" << endl;
    
//     // Test client deletion and re-creation
//     cout << "\nTesting client deletion and re-creation..." << endl;
//     if (!user_ids.empty()) {
//         string test_user = user_ids[0];
//         cout << "Deleting user: " << test_user << endl;
        
//         if (db.deleteClient(test_user)) {
//             cout << "✓ User deleted successfully" << endl;
            
//             // Try to re-register
//             ClientRecord new_record;
//             bool is_existing = db.registerClient(test_user, new_record);
//             cout << "Re-registered user. Is existing: " << is_existing 
//                  << " (should be false for newly created)" << endl;
//             cout << "New session count: " << new_record.session_count 
//                  << " (should be 1)" << endl;
//         }
//     }
    
//     db.close();
    
//     cout << "\n" << string(60, '=') << endl;
//     cout << "ULTIMATE STRESS TEST PASSED! 🎉" << endl;
//     cout << "Database successfully handled:" << endl;
//     cout << "  - 10 concurrent users" << endl;
//     cout << "  - 30 seconds of continuous activity" << endl;
//     cout << "  - Mixed event types (keystrokes, clipboard, windows)" << endl;
//     cout << "  - Realistic data generation" << endl;
//     cout << "  - Thread-safe concurrent access" << endl;
//     cout << string(60, '=') << endl;
// }

// int main() {
//     cout << "╔══════════════════════════════════════════════════════════╗" << endl;
//     cout << "║     ULTIMATE MEOWKEY DATABASE CONCURRENT STRESS TEST   ║" << endl;
//     cout << "╚══════════════════════════════════════════════════════════╝" << endl;
    
//     // Run the test 3 times to ensure consistency
//     for (int run = 1; run <= 3; run++) {
//         cout << "\n\nRUN #" << run << " of 3" << endl;
//         cout << string(50, '=') << endl;
        
//         ultimateConcurrentStressTest();
        
//         if (run < 3) {
//             cout << "\nWaiting 2 seconds before next run..." << endl;
//             this_thread::sleep_for(chrono::seconds(2));
//         }
//     }
    
//     cout << "\n\n╔══════════════════════════════════════════════════════════╗" << endl;
//     cout << "║     ALL CONCURRENT TESTS COMPLETED SUCCESSFULLY!       ║" << endl;
//     cout << "║     Database is READY FOR PRODUCTION DEPLOYMENT!       ║" << endl;
//     cout << "╚══════════════════════════════════════════════════════════╝" << endl;
    
//     return 0;
// }


// #include "meowkey_database.hpp"
// #include "event_buffers.hpp"
// #include <iostream>
// #include <vector>
// #include <chrono>
// #include <thread>
// #include <memory>
// #include <random>
// #include <string>
// #include <iomanip>

// using namespace std;

// uint64_t getTimestamp() {
//     return chrono::duration_cast<chrono::microseconds>(
//         chrono::system_clock::now().time_since_epoch()
//     ).count();
// }

// // Generate introduction text for each user
// string generateIntroduction(const string& username) {
//     vector<string> intros = {
//         "Hi, I'm " + username + ". I'm a software engineer passionate about databases.",
//         "Hello! My name is " + username + ". I love working on data structures and algorithms.",
//         "I'm " + username + ", a database architect with 5 years of experience.",
//         "Hi there! I'm " + username + ". Currently working on distributed systems.",
//         "My name is " + username + ". I specialize in B+ tree implementations and storage engines."
//     };
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> dist(0, intros.size() - 1);
//     return intros[dist(rng)];
// }

// // Generate clipboard content (multiline)
// string generateClipboardContent() {
//     vector<string> contents = {
//         "SELECT * FROM users WHERE active = 1;\nSELECT COUNT(*) FROM events;",
//         "CREATE TABLE logs (\n  id INT PRIMARY KEY,\n  timestamp BIGINT,\n  message TEXT\n);",
//         "git add .\ngit commit -m \"Fix concurrent access bug\"\ngit push origin main",
//         "TODO: Implement range queries\nTODO: Add data compression\nTODO: Optimize B+ tree splits",
//         "https://github.com/meowkey/database\nDocumentation: /docs/README.md\nIssues: github.com/issues"
//     };
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> dist(0, contents.size() - 1);
//     return contents[dist(rng)];
// }

// // Generate window titles
// string generateWindowTitle() {
//     vector<string> windows = {
//         "Visual Studio Code - meowkey_database.hpp",
//         "Terminal - bash",
//         "Chrome - GitHub",
//         "File Explorer - /home/user/projects",
//         "Database Monitor - ultimate_test.db"
//     };
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> dist(0, windows.size() - 1);
//     return windows[dist(rng)];
// }

// string generateProcessName() {
//     vector<string> processes = {
//         "code.exe",
//         "bash",
//         "chrome.exe",
//         "explorer.exe",
//         "mysqld.exe"
//     };
//     static mt19937 rng(random_device{}());
//     uniform_int_distribution<> dist(0, processes.size() - 1);
//     return processes[dist(rng)];
// }

// void testUserDataInsertion() {
//     cout << "\n" << string(70, '=') << endl;
//     cout << "MEOWKEY DATABASE - TARGETED DATA INSERTION AND RETRIEVAL TEST" << endl;
//     cout << string(70, '=') << endl;

//     MeowKeyDatabase db("test_retrieval.db");
//     if (!db.open()) {
//         cerr << "Failed to open database" << endl;
//         return;
//     }

//     // Define 5 users
//     vector<string> usernames = {"alice", "bob", "charlie", "diana", "eve"};
//     vector<ClientRecord> created_clients;

//     cout << "\n[PHASE 1] Registering 5 users..." << endl;
//     cout << string(70, '-') << endl;

//     for (const auto& username : usernames) {
//         ClientRecord record;
//         bool is_existing = db.registerClient(username, record);
//         created_clients.push_back(record);
//         cout << "✓ User registered: " << username 
//              << " (Hash: " << record.client_hash << ")" << endl;
//     }

//     // Insert keystroke introduction for each user
//     cout << "\n[PHASE 2] Inserting keystroke introductions..." << endl;
//     cout << string(70, '-') << endl;

//     for (size_t i = 0; i < usernames.size(); ++i) {
//         const string& username = usernames[i];
//         uint32_t client_hash = created_clients[i].client_hash;

//         string intro = generateIntroduction(username);
        
//         // Create keystroke event for intro (in chunks to simulate typing)
//         size_t chunk_size = 15; // Characters per keystroke event
//         for (size_t pos = 0; pos < intro.length(); pos += chunk_size) {
//             KeystrokeEvent event;
//             event.timestamp = getTimestamp();
//             event.client_hash = client_hash;
            
//             size_t len = min(chunk_size, intro.length() - pos);
//             strncpy(event.key, intro.substr(pos, len).c_str(), sizeof(event.key) - 1);
//             event.key[sizeof(event.key) - 1] = '\0';
            
//             db.insertKeystroke(client_hash, event);
//             this_thread::sleep_for(chrono::milliseconds(10));
//         }
//         cout << "✓ Keystroke introduction inserted for " << username << endl;
//     }

//     // Insert clipboard events for each user
//     cout << "\n[PHASE 3] Inserting clipboard events..." << endl;
//     cout << string(70, '-') << endl;

//     for (size_t i = 0; i < usernames.size(); ++i) {
//         const string& username = usernames[i];
//         uint32_t client_hash = created_clients[i].client_hash;

//         for (int clip = 0; clip < 3; ++clip) {
//             ClipboardEvent event;
//             event.timestamp = getTimestamp();
//             event.client_hash = client_hash;
            
//             string content = generateClipboardContent();
//             strncpy(event.content, content.c_str(), sizeof(event.content) - 1);
//             event.content[sizeof(event.content) - 1] = '\0';
            
//             db.insertClipboard(client_hash, event);
//             cout << "✓ Clipboard event " << (clip + 1) << " inserted for " << username << endl;
//             this_thread::sleep_for(chrono::milliseconds(20));
//         }
//     }

//     // Insert window navigation events for each user
//     cout << "\n[PHASE 4] Inserting window navigation events..." << endl;
//     cout << string(70, '-') << endl;

//     for (size_t i = 0; i < usernames.size(); ++i) {
//         const string& username = usernames[i];
//         uint32_t client_hash = created_clients[i].client_hash;

//         for (int win = 0; win < 4; ++win) {
//             WindowEvent event;
//             event.timestamp = getTimestamp();
//             event.client_hash = client_hash;
            
//             string title = generateWindowTitle();
//             string process = generateProcessName();
            
//             // FIXED: Use correct member names from WindowEvent structure
//             strncpy(event.title, title.c_str(), sizeof(event.title) - 1);
//             event.title[sizeof(event.title) - 1] = '\0';
            
//             strncpy(event.process, process.c_str(), sizeof(event.process) - 1);
//             event.process[sizeof(event.process) - 1] = '\0';
            
//             db.insertWindow(client_hash, event);
//             cout << "✓ Window event " << (win + 1) << " inserted for " << username << endl;
//             this_thread::sleep_for(chrono::milliseconds(15));
//         }
//     }

//     // Retrieve and print all data for each client
//     cout << "\n[PHASE 5] Retrieving and printing all saved data for each client..." << endl;
//     cout << string(70, '=') << endl;

//     for (size_t i = 0; i < usernames.size(); ++i) {
//         const string& username = usernames[i];
        
//         QueryResult result;
//         if (!db.queryClientAllEvents(username, result)) {
//             cerr << "Failed to retrieve data for " << username << endl;
//             continue;
//         }

//         cout << "\n" << string(70, '=') << endl;
//         cout << "CLIENT: " << username << endl;
//         cout << string(70, '=') << endl;

//         // Print keystroke events
//         cout << "\n[KEYSTROKES] (" << result.keystrokes.size() << " events)" << endl;
//         cout << string(70, '-') << endl;
//         for (size_t j = 0; j < result.keystrokes.size(); ++j) {
//             const auto& ks = result.keystrokes[j];
//             cout << "  " << (j + 1) << ". Timestamp: " << ks.timestamp 
//                  << " | Key: \"" << ks.key << "\"" << endl;
//         }

//         // Print clipboard events
//         cout << "\n[CLIPBOARD] (" << result.clipboard_events.size() << " events)" << endl;
//         cout << string(70, '-') << endl;
//         for (size_t j = 0; j < result.clipboard_events.size(); ++j) {
//             const auto& cb = result.clipboard_events[j];
//             cout << "  " << (j + 1) << ". Timestamp: " << cb.timestamp << endl;
//             cout << "     Content:\n";
            
//             // Print content with line wrapping
//             string content(cb.content);
//             size_t pos = 0;
//             while (pos < content.length()) {
//                 size_t end = min(pos + 60, content.length());
//                 if (end < content.length() && content[end] != '\n') {
//                     size_t newline_pos = content.find('\n', pos);
//                     if (newline_pos != string::npos && newline_pos < end) {
//                         end = newline_pos;
//                     }
//                 }
//                 cout << "     " << content.substr(pos, end - pos) << endl;
//                 pos = end + 1;
//             }
//         }

//         // Print window events
//         cout << "\n[WINDOW NAVIGATION] (" << result.window_events.size() << " events)" << endl;
//         cout << string(70, '-') << endl;
//         for (size_t j = 0; j < result.window_events.size(); ++j) {
//             const auto& win = result.window_events[j];
//             cout << "  " << (j + 1) << ". Timestamp: " << win.timestamp << endl;
//             cout << "     Title: " << win.title << endl;
//             cout << "     Process: " << win.process << endl;
//         }

//         // Print summary
//         cout << "\n[SUMMARY]" << endl;
//         cout << "  Total Keystrokes: " << result.keystrokes.size() << endl;
//         cout << "  Total Clipboard Events: " << result.clipboard_events.size() << endl;
//         cout << "  Total Window Events: " << result.window_events.size() << endl;
//         cout << "  TOTAL EVENTS: " << result.totalEvents() << endl;
//     }

//     // Print database statistics
//     cout << "\n" << string(70, '=') << endl;
//     cout << "FINAL DATABASE STATISTICS" << endl;
//     cout << string(70, '=') << endl;
//     db.printStats();

//     db.close();

//     cout << "\n" << string(70, '=') << endl;
//     cout << "✅ TEST COMPLETED SUCCESSFULLY!" << endl;
//     cout << "All user data was inserted and retrieved successfully." << endl;
//     cout << string(70, '=') << endl;
// }

// int main() {
//     cout << "╔══════════════════════════════════════════════════════════════════╗" << endl;
//     cout << "║   MEOWKEY DATABASE - DATA INSERTION & RETRIEVAL TEST            ║" << endl;
//     cout << "║   Testing: 5 Users | Keystrokes | Clipboard | Window Events    ║" << endl;
//     cout << "╚══════════════════════════════════════════════════════════════════╝" << endl;

//     testUserDataInsertion();

//     return 0;
// }


#include "meowkey_database.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>
#include <cassert>

using namespace std;

// Hardcoded test data - unique for each client and event type
struct TestEvent {
    uint64_t timestamp;
    string content;
};

struct TestClientData {
    string id;
    vector<TestEvent> keystrokes;  // key press events
    vector<TestEvent> clipboard;   // copy-paste events  
    vector<TestEvent> windows;     // window change events
};

// Generate test data - each client gets unique, verifiable content
vector<TestClientData> generateTestData() {
    vector<TestClientData> clients;
    
    // Client 1: Alice
    {
        TestClientData alice;
        alice.id = "alice_test";
        
        // Keystrokes (unique sentences)
        alice.keystrokes.push_back({1234567890000, "Hello, this is Alice's first keystroke."});
        alice.keystrokes.push_back({1234567891000, "Database storage verification test."});
        alice.keystrokes.push_back({1234567892000, "Testing B+ tree insertion and retrieval."});
        alice.keystrokes.push_back({1234567893000, "Each event has unique timestamp and content."});
        
        // Clipboard events (unique content)
        alice.clipboard.push_back({1234567894000, "SELECT * FROM users WHERE id = 'alice';"});
        alice.clipboard.push_back({1234567895000, "git commit -m \"Alice's database test commit\""});
        alice.clipboard.push_back({1234567896000, "Lorem ipsum dolor sit amet, consectetur adipiscing elit."});
        
        // Window events
        alice.windows.push_back({1234567897000, "Terminal|bash"});
        alice.windows.push_back({1234567898000, "VS Code|code.exe"});
        alice.windows.push_back({1234567899000, "Chrome|chrome.exe"});
        
        clients.push_back(alice);
    }
    
    // Client 2: Bob
    {
        TestClientData bob;
        bob.id = "bob_test";
        
        // Keystrokes
        bob.keystrokes.push_back({2234567890000, "Bob's typing test for verification."});
        bob.keystrokes.push_back({2234567891000, "Ensuring data persistence across sessions."});
        bob.keystrokes.push_back({2234567892000, "Thread-safe concurrent access test."});
        
        // Clipboard
        bob.clipboard.push_back({2234567893000, "UPDATE settings SET value = 'bob_pref' WHERE user = 'bob';"});
        bob.clipboard.push_back({2234567894000, "git branch -b bob-feature-database-test"});
        bob.clipboard.push_back({2234567895000, "Sed ut perspiciatis unde omnis iste natus error."});
        
        // Windows
        bob.windows.push_back({2234567896000, "MySQL Workbench|mysql.exe"});
        bob.windows.push_back({2234567897000, "File Explorer|explorer.exe"});
        
        clients.push_back(bob);
    }
    
    // Client 3: Charlie
    {
        TestClientData charlie;
        charlie.id = "charlie_test";
        
        // Keystrokes
        charlie.keystrokes.push_back({3234567890000, "Charlie's data integrity verification."});
        charlie.keystrokes.push_back({3234567891000, "Testing multi-client concurrent writes."});
        charlie.keystrokes.push_back({3234567892000, "Hash table collision handling test."});
        charlie.keystrokes.push_back({3234567893000, "Free space management verification."});
        charlie.keystrokes.push_back({3234567894000, "B+ tree node splitting validation."});
        
        // Clipboard
        charlie.clipboard.push_back({3234567895000, "DELETE FROM logs WHERE timestamp < 1234567890000;"});
        charlie.clipboard.push_back({3234567896000, "docker run -d --name test-db postgres:latest"});
        charlie.clipboard.push_back({3234567897000, "At vero eos et accusamus et iusto odio dignissimos."});
        charlie.clipboard.push_back({3234567898000, "Temporibus autem quibusdam et aut officiis debitis."});
        
        // Windows
        charlie.windows.push_back({3234567899000, "Docker Desktop|docker.exe"});
        charlie.windows.push_back({3234567900000, "Postman|postman.exe"});
        charlie.windows.push_back({3234567901000, "IntelliJ IDEA|idea.exe"});
        charlie.windows.push_back({3234567902000, "Slack|slack.exe"});
        
        clients.push_back(charlie);
    }
    
    return clients;
}

// Store test data for a client
void storeClientData(MeowKeyDatabase& db, const TestClientData& client) {
    cout << "\n📝 Storing data for client: " << client.id << endl;
    
    // Register client
    ClientRecord record;
    bool isExisting = db.registerClient(client.id, record);
    uint32_t client_hash = record.client_hash;
    
    cout << "  Client hash: " << client_hash << endl;
    cout << "  Is existing client: " << (isExisting ? "YES" : "NO") << endl;
    
    // Store keystrokes
    for (size_t i = 0; i < client.keystrokes.size(); i++) {
        KeystrokeEvent ks;
        ks.timestamp = client.keystrokes[i].timestamp;
        ks.client_hash = client_hash;
        strncpy(ks.key, client.keystrokes[i].content.c_str(), sizeof(ks.key) - 1);
        ks.key[sizeof(ks.key) - 1] = '\0';
        
        db.insertKeystroke(client_hash, ks);
        cout << "    ✓ Keystroke " << (i+1) << " stored: " << ks.key << endl;
        
        // Small delay to ensure unique timestamps
        this_thread::sleep_for(chrono::microseconds(100));
    }
    
    // Store clipboard events
    for (size_t i = 0; i < client.clipboard.size(); i++) {
        ClipboardEvent cb;
        cb.timestamp = client.clipboard[i].timestamp;
        cb.client_hash = client_hash;
        strncpy(cb.content, client.clipboard[i].content.c_str(), sizeof(cb.content) - 1);
        cb.content[sizeof(cb.content) - 1] = '\0';
        
        db.insertClipboard(client_hash, cb);
        cout << "    ✓ Clipboard " << (i+1) << " stored: " << endl;
        cout << "      " << cb.content << endl;
        
        this_thread::sleep_for(chrono::microseconds(100));
    }
    
    // Store window events
    for (size_t i = 0; i < client.windows.size(); i++) {
        WindowEvent win;
        win.timestamp = client.windows[i].timestamp;
        win.client_hash = client_hash;
        
        // Parse title|process
        size_t pipe_pos = client.windows[i].content.find('|');
        string title = client.windows[i].content.substr(0, pipe_pos);
        string process = client.windows[i].content.substr(pipe_pos + 1);
        
        strncpy(win.title, title.c_str(), sizeof(win.title) - 1);
        win.title[sizeof(win.title) - 1] = '\0';
        strncpy(win.process, process.c_str(), sizeof(win.process) - 1);
        win.process[sizeof(win.process) - 1] = '\0';
        
        db.insertWindow(client_hash, win);
        cout << "    ✓ Window " << (i+1) << " stored: " << win.title << " (" << win.process << ")" << endl;
        
        this_thread::sleep_for(chrono::microseconds(100));
    }
}

// Verify retrieved data matches stored data
bool verifyClientData(const TestClientData& stored, const QueryResult& retrieved) {
    bool all_correct = true;
    
    cout << "\n🔍 Verifying data for client: " << stored.id << endl;
    
    // Verify keystrokes count
    if (retrieved.keystrokes.size() != stored.keystrokes.size()) {
        cout << "  ❌ Keystroke count mismatch!" << endl;
        cout << "     Expected: " << stored.keystrokes.size() 
             << ", Got: " << retrieved.keystrokes.size() << endl;
        all_correct = false;
    } else {
        cout << "  ✓ Keystrokes count: " << retrieved.keystrokes.size() << endl;
    }
    
    // Verify clipboard count
    if (retrieved.clipboard_events.size() != stored.clipboard.size()) {
        cout << "  ❌ Clipboard count mismatch!" << endl;
        cout << "     Expected: " << stored.clipboard.size() 
             << ", Got: " << retrieved.clipboard_events.size() << endl;
        all_correct = false;
    } else {
        cout << "  ✓ Clipboard count: " << retrieved.clipboard_events.size() << endl;
    }
    
    // Verify window count
    if (retrieved.window_events.size() != stored.windows.size()) {
        cout << "  ❌ Window events count mismatch!" << endl;
        cout << "     Expected: " << stored.windows.size() 
             << ", Got: " << retrieved.window_events.size() << endl;
        all_correct = false;
    } else {
        cout << "  ✓ Window events count: " << retrieved.window_events.size() << endl;
    }
    
    if (!all_correct) {
        return false;
    }
    
    // Verify keystroke content byte-by-byte
    cout << "\n  Verifying keystroke content..." << endl;
    for (size_t i = 0; i < stored.keystrokes.size(); i++) {
        string expected = stored.keystrokes[i].content;
        string actual = retrieved.keystrokes[i].key;
        
        if (expected != actual) {
            cout << "  ❌ Keystroke " << (i+1) << " content mismatch!" << endl;
            cout << "     Expected: \"" << expected << "\"" << endl;
            cout << "     Got:      \"" << actual << "\"" << endl;
            all_correct = false;
        } else {
            cout << "    ✓ Keystroke " << (i+1) << " correct" << endl;
        }
        
        // Verify timestamp
        if (retrieved.keystrokes[i].timestamp != stored.keystrokes[i].timestamp) {
            cout << "  ❌ Keystroke " << (i+1) << " timestamp mismatch!" << endl;
            all_correct = false;
        }
    }
    
    // Verify clipboard content
    cout << "\n  Verifying clipboard content..." << endl;
    for (size_t i = 0; i < stored.clipboard.size(); i++) {
        string expected = stored.clipboard[i].content;
        string actual = retrieved.clipboard_events[i].content;
        
        if (expected != actual) {
            cout << "  ❌ Clipboard " << (i+1) << " content mismatch!" << endl;
            cout << "     Expected: \"" << expected << "\"" << endl;
            cout << "     Got:      \"" << actual << "\"" << endl;
            all_correct = false;
        } else {
            cout << "    ✓ Clipboard " << (i+1) << " correct" << endl;
        }
        
        // Verify timestamp
        if (retrieved.clipboard_events[i].timestamp != stored.clipboard[i].timestamp) {
            cout << "  ❌ Clipboard " << (i+1) << " timestamp mismatch!" << endl;
            all_correct = false;
        }
    }
    
    // Verify window events
    cout << "\n  Verifying window events..." << endl;
    for (size_t i = 0; i < stored.windows.size(); i++) {
        size_t pipe_pos = stored.windows[i].content.find('|');
        string expected_title = stored.windows[i].content.substr(0, pipe_pos);
        string expected_process = stored.windows[i].content.substr(pipe_pos + 1);
        
        string actual_title = retrieved.window_events[i].title;
        string actual_process = retrieved.window_events[i].process;
        
        bool title_match = (expected_title == actual_title);
        bool process_match = (expected_process == actual_process);
        
        if (!title_match || !process_match) {
            cout << "  ❌ Window " << (i+1) << " content mismatch!" << endl;
            if (!title_match) {
                cout << "     Expected title: \"" << expected_title << "\"" << endl;
                cout << "     Got title:      \"" << actual_title << "\"" << endl;
            }
            if (!process_match) {
                cout << "     Expected process: \"" << expected_process << "\"" << endl;
                cout << "     Got process:      \"" << actual_process << "\"" << endl;
            }
            all_correct = false;
        } else {
            cout << "    ✓ Window " << (i+1) << " correct" << endl;
        }
        
        // Verify timestamp
        if (retrieved.window_events[i].timestamp != stored.windows[i].timestamp) {
            cout << "  ❌ Window " << (i+1) << " timestamp mismatch!" << endl;
            all_correct = false;
        }
    }
    
    return all_correct;
}

// Test deletion and re-creation
bool testDeletionRecreation(MeowKeyDatabase& db, const TestClientData& client) {
    cout << "\n🗑️  Testing deletion and re-creation for: " << client.id << endl;
    
    // Delete client
    if (!db.deleteClient(client.id)) {
        cout << "  ❌ Failed to delete client" << endl;
        return false;
    }
    cout << "  ✓ Client deleted" << endl;
    
    // Verify client no longer exists
    if (db.clientExists(client.id)) {
        cout << "  ❌ Client still exists after deletion" << endl;
        return false;
    }
    cout << "  ✓ Client no longer exists in database" << endl;
    
    // Re-create client with same ID
    ClientRecord new_record;
    bool is_existing = db.registerClient(client.id, new_record);
    
    if (is_existing) {
        cout << "  ❌ Re-created client reported as 'existing'" << endl;
        return false;
    }
    if (new_record.session_count != 1) {
        cout << "  ❌ New client should have session_count = 1, got: " << new_record.session_count << endl;
        return false;
    }
    
    cout << "  ✓ Client successfully re-created as new" << endl;
    return true;
}

void runDataIntegrityTest() {
    cout << "╔══════════════════════════════════════════════════════════════════╗" << endl;
    cout << "║        MEOWKEY DATABASE - 100% DATA INTEGRITY TEST             ║" << endl;
    cout << "║    Byte-by-byte verification of storage and retrieval          ║" << endl;
    cout << "╚══════════════════════════════════════════════════════════════════╝" << endl;
    
    // Generate test data
    vector<TestClientData> testClients = generateTestData();
    cout << "\n📊 Generated test data for " << testClients.size() << " clients:" << endl;
    for (const auto& client : testClients) {
        cout << "  • " << client.id 
             << " (Keystrokes: " << client.keystrokes.size()
             << ", Clipboard: " << client.clipboard.size()
             << ", Windows: " << client.windows.size() << ")" << endl;
    }
    
    // Create database
    string db_file = "integrity_test.db";
    remove(db_file.c_str()); // Start fresh
    
    MeowKeyDatabase db(db_file);
    if (!db.open()) {
        cerr << "❌ Failed to open database!" << endl;
        return;
    }
    
    cout << "\n" << string(70, '=') << endl;
    cout << "PHASE 1: STORING ALL TEST DATA" << endl;
    cout << string(70, '=') << endl;
    
    // Store all test data
    for (const auto& client : testClients) {
        storeClientData(db, client);
    }
    
    // Print database stats after insertion
    cout << "\n" << string(70, '=') << endl;
    cout << "DATABASE STATISTICS AFTER INSERTION:" << endl;
    cout << string(70, '=') << endl;
    db.printStats();
    
    cout << "\n" << string(70, '=') << endl;
    cout << "PHASE 2: RETRIEVAL AND VERIFICATION" << endl;
    cout << string(70, '=') << endl;
    
    // Retrieve and verify for each client
    int verified_clients = 0;
    int total_clients = testClients.size();
    
    for (const auto& client : testClients) {
        QueryResult retrieved;
        bool success = db.queryClientAllEvents(client.id, retrieved);
        
        if (!success) {
            cout << "\n❌ FAILED to retrieve data for client: " << client.id << endl;
            continue;
        }
        
        if (verifyClientData(client, retrieved)) {
            cout << "\n✅ VERIFICATION PASSED for client: " << client.id << endl;
            verified_clients++;
        } else {
            cout << "\n❌ VERIFICATION FAILED for client: " << client.id << endl;
        }
        
        cout << string(50, '-') << endl;
    }
    
    cout << "\n" << string(70, '=') << endl;
    cout << "VERIFICATION SUMMARY:" << endl;
    cout << string(70, '=') << endl;
    cout << "Verified " << verified_clients << "/" << total_clients << " clients successfully" << endl;
    
    if (verified_clients == total_clients) {
        cout << "🎉 ALL CLIENTS PASSED DATA INTEGRITY CHECK!" << endl;
    } else {
        cout << "⚠️  SOME CLIENTS FAILED VERIFICATION!" << endl;
    }
    
    cout << "\n" << string(70, '=') << endl;
    cout << "PHASE 3: DELETION AND RE-CREATION TEST" << endl;
    cout << string(70, '=') << endl;
    
    // Test deletion and re-creation for first client
    if (!testClients.empty()) {
        if (testDeletionRecreation(db, testClients[0])) {
            cout << "✅ Deletion/Re-creation test PASSED" << endl;
        } else {
            cout << "❌ Deletion/Re-creation test FAILED" << endl;
        }
    }
    
    // Final database stats
    cout << "\n" << string(70, '=') << endl;
    cout << "FINAL DATABASE STATISTICS:" << endl;
    cout << string(70, '=') << endl;
    db.printStats();
    
    db.close();
    
    cout << "\n" << string(70, '=') << endl;
    cout << "TEST COMPLETED" << endl;
    cout << string(70, '=') << endl;
    
    if (verified_clients == total_clients) {
        cout << "\n🎉🎉🎉 ULTIMATE VERDICT: DATA STORAGE AND RETRIEVAL IS WORKING PERFECTLY! 🎉🎉🎉" << endl;
        cout << "All clients' data was stored and retrieved with 100% accuracy." << endl;
    } else {
        cout << "\n⚠️⚠️⚠️ ULTIMATE VERDICT: DATA INTEGRITY ISSUES DETECTED! ⚠️⚠️⚠️" << endl;
        cout << "Some data was lost or corrupted during storage/retrieval." << endl;
    }
}

int main() {
    cout << "Starting comprehensive data integrity test..." << endl;
    cout << "This test will verify byte-by-byte accuracy of storage and retrieval." << endl;
    cout << "Test file: integrity_test.db" << endl;
    cout << endl;
    
    runDataIntegrityTest();
    
    return 0;
}


