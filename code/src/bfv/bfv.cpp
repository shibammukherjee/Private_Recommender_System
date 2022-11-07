#pragma once
#include <algorithm>
#include "seal/seal.h"

#include "../utils/print.cpp"
#include "../utils/config.cpp"

using namespace std;
using namespace seal;

// Client space
Encryptor* encryptor;
Decryptor* decryptor;
// Server space
Evaluator* evaluator;
RelinKeys relinkeys;
// Common
KeyGenerator* keygen_ref;

// Server sends the common information like context, publickey, keygen etc so the client can initialize the BFV
void initializeBFVclient(SEALContext context, PublicKey publickey) {

    // dprint("Client Initializing BFV");
    // dprint("Client generating secret key, encryptor and decryptor");
    SecretKey secret_key = keygen_ref->secret_key();
    encryptor = new Encryptor(context, publickey);
    decryptor = new Decryptor(context, secret_key);
    // dprint("Client initialization done");
}

// Server intializes first the BFV before client
void initializeBFVserver() {

    // dprint("Server Initializing BFV");
    // dprint("Initializing encryption parameters and generating context");
    EncryptionParameters params(scheme_type::bfv);
    params.set_poly_modulus_degree(POLY_MODULUS_DEGREE);
    params.set_coeff_modulus(CoeffModulus::BFVDefault(POLY_MODULUS_DEGREE));
    params.set_plain_modulus(PLAIN_MODULUS);
    SEALContext context(params);
    // dprint("Server generating public and relin key");
    KeyGenerator keygen(context);
    keygen_ref = &keygen;
    PublicKey publickey;
    keygen.create_public_key(publickey);
    keygen.create_relin_keys(relinkeys);
    // dprint("Generating evaluator");
    evaluator = new Evaluator(context);
    // dprint("Server initialization done");

    // Telling client to initalize its BFV
    initializeBFVclient(context, publickey);
}

// Asking server and client to initialize its BFV
void initializeBFV() {
    initializeBFVserver();
}

// Sets an unsigned decimal number(plaintext) to the Plaintext parameter(hex)
Plaintext setPlaintext(uint64_t num) {
    stringstream sstream;
    sstream << hex << num;
    string result = sstream.str();
    Plaintext plaintext(result);
    // dprint("Plaintext generated: " + to_string(num));
    return plaintext;
}

// Sets an unsigned decimal number(plaintext) to the Plaintext parameter(hex)
Plaintext setPlaintext(vector<uint64_t> num) {
    Plaintext plaintext(num);
    // dprint("Plaintext generated: VECTOR");
    return plaintext;
}

// Gets the Ciphertext calculated from the Plaintext
Ciphertext getCiphertext(Plaintext p) {
    Ciphertext c;
    encryptor->encrypt(p, c);
    // dprint("Ciphertext calculated: " + to_string(stol(p.to_string(),nullptr,16)));
    return c;
}

// Gets the Plaintext calculated from the Ciphertext
Plaintext getPlaintext(Ciphertext c) {
    Plaintext p;
    decryptor->decrypt(c, p);
    // dprint("Plaintext calculated");
    return p;
}

// Gets the plaintext as string in decimal representation
string getPlaintextString(Ciphertext c) {
    Plaintext p;
    decryptor->decrypt(c, p);
    // dprint("Plaintext: " + to_string(stol(p.to_string(),nullptr,16)));
    return to_string(stol(p.to_string(), nullptr, 16));
}

// Converts the plaintext coeffs to vector values
vector<uint64_t> getVectorFromPlaintext(Plaintext p) {

    string s = p.to_string();
    uint64_t itemcount = std::count(s.begin(), s.end(), '+') + 1;

    vector<uint64_t> vec;
    for (uint64_t itemindex = 0; itemindex < itemcount; itemindex++) {

        uint64_t pos = s.find("x^");
        string substr = s.substr(0, pos);
        vec.push_back(stol(substr, nullptr, 16));

        uint64_t pluspos = s.find("+");
        s = s.substr(pluspos + 2);

    }

    reverse(vec.begin(), vec.end());
    return vec;
}

// Gets the Ciphertext size
uint64_t getCiphertextSize(Ciphertext c) {
    uint64_t size = c.size();
    // dprint("Size of Ciphertext: " + to_string(size));
    return size;
}

// Gets the available noise budget for the decryptor in bits
int getNoiseBudget(Ciphertext c) {
    int noisebudget = decryptor->invariant_noise_budget(c);
    // dprint("Noise Budget: " + to_string(noisebudget) + " bits");
    return noisebudget;
}

// Adds c1 and c2 to d
Ciphertext bfvAdd(Ciphertext c1, Ciphertext c2) {
    Ciphertext d;
    evaluator->add(c1, c2, d);
    // dprint("Add calculated");
    return d;
}

// Relinearlize Ciphertext
Ciphertext bfvRelinearize(Ciphertext c) {
    Ciphertext d;
    evaluator->relinearize(c, relinkeys, d);
    // dprint("Relinearized");
    return d;
}

// Adds c vector to d
Ciphertext bfvAddMany(vector<Ciphertext> c) {
    Ciphertext d;
    evaluator->add_many(c, d);
    // dprint("Add many calculated");
    return d;
}

// Adds c and p to d
Ciphertext bfvAddPlain(Ciphertext c, Plaintext p) {
    Ciphertext d;
    evaluator->add_plain(c, p, d);
    // dprint("Add plain calculated");
    return d;
}

// Sub c2 from c1 to d
Ciphertext bfvSub(Ciphertext c1, Ciphertext c2) {
    Ciphertext d;
    evaluator->sub(c1, c2, d);
    // dprint("Substract calculated");
    return d;
}

// Sub p from c to d
Ciphertext bfvSubPlain(Ciphertext c, Plaintext p) {
    Ciphertext d;
    evaluator->sub_plain(c, p, d);
    // dprint("Substract plain calculated");
    return d;
}

// Sqaure c to d
Ciphertext bfvSquare(Ciphertext c) {
    Ciphertext d;
    evaluator->square(c, d);
    // dprint("Square calculated");
    return d;
}

// Multiplies c and p to d
Ciphertext bfvMulPlain(Ciphertext c, Plaintext p) {
    Ciphertext d;
    evaluator->multiply_plain(c, p, d);
    // dprint("Product calculated");
    return d;
}

// Multiplies c and p to d
Ciphertext bfvMul(Ciphertext c1, Ciphertext c2) {
    Ciphertext d;
    evaluator->multiply(c1, c2, d);
    // dprint("Product calculated");
    return d;
}
