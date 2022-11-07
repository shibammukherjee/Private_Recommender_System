#pragma once

#include "../bfv/bfv.cpp"

#include "../utils/config.cpp"
#include "../utils/security.cpp"
#include "../utils/globalvarserver.cpp"
#include "../utils/globalvarclient.cpp"
#include "../utils/math.cpp"
#include "../utils/convert.cpp"

#include "../kreyvium/kreyviumplain.cpp"

#include "../clientside/client.cpp"
#include "seal/seal.h"

using namespace std;
using namespace seal;

// Calculates the distance of the encrypted user given point with the available server dataset points and Adds the random value to the distance after distance calculation
void serverCalculateDatasetDistance(vector<vector<uint64_t>> dataset) {
    // dprint("Performing bfv dataset distance calculation");

    //dim -> inside one plaintext contains one dimension of all the points
    vector<Plaintext> server_p;
    server_p.reserve(DIMENSION_NUM);
    vector<Ciphertext> server_c;
    server_c.reserve(DIMENSION_NUM);
    Ciphertext c;

    for (uint64_t dimensionindex = 0; dimensionindex < DIMENSION_NUM; dimensionindex++) {

        // Checking the size of the vector because if it is bigger than POLY_MODULUS_DEGREE then we have a problem it converting it
        // ... to Plaintext vector
        if (dataset.at(dimensionindex).size() > POLY_MODULUS_DEGREE) {
            eprint("Number of items is more than the POLY_MODULUS_DEGREE", __FUNCTION__, __LINE__);
            exit(EXIT_FAILURE);
        }

        if (PRINT_NETWORK_USAGE_INFO) {
            iprint("HE info:");
            iprint("Client -> Server Ciphertext Size - " + to_string(user_c.at(dimensionindex).save_size()));
        }

        server_p.push_back(setPlaintext(dataset.at(dimensionindex)));
        // Multiplication as mentioned in the SAANS paper for distance calculation
        // (a - b)^2 = a^2 + b^2 - 2.a.b
        // Here we are doing the a.b = c
        c = bfvMulPlain(user_c.at(dimensionindex), server_p.at(dimensionindex));
        // 2.c
        server_c.push_back(bfvAdd(c, c));

    }
    // followed by addition (2.a.b)
    Ciphertext partialdistance = bfvAddMany(server_c);
    randomservershareplusdatapointdimensions.reserve(SHARE_NUM);
    vector<uint64_t> bsqminusrandom;
    bsqminusrandom.reserve(SHARE_NUM);
    for (uint64_t itemindex = 0; itemindex < SHARE_NUM; itemindex++) {
        // Adding a random value from the distance
        randomservershareplusdatapointdimensions.push_back(0);
        uint64_t datapointdimensionsquared = 0;
        for (uint64_t dimensionindex = 0; dimensionindex < DIMENSION_NUM; dimensionindex++) {
            // Here we are adding the bi^2 part of every dimension for a server item OUTSIDE THE HE WHICH MAKES IT MUCH FASTER
            uint64_t sq = mathSquarePlainModuloOverflowSafe(dataset.at(dimensionindex).at(itemindex), PLAIN_MODULUS);
            datapointdimensionsquared = mathAdditionPlainModuloOverflowSafe(datapointdimensionsquared, sq, PLAIN_MODULUS);
        }

        // This subtracted bi^2 will get added when we subtract the whole server share in the GC share combining
        bsqminusrandom.push_back(mathModuloSubtract(datapointdimensionsquared, randomservershareplusdatapointdimensions.at(itemindex), PLAIN_MODULUS));
    }

    Plaintext randomservershare_p(bsqminusrandom);
    bsqminusrandom.clear();
    partialdistance = bfvSubPlain(partialdistance, randomservershare_p);
    // This is always a vector of size 1, but if we put just ciphertext as the variable it throws error
    cipherdistanceshare.push_back(partialdistance);

    if (PRINT_NETWORK_USAGE_INFO) {
        iprint("HE info:");
        iprint("Server -> Client Ciphertext Size - " + to_string(partialdistance.save_size()));
    }

}

// Takes all the groups with less than minimum clusters num, takes all the clusters in them and puts their datapoints in the stash
vector<vector<uint64_t>> serverGeneratesStash() {
    // dprint("Server generating stash");
    return parseCSVDataset(readTextFile("../dataset/groupoutput/stash.csv", true), DIMENSION_NUM);             // *** (NOT CONSIDERING THE TIMESTAMP RIGHT NOW) ***
}

