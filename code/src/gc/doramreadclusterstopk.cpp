#pragma once

#include "abycore/aby/abyparty.h"
#include "abycore/circuit/circuit.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"

#include "../utils/globalvarclient.cpp"
#include "../utils/math.cpp"

#include "../algorithms/uniformrandshuffle.cpp"

#include <ENCRYPTO_utils/crypto/crypto.h>

using namespace std;


class GCDoramReadClusterTopK {
protected:
private:
public:
    // The actual GC Top-K circuit making and calculation work happens here ;)
    void buildSharedDistanceTopKCircuit(share* s_client[], share* s_label[], share* s_serverkreyviumshare[], share* s_randid[], share* s_randdistance[], share* s_out_topkdistance[],
        share* s_out_topkid[], share* distance_max_value, share* id_min_value, BooleanCircuit* bc) {

        // dprint("Building approx top-k GC circuit (server/client)");

        share* calculated_dist[SHARE_NUM *
            MAXIMUM_DORAM_CLUSTER_SIZE];

        share* calculated_id[SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE];


        share* dx;
        share* idx;
        share* check_gt;
        share* temp_dist;
        share* temp_id;

        share* check_eq;

        // ######## COMBINED DISTANCE SHARE AND SQUARE CALCULATION ########
        // The s_server_random is the doramreadrandomshare that was added to the disnatnce after the AHE
        // This is the summation of (a-b)^2 for each dimension

        // This is the index for the SHARE_NUM*DIMENSION_NUM
        uint64_t clientshareindex = 0;
        for (uint64_t index = 0; index < SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE; index++) {

            share* temp_dim_dist;
            // Iterating through each dimension of a point
            for (uint64_t dimensionindex = 0; dimensionindex < DIMENSION_NUM; dimensionindex++) {
                if (dimensionindex == 0) {
                    // Subtracting the added kreyvium stream share
                    share* a_minus_b = bc->PutSUBGate(s_client[clientshareindex], s_serverkreyviumshare[index]);
                    temp_dim_dist = bc->PutMULGate(a_minus_b, a_minus_b);
                }
                else {
                    // Subtracting the added kreyvium stream share
                    share* a_minus_b = bc->PutSUBGate(s_client[clientshareindex], s_serverkreyviumshare[index]);
                    temp_dim_dist = bc->PutADDGate(temp_dim_dist, bc->PutMULGate(a_minus_b, a_minus_b));
                }
                clientshareindex++;
            }
            // // Here we get the final distance (a1 - b1)^2 + (a2 - b2)^2 + (a3 - b3)^2
            calculated_dist[index] = temp_dim_dist;

            // Subtracting the added kreyvium stream share
            calculated_id[index] = bc->PutSUBGate(s_label[index], s_serverkreyviumshare[index]);
        }

        // ######## BIT REDUCTION ########
        // Even though in the paper the bit reduction part seems to be a part of GC but
        // it can be done at the server and client side seperately and then the GC substraction is performed as above, I think it is the same, maybe even better

        // CUSTOM TOP-K ALOGORITHM TO AVOID DUPLICATES
        // ######## NAIVE CUSTOM TOP-K ALGORITHM IN GC ########
        for (uint64_t i = 0; i < SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE; i++) {

            dx = calculated_dist[i];
            idx = calculated_id[i];

            // IMPORTANT: This is the extra step where we are checking if an id already exists in the top-k list, if exists then we are repeating it
            // and thus it will be replaced by the distance dx by the "maximum value" and id idx by "minimum value" so it can never be a part of the top-k
            for (uint64_t j = 0; j < TOP_K_NUM; j++) {

                check_eq = bc->PutEQGate(s_out_topkid[j], idx);

                dx = bc->PutMUXGate(distance_max_value, dx, check_eq);
                idx = bc->PutMUXGate(id_min_value, idx, check_eq);
            }

            // Here we perform the actual top-k
            for (uint64_t j = 0; j < TOP_K_NUM; j++) {

                check_gt = bc->PutGTGate(s_out_topkdistance[j], dx);

                temp_dist = bc->PutMUXGate(dx, s_out_topkdistance[j], check_gt);
                dx = bc->PutMUXGate(s_out_topkdistance[j], dx, check_gt);
                s_out_topkdistance[j] = temp_dist;

                temp_id = bc->PutMUXGate(idx, s_out_topkid[j], check_gt);
                idx = bc->PutMUXGate(s_out_topkid[j], idx, check_gt);
                s_out_topkid[j] = temp_id;
            }

        }

        // --- XOR ID and SUBSTRACT DISTANCE ---
        for (uint64_t i = 0; i < TOP_K_NUM; i++) {
            s_out_topkid[i] = bc->PutXORGate(s_out_topkid[i], s_randid[i]);
            s_out_topkdistance[i] = bc->PutSUBGate(s_out_topkdistance[i], s_randdistance[i]);
        }

    }

