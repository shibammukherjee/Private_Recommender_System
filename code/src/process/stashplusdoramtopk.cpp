#pragma once
#include "../utils/config.cpp"
#include "../utils/vectors.cpp"

#include "../serverside/server.cpp"
#include "../clientside/client.cpp"

#include "../gc/stashplusdoramdatapointtopk.cpp"

using namespace std;

// This gives the top-k of the (stash + doram topk datapoints)
void processStashPlusDoramTopk() {

    // Setting the top k for the combined
    TOP_K_NUM = 5;

    // ##### CLIENT #####
    // Combing the stash vcetor and the doram top-k vector into one single vector for the final top-k
    clientCombineStashAndDoramTopKDatapointShareResult();

    // ##### SERVER #####
    // Combining the stash vector and the doram top-k vector into one single vector for the final top-k
    serverCombineStashAndDoramTopKDatapointShareResult();
    // server setting the total number of shares that we have for the GC
    SHARE_NUM = gcstashplusdoramdatapointdistance.size();

    // ##### CLIENT + SERVER (SEPERATELY) #####
    // Clearining the old vector
    clearOldResultVectors();

    // ######## SERVER + CLIENT (WORK TOGETHER, BUT IN SECERET) ######## 
    // ---- Perform Garble Circuit ---- 
    gcStashPlusDoramDatapointTopKStart();

    // ##### CLIENT #####
    // Client getting the final top-k out of the stash and the doram dataset
    gcStashPlusDoramDatapointTopKGetResult("FINAL DATAPOINT");

}