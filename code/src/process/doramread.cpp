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

#include "../gc/doramdatabaseread.cpp"

using namespace std;

// Here we read the all the data items of the top-k clusters we get from the previous group process.
void processDoramRead() {
    // dprint("Client performing DORAM read to get the masked DB for the requested IDs");

    // ######## SERVER ########
    // ---- Performs the kreyvium encryption -----
    // Do encryption on each cluster share with some key
    encryptClientClusterShare();

    // ######## SERVER ########
    // ---- Sends the shared encrypted data to client -----
    sendSharesToClient();

    // ######## SERVER + CLIENT (WORK TOGETHER, BUT IN SECERET) ######## 
    // ---- Perform Garble Circuit For DORAM Read ---- 
    // In Doram read GC we retrive a secret share of all the datapoints and labels of all the top-k clusters from all the groups
    gcDoramReadStart();

    // ######## CLIENT ########
    // ---- Print the GC result ;) ---- 
    gcDoramReadCheckTopkClusterIDs();

    // ###### CLIENT #########
    // ---- Perform the cluster id check and decryption with the correct key -----
    chooseTopClientClusterShares();

}