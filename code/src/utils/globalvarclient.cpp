#pragma once
#include <cstddef>
#include <iostream>
#include <vector>
#include "seal/seal.h"

using namespace std;
using namespace seal;

vector<uint64_t> userpoint;                 // User query point in N-dimensions
// dimension
vector<Plaintext> user_p;                   // User query point represented in HE Plaintext
// dimension
vector<Ciphertext> user_c;                  // User query point encrypted with the secret key in HE Ciphertext
vector<uint64_t> vectordistance;            // Decrypted client distance share after the server returns the encrypted client share

vector<uint64_t> gcoutputdistanceshares;    // Output of the GC top k distance share
vector<uint64_t> gcoutputidshares;          // Output of the GC top k item id(stash)/cluster id(group) share based on distance


vector<uint64_t> gcstashdistanceshare;      // Saves the GC output of the stash process for the datapoint distance share
vector<uint64_t> gcstashidshare;            // Saves the GC output of the stash process for the datapoint id share
//      (Cluster ID share) --> Shape should be [Group number, top k cluster id]
vector<vector<uint64_t>> gctopkclusteridshare;     // Saves the the top-k cluster id share for the client side

vector<uint64_t> gcdoramtopkdatapointdistanceshare;      // Saves the GC output of the doram top-k datapoints process for the datapoint distance share
vector<uint64_t> gcdoramtopkdatapointidshare;            // Saves the GC output of the doram top-k datapoints process for the datapoint id share
vector<uint64_t> gcstashplusdoramdatapointdistance;     // Contains the gcstashdistanceshare and gcdoramtopkdatapointdistanceshare together
vector<uint64_t> gcstashplusdoramdatapointid;           // Contains the gcstashdistanceshare and gcdoramtopkdatapointidshare together


//grpno  clusterindex
vector<vector<uint64_t>> clientgroupclusterid;     // This contains the order of shuffle of the cluster id inside a group which is same 
//as doramgroupclusterid
//                                                                  1..T           1..N              3(kind of constant)     MAXIMUM_DORAM_CLUSTER_SIZE
//  Data structure of encrypted client cluster share --- vector shape --- (number of group, number of cluster, number of dimension, number of points)
vector<vector<vector<vector<uint64_t>>>> clientclustershare;  // Contains all the encrypted clusters and their datapoint shares
//groupno  clusterno  masked clusterlabels
vector<vector<vector<uint64_t>>> clientclusterlabelshare;           // Contains all the encrypted cluster label shares

//                                                                  1..T           1..N              3(kind of constant)     MAXIMUM_DORAM_CLUSTER_SIZE
//  Data structure of masked doramdataset --- vector shape --- (number of group, number of cluster, number of dimension, number of points)
vector<vector<vector<vector<uint64_t>>>> maskedclientdoramdataset;  // Contains all the clusters and their datapoints which will be qualified to be in 
// ... the group along with the dummy items to reach the max size

//groupno  clusterno  masked dataitem labels
vector<vector<vector<uint64_t>>> maskedclientdoramlabels;   // Contains all the dataitem labels of the clusters which will be qualified to be in 
// ... the group along with the dummy items to reach the max size
//groupno  clusterno  masked dataitem labels
vector<uint64_t> maskedclientdoramlabelsflat;   // Contains all the dataitem labels of the clusters which will be qualified to be in 
// ... the group along with the dummy items to reach the max size

// [group num] client cluster id
vector<vector<uint64_t>> clientclusteridshare; // The client uses the client cluster id share to identify the cluster to be sent to the server for 
// ... further distance computations

//  [grpno]   [cluster seed]
vector<vector<uint64_t>> clientseedshares; // Stores here the seed to generate the random shares of the server which are
// ... xored/added to the cluster items and labels.

//  [grpno]   [cluster seed]
vector<vector<uint64_t>> clientseedtopkshares; // Stores here the seed to generate the random shares of the server which are
// ... xored/added to the TOP-K cluster items and labels.

vector<uint8_t> kreyviumencryptoutstate;    // Conatians the out state of the kreyvium generated from the seed