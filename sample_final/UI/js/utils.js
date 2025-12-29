

// class Utils {
//     constructor() {
//         this.serverUrl = 'http://localhost:3000';
//     }

//     formatTimestamp(timestamp) {
//         if (!timestamp) return 'Just now';
        
//         const date = new Date(timestamp);
//         const now = new Date();
//         const diffMs = now - date;
//         const diffMins = Math.floor(diffMs / 60000);
//         const diffHours = Math.floor(diffMs / 3600000);
//         const diffDays = Math.floor(diffMs / 86400000);

//         if (diffMins < 1) return 'Just now';
//         if (diffMins < 60) return `${diffMins}m ago`;
//         if (diffHours < 24) return `${diffHours}h ago`;
//         if (diffDays < 7) return `${diffDays}d ago`;
        
//         return date.toLocaleDateString() + ' ' + date.toLocaleTimeString();
//     }

//     formatMicroTimestamp(microseconds) {
//         if (!microseconds) return 'Unknown';
//         // Convert microseconds to milliseconds
//         const date = new Date(microseconds / 1000);
//         return this.formatTimestamp(date.getTime());
//     }

//     getClientColor(clientId) {
//         const colors = [
//             '#1a73e8', '#34a853', '#f9ab00', '#e52592',
//             '#d93025', '#9334e6', '#00bcd4', '#ff6d00'
//         ];
        
//         let hash = 0;
//         for (let i = 0; i < clientId.length; i++) {
//             hash = clientId.charCodeAt(i) + ((hash << 5) - hash);
//         }
        
//         return colors[Math.abs(hash) % colors.length];
//     }

//     getClientInitials(clientId) {
//         const parts = clientId.split(/[_-]/);
//         if (parts.length >= 2) {
//             return (parts[0][0] + parts[1][0]).toUpperCase();
//         }
//         return clientId.substring(0, 2).toUpperCase();
//     }

//     updateCurrentTime() {
//         const timeElement = document.getElementById('current-time');
//         if (timeElement) {
//             const now = new Date();
//             timeElement.textContent = now.toLocaleString('en-US', {
//                 weekday: 'long',
//                 year: 'numeric',
//                 month: 'long',
//                 day: 'numeric',
//                 hour: '2-digit',
//                 minute: '2-digit',
//                 second: '2-digit',
//                 hour12: true
//             });
//         }
//     }

//     showNotification(message, type = 'info') {
//         const notification = document.createElement('div');
//         notification.className = `notification notification-${type}`;
//         notification.innerHTML = `
//             <div class="notification-content">
//                 <i class="fas fa-${type === 'success' ? 'check-circle' : type === 'error' ? 'exclamation-circle' : 'info-circle'}"></i>
//                 <span>${message}</span>
//             </div>
//             <button class="notification-close">&times;</button>
//         `;
        
//         document.body.appendChild(notification);
        
//         setTimeout(() => {
//             if (notification.parentNode) {
//                 notification.remove();
//             }
//         }, 5000);
        
//         notification.querySelector('.notification-close').addEventListener('click', () => {
//             notification.remove();
//         });
//     }

//     showModal(modalId) {
//         const modal = document.getElementById(modalId);
//         if (modal) {
//             modal.classList.add('show');
//             document.body.style.overflow = 'hidden';
//         }
//     }

//     hideModal(modalId) {
//         const modal = document.getElementById(modalId);
//         if (modal) {
//             modal.classList.remove('show');
//             document.body.style.overflow = '';
//         }
//     }

//     // In the apiCall method of UI/js/utils.js, update error handling:
// async apiCall(endpoint, method = 'GET', data = null) {
//     try {
//         const options = {
//             method,
//             headers: {
//                 'Content-Type': 'application/json',
//             },
//         };

//         if (data && (method === 'POST' || method === 'PUT' || method === 'DELETE')) {
//             options.body = JSON.stringify(data);
//         }

//         const response = await fetch(`${this.serverUrl}${endpoint}`, options);
        
//         if (!response.ok) {
//             const errorText = await response.text();
//             throw new Error(`API error ${response.status}: ${errorText}`);
//         }

//         return await response.json(); 
//     } catch (error) {
//         console.error('API call failed:', error);
//         // Show more specific error message
//         if (error.message.includes('Failed to fetch')) {
//             this.showNotification('Cannot connect to UI server. Make sure it\'s running on port 3000', 'error');
//         } else if (error.message.includes('Connection error')) {
//             this.showNotification('Cannot connect to C++ server API', 'error');
//         } else {
//             this.showNotification('Connection failed: ' + error.message, 'error');
//         }
//         throw error;
//     }
// }
    
