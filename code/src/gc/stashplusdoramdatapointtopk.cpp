#pragma once

#include "abycore/aby/abyparty.h"
#include "abycore/circuit/circuit.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"

#include "../utils/globalvarclient.cpp"

#include "../algorithms/uniformrandshuffle.cpp"

#include <ENCRYPTO_utils/crypto/crypto.h>

using namespace std;


class GCStashPlusDoramDatapointTopK {
protected:
private:
public:
    // The actual GC Top-K circuit making and calculation work happens here ;)
    void buildSharedDistanceTopKCircuit(share* s_distanceclient[], share* s_distancesubstractserver[], share* s_idclient[], share* s_idxorserver[], share* s_out_topkdistance[], share* s_out_topkid[], BooleanCircuit* bc) {

        // dprint("Building approx top-k GC circuit (server/client)");

        share* calculated_dist[SHARE_NUM];
        share* calculated_label[SHARE_NUM];
        share* dx;
        share* idx;
        share* check_gt;

        share* temp_dist;
        share* temp_id;

        // ######## COMBINED DISTANCE AND ID(LABEL) CALCULATION ########
        for (uint64_t i = 0; i < SHARE_NUM; i++) {
            calculated_dist[i] = bc->PutADDGate(s_distanceclient[i], s_distancesubstractserver[i]);
            calculated_label[i] = bc->PutXORGate(s_idclient[i], s_idxorserver[i]);
        }

        // ######## BIT REDUCTION ########
        // Even though in the paper the bit reduction part seems to be a part of GC but
        // it can be done at the server and client side seperately and then the GC substraction is performed as above, I think it is the same, maybe even better


        // ######## NAIVE TOP-K ALGORITHM IN GC ########
        for (uint64_t i = 0; i < SHARE_NUM; i++) {
            dx = calculated_dist[i];
            idx = calculated_label[i];
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

    // Mostly initializing the normal parameters and the gc share and output paramters
    void gcSharedDistanceTopK(e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {

        // dprint("Initializing GC Tok-K");

        // Describing the GC
        ABYParty* party = new ABYParty(role, address, port, seclv, output_bitlen, nthreads, mt_alg);
        std::vector<Sharing*>& sharings = party->GetSharings();
        Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

        // The GC share paramters, both input and output
        share* s_distanceclient[SHARE_NUM], * s_distancesubstractserver[SHARE_NUM], * s_datapointlabelclient[SHARE_NUM], * s_datapointlabelxorserver[SHARE_NUM],
            * s_out_topkdistance[TOP_K_NUM], * s_out_topkid[TOP_K_NUM];

        // The seperate share of client and the server which we get from the secretshare when called individually
        vector<uint64_t> client_share, server_share;

        if (role == SERVER) {
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                // Filling the client share with the dummy values since it is server role
                s_distanceclient[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the actual server distance shares
                s_distancesubstractserver[i] = circ->PutINGate(gcstashplusdoramdatapointdistancesubstract.at(i), output_bitlen, SERVER);
                // Filling with the ids with the dummy values since it is server role
                s_datapointlabelclient[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the datapoint label with the server share
                s_datapointlabelxorserver[i] = circ->PutINGate(gcstashplusdoramdatapointidxor.at(i), output_bitlen, SERVER);
            }
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                // Here we are just filling the top_k distance shares with the plain modulus value, there will be the output top-k shares received by the client
                s_out_topkdistance[i] = circ->PutINGate(PLAIN_MODULUS, output_bitlen, SERVER);
                // Same as above s_out_topkdistance, but instaed filling with the minimum value
                s_out_topkid[i] = circ->PutINGate((uint64_t)0, output_bitlen, SERVER);
            }
        }
        else {
            for (uint64_t i = 0; i < SHARE_NUM; i++) {
                // Filling with the actual client distance shares
                s_distanceclient[i] = circ->PutINGate(gcstashplusdoramdatapointdistance.at(i), output_bitlen, CLIENT);
                // Filling the server share with the dummy values since it is client role
                s_distancesubstractserver[i] = circ->PutDummyINGate(output_bitlen);
                // Filling with the actual client datapoint label shares
                s_datapointlabelclient[i] = circ->PutINGate(gcstashplusdoramdatapointid.at(i), output_bitlen, CLIENT);
                // Filling with the ids with dummy values since the server have the real values, it is in the same sequence as the shares were sent to the client 
                // ... after the AHE, the uniform shuffle happens later
                s_datapointlabelxorserver[i] = circ->PutDummyINGate(output_bitlen);
            }
            for (uint64_t i = 0; i < TOP_K_NUM; i++) {
                // Here we are just filling the top_k distance shares with dummy values since the assumption was made that the gc calculation will be 
                // ... performed in the server side
                s_out_topkdistance[i] = circ->PutDummyINGate(output_bitlen);
                // Same as above s_topkdistance
                s_out_topkid[i] = circ->PutDummyINGate(output_bitlen);
            }
        }

        // This actually performs the GC calculations with all the input shares and the output shares ;)
        buildSharedDistanceTopKCircuit(s_distanceclient, s_distancesubstractserver, s_datapointlabelclient, s_datapointlabelxorserver, s_out_topkdistance, s_out_topkid, (BooleanCircuit*)circ);

        // Receving the output shares after the GC by the client
        for (uint64_t i = 0; i < TOP_K_NUM; i++) {
            s_out_topkdistance[i] = circ->PutOUTGate(s_out_topkdistance[i], CLIENT);
            s_out_topkid[i] = circ->PutOUTGate(s_out_topkid[i], CLIENT);
        }

        // Executing the circuit
        // dprint("Executing GC Top-K circuit (server/client)");
        party->ExecCircuit();

        if (PRINT_NETWORK_USAGE_INFO) {
            iprint("Stash+Doram Datapoint GC Top-K Info:");
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

// Checking the topk by combining the shares
void gcStashPlusDoramDatapointTopKGetResult(string itemname) {
    // dprint("Getting results of the GC Top-K");
    for (uint64_t i = 0; i < TOP_K_NUM; i++) {
        // Here we are just printing the id and the distance just for the visual purpose, in reality the actual saving of the data takes place after this function call
        iprint("  --- " + itemname + " " + to_string(i) + " ---");
        iprint("    GC Distance : " + to_string(gcoutputdistanceshares.at(i)) + " ID : " + to_string(gcoutputidshares.at(i)));
    }
}

// Initializes the GC as will be done at the server and client side
void gcStashPlusDoramDatapointTopKStart() {
    // dprint("Top-K calculation with GC started");
    e_mt_gen_alg mt_alg = MT_OT;
    seclvl seclv = get_sec_lvl(128);
    GCStashPlusDoramDatapointTopK* gcserver = new GCStashPlusDoramDatapointTopK();
    GCStashPlusDoramDatapointTopK* gcclient = new GCStashPlusDoramDatapointTopK();
    // Ideally this should be done on the seperate systems (client and server)
    // Client does this on its side
    thread threadclient = gcclient->threadGCSharedDistanceTopK(CLIENT, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
    // Server does this on its side
    thread threadserver = gcserver->threadGCSharedDistanceTopK(SERVER, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
    threadclient.join();
    threadserver.join();
}