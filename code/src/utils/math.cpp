#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "security.cpp"

#include "../utils/globalvarserver.cpp"

using namespace std;

// Performing modulo substraction
uint64_t mathModuloSubtract(uint64_t a, uint64_t b, uint64_t modulo) {
    // dprint("Performing modulo substraction");
    if (a >= b) {
        return (a - b);
    }
    else {
        return (modulo - b + a);
    }
}

// Performing modulo addition
uint64_t mathModuloAdd(uint64_t a, uint64_t b, uint64_t modulo) {
    // dprint("Performing modulo addition");
    if (b == 0) {
        return a;
    }
    else {
        return mathModuloSubtract(a, modulo - b, modulo);
    }
}

// This checks if it does an over and then performs the calculation, the above does the calculation anyway
uint64_t mathAdditionPlainModuloOverflowSafe(uint64_t a, uint64_t b, uint64_t modulo) {
    // dprint("Performing check for uint64 safe addition");
    if (mathModuloAdd(a, b, modulo) < a | mathModuloAdd(a, b, modulo) < b) {
        pprint(a); pprint(b); pprint(modulo);
        wprint("ADDING WILL LEAD TO UINT64 OVERFLOW");
        return 0;
    }
    else {
        return a + b;
    }
}

// This checks if it does an over and then performs the calculation, the above does the calculation anyway
uint64_t mathSquarePlainModuloOverflowSafe(uint64_t number, uint64_t modulo) {
    // dprint("Performing check for uint64 safe square");
    // +1 because, whe we say modulo 32 then it has value from 0..31, whereas PLAIN_MODULUS value is actually (2^BIT_LENGTH)-1, thus we need to add +1
    modulo = modulo + 1;
    if ((number * number) % modulo < number) {
        wprint("SQUARING WILL LEAD TO UINT64 OVERFLOW");
        return 0;
    }
    else {
        return number * number;
    }
}

// This checks if multiplying by 2 wull overflow a uint64
void mathMultiplyByTwoOverflowUint64Check(uint64_t number) {
    // dprint("Performing check for uint64 safe multiplication by 2");
    if (number * 2 < number) {
        wprint("MULTIPLY BY 2 WILL LEAD TO UINT64 OVERFLOW");
    }
}

// Modulo squaring
uint64_t mathModuloSquare(uint64_t a, uint64_t modulo) {
    // dprint("Performing modulo multiplication");

    mathMultiplyByTwoOverflowUint64Check(a);

    uint64_t b = a;

    uint64_t res = 0; // Initialize result
    a = a % modulo;
    while (b > 0)
    {
        // If b is odd, add 'a' to result
        if (b % 2 == 1)
            res = (res + a) % modulo;

        // Multiply 'a' with 2
        a = (a * 2) % modulo;

        // Divide b by 2
        b /= 2;
    }

    // Return result
    return res % modulo;

}

// Modulo Multiplication
uint64_t mathModuloMultiplication(uint64_t a, uint64_t b, uint64_t modulo) {
    // dprint("Performing modulo multiplication");

    mathMultiplyByTwoOverflowUint64Check(a);

    uint64_t res = 0; // Initialize result
    a = a % modulo;
    while (b > 0)
    {
        // If b is odd, add 'a' to result
        if (b % 2 == 1)
            res = (res + a) % modulo;

        // Multiply 'a' with 2
        a = (a * 2) % modulo;

        // Divide b by 2
        b /= 2;
    }

    // Return result
    return res % modulo;

}


