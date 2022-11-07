#pragma once
#include "../utils/config.cpp"
#include "../utils/security.cpp"
#include "../bfv/bfv.cpp"
#include "../utils/globalvarserver.cpp"
#include "../utils/globalvarclient.cpp"
#include "seal/seal.h"

#include "../utils/pythonfunc.cpp"
#include "../utils/config.cpp"
#include "../utils/filehandler.cpp"

using namespace std;

// In the init Doram we must first fill with dummy UINT32_MAX, then we must overwrite with the actual shuffled data and then we must fill the rest of the dummy UINT32_MAX 
// ... in the cluster with the first N real values because..
// IMPORTANT: For the dummy points we use the same real points because technically it will be shared with a server random to the client, and also its distance will be 
// shared with the server random to the client and in the GC topk it doesn't replace if it's equal, it replaces only if it is less, so repetative points won't be a problem 
// in the GC top-k

void initDoram() {
    // dprint("Server performing DORAM initialization");

    // dprint("Counting the DORAM_CLUSTER_NUM and the MAXIMUM_DORAM_CLUSTER_SIZE");
    // Get the cluster labels in each group from allgrouplabels and then iterate over each cluster file to check the maximum number of items a cluster includes, 
    // ... set this number as the maximum (also minimum) cluster size for the doram
    for (uint64_t groupno = 0; groupno < allgroupclusterlabels.size(); groupno++) {
        if (allgroupclusterlabels.at(groupno).size() > MAXIMUM_DORAM_CLUSTER_NUM) {
            MAXIMUM_DORAM_CLUSTER_NUM = allgroupclusterlabels.at(groupno).size();
        }
        for (uint64_t clusterno = 0; clusterno < allgroupclusterlabels.at(groupno).size(); clusterno++) {
            uint64_t clustersize = parseCSVLabels(readTextFile("../dataset/clusteroutput/clusters/" + to_string(allgroupclusterlabels.at(groupno).at(clusterno)) + ".csv", true)).size();
            if (clustersize > MAXIMUM_DORAM_CLUSTER_SIZE) {
                MAXIMUM_DORAM_CLUSTER_SIZE = clustersize;
            }
            DORAM_CLUSTER_NUM++;                                    // Setting the total number of clusters in the DORAM DB
        }
    }

    // dprint("Filling with dummy UINT32_MAX values");
    // Fill the doram dataset vector with dummy UINT32_MAX values which will be replaced later **READ ABOVE**
    // Iterating over the groups
    doramshuffledataset.reserve(GROUP_NUM);
    for (uint64_t groupno = 0; groupno < allgroupclusterdataset.size(); groupno++) {
        doramshuffledataset.push_back({ vector<vector<vector<uint64_t>>>{} });

        doramshuffledataset.at(groupno).reserve(MAXIMUM_DORAM_CLUSTER_NUM);
        // Iterating over the clusters via labels (why labels, see standards or the variable declaration)
        for (uint64_t clusterno = 0; clusterno < allgroupclusterlabels.at(groupno).size(); clusterno++) {
            doramshuffledataset.at(groupno).push_back({ vector<vector<uint64_t>>{} });

            doramshuffledataset.at(groupno).at(clusterno).reserve(DIMENSION_NUM);
            // Iterating over the dimensions of the datapoints
            for (uint64_t dimension = 0; dimension < DIMENSION_NUM; dimension++) {
                doramshuffledataset.at(groupno).at(clusterno).push_back({ vector<uint64_t>{} });

                doramshuffledataset.at(groupno).at(clusterno).at(dimension).reserve(MAXIMUM_DORAM_CLUSTER_SIZE);
            }
        }
    }

    // Fill the doram labels vector with dummy UINT32_MAX values which will be replaces later **READ ABOVE**
    // Iterating over the groups
    for (uint64_t groupno = 0; groupno < allgroupclusterlabels.size(); groupno++) {
        doramshufflelabels.push_back({ vector<vector<uint64_t>>{} });

        doramshufflelabels.at(groupno).reserve(allgroupclusterlabels.at(groupno).size());
        // Iterating over the clusters 
        for (uint64_t clusterno = 0; clusterno < allgroupclusterlabels.at(groupno).size(); clusterno++) {
            doramshufflelabels.at(groupno).push_back({ vector<uint64_t>{} });

            doramshufflelabels.at(groupno).at(clusterno).reserve(MAXIMUM_DORAM_CLUSTER_SIZE);
        }
    }

}

