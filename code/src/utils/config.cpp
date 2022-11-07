#pragma once
#include <cstddef>
#include <iostream>

using namespace std;

// ### COMMON CONFIG ###
uint64_t POLY_MODULUS_DEGREE = 16384; // Modulus degree for AHE, 32768, 16384, 8192, 4096, 2048
uint64_t BIT_LENGTH = 60;             // Used for AHE and other GC stuff
uint64_t PLAIN_MODULUS = 0;           // The modulus for the plaintext, basically the unecrypted data we have from AHE
uint64_t GC_THREADS = 1;              // Number of parallel GC executions

bool PRINT_NETWORK_USAGE_INFO = true;                 // Enable to print the GC and HE informations

// ### SERVER CONFIG ###
bool REDUCED_MODE = 0;                // use reduced dataset
bool ORIGINAL_MODE = 0;               // use original dataset

bool PROCESS_DATASET = false;          // Set to true to do the initial process on dataset with the python
bool PROCESS_DATASET_AND_EXIT = false; // Set to true to do just the dataset processing part and exit
bool REDUCE_DATASET = false;           // Set to true to do the neighbourhood reduction (REALLY TAKES TIME)
bool REDUCE_DATASET_AND_EXIT = false;  // Set to true to just do the dataset reduction and exit

uint64_t NORMALIZATION_MAGNITUDE = 2; // The magnitude by which the dataset has to be multipled to make decimal number to integers
uint64_t SHARE_NUM = 0;               // Number of shares we have for any GC phase

uint64_t DIMENSION_NUM = 3; // Total dimensions the dataset has, like now we have (userid, movieid, ratings) - 3 dimensions

uint64_t BITS_TO_REDUCE = 0; // To reduce accuracy of distance in knnsLinearScan alogorithm
uint64_t BIN_NUM = 0;        // Alwayss <= Dataset size(GROUP SIZE or STASH SIZE),, Number of bins that will be used, less number of bins -> larger bin size
// ... SET SEPERATE FOR EACH IN STASH.CPP AND GROUP.CPP
// ... IMPORTANT - When BIN_NUM is 0, then we perform the simple top-k in GC instead of approximate top-k, make it 1%
uint64_t BIN_SIZE = 0;       // Set later on, size of each bin that get from SHARE_NUM/BIN_NUM

// 4096 -> 80
uint64_t MAX_CLUSTER_NUM = 1500; // The maximum number of clusters that can be made from the datapoints we have BEFORE OPTIMISING with the MAX_CLUSTER_SIZE parameter
// ... to generate even cluster sizes, ideally(personal) 1-3% of dataset
// 4096 -> 100
uint64_t MAX_CLUSTER_SIZE = 20; // The maximum number of points allowed in the "first" constructed cluster, anything more will be sub clustered once for the
// ... optimisation part
// **** TODO MAYBE MORE SUBCLUSTERING WOULD BE NICE AND PROPER ****
// 4096 -> 36
// standard options 2-7
uint64_t MAX_GROUP_NUM = 4; // The maximum number of groups that can be made BEFORE STASHING with the cluster center points we have
// (STRANGELY CANT SET IT TO 2 OR LESS)
// 4096 -> 5
uint64_t MIN_GROUP_SIZE = 600; // This is used to check if the group should be put in the stash because it has less clusters than this value
uint64_t GROUP_NUM = 0;       // The number of groups we have after making the stash

bool IS_GROUP_MODE = false;      // This is to set if the GC top-k will be in the group mode since the process is almost same as stash except a small part

uint64_t MAXIMUM_DORAM_CLUSTER_SIZE = 0;          // This is the MAXIMUM and the MINIMUM number of the points each doram block representing a cluster will have
uint64_t MAXIMUM_DORAM_CLUSTER_NUM = 0;           // This is the maximum number of clusters we have among all the group, mainly used to make the GC share arrays
uint64_t DORAM_CLUSTER_NUM = 0;                   // This is the number of clusters we have from all the groups for the DORAM dataset
uint64_t DORAM_GROUP_CLUSTER_INDEX_DIMENSION = 2; // This is the number of dimensions we have for the group and cluster index, which is naturally 2 (groupid, clusterid)

uint64_t KREYVIUM_ADJUSTED_KEY_SIZE = 0;                 // BIT_LENGTH of HE is adjusted to the appropriate key size for kreyvium ex. BIT_LENGTH = 60 -> ADJUSTED_KEY_SIZE = 64

// ### CLIENT CONFIG ###
uint64_t TOP_K_NUM = 0; // Sets the number of top-k we need from the distance set

uint64_t CLIENTID = 2; // Set the ID of the client that will be used to reduce the dataset and perform the query
