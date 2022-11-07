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

// To tell which group it is when we are doing the group process, the output to the client will be substracted by a server random uint32 in the modulo uint32
// ... client gets its substracted group id share, server keeps its random uint32
uint64_t groupid = 0;

class GCTopK {
protected:
private:
public:
    // The actual GC Top-K circuit making and calculation work happens here ;)
    void buildSharedDistanceTopKCircuitStash(share* s_client[], share* s_server[], share* s_id[], share* s_randid[], share* s_randdistance[], share* s_out_topkdistance[],
        share* s_out_topkid[], BooleanCircuit* bc) {

        // dprint("Building approx top-k GC circuit (server/client)");

        share* calculated_dist[SHARE_NUM];
        share* dx;
        share* idx;
        share* check_gt;

        share* min_distance_list[BIN_NUM];
        share* min_id_list[BIN_NUM];

        share* temp_dist;
        share* temp_id;

        // ######## COMBINED DISTANCE CALCULATION ########
        for (uint64_t i = 0; i < SHARE_NUM; i++) {
            // does (a^2 + b^2 - 2ab - r) + r
            calculated_dist[i] = bc->PutADDGate(s_client[i], s_server[i]);
        }

        // ######## BIT REDUCTION ########
        // Even though in the paper the bit reduction part seems to be a part of GC but
        // it can be done at the server and client side seperately and then the GC substraction is performed as above, I think it is the same, maybe even better


        // ######## APPROXIMATE TOP-K ALGORITHM IN GC ########
        if (BIN_NUM != 0) {
            // --- Server performing uniform random shuffle of the GC shares ---
            shuffleGCShares(calculated_dist, s_id);
            // --- Finding minimum of the BIN_NUM bins of size SHARE_NUM/BIN_NUM ---
            uint64_t bin_item_count = 0;
            uint64_t min_list_index = 0;
            share* temp_min_distance, * temp_min_id, * temp_min_check_gt;
            bool divisibility_check = false;

            // Dummy maximum value so it can take minimum from the bin again ---
            temp_min_distance = s_out_topkdistance[0];
            // Some value, doesn't matter ---       
            temp_min_id = s_out_topkid[0];
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                divisibility_check = false;

                temp_min_check_gt = bc->PutGTGate(temp_min_distance, calculated_dist[i]);
                temp_min_distance = bc->PutMUXGate(calculated_dist[i], temp_min_distance, temp_min_check_gt);
                temp_min_id = bc->PutMUXGate(s_id[i], temp_min_id, temp_min_check_gt);
                bin_item_count++;

                if (bin_item_count == BIN_SIZE) {
                    divisibility_check = true;
                    bin_item_count = 0;
                    min_distance_list[min_list_index] = temp_min_distance;
                    min_id_list[min_list_index] = temp_min_id;
                    min_list_index++;
                    // Resetting the min distance with a maximum value ---
                    temp_min_distance = s_out_topkdistance[0];
                    temp_min_id = s_out_topkid[0];
                }
            }
            // This check is for the remainder points which fit in the last bin and thus the last bin is not full
            if (divisibility_check == false) {
                min_distance_list[min_list_index] = temp_min_distance;
                min_id_list[min_list_index] = temp_min_id;
            }
            // --- Naive Top-K out of the BINS ---
            for (uint64_t i = 0; i < BIN_NUM; i++) {
                dx = min_distance_list[i];
                idx = min_id_list[i];
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
        }
        // ######## NAIVE TOP-K ALGORITHM IN GC ########
        else {
            // --- Server performing uniform random shuffle of the GC shares ---
            shuffleGCShares(calculated_dist, s_id);
            // --- top-k ---
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                dx = calculated_dist[i];
                idx = s_id[i];
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
        }

        // --- XOR ID and SUBSTRACT DISTANCE ---
        for (uint64_t i = 0; i < TOP_K_NUM; i++) {
            // the shared distance and the id are returned
            s_out_topkid[i] = bc->PutXORGate(s_out_topkid[i], s_randid[i]);
            s_out_topkdistance[i] = bc->PutSUBGate(s_out_topkdistance[i], s_randdistance[i]);
        }

    }

