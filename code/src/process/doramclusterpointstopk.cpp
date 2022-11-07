#pragma once
#include "../bfv/bfv.cpp"

#include "../utils/print.cpp"
#include "../utils/filehandler.cpp"
#include "../utils/pythonfunc.cpp"
#include "../utils/config.cpp"
#include "../utils/globalvarclient.cpp"
#include "../utils/globalvarserver.cpp"
#include "../utils/vectors.cpp"
#include "../utils/math.cpp"
#include "../utils/timer.cpp"

#include "../serverside/server.cpp"
#include "../clientside/client.cpp"

#include "../gc/shareddistancetopk.cpp"
#include "../gc/doramreadclusterstopk.cpp"
#include "../gc/stashplusdoramdatapointtopk.cpp"

#include "../process/stash.cpp"
#include "../process/group.cpp"
#include "../process/doramread.cpp"

#include "../doram/initdoram.cpp"

using namespace std;

// Checking if the server and client share combining works as expected ----
void checkDoramDatapointDistanceShares() {
    iprint("CHECKING COMBINED SHARE OF DORAM DATAPOINT DISTANCE");
    // Converting flat vector to dimensional vector, for easy coding
    // group  cluster  dim   item
    vector<vector<vector<vector<uint64_t>>>> shareddata;
    uint64_t index = 0;
    for (uint64_t groupnum = 0; groupnum < maskedclientdoramdataset.size(); groupnum++) {
        shareddata.push_back({ vector<vector<vector<uint64_t>>>{} });
        for (uint64_t clusternum = 0; clusternum < maskedclientdoramdataset.at(groupnum).size(); clusternum++) {
            shareddata.at(groupnum).push_back({ vector<vector<uint64_t>>{} });
            for (uint64_t dimensionnum = 0; dimensionnum < maskedclientdoramdataset.at(groupnum).at(clusternum).size(); dimensionnum++) {
                shareddata.at(groupnum).at(clusternum).push_back({ vector<uint64_t>{} });
                for (uint64_t itemnum = 0; itemnum < maskedclientdoramdataset.at(groupnum).at(clusternum).at(dimensionnum).size(); itemnum++) {
                    shareddata.at(groupnum).at(clusternum).at(dimensionnum).push_back(vectordistance.at(index));
                    index++;
                }
            }
        }
    }
    // Here we combine the shares and see if things are working 
    uint64_t continious_item_index = 0;
    for (uint64_t groupnum = 0; groupnum < shareddata.size(); groupnum++) {
        for (uint64_t clusternum = 0; clusternum < shareddata.at(groupnum).size(); clusternum++) {
            for (uint64_t itemnum = 0; itemnum < shareddata.at(groupnum).at(clusternum).at(0).size(); itemnum++) {
                uint64_t dist = 0;
                for (uint64_t dimensionnum = 0; dimensionnum < shareddata.at(groupnum).at(clusternum).size(); dimensionnum++) {
                    // Substracting server share
                    uint64_t a_minus_b = mathModuloSubtract(shareddata.at(groupnum).at(clusternum).at(dimensionnum).at(itemnum),
                        doramreadrandomshare.at(groupnum).at(clusternum).at(itemnum), PLAIN_MODULUS);
                    // doing (a-b)^2
                    uint64_t a_minus_b_sq = mathModuloMultiplication(a_minus_b, a_minus_b, PLAIN_MODULUS);
                    // Adding all the (a-b)^2 dimensions
                    dist = mathModuloAdd(dist, a_minus_b_sq, PLAIN_MODULUS);
                }
                iprint("    Distance :" + to_string(dist) + " ID: " + to_string(id.at(continious_item_index) ^ doramreadrandomshareflat.at(continious_item_index)));
                continious_item_index++;
                if (continious_item_index % 5 == 0) {
                    iprint("    .");
                    break;
                }
            }
        }
    }
}