// **READ ABOVE**
void overwriteDoramWithShuffledData(uint64_t groupno, vector<uint64_t> shuffledclusterlabels) {
    // dprint("Overwriting the dummy UINT32_MAX values in the doramdataset with the real shuffled values");

    // Overwrite the doram dataset vector containing the dummy data with the actual data from the cluster files **READ ABOVE**
    // Iterating over the clusters via labels (why labels, see standards or the variable declaration)
    for (uint64_t clusterno = 0; clusterno < allgroupclusterlabels.at(groupno).size(); clusterno++) {
        // Contains all the points dataset inside a cluster
        vector<vector<uint64_t>> tmpclusterdata = parseCSVDataset(readTextFile("../dataset/clusteroutput/clusters/" + to_string(shuffledclusterlabels.at(clusterno)) + ".csv", true), DIMENSION_NUM);
        // Now we are looping though the groupclusterdataset to look for the dimension loop and then item loop
        for (uint64_t dimensionno = 0; dimensionno < tmpclusterdata.size(); dimensionno++) {
            // Iterating over the datapoints (x1, x2, ...) and not (x1, y1, z1)
            uint64_t startindex = 0;
            for (uint64_t itemno = 0; itemno < MAXIMUM_DORAM_CLUSTER_SIZE; itemno++) {
                if (startindex == tmpclusterdata.at(dimensionno).size()) {
                    startindex = 0;
                }
                // Filling all the dummy datapoints with repeting real points
                doramshuffledataset.at(groupno).at(clusterno).at(dimensionno).push_back(tmpclusterdata.at(dimensionno).at(startindex));
                startindex++;
            }
        }
    }
    // Padding with fake clusters
    uint64_t idx = 0;
    for (uint64_t clusterno = allgroupclusterlabels.at(groupno).size(); clusterno < MAXIMUM_DORAM_CLUSTER_NUM; clusterno++) {
        if (idx == allgroupclusterlabels.at(groupno).size()) {
            idx = 0;
        }
        doramshuffledataset.at(groupno).push_back(doramshuffledataset.at(groupno).at(idx));
        idx++;
    }

    // Overwrite the doram labels vector containing the dummy data with the actual data from the cluster files
    // Iterating over the clusters 
    for (uint64_t clusterno = 0; clusterno < allgroupclusterlabels.at(groupno).size(); clusterno++) {
        vector<uint64_t> tmpclusterlabels = parseCSVLabels(readTextFile("../dataset/clusteroutput/clusters/" + to_string(shuffledclusterlabels.at(clusterno)) + ".csv", true));
        // Iterating over the datapoints
        uint64_t startindex = 0;
        for (uint64_t itemno = 0; itemno < MAXIMUM_DORAM_CLUSTER_SIZE; itemno++) {
            if (startindex == tmpclusterlabels.size()) {
                startindex = 0;
            }
            // Filling all the dummy datapoints with repeting real points
            doramshufflelabels.at(groupno).at(clusterno).push_back(tmpclusterlabels.at(startindex));
            startindex++;
        }
    }
    // Padding with fake clusters
    idx = 0;
    for (uint64_t clusterno = allgroupclusterlabels.at(groupno).size(); clusterno < MAXIMUM_DORAM_CLUSTER_NUM; clusterno++) {
        if (idx == allgroupclusterlabels.at(groupno).size()) {
            idx = 0;
        }
        doramshufflelabels.at(groupno).push_back(doramshufflelabels.at(groupno).at(idx));
        idx++;
    }
}