    // Mostly initializing the normal parameters and the gc share and output paramters
    void gcSharedDistanceTopKStash(vector<uint64_t> secretshare, e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {

        // dprint("Initializing GC Tok-K Stash");

        // Describing the GC
        ABYParty* party = new ABYParty(role, address, port, seclv, output_bitlen, nthreads, mt_alg);
        std::vector<Sharing*>& sharings = party->GetSharings();
        Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

        // The GC share paramters, both input and output
        share* s_client[SHARE_NUM], * s_server[SHARE_NUM], * s_id[SHARE_NUM], * s_randdistance[TOP_K_NUM], * s_randid[TOP_K_NUM],
            * s_out_topkdistance[TOP_K_NUM], * s_out_topkid[TOP_K_NUM];

        // The seperate share of client and the server which we get from the secretshare when called individually
        vector<uint64_t> client_share, server_share;

        if (role == SERVER) {
            // Since it is server role, we assign the secret share to the server share 
            server_share = secretshare;
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                // Filling the client share with the dummy values since it is server role
                s_client[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the actual server shares
                s_server[i] = circ->PutINGate(server_share.at(i), output_bitlen, SERVER);
                // Filling with the ids that the server has, it is in the same sequence as the shares were sent to the client after the AHE, 
                // ... the uniform shuffle happens later
                s_id[i] = circ->PutINGate(stashlabels.at(i), output_bitlen, SERVER);
            }
            stashrandomidxor.reserve(TOP_K_NUM);
            stashrandomdistancesubstract.reserve(TOP_K_NUM);
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                // Here we are just filling the top_k distance shares with the plain modulus value, there will be the output top-k shares received by the client
                s_out_topkdistance[i] = circ->PutINGate(PLAIN_MODULUS, output_bitlen, SERVER);
                // Same as above s_out_topkdistance
                s_out_topkid[i] = circ->PutINGate((uint64_t)0, output_bitlen, SERVER);

                // Saving the random uint that is xored with the id for later use
                stashrandomidxor.push_back(getRandomBitLengthSize());
                // The random uint that will be xored with the id
                s_randid[i] = circ->PutINGate(stashrandomidxor[i], output_bitlen, SERVER);

                // Saving the random uint that will be substracted from the distance in modulo
                stashrandomdistancesubstract.push_back(getRandomBitLengthSize());
                // The random uint that will substracted from the distance in modulo
                s_randdistance[i] = circ->PutINGate(stashrandomdistancesubstract[i], output_bitlen, SERVER);
            }
        }
        else if (role == CLIENT) {
            // Since it is client role, we assign the secret share to the client share 
            client_share = secretshare;
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                // Filling with the actual client shares, + 1 has been added to add an inacccuracy of +1 in teh distance such that the 0 distance is also counted
                // ... see notes
                s_client[i] = circ->PutINGate(client_share.at(i) + 1, output_bitlen, CLIENT);
                // Filling the server share with the dummy values since it is client role
                s_server[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the ids with dummy values since the server have the real values, it is in the same sequence as the shares were sent to the client 
                // ... after the AHE, the uniform shuffle happens later
                s_id[i] = circ->PutDummyINGate(output_bitlen);
            }
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                // Here we are just filling the top_k distance shares with dummy values since the assumption was made that the gc calculation will be 
                // ... performed in the server side
                s_out_topkdistance[i] = circ->PutDummyINGate(output_bitlen);
                // Same as above s_topkdistance
                s_out_topkid[i] = circ->PutDummyINGate(output_bitlen);

                // Dummy value of the random uint that will be xored with the id, this value belong to the server
                s_randid[i] = circ->PutDummyINGate(output_bitlen);
                // Dummy value of the random uint that will substracted from the distance in modulo, this value belongs to the server
                s_randdistance[i] = circ->PutDummyINGate(output_bitlen);
            }
        }

        // This actually performs the GC calculations with all the input shares and the output shares ;)
        buildSharedDistanceTopKCircuitStash(s_client, s_server, s_id, s_randid, s_randdistance, s_out_topkdistance, s_out_topkid, (BooleanCircuit*)circ);

        // Receving the output shares after the GC by the client
        for (uint64_t i = 0; i < TOP_K_NUM; i++) {
            s_out_topkdistance[i] = circ->PutOUTGate(s_out_topkdistance[i], CLIENT);
            s_out_topkid[i] = circ->PutOUTGate(s_out_topkid[i], CLIENT);
        }

        // Executing the circuit
        // dprint("Executing GC Top-K circuit (server/client)");
        party->ExecCircuit();

        if (PRINT_NETWORK_USAGE_INFO) {
            iprint("Stash GC Info:");
            iprint("Total Depth - " + to_string(party->GetTotalDepth()));
            iprint("Total Gates - " + to_string(party->GetTotalGates()));
            iprint("Total Bytes Sent - " + to_string(party->GetSentData(P_TOTAL)));
            iprint("Total Bytes Received - " + to_string(party->GetReceivedData(P_TOTAL)));
            iprint("Timing - " + to_string(party->GetTiming(P_TOTAL)));
        }