//     classifyKeystroke(key) {
//         if (!key) return 'normal';
//         if (key.length === 1) return 'normal';
//         if (key.includes('Ctrl') || key.includes('Alt') || key.includes('Shift') || key.includes('Cmd')) return 'modifier';
//         if (key.includes('Enter') || key.includes('Tab') || key.includes('Space') || key.includes('Backspace')) return 'special';
//         if (key.includes('Arrow') || key.includes('Page') || key.includes('Home') || key.includes('End')) return 'navigation';
//         if (key.includes('F') && key.length <= 3) return 'function';
//         return 'normal';
//     }
// }

// // Initialize
// document.addEventListener('DOMContentLoaded', () => {
//     window.utils = new Utils();
//     window.utils.updateCurrentTime();
    
//     // Update time every second
//     setInterval(() => {
//         window.utils.updateCurrentTime();
//     }, 1000);
    
//     // Close modal when clicking X
//     document.addEventListener('click', (e) => {
//         if (e.target.classList.contains('close-modal')) {
//             const modal = e.target.closest('.modal');
//             if (modal) {
//                 window.utils.hideModal(modal.id);
//             }
//         }
//     });
    
//     // Close modal when clicking outside
//     document.addEventListener('click', (e) => {
//         if (e.target.classList.contains('modal')) {
//             window.utils.hideModal(e.target.id);
//         }
//     });
    
//     // Escape key closes modals
//     document.addEventListener('keydown', (e) => {
//         if (e.key === 'Escape') {
//             document.querySelectorAll('.modal.show').forEach(modal => {
//                 window.utils.hideModal(modal.id);
//             });
//         }
//     });
// });

// // Notification styles
// const style = document.createElement('style');
// style.textContent = `
//     .notification {
//         position: fixed;
//         top: 20px;
//         right: 20px;
//         background: white;
//         border-radius: 6px;
//         box-shadow: 0 4px 12px rgba(0,0,0,0.15);
//         padding: 1rem;
//         display: flex;
//         align-items: center;
//         justify-content: space-between;
//         min-width: 300px;
//         max-width: 400px;
//         z-index: 1000;
//         animation: slideIn 0.3s ease;
//         border-left: 4px solid #1a73e8;
//     }
    
//     .notification-success { border-left-color: #34a853; }
//     .notification-error { border-left-color: #d93025; }
//     .notification-info { border-left-color: #1a73e8; }
    
//     .notification-content {
//         display: flex;
//         align-items: center;
//         gap: 0.75rem;
//         flex: 1;
//     }
    
//     .notification-content i { 
//         font-size: 1.25rem; 
//         flex-shrink: 0;
//     }
//     .notification-success .notification-content i { color: #34a853; }
//     .notification-error .notification-content i { color: #d93025; }
//     .notification-info .notification-content i { color: #1a73e8; }
    
//     .notification-content span {
//         flex: 1;
//         word-break: break-word;
//     }
    
//     .notification-close {
//         background: none;
//         border: none;
//         font-size: 1.25rem;
//         color: #5f6368;
//         cursor: pointer;
//         padding: 0.25rem;
//         line-height: 1;
//         margin-left: 0.5rem;
//         flex-shrink: 0;
//     }
    
//     .notification-close:hover { color: #d93025; }
    
//     @keyframes slideIn {
//         from {
//             transform: translateX(100%);
//             opacity: 0;
//         }
//         to {
//             transform: translateX(0);
//             opacity: 1;
//         }
//     }
// `;
// document.head.appendChild(style);




// UI/js/utils.js - AWS DEPLOYMENT VERSION
class Utils {
    constructor() {
        // ✅ Connect to LOCAL Node.js proxy (running on your machine)
        this.apiServerHost = 'localhost';  // Local Node.js
        this.apiServerPort = 3000;         // Node.js port
        this.serverUrl = `http://${this.apiServerHost}:${this.apiServerPort}`;
        
        console.log(`📡 Connecting to LOCAL Node.js proxy: ${this.serverUrl}`);
        console.log(`   (which connects to VM at 13.48.25.178:3002)`);
    }

   

