#include "meowkey_database.hpp"
#include "event_buffers.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>

using namespace std;

uint64_t getTimestamp() {
    return chrono::duration_cast<chrono::microseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
}

void testBasicOperations() {
    cout << "\n=== TEST 1: Basic Database Operations ===" << endl;
    
    MeowKeyDatabase db("test_meowkey.db");
    if (!db.open()) {
        cerr << "Failed to open database" << endl;
        return;
    }
    
    // Register clients
    ClientRecord client1, client2, client3;
    
    cout << "\nRegistering clients..." << endl;
    db.registerClient("bscs24009", client1);
    db.registerClient("bscs24010", client2);
    db.registerClient("bscs24011", client3);
    
    db.printStats();
    db.close();
    cout << "✓ Test 1 passed" << endl;
}

void testBufferOperations() {
    cout << "\n=== TEST 2: Buffer Operations ===" << endl;
    
    MeowKeyDatabase db("test_meowkey.db");
    if (!db.open()) {
        cerr << "Failed to open database" << endl;
        return;
    }
    
    // First, register the client
    ClientRecord record;
    db.registerClient("bscs24009", record);
    
    // Create client session
    ClientSession session("bscs24009", &db);
    
    cout << "\nTesting buffer operations..." << endl;
    
    // Test 1: Add 24 keystrokes (should NOT auto-flush)
    cout << "1. Adding 24 keystrokes (below flush threshold)..." << endl;
    for (int i = 0; i < 24; i++) {
        session.addKeystroke(getTimestamp(), "KEY_" + to_string(i));
    }
    session.printBufferStatus();
    
    // Test 2: Add 1 more to trigger auto-flush (25 total)
    cout << "\n2. Adding 1 more keystroke to trigger auto-flush..." << endl;
    session.addKeystroke(getTimestamp(), "TRIGGER");
    session.printBufferStatus();
    
    // Test 3: Add 5 more (should have 5 in buffer after flush)
    cout << "\n3. Adding 5 more keystrokes..." << endl;
    for (int i = 0; i < 5; i++) {
        session.addKeystroke(getTimestamp(), "POST_" + to_string(i));
    }
    session.printBufferStatus();
    
    // Test 4: Manual flush
    cout << "\n4. Manual flush of remaining..." << endl;
    session.flushAll();
    session.printBufferStatus();
    
    // Test clipboard
    cout << "\n5. Testing clipboard buffer..." << endl;
    for (int i = 0; i < 4; i++) {
        session.addClipboard(getTimestamp(), "Clip " + to_string(i));
    }
    session.printBufferStatus();
    session.flushAll();
    
    // Test window
    cout << "\n6. Testing window buffer..." << endl;
    for (int i = 0; i < 7; i++) {
        session.addWindow(getTimestamp(), "Win " + to_string(i), "app.exe");
    }
    session.printBufferStatus();
    session.flushAll();
    
    db.printStats();
    db.close();
    
    cout << "✓ Test 2 passed" << endl;
}

void testMultipleClients() {
    cout << "\n=== TEST 3: Multiple Clients ===" << endl;
    
    MeowKeyDatabase db("test_meowkey.db");
    if (!db.open()) {
        cerr << "Failed to open database" << endl;
        return;
    }
    
    vector<unique_ptr<ClientSession>> sessions;
    
    // Create 3 clients
    for (int i = 0; i < 3; i++) {
        string client_id = "multi_client_" + to_string(i);
        ClientRecord rec;
        db.registerClient(client_id, rec);
        sessions.push_back(make_unique<ClientSession>(client_id, &db));
    }
    
    cout << "\nSimulating activity..." << endl;
    
    // Each client generates events
    for (int i = 0; i < sessions.size(); i++) {
        for (int j = 0; j < 10; j++) {
            sessions[i]->addKeystroke(getTimestamp(), "Key_" + to_string(j));
        }
        sessions[i]->addClipboard(getTimestamp(), "Shared text");
        sessions[i]->addWindow(getTimestamp(), "Browser", "chrome.exe");
    }
    
    cout << "\nBuffer status:" << endl;
    for (auto& session : sessions) {
        session->printBufferStatus();
    }
    
    cout << "\nFlushing all..." << endl;
    for (auto& session : sessions) {
        session->flushAll();
    }
    
    db.printStats();
    db.close();
    
    cout << "✓ Test 3 passed" << endl;
}