// Retrive the stash labels for the stash process
vector<uint64_t> serverGetsStashLabels() {
    // dprint("Server getting stash labels");
    return parseCSVLabels(readTextFile("../dataset/groupoutput/stash.csv", true));
}

// Retrieves the the cluster datasets from the group files
vector<vector<vector<uint64_t>>> serverGetsClustersFromGroups() {
    // dprint("Server getting clusters from groups");
    vector<vector<vector<uint64_t>>> data;
    for (uint64_t i = 0; true; i++) {
        string str = readTextFile("../dataset/groupoutput/groups/" + to_string(i) + ".csv", false);
        if (str.compare("error") != 0) {                 // 0 means same
            data.push_back(parseCSVDataset(str, DIMENSION_NUM));
            GROUP_NUM++;                                // Setting the total number of groups in the DORAM DB
        }
        else {
            return data;
        }
    }
}

// Retrieves the the cluster labels from the group files
vector<vector<uint64_t>> serverGetsClustersLabelsFromGroups() {
    // dprint("Server getting clusters labels from groups");
    vector<vector<uint64_t>> data;
    for (uint64_t i = 0; true; i++) {
        string str = readTextFile("../dataset/groupoutput/groups/" + to_string(i) + ".csv", false);
        if (str.compare("error") != 0) {                // 0 means same 
            data.push_back(parseCSVLabels(str));
        }
        else {
            return data;
        }
    }
}

// Setting the bin size for the optimisation part in the GC phase
void serverSetBinSize() {
    // dprint("Server setting the BIN_SIZE as per the SHARE_NUM and BIN_NUM");
    // --- Sets to the floor of the division result (so -1 bins) but the implementation later on take cares of the
    // ... (-1) bin that we have by making the last bin smaller than the other bin sizes ---
    if (BIN_NUM != 0) {
        BIN_SIZE = (SHARE_NUM / BIN_NUM);
    }
}

// Setting continuous ids what will be used to perform the GC with the shared secret of this id
void serverGenerateContiniousID() {
    // dprint("Server generating continious IDs");
    for (uint64_t i = 0; i < SHARE_NUM; i++) {
        id.push_back(i);
    }
}

// Setting random ids what will be used to perform the GC with the shared secret of this id
// Also adding some xtra to pad it to the same size
void serverGenerateRandomID(size_t groupnum) {
    // dprint("Server generating random IDs");
    doramgroupclusterid.push_back({ vector<uint64_t>{} });
    doramgroupclusterid.at(groupnum).reserve(MAXIMUM_DORAM_CLUSTER_NUM);

    for (size_t i = 0; i < SHARE_NUM; i++) {
        doramgroupclusterid.at(groupnum).push_back(getRandomBitLengthSize());
    }
    // Filling dummy till we reach max number
    for (uint64_t idx = SHARE_NUM; idx < MAXIMUM_DORAM_CLUSTER_NUM; idx++) {
        doramgroupclusterid.at(groupnum).push_back(getRandomBitLengthSize());
    }
}

// Optimisation for the GC phase
void serverReducesPlainDistanceAccuracy() {
    // dprint("Server reducing plain distance accuracy");
    if (BITS_TO_REDUCE != 0) {
        for (uint64_t i = 0; i < randomservershareplusdatapointdimensions.size(); i++) {
            randomservershareplusdatapointdimensions[i] = randomservershareplusdatapointdimensions.at(i) >> BITS_TO_REDUCE;
        }
    }
    else {
        // dprint("Aborted, BITS_TO_REDUCE was 0");
    }
}

// Used to set the new bit length in the global bit length variable that we use
void serverReduceBitLengthWithBitsToReduce() {
    BIT_LENGTH = BIT_LENGTH - BITS_TO_REDUCE;
}

// Used to set the modulus according to the new bit length variable
void serverSetsPlainModulusWithBitLength() {
    PLAIN_MODULUS = ((uint64_t)1 << BIT_LENGTH) - (uint64_t)1;
}

// BIT_LENGTH of Kreyvium GC and plain is adjusted to the appropriate key size (64/128)
void serverSetsKreyviumAdjustedBitLength() {
    // ex. BIT_LENGTH = 60 -> key size = 64
    if (BIT_LENGTH <= 64) {
        KREYVIUM_ADJUSTED_KEY_SIZE = 64;
    }
    else if (BIT_LENGTH > 64 && BIT_LENGTH <= 128) {
        KREYVIUM_ADJUSTED_KEY_SIZE = 128;
    }
    else {
        std::cout << "fuct up big kei" << std::endl;
    }
}