    formatTimestamp(timestamp) {
        if (!timestamp) return 'Just now';
        
        const date = new Date(timestamp);
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
    }

    formatMicroTimestamp(microseconds) {
        if (!microseconds) return 'Unknown';
        const date = new Date(microseconds / 1000);
        return this.formatTimestamp(date.getTime());
    }

    getClientColor(clientId) {
        const colors = [
            '#1a73e8', '#34a853', '#f9ab00', '#e52592',
            '#d93025', '#9334e6', '#00bcd4', '#ff6d00'
        ];
        
        let hash = 0;
        for (let i = 0; i < clientId.length; i++) {
            hash = clientId.charCodeAt(i) + ((hash << 5) - hash);
        }
        
        return colors[Math.abs(hash) % colors.length];
    }

    getClientInitials(clientId) {
        const parts = clientId.split(/[_-]/);
        if (parts.length >= 2) {
            return (parts[0][0] + parts[1][0]).toUpperCase();
        }
        return clientId.substring(0, 2).toUpperCase();
    }

    updateCurrentTime() {
        const timeElement = document.getElementById('current-time');
        if (timeElement) {
            const now = new Date();
            timeElement.textContent = now.toLocaleString('en-US', {
                weekday: 'long',
                year: 'numeric',
                month: 'long',
                day: 'numeric',
                hour: '2-digit',
                minute: '2-digit',
                second: '2-digit',
                hour12: true
            });
        }
    }

    showNotification(message, type = 'info') {
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.innerHTML = `
            <div class="notification-content">
                <i class="fas fa-${type === 'success' ? 'check-circle' : type === 'error' ? 'exclamation-circle' : 'info-circle'}"></i>
                <span>${message}</span>
            </div>
            <button class="notification-close">&times;</button>
        `;
        
        document.body.appendChild(notification);
        
        setTimeout(() => {
            if (notification.parentNode) {
                notification.remove();
            }
        }, 5000);
        
        notification.querySelector('.notification-close').addEventListener('click', () => {
            notification.remove();
        });
    }

    showModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) {
            modal.classList.add('show');
            document.body.style.overflow = 'hidden';
        }
    }

    hideModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) {
            modal.classList.remove('show');
            document.body.style.overflow = '';
        }
    }

    async apiCall(endpoint, method = 'GET', data = null) {
        try {
            const options = {
                method,
                headers: {
                    'Content-Type': 'application/json',
                },
            };

            if (data && (method === 'POST' || method === 'PUT' || method === 'DELETE')) {
                options.body = JSON.stringify(data);
            }

            const response = await fetch(`${this.serverUrl}${endpoint}`, options);
            
            if (!response.ok) {
                const errorText = await response.text();
                throw new Error(`API error ${response.status}: ${errorText}`);
            }

            return await response.json(); 
        } catch (error) {
            console.error('API call failed:', error);
            if (error.message.includes('Failed to fetch')) {
                this.showNotification(`Cannot connect to server at ${this.apiServerHost}:${this.apiServerPort}`, 'error');
            } else {
                this.showNotification('Connection failed: ' + error.message, 'error');
            }
            throw error;
        }
    }
    
    classifyKeystroke(key) {
        if (!key) return 'normal';
        if (key.length === 1) return 'normal';
        if (key.includes('Ctrl') || key.includes('Alt') || key.includes('Shift') || key.includes('Cmd')) return 'modifier';
        if (key.includes('Enter') || key.includes('Tab') || key.includes('Space') || key.includes('Backspace')) return 'special';
        if (key.includes('Arrow') || key.includes('Page') || key.includes('Home') || key.includes('End')) return 'navigation';
        if (key.includes('F') && key.length <= 3) return 'function';
        return 'normal';
    }
}

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    window.utils = new Utils();
    window.utils.updateCurrentTime();
    
    setInterval(() => {
        window.utils.updateCurrentTime();
    }, 1000);
    
    document.addEventListener('click', (e) => {
        if (e.target.classList.contains('close-modal')) {
            const modal = e.target.closest('.modal');
            if (modal) {
                window.utils.hideModal(modal.id);
            }
        }
    });
    
    document.addEventListener('click', (e) => {
        if (e.target.classList.contains('modal')) {
            window.utils.hideModal(e.target.id);
        }
    });
    
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            document.querySelectorAll('.modal.show').forEach(modal => {
                window.utils.hideModal(modal.id);
            });
        }
    });
});