    // Mostly initializing the normal parameters and the gc share and output paramters
    void gcSharedDistanceTopK(e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {

        // dprint("Initializing GC Tok-K");

        timerStartSub();

        // Describing the GC
        ABYParty* party = new ABYParty(role, address, port, seclv, output_bitlen, nthreads, mt_alg);
        std::vector<Sharing*>& sharings = party->GetSharings();
        Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

        // The GC share parameters, SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE * DIMENSION_NUM
        share* s_client[SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE * DIMENSION_NUM],     // s_client is the (a'-b) for each dimension share that client sends

            // The GC share paramters, SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE
            * s_label[SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE],        // This is the label share of the client

            // Server kreyvium stream share
            * s_serverkreyviumshare[SHARE_NUM * MAXIMUM_DORAM_CLUSTER_SIZE],

            // The server random share that will be subtracted and xored
            * s_randdistance[TOP_K_NUM], * s_randid[TOP_K_NUM],

            // The client share of the top-k
            * s_out_topkdistance[TOP_K_NUM], * s_out_topkid[TOP_K_NUM],

            // These are the min and max values that will be used in the modified top-k algorithm to remove redundancies
            * distance_max_value,
            * id_min_value;

        uint64_t sclient_idx = 0;
        uint64_t slabelserverkreyviumshare_idx = 0;

        uint64_t CLUSTER_NUM = SHARE_NUM / GROUP_NUM;

        if (role == SERVER) {

            // Assign the shares of the max and min values for the custom top-k algorithm
            distance_max_value = circ->PutINGate(PLAIN_MODULUS - 1, output_bitlen, SERVER);
            id_min_value = circ->PutINGate((uint64_t)0, output_bitlen, SERVER);


            for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
                for (uint64_t clusternum = 0; clusternum < CLUSTER_NUM; clusternum++) {
                    for (uint64_t itemnum = 0; itemnum < MAXIMUM_DORAM_CLUSTER_SIZE; itemnum++) {
                        for (uint64_t dimensionnum = 0; dimensionnum < DIMENSION_NUM; dimensionnum++) {
                            // Filling the client share with the dummy values since it is server role
                            s_client[sclient_idx] = circ->PutDummyINGate(output_bitlen);
                            sclient_idx++;
                        }
                        // Filling with the dummy ids that the server puts, the ID share comes from the client side, this is actually maskedsclientdoramlabels
                        s_label[slabelserverkreyviumshare_idx] = circ->PutDummyINGate(output_bitlen);
                        // FIlling the server kreyvium stream share
                        //s_serverkreyviumshare[slabelserverkreyviumshare_idx] = circ->PutINGate(kreyviumserverrandomshares.at(clusternum).at(itemnum), output_bitlen, SERVER);
                        // Filling with some "random value" 1 for now
                        s_serverkreyviumshare[slabelserverkreyviumshare_idx] = circ->PutINGate((uint64_t)1, output_bitlen, SERVER);
                        slabelserverkreyviumshare_idx++;
                    }
                }

            }

            doramrandomidxor.reserve(TOP_K_NUM);
            doramrandomdistancesubstract.reserve(TOP_K_NUM);
            for (uint64_t index = 0; index < TOP_K_NUM; index++) {
                // Here we are just filling the top_k distance shares with the plain modulus value, there will be the output top-k shares received by the client
                s_out_topkdistance[index] = circ->PutINGate(PLAIN_MODULUS, output_bitlen, SERVER);
                // Same as above s_out_topkdistance
                s_out_topkid[index] = circ->PutINGate((uint64_t)0, output_bitlen, SERVER);

                // Saving the random uint that is xored with the id for later use
                doramrandomidxor.push_back(getRandomBitLengthSize());
                // The random uint that will be xored with the id
                s_randid[index] = circ->PutINGate(doramrandomidxor.at(index), output_bitlen, SERVER);
                // Saving the random uint that will be substracted from the distance in modulo
                doramrandomdistancesubstract.push_back(getRandomBitLengthSize());
                // The random uint that will substracted from the distance in modulo
                s_randdistance[index] = circ->PutINGate(doramrandomdistancesubstract.at(index), output_bitlen, SERVER);
            }

        }
        else if (role == CLIENT) {
            // Assign the shares of the max and min values for the custom top-k algorithm
            distance_max_value = circ->PutDummyINGate(output_bitlen);
            id_min_value = circ->PutDummyINGate(output_bitlen);

            for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
                for (uint64_t clusternum = 0; clusternum < CLUSTER_NUM; clusternum++) {
                    for (uint64_t itemnum = 0; itemnum < MAXIMUM_DORAM_CLUSTER_SIZE; itemnum++) {
                        for (uint64_t dimensionnum = 0; dimensionnum < DIMENSION_NUM; dimensionnum++) {
                            // Filling with the client masked dataset
                            s_client[sclient_idx] = circ->PutINGate(maskedclientdoramdataset.at(groupnum).at(clusternum).at(dimensionnum).at(itemnum),
                                output_bitlen, CLIENT);
                            sclient_idx++;
                        }
                        // FIlling the client label shares
                        s_label[slabelserverkreyviumshare_idx] = circ->PutINGate(maskedclientdoramlabels.at(groupnum).at(clusternum).at(itemnum),
                            output_bitlen, CLIENT);
                        // Filling with dummy
                        s_serverkreyviumshare[slabelserverkreyviumshare_idx] = circ->PutDummyINGate(output_bitlen);
                        slabelserverkreyviumshare_idx++;
                    }

                }

            }

            for (uint64_t index = 0; index < TOP_K_NUM; index++) {

                // Here we are just filling the top_k distance shares with dummy values since the assumption was made that the gc calculation will be 
                // ... performed in the server side
                s_out_topkdistance[index] = circ->PutDummyINGate(output_bitlen);
                // Same as above s_topkdistance
                s_out_topkid[index] = circ->PutDummyINGate(output_bitlen);

                // Dummy value of the random uint that will be xored with the id, this value belong to the server
                s_randid[index] = circ->PutDummyINGate(output_bitlen);
                // Dummy value of the random uint that will substracted from the distance in modulo, this value belongs to the server
                s_randdistance[index] = circ->PutDummyINGate(output_bitlen);
            }
        }