// Partilally unmasks the doramread database using the client share of the kreyvium key in GC
// Then calculates the distance (a'-b) between the user query point and the dataset share retrieved after the DORAMread operation 
// ... based on the kreyvium seed. After which later in gc we do the (a-b)^2
// Implemented in Plain and not HE
void calculatePartialUnmaskedDoramReadDistance() {
    // dprint("Performing bfv dataset distance calculation");

    // reserving with the appropriate size
    kreyviumserverrandomshares.reserve(GROUP_NUM * TOP_K_NUM);

    for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {

        for (uint64_t clusternum = 0; clusternum < TOP_K_NUM; clusternum++) {

            // Getting the kreyvium stream share from the seed shares
            gcKreyviumEncryptStart(uint64_to_uint8_vec(clientseedtopkshares.at(groupnum).at(clusternum)), uint64_to_uint8_vec(seedshareconst));

            vector<uint64_t> outstate = uint8_vec_to_uint64_vec(kreyviumencryptoutstate);

            for (uint64_t itemindex = 0; itemindex < MAXIMUM_DORAM_CLUSTER_SIZE; itemindex++) {

                for (uint64_t dimensionindex = 0; dimensionindex < DIMENSION_NUM; dimensionindex++) {

                    // Adding the shared kreyvium stream generated from the seed share to the point
                    maskedclientdoramdataset.at(groupnum).at(clusternum).at(dimensionindex).at(itemindex) =
                        mathModuloAdd(maskedclientdoramdataset.at(groupnum).at(clusternum).at(dimensionindex).at(itemindex),
                            outstate.at(itemindex), PLAIN_MODULUS);

                    // Doing a' - b, where a' is (a + kreyvium share)
                    maskedclientdoramdataset.at(groupnum).at(clusternum).at(dimensionindex).at(itemindex) =
                        mathModuloSubtract(maskedclientdoramdataset.at(groupnum).at(clusternum).at(dimensionindex).at(itemindex),
                            userpoint.at(dimensionindex), PLAIN_MODULUS);
                }

                // Adding the shared kreyvium stream generated from the seed share to the label
                maskedclientdoramlabels.at(groupnum).at(clusternum).at(itemindex) =
                    mathModuloAdd(maskedclientdoramlabels.at(groupnum).at(clusternum).at(itemindex),
                        outstate.at(itemindex), PLAIN_MODULUS);

            }
        }
    }
    SHARE_NUM = GROUP_NUM * TOP_K_NUM;
}

// ---- First performs the distance calculation of all the datapoints from the top-k clusters we got from the doramread, 
// ... then perform top-k of all the datapoints from all these topk clusters with GC ----
void processDoramClusterPointsTopK() {
    // dprint("Client and Server performing the BFV distance calculation and then using GC to find the top-k points in the top-k clusters");

    // ######## SERVER + CLIENT (SEPERATELY) ########
    // ---- Clear the old output results stored in the vectors, since we are reusing the vector variables ----
    clearOldResultVectors();

    // ######## SERVER + CLIENT (WORK TOGETHER, BUT IN SECRET)
    timerStartSub();
    // ---- The client calculates the a - b, where a is the masked dataitem and b is the client query, later a-b is sqaured in gc ----
    calculatePartialUnmaskedDoramReadDistance();
    timerEndSub("Doram Cluster Points Kreyvium: ");

    // ######## CLIENT ########
    // ---- Client setting the top k datapoints it needs from the doram dataset ----
    TOP_K_NUM = 5;

    // ######## SERVER + CLIENT (WORK TOGETHER, BUT IN SECERET) ######## 
    timerStartSub();
    // ---- Perform Garble Circuit ---- 
    gcDoramReadClusterTopKStart();
    timerEndSub("Doram Cluster Points Top-k GC: ");

    // ######## DUMMY #########
    // ---- Checking the GC share result by combining the shares ;) ---- 
    gcDoramReadClusterTopKCheckingCombineShareResult("DATAPOINT");

    // ######## CLIENT ########
    // ---- Save the doram top-k datapoints client shares we got after GC ----
    clientSavesGCDoramTopKDatapointSecretShareResult();

}