        // Client getting the final clear value result shares
        gcoutputdistanceshares.reserve(TOP_K_NUM);
        gcoutputidshares.reserve(TOP_K_NUM);
        if (role == CLIENT) {
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                gcoutputdistanceshares.push_back(s_out_topkdistance[i]->get_clear_value<uint64_t>());
                gcoutputidshares.push_back(s_out_topkid[i]->get_clear_value<uint64_t>());
            }
        }
    }

    // The actual GC Top-K circuit making and calculation work happens here ;)
    void buildSharedDistanceTopKCircuitGroup(share* s_client[], share* s_server[], share* s_id[], share* s_topkdistance[],
        share* s_out_topkid[], BooleanCircuit* bc) {

        // dprint("Building approx top-k GC circuit (server/client)");

        share* calculated_dist[SHARE_NUM];
        share* dx;
        share* idx;
        share* check_gt;

        share* min_distance_list[BIN_NUM];
        share* min_id_list[BIN_NUM];

        share* temp_dist;
        share* temp_id;

        // ######## COMBINED DISTANCE CALCULATION ########
        for (uint64_t i = 0; i < SHARE_NUM; i++) {
            calculated_dist[i] = bc->PutADDGate(s_client[i], s_server[i]);
        }

        // ######## BIT REDUCTION ########
        // Even though in the paper the bit reduction part seems to be a part of GC but
        // it can be done at the server and client side seperately and then the GC substraction is performed as above, I think it is the same, maybe even better


        // ######## APPROXIMATE TOP-K ALGORITHM IN GC ########
        if (BIN_NUM != 0) {
            // --- Server performing uniform random shuffle of the GC shares ---
            shuffleGCShares(calculated_dist, s_id);
            // --- Finding minimum of the BIN_NUM bins of size SHARE_NUM/BIN_NUM ---
            uint64_t bin_item_count = 0;
            uint64_t min_list_index = 0;
            share* temp_min_distance, * temp_min_id, * temp_min_check_gt;
            bool divisibility_check = false;

            // Dummy maximum value so it can take minimum from the bin again ---
            temp_min_distance = s_topkdistance[0];
            // Some value, doesn't matter ---       
            temp_min_id = s_out_topkid[0];
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                divisibility_check = false;

                temp_min_check_gt = bc->PutGTGate(temp_min_distance, calculated_dist[i]);
                temp_min_distance = bc->PutMUXGate(calculated_dist[i], temp_min_distance, temp_min_check_gt);
                temp_min_id = bc->PutMUXGate(s_id[i], temp_min_id, temp_min_check_gt);
                bin_item_count++;

                if (bin_item_count == BIN_SIZE) {
                    divisibility_check = true;
                    bin_item_count = 0;
                    min_distance_list[min_list_index] = temp_min_distance;
                    min_id_list[min_list_index] = temp_min_id;
                    min_list_index++;
                    // Resetting the min distance with a maximum value ---
                    temp_min_distance = s_topkdistance[0];
                    temp_min_id = s_out_topkid[0];
                }
            }
            // This check is for the remainder points which fit in the last bin and thus the last bin is not full
            if (divisibility_check == false) {
                min_distance_list[min_list_index] = temp_min_distance;
                min_id_list[min_list_index] = temp_min_id;
            }
            // --- Naive Top-K out of the BINS ---
            for (uint64_t i = 0; i < BIN_NUM; i++) {
                dx = min_distance_list[i];
                idx = min_id_list[i];
                for (uint64_t j = 0; j < TOP_K_NUM; j++) {
                    check_gt = bc->PutGTGate(s_topkdistance[j], dx);

                    temp_dist = bc->PutMUXGate(dx, s_topkdistance[j], check_gt);
                    dx = bc->PutMUXGate(s_topkdistance[j], dx, check_gt);
                    s_topkdistance[j] = temp_dist;

                    temp_id = bc->PutMUXGate(idx, s_out_topkid[j], check_gt);
                    idx = bc->PutMUXGate(s_out_topkid[j], idx, check_gt);
                    s_out_topkid[j] = temp_id;
                }
            }
        }
        // ######## NAIVE TOP-K ALGORITHM IN GC ########
        else {
            // --- Server performing uniform random shuffle of the GC shares ---
            shuffleGCShares(calculated_dist, s_id);
            // --- top-k ---
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                dx = calculated_dist[i];
                idx = s_id[i];
                for (uint64_t j = 0; j < TOP_K_NUM; j++) {
                    check_gt = bc->PutGTGate(s_topkdistance[j], dx);

                    temp_dist = bc->PutMUXGate(dx, s_topkdistance[j], check_gt);
                    dx = bc->PutMUXGate(s_topkdistance[j], dx, check_gt);
                    s_topkdistance[j] = temp_dist;

                    temp_id = bc->PutMUXGate(idx, s_out_topkid[j], check_gt);
                    idx = bc->PutMUXGate(s_out_topkid[j], idx, check_gt);
                    s_out_topkid[j] = temp_id;
                }
            }
        }
    }

    // Mostly initializing the normal parameters and the gc share and output paramters
    void gcKreyviumDecrypt(vector<uint64_t> secretshare, e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {

        // dprint("Initializing GC Tok-K");

        // Describing the GC
        ABYParty* party = new ABYParty(role, address, port, seclv, output_bitlen, nthreads, mt_alg);
        std::vector<Sharing*>& sharings = party->GetSharings();
        Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

        // The GC share paramters, both input and output
        share* s_client[SHARE_NUM], * s_server[SHARE_NUM], * s_id[SHARE_NUM],
            * s_topkdistance[TOP_K_NUM], * s_out_topkid[TOP_K_NUM];

        // The seperate share of client and the server which we get from the secretshare when called individually
        vector<uint64_t> client_share, server_share;

        if (role == SERVER) {
            // Since it is server role, we assign the secret share to the server share 
            server_share = secretshare;
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                // Filling the client share with the dummy values since it is server role
                s_client[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the actual server shares
                s_server[i] = circ->PutINGate(server_share.at(i), output_bitlen, SERVER);
                // Filling with the ids that the server has, it is in the same sequence as the shares were sent to the client after the AHE, the uniform shuffle happens later
                s_id[i] = circ->PutINGate(doramgroupclusterid.at(groupid).at(i), output_bitlen, SERVER);
            }
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                // Here we are just filling the top_k distance shares with the plain modulus value, there will be the output top-k shares received by the client
                s_topkdistance[i] = circ->PutINGate(PLAIN_MODULUS, output_bitlen, SERVER);
                // Same as above s_out_topkdistance
                s_out_topkid[i] = circ->PutINGate((uint64_t)0, output_bitlen, SERVER);
            }
        }
        else if (role == CLIENT) {
            // Since it is client role, we assign the secret share to the client share 
            client_share = secretshare;
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                // Filling with the actual cleint shares, + 1 has been added to add an inacccuracy of +1 in teh distance such that the 0 distance is also counted
                // ... see notes
                s_client[i] = circ->PutINGate(client_share.at(i), output_bitlen, CLIENT);
                // Filling the server share with the dummy values since it is client role
                s_server[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the ids with dummy values since the server have the real values, it is in the same sequence as the shares were sent to the client 
                // ... after the AHE, the uniform shuffle happens later
                s_id[i] = circ->PutDummyINGate(output_bitlen);
            }
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                // Here we are just filling the top_k distance shares with dummy values since the assumption was made that the gc calculation will be 
                // ... performed in the server side
                s_topkdistance[i] = circ->PutDummyINGate(output_bitlen);
                // Same as above s_topkdistance
                s_out_topkid[i] = circ->PutDummyINGate(output_bitlen);
            }
        }

        // This actually performs the GC calculations with all the input shares and the output shares ;)
        buildSharedDistanceTopKCircuitGroup(s_client, s_server, s_id, s_topkdistance, s_out_topkid, (BooleanCircuit*)circ);

        // Receving the output shares after the GC by the client
        for (uint64_t i = 0; i < TOP_K_NUM; i++) {
            s_out_topkid[i] = circ->PutOUTGate(s_out_topkid[i], CLIENT);
        }

        // Executing the circuit
        // dprint("Executing GC Top-K circuit (server/client)");
        party->ExecCircuit();

        if (PRINT_NETWORK_USAGE_INFO) {
            iprint("Group GC Info:");
            iprint("Total Depth - " + to_string(party->GetTotalDepth()));
            iprint("Total Gates - " + to_string(party->GetTotalGates()));
            iprint("Total Bytes Sent - " + to_string(party->GetSentData(P_TOTAL)));
            iprint("Total Bytes Received - " + to_string(party->GetReceivedData(P_TOTAL)));
            iprint("Timing - " + to_string(party->GetTiming(P_TOTAL)));
        }

        // Client getting the final clear value result shares
        if (role == CLIENT) {
            gcoutputidshares.reserve(TOP_K_NUM);
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                gcoutputidshares.push_back(s_out_topkid[i]->get_clear_value<uint64_t>());
            }
        }
    }

    // Just a threading way of calling the main function
    std::thread threadGCSharedDistanceTopKStash(vector<uint64_t> secretshare, e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
        return std::thread([=] { gcSharedDistanceTopKStash(secretshare, role, address, port, seclv, output_bitlen, nthreads, mt_alg, sharing); });
    }

    // Just a threading way of calling the main function
    std::thread threadGCKreyviumDecrypt(vector<uint64_t> secretshare, e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
        return std::thread([=] { gcKreyviumDecrypt(secretshare, role, address, port, seclv, output_bitlen, nthreads, mt_alg, sharing); });
    }
};