// Saves the GC output to seperate vectors dedicated for the stash process, so this can be used later 
void serverSavesGCStashSecretShareResult() {
    randomstashidxor = stashrandomidxor;
    randomstashdistancesubstract = stashrandomdistancesubstract;
}

// This combines the server share vectors we have from stash and topk doram datapoint to one single vector for distance and id(labels)
void serverCombineStashAndDoramTopKDatapointShareResult() {
    for (uint64_t stashindex = 0; stashindex < randomstashidxor.size(); stashindex++) {
        gcstashplusdoramdatapointidxor.push_back(randomstashidxor.at(stashindex));
        gcstashplusdoramdatapointdistancesubstract.push_back(randomstashdistancesubstract.at(stashindex));
    }
    for (uint64_t doramtopkdatapointindex = 0; doramtopkdatapointindex < doramrandomidxor.size(); doramtopkdatapointindex++) {
        gcstashplusdoramdatapointidxor.push_back(doramrandomidxor.at(doramtopkdatapointindex));
        gcstashplusdoramdatapointdistancesubstract.push_back(doramrandomdistancesubstract.at(doramtopkdatapointindex));
    }
}

// Encrypts the doramshufflelabels with kreyvium by converting all labels to a stream, encrypting it, and then again converting
// the stream back to uint64 label format
vector<uint64_t> serverEncryptsDoramShuffleLabels(vector<uint64_t> cluster, uint64_t clusterkey) {

    vector<uint8_t> plainstream = uint64_vec_to_uint8_vec(cluster);
    vector<uint8_t> key = uint64_to_uint8_vec(clusterkey);

    return uint8_vec_to_uint64_vec(kreyviumEncrypt(plainstream, key, 8 * plainstream.size()));

}

// Decrypts the doramshufflelabels with kreyvium by converting all enc labels to a stream, decrypting it, and then again converting
// the stream back to uint64 label format
vector<uint64_t> serverDecryptsDoramShuffleLabels(vector<uint64_t> cluster, uint64_t clusterkey) {

    vector<uint8_t> cipherstream = uint64_vec_to_uint8_vec(cluster);
    vector<uint8_t> key = uint64_to_uint8_vec(clusterkey);

    return uint8_vec_to_uint64_vec(kreyviumDecrypt(cipherstream, key, 8 * cipherstream.size()));

}

// Encrypts the doramshuffledataset with kreyvium by converting all dataitems to a stream, encrypting it, and then again converting
// the stream back to uint64 dataitem format
vector<vector<uint64_t>> serverEncryptsDoramShuffleDataset(vector<vector<uint64_t>> cluster, uint64_t clusterkey) {

    vector<uint8_t> plainstream = uint64_vec_vec_to_uint8_vec(cluster);

    vector<uint8_t> key = uint64_to_uint8_vec(clusterkey);

    return uint8_vec_to_uint64_vec_vec(kreyviumEncrypt(plainstream, key, 8 * plainstream.size()), DIMENSION_NUM, MAXIMUM_DORAM_CLUSTER_SIZE);

}

// Decrypt the doramshuffledataset with kreyvium by converting all enc dataitems to a stream, decrypting it, and then again converting
// the stream back to uint64 dataitem format
vector<vector<uint64_t>> serverDecryptsDoramShuffleDataset(vector<vector<uint64_t>> cluster, uint64_t clusterkey) {

    vector<uint8_t> cipherstream = uint64_vec_vec_to_uint8_vec(cluster);

    vector<uint8_t> key = uint64_to_uint8_vec(clusterkey);

    return uint8_vec_to_uint64_vec_vec(kreyviumDecrypt(cipherstream, key, 8 * cipherstream.size()), DIMENSION_NUM, MAXIMUM_DORAM_CLUSTER_SIZE);

}

// We generate kreyvium key stream with a random seed to generate doram random shares
vector<uint64_t> serverGeneratesDoramClusterRandomSharesFromSeed(uint64_t maxclustersize, uint64_t seed) {

    //                                                                           1 item -> 8 bytes, 1 byte -> 8 bits
    return uint8_vec_to_uint64_vec(getKreyviumKeyStream(uint64_to_uint8_vec(seed), maxclustersize * 64));
}