        // This actually performs the GC calculations with all the input shares and the output shares ;)
        buildSharedDistanceTopKCircuit(s_client, s_label, s_serverkreyviumshare, s_randid, s_randdistance, s_out_topkdistance, s_out_topkid, distance_max_value,
            id_min_value, (BooleanCircuit*)circ);

        // Receving the output shares after the GC by the client
        for (uint64_t i = 0; i < TOP_K_NUM; i++) {
            s_out_topkdistance[i] = circ->PutOUTGate(s_out_topkdistance[i], CLIENT);
            s_out_topkid[i] = circ->PutOUTGate(s_out_topkid[i], CLIENT);
        }

        // Executing the circuit
        // dprint("Executing GC Top-K circuit (server/client)");
        party->ExecCircuit();

        if (PRINT_NETWORK_USAGE_INFO) {
            iprint("Doram Cluster Dataitem GC Top-K Info:");
            iprint("Total Depth - " + to_string(party->GetTotalDepth()));
            iprint("Total Gates - " + to_string(party->GetTotalGates()));
            iprint("Total Bytes Sent - " + to_string(party->GetSentData(P_TOTAL)));
            iprint("Total Bytes Received - " + to_string(party->GetReceivedData(P_TOTAL)));
            iprint("Timing - " + to_string(party->GetTiming(P_TOTAL)));
        }

        // Client getting the final clear value result share
        if (role == CLIENT) {
            gcoutputdistanceshares.reserve(TOP_K_NUM);
            gcoutputidshares.reserve(TOP_K_NUM);
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                gcoutputdistanceshares.push_back(s_out_topkdistance[i]->get_clear_value<uint64_t>());
                gcoutputidshares.push_back(s_out_topkid[i]->get_clear_value<uint64_t>());
            }
        }
    }

    // Just a threading way of calling the main function
    std::thread threadGCSharedDistanceTopK(e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
        return std::thread([=] { gcSharedDistanceTopK(role, address, port, seclv, output_bitlen, nthreads, mt_alg, sharing); });
    }
};

// Checking thetopk results by combining the shares
void gcDoramReadClusterTopKCheckingCombineShareResult(string itemname) {
    // dprint("Getting results of the GC Top-K");
    iprintlf("DORAM DATAPOINTS TOP-K");
    iprint("--- CHECKING COMBINED SHARE ---");
    for (uint64_t i = 0; i < TOP_K_NUM; i++) {
        // Here we are just printing the idand the distance just for the visual purpose, in reality the actual saving of the data 
        // takes place after this function call
        iprint("  --- " + itemname + " " + to_string(i) + " ---");
        iprint("    GC Distance : " + to_string(mathModuloAdd(gcoutputdistanceshares.at(i), doramrandomdistancesubstract.at(i), PLAIN_MODULUS))
            + " ID : " + to_string(gcoutputidshares.at(i) ^ doramrandomidxor.at(i)));
    }
}

// Initializes the GC as will be done at the server and client side
void gcDoramReadClusterTopKStart() {

    // dprint("Top-K calculation with GC started");
    e_mt_gen_alg mt_alg = MT_OT;
    seclvl seclv = get_sec_lvl(128);
    GCDoramReadClusterTopK* gcserver = new GCDoramReadClusterTopK();
    GCDoramReadClusterTopK* gcclient = new GCDoramReadClusterTopK();
    // Ideally this should be done on the seperate systems (client and server)
    // Client does this on its side
    thread threadclient = gcclient->threadGCSharedDistanceTopK(CLIENT, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
    // Server does this on its side
    thread threadserver = gcserver->threadGCSharedDistanceTopK(SERVER, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
    threadclient.join();
    threadserver.join();
}