// Checking the topk shares of server and client by combining them
void gcTopKCheckingCombinedSharesResultStash() {
    // dprint("Getting results of the GC Top-K");
    for (uint64_t i = 0; i < TOP_K_NUM; i++) {
        // Here we are just printing the id and the distance just for the visual purpose, in reality the actual saving of the data takes place after this function call
        iprint("  --- CHECKING COMBINED SHARE - DATAPOINT " + to_string(i) + " ---");
        iprint("    GC DISTANCE : " + to_string(mathModuloAdd(gcoutputdistanceshares.at(i), stashrandomdistancesubstract.at(i), PLAIN_MODULUS)) +
            " ID : " + to_string(gcoutputidshares.at(i) ^ stashrandomidxor.at(i)));

        // iprint("    GC DISTANCE : " + to_string(gcoutputdistanceshares.at(i)) +
        //     " ID : " + to_string(gcoutputidshares.at(i) ^ stashrandomidxor.at(i)));

        // iprint(" ID : " + to_string(gcoutputidshares.at(i) ^ stashrandomidxor.at(i)));
    }
}

// Checking the topk shares of server and client by combining them
void gcTopKCheckingCombinedSharesResultGroup() {
    // dprint("Getting results of the GC Top-K");
    for (uint64_t i = 0; i < TOP_K_NUM; i++) {
        // Here we are just printing the id and the distance just for the visual purpose, in reality the actual saving of the data takes place after this function call
        iprint("  --- CHECKING COMBINED SHARE - CLUSTER " + to_string(i) + " ---");
        iprint("    ID : " + to_string(gcoutputidshares.at(i)));

    }
}

