#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

#include "fcntl.h"
#include "unistd.h"

#include "print.cpp"

using namespace std;

// Should use a safer rand generation, but this is enough for proof of concept !!!
uint64_t getRandom64() {
    return ((uint64_t)random() << 32 | random());
}

uint64_t getRandom32WithModulo(uint64_t modulo) {
    return random() % modulo;
}

uint64_t getRandomBitLengthSize() {
    return ((uint64_t)random() << 32 | random()) % PLAIN_MODULUS;
}

// Generates random string of provided length
string getRandomString(uint64_t len) {
    // dprint("Generating Random String");
    string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    srand((unsigned)time(NULL) * getpid());
    tmp_s.reserve(len);
    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[getRandom32WithModulo(sizeof(alphanum) - 1)];

    return tmp_s;
}