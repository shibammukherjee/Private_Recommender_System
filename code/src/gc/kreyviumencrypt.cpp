#pragma once

#include "abycore/aby/abyparty.h"
#include "abycore/circuit/circuit.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"

#include "../utils/globalvarclient.cpp"
#include "../utils/math.cpp"
#include "../utils/convert.cpp"
#include "../utils/config.cpp"

#include "../kreyvium/kreyviumplain.cpp"

#include "../algorithms/uniformrandshuffle.cpp"

#include <ENCRYPTO_utils/crypto/crypto.h>

using namespace std;


class KreyviumEncryptGC {
protected:
private:
public:
    // The actual GC Top-K circuit making and calculation work happens here ;)
    void buildSharedKreyviumEncryption(share* s_clientkey[], share* s_serverkey[], share* s_serverrandom[], share* s_inner_iv[], share* s_state[], share* s_out[], size_t output_bitlen, e_role role, BooleanCircuit* bc) {

        // dprint("Building kreyvium encryption GC circuit (server/client)");

        share* inner_key[KREYVIUM_KEY_SIZE_BITS];
        share* s_key[KREYVIUM_ADJUSTED_KEY_SIZE];

        // This is the length of the zero string in bits that is encrypted
        size_t out_bit_size = MAXIMUM_DORAM_CLUSTER_SIZE * 64;

        // --- Combining the kreyvium keys ---

        share* zero;
        if (role == SERVER) {
            zero = bc->PutINGate((unsigned)0, output_bitlen, SERVER);
        }
        else {
            zero = bc->PutDummyINGate(output_bitlen);
        }

        // We are using ADJUSTED_KEY_SIZE bits random keys for the kreyvium but this can be easily switched to 128 bits by
        // generating 128 bit random key in the doramdatabaseread.cpp
        // Combing the first BIT_LENGTH key information
        for (uint64_t i = 0; i < KREYVIUM_ADJUSTED_KEY_SIZE; i++) {
            s_key[i] = bc->PutXORGate(s_clientkey[i], s_serverkey[i]);
        }

        // --- Initializing Kreyvium ---

        // First part - Initilaized according to the key bit length
        // We check the condition for appropriate initializing state with key
        if (KREYVIUM_ADJUSTED_KEY_SIZE == 64) {
            for (int i = 0; i < KREYVIUM_ADJUSTED_KEY_SIZE; i++) {
                s_state[i] = s_key[i];
            }
            for (int i = KREYVIUM_ADJUSTED_KEY_SIZE; i < 93; i++) {
                s_state[i] = zero;
            }
        }
        else {
            for (int i = 0; i < 93; i++) {
                s_state[i] = s_key[i];
            }
        }
        // --- The middle second and the third part is missing because it has been implemented in gcKreyviumEncrypt()
        // Fourth part - Initilaized according to the key bit length
        for (size_t i = 0; i < KREYVIUM_KEY_SIZE_BITS - KREYVIUM_ADJUSTED_KEY_SIZE; i++) {
            inner_key[i] = zero;
        }
        uint64_t idx = 0;
        for (size_t i = KREYVIUM_KEY_SIZE_BITS - KREYVIUM_ADJUSTED_KEY_SIZE; i < KREYVIUM_KEY_SIZE_BITS; i++) {
            inner_key[i] = s_key[KREYVIUM_ADJUSTED_KEY_SIZE - 1 - idx];
            idx++;
        }

        idx = 0;
        for (size_t i = 0; i < 64; i++) {
            s_out[idx] = s_key[i];
            idx++;
        }

        // --- Kreyvium Keystream ---
        size_t warmup = 4 * STATE_SIZE;

        for (size_t i = 0; i < warmup + out_bit_size; i++) {

            share* t4 = inner_key[KREYVIUM_KEY_SIZE_BITS - 1];
            share* t5 = s_inner_iv[KREYVIUM_IV_SIZE_BITS - 1];
            // //share* t1 = state[65] ^ state[92];
            share* t1 = bc->PutXORGate(s_state[65], s_state[92]);
            // //share* t2 = state[161] ^ state[176];
            share* t2 = bc->PutXORGate(s_state[161], s_state[176]);
            // // share* t3 = state[242] ^ state[287] ^ t4;
            share* t3 = bc->PutXORGate(bc->PutXORGate(s_state[242], s_state[287]), t4);

            if (i >= warmup) {
                size_t ind = i - warmup;
                // out[ind / 8] |= ((t1 ^ t2 ^ t3) << (7 - ind % 8));
                s_out[ind] =
                    bc->PutXORGate(bc->PutXORGate(t1, t2), t3);
            }

            //t1 = t1 ^ (state[90] & state[91]) ^ state[170] ^ t5;
            t1 = bc->PutXORGate(t1,
                bc->PutXORGate(
                    bc->PutXORGate(
                        bc->PutANDGate(s_state[90], s_state[91]), s_state[170]), t5));

            //t2 = t2 ^ (state[174] & state[175]) ^ state[263];
            t2 = bc->PutXORGate(t2,
                bc->PutXORGate(
                    bc->PutANDGate(s_state[174], s_state[175]), s_state[263]));

            //t3 = t3 ^ (state[285] & state[286]) ^ state[68];
            t3 = bc->PutXORGate(t3,
                bc->PutXORGate(
                    bc->PutANDGate(s_state[285], s_state[286]), s_state[68]));

            //state <<= 1;
            for (uint64_t i = STATE_SIZE - 1; i > 0; i--) {
                s_state[i] = s_state[i - 1];
            }
            s_state[0] = zero;

            //inner_key <<= 1;
            for (uint64_t i = KREYVIUM_KEY_SIZE_BITS - 1; i > 0; i--) {
                inner_key[i] = inner_key[i - 1];
            }
            inner_key[0] = zero;

            //inner_iv <<= 1;
            for (uint64_t i = KREYVIUM_KEY_SIZE_BITS - 1; i > 0; i--) {
                s_inner_iv[i] = s_inner_iv[i - 1];
            }
            s_inner_iv[0] = zero;

            s_state[0] = t3;
            s_state[93] = t1;
            s_state[177] = t2;
            inner_key[0] = t4;
            s_inner_iv[0] = t5;
        }

        // Masking the s_out with server random by implementing a FULL ADDER
        share* sum[MAXIMUM_DORAM_CLUSTER_SIZE * KREYVIUM_ADJUSTED_KEY_SIZE];
        for (uint64_t itemidx = 0; itemidx < MAXIMUM_DORAM_CLUSTER_SIZE; itemidx++) {

            share* c;

            //sum[i] = ((a[i] ^ b[i]) ^ c);
            //c = ((a[i] & b[i]) | (a[i] & c)) | (b[i] & c);
            sum[itemidx * 64] = bc->PutXORGate(s_out[itemidx * 64], s_serverrandom[itemidx * 64]);
            c = bc->PutANDGate(s_out[itemidx * 64], s_serverrandom[itemidx * 64]);

            for (uint64_t i = 1; i < BIT_LENGTH; i++) {

                sum[itemidx * 64 + i] = bc->PutXORGate(bc->PutXORGate(s_out[itemidx * 64 + i], s_serverrandom[itemidx * 64 + i]), c);
                c = bc->PutORGate(
                    bc->PutORGate(
                        bc->PutANDGate(s_out[itemidx * 64 + i], s_serverrandom[itemidx * 64 + i]),
                        bc->PutANDGate(s_out[itemidx * 64 + i], c)),
                    bc->PutANDGate(s_serverrandom[itemidx * 64 + i], c)
                );
            }
        }

        for (uint64_t i = 0; i < MAXIMUM_DORAM_CLUSTER_SIZE; i++) {
            for (uint64_t j = 0; j < BIT_LENGTH; j++) {
                s_out[(i * 64) + j] = sum[(i * 64) + j];
            }
        }
    }

