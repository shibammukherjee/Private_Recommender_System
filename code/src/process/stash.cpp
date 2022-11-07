#pragma once
#include <vector>

#include "../bfv/bfv.cpp"

#include "../utils/print.cpp"
#include "../utils/filehandler.cpp"
#include "../utils/pythonfunc.cpp"
#include "../utils/config.cpp"
#include "../utils/globalvarclient.cpp"
#include "../utils/globalvarserver.cpp"
#include "../utils/vectors.cpp"
#include "../utils/math.cpp"

#include "../serverside/server.cpp"
#include "../clientside/client.cpp"

#include "../gc/shareddistancetopk.cpp"

using namespace std;


// Topk retrival of the stash dataset which is basically just normal datapoint dataset
bool processStash(uint32_t binnumber) {

    pprint(" ------ STASH AHE STARTED -------- ");

    // ######## SERVER ########
    // ---- Server permutes the dataset along with the corresponding labels ---- 
    permuteDatasetandLabels(stashdataset, stashlabels);
    // ---- Write the permuted labels to a file for the correct OT, NOT NEED now because we are not doing any OT ---- 
    //writeCSVStringSingleColoumn("../dataset/groupoutput/permutedstashlabels.csv", stashlabels);
    // Sets the share number which will be used in the GC phase
    SHARE_NUM = stashdataset.at(0).size();

    timerStartSub();
    // ---- Calculates the distance(cipher) with respect to its points (STASH POINTS) and the client query point(cipher) ---- 
    serverCalculateDatasetDistance(stashdataset);
    timerEndSub("Stash HE: ");
    // Setting the top k for stash
    TOP_K_NUM = 5;
    // ---- Setting the BIN_NUM appropriate to the stash size, should be always less than eq to the later ----
    BIN_NUM = binnumber;
    if (BIN_NUM > SHARE_NUM) {
        eprint("process stash bin number assert fails", __FUNCTION__, __LINE__);
        return false;
    }
    // ---- Server gets the BIN_SIZE based on SHARE_NUM and BIN_NUM -----
    serverSetBinSize();

    // ######## CLIENT ######## 
    // ---- Client decrypts the cipher distance and views in plaintext ---- 
    clientDecryptsDistance(false);

    // ######## CLIENT ########
    // ---- Client reduces the accuracy of the plaintext distance by BITS_TO_REDUCE
    clientReducesPlainDistanceAccuracy();

    // ######## SERVER ########
    // ---- Server reduces the accuracy by BITS_TO_REDUCE of the random distance that was added to the distance
    serverReducesPlainDistanceAccuracy();

    pprintlb(" ------ STASH AHE DONE -> STASH GC STARTED ------");

    timerStartSub();
    // ######## SERVER + CLIENT (WORK TOGETHER, BUT IN SECERET) ######## 
    // ---- Perform Garble Circuit ---- 
    gcTopKStart(vectordistance, randomservershareplusdatapointdimensions, 0);
    timerEndSub("Stash GC Top-K: ");

    // ######## DUMMY ########
    // ---- Checking the combined top-k distance share done above ----
    gcTopKCheckingCombinedSharesResultStash();

    // ######## SERVER ########
    // ---- Save the random secret share of the server for the id and the distance for the stash process ----
    serverSavesGCStashSecretShareResult();

    // ######## CLIENT ########    
    // ---- Save the stash shared GC results ----
    clientSavesGCStashSecretShareResult();

    pprintlf(" ------ STASH GC DONE ------");

    return true;

}