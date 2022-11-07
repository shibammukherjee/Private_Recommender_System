#pragma once
#include <vector>

#include "abycore/aby/abyparty.h"
#include "abycore/circuit/circuit.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"

#include "../utils/config.cpp"
#include "../utils/security.cpp"
#include "../utils/globalvarserver.cpp"

using namespace std;

// Generates the random sequence that is needed to perform a uniform random shuffle mostly used in the GC shuffling
void generateRandomShuffleSequence() {
    // dprint("Server generating random shuffle sequence");
    uniformrandomshuffleindexlist.clear();
    for (uint64_t i = 0; i < SHARE_NUM - 1; i++) {
        uint64_t j;
        while (true) {
            j = getRandom32WithModulo(SHARE_NUM);
            bool exists = false;
            // when the vector already has 1 or more elements then we check if the new random number alredy exists in the vector
            for (uint64_t n = 0; n < uniformrandomshuffleindexlist.size(); n++) {
                if (j == uniformrandomshuffleindexlist.at(n)) {
                    exists = true;
                    break;
                }
            }
            // if the vector is of size 0 or the random number diesn't exist, we initialize or push the new rand value
            if (!exists) {
                uniformrandomshuffleindexlist.push_back(j);
                break;
            }
        }
    }
}

// Modern version of the Fisher–Yates shuffle algorithm, GC version
void shuffleGCShares(share* distance[], share* id[]) {
    // dprint("Server performing uniform random shuffles");
    for (uint64_t i = 0; i < SHARE_NUM - 1; i++) {

        share* temp_distance = distance[i];
        share* temp_id = id[i];

        distance[i] = distance[uniformrandomshuffleindexlist.at(i)];
        id[i] = id[uniformrandomshuffleindexlist.at(i)];

        distance[uniformrandomshuffleindexlist.at(i)] = temp_distance;
        id[uniformrandomshuffleindexlist.at(i)] = temp_id;
    }
}