    // Mostly initializing the normal parameters and the gc share and output paramters
    void gcKreyviumEncrypt(vector<uint8_t> secretshare, e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen,
        uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {

        // dprint("Initializing GC Kreyvium Encrypt");

        // Describing the GC
        ABYParty* party = new ABYParty(role, address, port, seclv, output_bitlen, nthreads, mt_alg);
        std::vector<Sharing*>& sharings = party->GetSharings();
        Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

        // key shares
        share* s_clientkey[KREYVIUM_ADJUSTED_KEY_SIZE], * s_serverkey[KREYVIUM_ADJUSTED_KEY_SIZE];
        // size of the krevium out state that is xored(no.items * 8(bytes) * 8(bits))
        share* s_out[MAXIMUM_DORAM_CLUSTER_SIZE * 64];
        // iv and inner iv
        share* s_inner_iv[KREYVIUM_IV_SIZE_BITS];
        // state
        share* s_state[STATE_SIZE];
        // server random used to mask the s_out
        share* s_serverrandom[MAXIMUM_DORAM_CLUSTER_SIZE * 64];

        if (role == SERVER) {

            // Since it is server role, we assign the secret share to the server share 
            vector<bool> serverkeyshare_bits = kreyviumConvertKeyBool(secretshare);

            for (uint64_t i = 0; i < KREYVIUM_ADJUSTED_KEY_SIZE; i++) {
                // Filling the client share with the dummy values since it is server role
                s_clientkey[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the actual server shares
                s_serverkey[i] = circ->PutINGate((unsigned)serverkeyshare_bits[i], output_bitlen, SERVER);
            }

            // Initializing the kreyvium server random share
            kreyviumserverrandomshares.push_back({ vector<uint64_t>{} });
            uint64_t clusteridx = kreyviumserverrandomshares.size() - 1;
            kreyviumserverrandomshares.at(clusteridx).reserve(MAXIMUM_DORAM_CLUSTER_SIZE);

            for (uint64_t itemidx = 0; itemidx < MAXIMUM_DORAM_CLUSTER_SIZE; itemidx++) {

                kreyviumserverrandomshares.at(clusteridx).push_back(getRandomBitLengthSize());
                // TODO - Something is wrong with the GC full adder, fails to add for number with mutiple binary 1s
                // Fixing it with a fake "random" number, too sleepy to fix for now....
                //vector<bool> serverrandombits = kreyviumConvertKeyBool(uint64_to_uint8_vec(1));
                vector<bool> serverrandombits = kreyviumConvertKeyBool(uint64_to_uint8_vec(kreyviumserverrandomshares.at(clusteridx).at(itemidx)));

                for (uint64_t bitidx = 0; bitidx < 64; bitidx++) {
                    s_serverrandom[itemidx * 64 + bitidx] = circ->PutINGate((unsigned)serverrandombits[bitidx], output_bitlen, SERVER);
                }

            }

            // Compute kreyvium IV
            vector<bool> serverIVshare_bits = kreyviumSimpleIVInitializationBool();

            // Initilializing the second part of the kreyvium
            for (size_t i = 93; i < 93 + KREYVIUM_IV_SIZE_BITS; i++) {

                if (((i - 93) % 4) != 3) {
                    s_state[i] = circ->PutINGate((unsigned)0, output_bitlen, SERVER);
                }
                else {
                    s_state[i] = circ->PutINGate((unsigned)1, output_bitlen, SERVER);

                }

                if ((KREYVIUM_IV_SIZE_BITS + 92 - i) % 4 != 3) {
                    s_inner_iv[i - 93] = circ->PutINGate((unsigned)0, output_bitlen, SERVER);
                }
                else {
                    s_inner_iv[i - 93] = circ->PutINGate((unsigned)1, output_bitlen, SERVER);
                }
            }
            // Initilializing the third part of the kreyvium
            for (size_t i = 93 + KREYVIUM_IV_SIZE_BITS; i < STATE_SIZE - 1; i++) {
                s_state[i] = circ->PutINGate((unsigned)1, output_bitlen, SERVER);
            }
            s_state[STATE_SIZE - 1] = circ->PutINGate((unsigned)0, output_bitlen, SERVER);

        }
        else if (role == CLIENT) {

            // Since it is client role, we assign the secret share to the client share 
            vector<bool> clientkeyshare_bits = kreyviumConvertKeyBool(secretshare);

            for (uint64_t i = 0; i < KREYVIUM_ADJUSTED_KEY_SIZE; i++) {
                // Filling with the actual cleint shares
                s_clientkey[i] = circ->PutINGate((unsigned)clientkeyshare_bits[i], output_bitlen, CLIENT);
                // Filling the server share with the dummy values since it is client role
                s_serverkey[i] = circ->PutDummyINGate(output_bitlen);
            }

            // Initializing out state and dummy server radnom
            for (uint64_t i = 0; i < MAXIMUM_DORAM_CLUSTER_SIZE * 64; i++) {
                s_serverrandom[i] = circ->PutDummyINGate(output_bitlen);
            }

            // Initilializing the second part of the kreyvium
            for (size_t i = 93; i < 93 + KREYVIUM_IV_SIZE_BITS; i++) {
                s_state[i] = circ->PutDummyINGate(output_bitlen);
                s_inner_iv[i - 93] = circ->PutDummyINGate(output_bitlen);
            }
            // Initilializing the third part of the kreyvium
            for (size_t i = 93 + KREYVIUM_IV_SIZE_BITS; i < STATE_SIZE - 1; i++) {
                s_state[i] = circ->PutDummyINGate(output_bitlen);
            }
            s_state[STATE_SIZE - 1] = circ->PutDummyINGate(output_bitlen);

        }

        // This actually performs the GC calculations with all the input shares and the output shares ;)
        buildSharedKreyviumEncryption(s_clientkey, s_serverkey, s_serverrandom, s_inner_iv, s_state, s_out, output_bitlen, role, (BooleanCircuit*)circ);

        for (uint64_t i = 0; i < MAXIMUM_DORAM_CLUSTER_SIZE * 64; i++) {
            s_out[i] = circ->PutOUTGate(s_out[i], CLIENT);
        }

        // Executing the circuit
        // dprint("Executing GC Kreyvium Encrypt circuit (server/client)");
        party->ExecCircuit();

        if (role == CLIENT) {

            kreyviumencryptoutstate.clear();
            kreyviumencryptoutstate.assign(MAXIMUM_DORAM_CLUSTER_SIZE * 8, 0);
            uint64_t outstate_bits_index = 0;
            uint64_t outstate_index = 0;

            for (uint64_t item = 0; item < MAXIMUM_DORAM_CLUSTER_SIZE; item++) {
                //uint8_t tmp = s_out[item]->get_clear_value<uint8_t>();
                for (uint64_t itembytes = 0; itembytes < 8; itembytes++) {
                    for (uint64_t itembits = 0; itembits < 8; itembits++) {
                        uint8_t tmp = s_out[outstate_bits_index]->get_clear_value<uint8_t>();
                        kreyviumencryptoutstate[outstate_index] |= (tmp << (7 - itembits));
                        outstate_bits_index++;
                    }
                    outstate_index++;
                }
            }
        }

    }

    // Just a threading way of calling the main function
    std::thread threadGCKreyviumEncrypt(vector<uint8_t> secretshare, e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
        return std::thread([=] { gcKreyviumEncrypt(secretshare, role, address, port, seclv, output_bitlen, nthreads, mt_alg, sharing); });
    }
};

// Initializes the GC as will be done at the server and client side
void gcKreyviumEncryptStart(vector<uint8_t> clientkeyshare, vector<uint8_t> serverkeyshare) {

    // dprint("Kreyvium Decrypt calculation with GC started");
    e_mt_gen_alg mt_alg = MT_OT;
    seclvl seclv = get_sec_lvl(128);
    KreyviumEncryptGC* gcserver = new KreyviumEncryptGC();
    KreyviumEncryptGC* gcclient = new KreyviumEncryptGC();
    // Ideally this should be done on the seperate systems (client and server)
    // Client does this on its side
    thread threadclient = gcclient->threadGCKreyviumEncrypt(clientkeyshare, CLIENT, "127.0.0.1", 8080, seclv, 1, GC_THREADS, mt_alg, S_BOOL);
    // Server does this on its side
    thread threadserver = gcserver->threadGCKreyviumEncrypt(serverkeyshare, SERVER, "127.0.0.1", 8080, seclv, 1, GC_THREADS, mt_alg, S_BOOL);
    threadclient.join();
    threadserver.join();

}