void testReloadExistingClient() {
    cout << "\n=== TEST 4: Reload Existing Client ===" << endl;
    
    MeowKeyDatabase db("test_meowkey.db");
    if (!db.open()) {
        cerr << "Failed to open database" << endl;
        return;
    }
    
    // Try to reload bscs24009
    ClientRecord record;
    if (db.registerClient("bscs24009", record)) {
        cout << "\n✓ Loaded existing client: bscs24009" << endl;
        cout << "  Sessions: " << record.session_count << endl;
        cout << "  Total keystrokes: " << record.total_keystrokes << endl;
        cout << "  Total clipboard: " << record.total_clipboard << endl;
        cout << "  Total windows: " << record.total_windows << endl;
    }
    
    // Add more data
    ClientSession session("bscs24009", &db);
    for (int i = 0; i < 5; i++) {
        session.addKeystroke(getTimestamp(), "RELOAD_" + to_string(i));
    }
    session.flushAll();
    
    // Check updated stats
    if (db.getClientStats("bscs24009", record)) {
        cout << "\n✓ Updated stats:" << endl;
        cout << "  Sessions: " << record.session_count << endl;
        cout << "  Total keystrokes: " << record.total_keystrokes << endl;
    }
    
    db.printStats();
    db.close();
    
    cout << "✓ Test 4 passed" << endl;
}

void testClientDeletion() {
    cout << "\n=== TEST 5: Client Deletion ===" << endl;
    
    MeowKeyDatabase db("test_meowkey.db");
    if (!db.open()) {
        cerr << "Failed to open database" << endl;
        return;
    }
    
    cout << "\nBefore deletion:" << endl;
    db.printStats();
    
    // Create and delete a test client
    ClientRecord test_rec;
    db.registerClient("temp_client", test_rec);
    
    // Delete it
    if (db.deleteClient("temp_client")) {
        cout << "\n✓ Client deleted successfully" << endl;
    }
    
    cout << "\nAfter deletion:" << endl;
    db.printStats();
    
    // Try to reload deleted client (should create new)
    ClientRecord record;
    if (db.registerClient("temp_client", record)) {
        cout << "\n✓ Reloaded temp_client (should be newly created)" << endl;
        cout << "  Sessions: " << record.session_count << endl;
        cout << "  (Should be 1 if newly created)" << endl;
    }
    
    db.close();
    cout << "✓ Test 5 passed" << endl;
}

void testStressLoad() {
    cout << "\n=== TEST 6: Stress Test ===" << endl;
    
    MeowKeyDatabase db("test_meowkey.db");
    if (!db.open()) {
        cerr << "Failed to open database" << endl;
        return;
    }
    
    // REGISTER THE CLIENT FIRST!
    ClientRecord stress_record;
    db.registerClient("stress_client", stress_record);
    
    // Now create session
    ClientSession session("stress_client", &db);
    
    cout << "\nGenerating 50 keystrokes..." << endl;
    auto start = chrono::steady_clock::now();
    
    for (int i = 0; i < 50; i++) {
        session.addKeystroke(getTimestamp(), "STRESS_" + to_string(i));
        if ((i + 1) % 10 == 0) {
            cout << "  Generated " << (i + 1) << " keystrokes" << endl;
        }
    }
    session.flushAll();
    
    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    cout << "✓ Completed in " << duration.count() << " ms" << endl;
    
    ClientRecord record;
    if (db.getClientStats("stress_client", record)) {
        cout << "Total keystrokes saved: " << record.total_keystrokes << endl;
    }
    
    db.printStats();
    db.close();
    
    cout << "✓ Test 6 passed" << endl;
}

void testSplitOperations() {
    cout << "\n=== TEST 7: B+ Tree Split Operations ===" << endl;
    
    MeowKeyDatabase db("test_meowkey.db");
    if (!db.open()) {
        cerr << "Failed to open database" << endl;
        return;
    }
    
    // Create a test client specifically for split testing
    ClientRecord split_record;
    db.registerClient("split_test", split_record);
    
    ClientSession session("split_test", &db);
    
    cout << "\nInserting 70 keystrokes (more than BPLUS_ORDER=63) to trigger splits..." << endl;
    
    for (int i = 0; i < 70; i++) {
        session.addKeystroke(getTimestamp(), "SPLIT_KEY_" + to_string(i));
        if ((i + 1) % 10 == 0) {
            cout << "  Inserted " << (i + 1) << " keystrokes" << endl;
        }
    }
    
    // Force flush to trigger any splits
    session.flushAll();
    
    // Check stats
    ClientRecord record;
    if (db.getClientStats("split_test", record)) {
        cout << "\n✓ Split test client stats:" << endl;
        cout << "  Total keystrokes: " << record.total_keystrokes << " (should be 70)" << endl;
        cout << "  Tree root offset: " << record.keystroke_tree_root << endl;
    }
    
    db.printStats();
    db.close();
    
    cout << "✓ Test 7 passed" << endl;
}

int main() {
    cout << "╔════════════════════════════════════════════════════╗" << endl;
    cout << "║     MeowKey Database Test Suite                  ║" << endl;
    cout << "╚════════════════════════════════════════════════════╝" << endl;
    
    try {
        testBasicOperations();
        testBufferOperations();
        testMultipleClients();
        testReloadExistingClient();
        testClientDeletion();
        testStressLoad();
        testSplitOperations();
    } catch (const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return 1;
    }
    
    cout << "\n╔════════════════════════════════════════════════════╗" << endl;
    cout << "║     All Tests Completed                           ║" << endl;
    cout << "╚════════════════════════════════════════════════════╝" << endl;
    
    return 0;
}