// Initializes the GC as will be done at the server and client side
void gcTopKStart(vector<uint64_t> clientshare, vector<uint64_t> servershare, uint64_t groupindex) {

    if (IS_GROUP_MODE) {
        // dprint("Top-K calculation with GC started - Group");
        // Setting if this GCtopK is in a group mode, if so what is the groupid
        groupid = groupindex;
        // Uniform shuffling sequence generation done by the server, this is done for the group and stash process both
        generateRandomShuffleSequence();
        e_mt_gen_alg mt_alg = MT_OT;
        seclvl seclv = get_sec_lvl(128);
        GCTopK* gcserver = new GCTopK();
        GCTopK* gcclient = new GCTopK();
        // Ideally this should be done on the seperate systems (client and server)
        // Client does this on its side
        thread threadclient = gcclient->threadGCKreyviumDecrypt(clientshare, CLIENT, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
        // Server does this on its side
        thread threadserver = gcserver->threadGCKreyviumDecrypt(servershare, SERVER, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
        threadclient.join();
        threadserver.join();
    }
    else {
        // dprint("Top-K calculation with GC started - Stash");
        // Uniform shuffling sequence generation done by the server, this is done for the group and stash process both
        generateRandomShuffleSequence();
        e_mt_gen_alg mt_alg = MT_OT;
        seclvl seclv = get_sec_lvl(128);
        GCTopK* gcserver = new GCTopK();
        GCTopK* gcclient = new GCTopK();
        // Ideally this should be done on the seperate systems (client and server)
        // Client does this on its side
        thread threadclient = gcclient->threadGCSharedDistanceTopKStash(clientshare, CLIENT, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
        // Server does this on its side
        thread threadserver = gcserver->threadGCSharedDistanceTopKStash(servershare, SERVER, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
        threadclient.join();
        threadserver.join();
    }

}