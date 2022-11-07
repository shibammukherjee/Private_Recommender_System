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

#include "../serverside/server.cpp"
#include "../clientside/client.cpp"
#include "../gc/shareddistancetopk.cpp"
#include "../doram/initdoram.cpp"


using namespace std;

void checkClusterDatapointDistanceShares() {
    iprint("CHECKING COMBINED SHARE OF CLUSTER DATAPOINT DISTANCE");
    for (uint64_t j = 0; j < SHARE_NUM; j++) {
        iprint("  CLIENT SHARE: " + to_string(vectordistance.at(j)) + " SERVER SHARE: " + to_string(randomservershareplusdatapointdimensions.at(j)));
        iprint("  CLUSTER DISTANCE: " + to_string(mathModuloSubtract(vectordistance.at(j), randomservershareplusdatapointdimensions.at(j), PLAIN_MODULUS)));
    }
}

// Top-k retrival of the clusters from the groups
bool processGroup(uint32_t binnumber) {

    pprintlb(" ------ ALL GROUP AHE + GC STARTED -------- ");

    // We will be performing similar to what we do for stash on each group seperately as well go through the for loop, 
    // ... grouplabels contains the labels of the clusters inside the group
    vector<vector<uint64_t>> groupdataset;
    groupdataset.reserve(allgroupclusterdataset.size());
    vector<uint64_t> grouplabels;
    grouplabels.reserve(allgroupclusterdataset.size());

    // ######## SERVER ########
    // ---- Here we are setting the TOP-K cluster paramter (let's say 1) we need when we looking in the group since it can be different from the stash ----
    TOP_K_NUM = 1;
    // ---- Setting the BIN_NUM appropriate to the MIN_GROUP_SIZE, should be always less than eq to the later ----
    BIN_NUM = binnumber;
    if (BIN_NUM > MIN_GROUP_SIZE) {
        eprint("process group bin number assert fails", __FUNCTION__, __LINE__);
        return false;
    }
    // ---- Setting for group process, since the code is almost same except for a small part ----
    IS_GROUP_MODE = true;

    // ######## SERVER ########
    // reserving with appropriate group count for faster push_back
    doramgroupclusterid.reserve(GROUP_NUM);
    gctopkclusteridshare.reserve(GROUP_NUM);

    for (uint64_t groupnumber = 0; groupnumber < GROUP_NUM; groupnumber++) {
        groupdataset = allgroupclusterdataset.at(groupnumber);
        grouplabels = allgroupclusterlabels.at(groupnumber);

        pprint(" --- GROUP " + to_string(groupnumber) + " --- ");

        // ######## SERVER + CLIENT (SEPERATELY) ########
        // ---- Clear the old output results stored in the vectors, since we are reusing the vector variables ----
        clearOldResultVectors();

        // ######## SERVER ########
        // reserving with appropriate group count for faster push_back
        gcoutputidshares.reserve(TOP_K_NUM);
        // Sets the share number which will be used in the GC phase
        SHARE_NUM = groupdataset.at(0).size();

        // ######## SERVER ########
        // ---- Server permutes the dataset along with the corresponding labels ---- 
        permuteDatasetandLabels(groupdataset, grouplabels);
        // Saving the shuffled dataset and labels to the doramdataset and doramlabels, here we just pass the grouplabels because we use the group labels to
        // ... directly access the files and retrive the datapoints inside a cluster
        overwriteDoramWithShuffledData(groupnumber, grouplabels);
        // ---- Write the permuted labels to a file for the correct OT, NOT NEED now because we are not doing any OT ---- 
        //writeCSVStringSingleColoumn("../dataset/groupoutput/permutedgroupslabels/"+to_string(groupnumber)+".csv", grouplabels);
        // ---- Calculates the distance(cipher) with respect to its points (CLUSTER POINT) and the client query point(cipher) ---- 
        serverCalculateDatasetDistance(groupdataset);
        // ---- Server gets the BIN_SIZE based on SHARE_NUM and BIN_NUM -----
        serverSetBinSize();
        // ---- Generates ids for each data point ---- 
        serverGenerateRandomID(groupnumber);

        // ######## CLIENT ######## 
        // ---- Client decrypts the cipher distance and views in plaintext ---- 
        clientDecryptsDistance(false);

        // ######## CLIENT ######## 
        // ---- Client reduces the accuracy of the plaintext distance by BITS_TO_REDUCE
        clientReducesPlainDistanceAccuracy();

        // ######## SERVER ########
        // ---- Server reduces the accuracy by BITS_TO_REDUCE of the random distance that was added to the distance
        serverReducesPlainDistanceAccuracy();

        // ######## SERVER + CLIENT (WORK TOGETHER, BUT IN SECERET) ######## 
        // ---- Perform Garble Circuit ---- 
        gcTopKStart(vectordistance, randomservershareplusdatapointdimensions, groupnumber);

        // ######## DUMMY ########
        // ---- Checking the combined top-k distance share done above ----
        gcTopKCheckingCombinedSharesResultGroup();

        // ######## CLIENT ########
        // ---- Save the group GC results ----
        clientSavesGCGroupSecretShareResult(groupnumber);

    }

    pprintlf(" ------ ALL GROUP AHE + GC DONE ------");

    return true;
}