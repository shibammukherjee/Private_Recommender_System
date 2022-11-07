#pragma once

#include <math.h>
#include <bitset>
#include <string>
#include <vector>

#include "../utils/convert.cpp"
#include "../utils/config.cpp"

using namespace std;

// Blockcipher just for testing a plaintext/ciphertext with specific size
constexpr unsigned Kreyvium_params[6] = { 16, 128, 6, 46, 6, 46 };

constexpr unsigned STATE_SIZE = 288;
constexpr unsigned KREYVIUM_KEY_SIZE_BITS = Kreyvium_params[1];
constexpr unsigned KREYVIUM_IV_SIZE_BITS = Kreyvium_params[1];
constexpr unsigned KREYVIUM_KEY_SIZE_BYTES = Kreyvium_params[0];

typedef std::bitset<STATE_SIZE> stateblock;
typedef std::bitset<KREYVIUM_IV_SIZE_BITS> ivblock;
typedef std::bitset<KREYVIUM_KEY_SIZE_BITS> keyblock;

// Here we initialize the kreyvium
void kreyviumInit(const keyblock& key, const ivblock& iv, stateblock& state,
    ivblock& inner_iv, keyblock& inner_key) {
    for (int i = 0; i < 93; i++) {
        state[i] = key[i];
    }
    for (size_t i = 93; i < 93 + KREYVIUM_IV_SIZE_BITS; i++) {
        state[i] = iv[i - 93];
        inner_iv[i - 93] = iv[KREYVIUM_IV_SIZE_BITS + 92 - i];
    }
    for (size_t i = 93 + KREYVIUM_IV_SIZE_BITS; i < STATE_SIZE - 1; i++) {
        state[i] = 1;
    }
    state[STATE_SIZE - 1] = 0;
    for (size_t i = 0; i < KREYVIUM_KEY_SIZE_BITS; i++) {
        inner_key[i] = key[KREYVIUM_KEY_SIZE_BITS - 1 - i];
    }
}

static keyblock kreyviumConvertKey(const std::vector<uint8_t>& key) {
    keyblock k;
    for (size_t i = 0; i < k.size(); i++) {
        if (i / 8 >= KREYVIUM_ADJUSTED_KEY_SIZE / 8) {
            return k;
        }
        k[i] = (key[i / 8] >> (7 - i % 8)) & 1;
    }
    return k;
}

// 8 -> 00001000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
vector<bool> kreyviumConvertKeyBool(const std::vector<uint8_t>& key) {
    vector<bool> k(KREYVIUM_KEY_SIZE_BITS);
    for (size_t i = 0; i < KREYVIUM_KEY_SIZE_BITS; i++) {
        if (i / 8 >= KREYVIUM_ADJUSTED_KEY_SIZE / 8) {
            return k;
        }
        k[i] = ((key[i / 8] >> (7 - i % 8)) & 1);
    }
    return k;
}

static ivblock kreyviumConvertIV(const std::vector<uint8_t>& iv) {
    ivblock iv_out;
    for (size_t i = 0; i < iv_out.size(); i++) {
        iv_out[i] = (iv[i / 8] >> (7 - i % 8)) & 1;
    }
    return iv_out;
}

vector<bool> kreyviumConvertIVBool(const std::vector<uint8_t>& iv) {
    vector<bool> iv_out(KREYVIUM_IV_SIZE_BITS);
    for (size_t i = 0; i < KREYVIUM_IV_SIZE_BITS; i++) {
        iv_out[i] = ((iv[i / 8] >> (7 - i % 8)) & 1);
    }
    return iv_out;
}

// Initilaized with default 0x11 filled in 16 bytes
ivblock kreyviumSimpleIVInitialization() {
    ivblock iv;
    for (uint64_t i = 3; i < 128; i += 4) {
        iv[i] = (uint8_t)1;
    }
    return iv;
}

// Initilaized with default 0x11 filled in 16 bytes
vector<bool> kreyviumSimpleIVInitializationBool() {
    vector<bool> iv(KREYVIUM_IV_SIZE_BITS);
    for (uint64_t i = 3; i < 128; i += 4) {
        iv[i] = (uint8_t)1;
    }
    return iv;
}

std::vector<uint8_t> keystream(const keyblock& key, const ivblock& iv,
    size_t bits) {
    size_t bytes = ceil((double)bits / 8);
    std::vector<uint8_t> out(bytes, 0);

    stateblock state;
    ivblock inner_iv;
    keyblock inner_key;
    kreyviumInit(key, iv, state, inner_iv, inner_key);

    size_t warmup = 4 * STATE_SIZE;

    for (size_t i = 0; i < warmup + bits; i++) {
        bool t4 = inner_key[KREYVIUM_KEY_SIZE_BITS - 1];
        bool t5 = inner_iv[KREYVIUM_IV_SIZE_BITS - 1];
        bool t1 = state[65] ^ state[92];
        bool t2 = state[161] ^ state[176];
        bool t3 = state[242] ^ state[287] ^ t4;

        if (i >= warmup) {
            size_t ind = i - warmup;
            out[ind / 8] |= ((t1 ^ t2 ^ t3) << (7 - ind % 8));
        }

        t1 = t1 ^ (state[90] & state[91]) ^ state[170] ^ t5;
        t2 = t2 ^ (state[174] & state[175]) ^ state[263];
        t3 = t3 ^ (state[285] & state[286]) ^ state[68];

        state <<= 1;
        inner_key <<= 1;
        inner_iv <<= 1;

        state[0] = t3;
        state[93] = t1;
        state[177] = t2;
        inner_key[0] = t4;
        inner_iv[0] = t5;
    }

    return out;
}

std::vector<uint8_t> kreyviumEncrypt(std::vector<uint8_t> plaintext, std::vector<uint8_t> keyin,
    size_t bits) {
    keyblock key = kreyviumConvertKey(keyin);
    ivblock iv_ = kreyviumSimpleIVInitialization();

    std::vector<uint8_t> ks = keystream(key, iv_, bits);

    for (size_t i = 0; i < ks.size(); i++) ks[i] ^= plaintext[i];
    return ks;
}

std::vector<uint8_t> kreyviumDecrypt(std::vector<uint8_t> ciphertext, std::vector<uint8_t> keyin,
    size_t bits) {
    keyblock key = kreyviumConvertKey(keyin);
    ivblock iv_ = kreyviumSimpleIVInitialization();

    std::vector<uint8_t> ks = keystream(key, iv_, bits);

    for (size_t i = 0; i < ks.size(); i++) ks[i] ^= ciphertext[i];
    return ks;
}


std::vector<uint8_t> getKreyviumKeyStream(std::vector<uint8_t> keyin,
    size_t bits) {

    keyblock key = kreyviumConvertKey(keyin);
    ivblock iv_ = kreyviumSimpleIVInitialization();

    std::vector<uint8_t> ks = keystream(key, iv_, bits);

    return ks;
}


