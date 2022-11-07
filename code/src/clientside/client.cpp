#pragma once
#include "../utils/config.cpp"
#include "../utils/security.cpp"
#include "../utils/globalvarserver.cpp"
#include "../utils/globalvarclient.cpp"
#include "../utils/math.cpp"
#include "../gc/kreyviumencrypt.cpp"

#include "../bfv/bfv.cpp"

#include "seal/seal.h"

using namespace std;
using namespace seal;

// Sets the query point, for now we use 3 dimensions
void clientSetNormalizedQueryPoint(uint64_t userid, uint64_t movieid, uint64_t rating) {
    // dprint("Generating client query point");
    userpoint = { userid, movieid, rating };
}

// The query point is encrypted for the BFV distance calculation
void clientEncryptPoints() {
    // dprint("Client encrypting his query points");
    // Clearning the old vector
    user_p.clear();
    user_c.clear();
    for (uint64_t dimensionindex = 0; dimensionindex < userpoint.size(); dimensionindex++) {
        vector<uint64_t> p;
        p.reserve(1);
        p.push_back(userpoint.at(dimensionindex));

        user_p.push_back(setPlaintext(p));
        user_c.push_back(getCiphertext(user_p.at(dimensionindex)));
    }
}

// The encrypted share of the distance after the BFV distance calculation is decrypted here
// Also the a^2 is "added" to the result to make it a^2 + b^2 - 2ab - r
void clientDecryptsDistance(bool isDoram) {
    // dprint("Client decrypts cipher distance and the adds the ");

    uint64_t querypointdimensionsquared = 0;
    if (isDoram == false) {
        for (uint64_t dimensionindex = 0; dimensionindex < DIMENSION_NUM; dimensionindex++) {
            // ai^2 parts which is calculated on ther client side
            querypointdimensionsquared += mathSquarePlainModuloOverflowSafe(userpoint.at(dimensionindex), PLAIN_MODULUS);
        }
    }

    Plaintext plaintext = getPlaintext(cipherdistanceshare.at(0));
    vector<uint64_t> vec = getVectorFromPlaintext(plaintext);
    // Iterates the dataitems
    vectordistance.reserve(vec.size());
    if (isDoram == false) {
        for (uint64_t i = 0; i < vec.size(); i++) {
            vectordistance.push_back(mathModuloSubtract(querypointdimensionsquared, vec.at(i), PLAIN_MODULUS));
        }
    }
    else {
        for (uint64_t i = 0; i < vec.size(); i++) {
            vectordistance.push_back(vec.at(i));
        }
    }

}

// Optimisation for the GC phase where we reduce the disatnce accuracy by dividing the number by a power of 2 and and eventually also having less BIT_LEN for the GC phase
void clientReducesPlainDistanceAccuracy() {
    // dprint("Client reducing plain distance accuracy");
    if (BITS_TO_REDUCE != 0) {
        for (uint64_t i = 0; i < vectordistance.size(); i++) {
            vectordistance[i] = vectordistance.at(i) >> BITS_TO_REDUCE;
        }
    }
    else {
        // dprint("Aborted, BITS_TO_REDUCE was 0");
    }
}

// Saves the GC output to seperate vectors dedicated for the stash process, so this can be used later 
void clientSavesGCStashSecretShareResult() {
    gcstashdistanceshare = gcoutputdistanceshares;
    gcstashidshare = gcoutputidshares;
}

// Saves the GC output to seperate vectors dedicated for the doram top-k datapoint process, so this can be used later 
void clientSavesGCDoramTopKDatapointSecretShareResult() {
    gcdoramtopkdatapointdistanceshare = gcoutputdistanceshares;
    gcdoramtopkdatapointidshare = gcoutputidshares;
}

// This combines the client share vectors we have from stash and topk doram datapoint to one single vector for distance and id(labels)
void clientCombineStashAndDoramTopKDatapointShareResult() {
    for (uint64_t stashindex = 0; stashindex < gcstashdistanceshare.size(); stashindex++) {
        gcstashplusdoramdatapointdistance.push_back(gcstashdistanceshare.at(stashindex));
        gcstashplusdoramdatapointid.push_back(gcstashidshare.at(stashindex));
    }
    for (uint64_t doramtopkdatapointindex = 0; doramtopkdatapointindex < gcdoramtopkdatapointdistanceshare.size(); doramtopkdatapointindex++) {
        gcstashplusdoramdatapointdistance.push_back(gcdoramtopkdatapointdistanceshare.at(doramtopkdatapointindex));
        gcstashplusdoramdatapointid.push_back(gcdoramtopkdatapointidshare.at(doramtopkdatapointindex));
    }
}

// Saves the GC output to seperate vectors dedicated for the group process, so this can be used later 
void clientSavesGCGroupSecretShareResult(uint64_t groupnum) {
    uint64_t outputindex = 0;

    gctopkclusteridshare.push_back({ vector<uint64_t>{} });
    gctopkclusteridshare.at(groupnum).reserve(TOP_K_NUM);

    for (uint64_t clusterno = 0; clusterno < TOP_K_NUM; clusterno++) {
        gctopkclusteridshare.at(groupnum).push_back(gcoutputidshares.at(outputindex));
        outputindex++;
    }
}

