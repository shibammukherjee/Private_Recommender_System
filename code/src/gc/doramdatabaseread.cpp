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

class GCDoramRead {
protected:
private:
public:
    // Contains the cluster id share of the client for the top k clusters
    share*** s_doramclusteridclientshare;
    // Contains the cluster id of all the clusters on the server
    share*** s_doramgroupclusterid;
    // Contains the client cluster index share. This is the same index share which the client sends to the GC to combine with the server cluster 
    // index share. We send back the same client index share such that the client can identify which encrypted client cluster share can be 
    // decrypted using the particular cipher key.
    share*** s_out_clusteridclientshare;


    void initializeShareVariables(e_role role) {
        // if (role == SERVER) {
        //     // dprint("SERVER: Initializing the global share variables");
        // }
        // else {
        //     // dprint("CLIENT: Initializing the global share variables");
        // }

        s_doramclusteridclientshare = new share * *[GROUP_NUM];
        s_doramgroupclusterid = new share * *[GROUP_NUM];
        s_out_clusteridclientshare = new share * *[GROUP_NUM];


        // Initializing the variables with GROUP_NUM each pointer by pointer ':)
        for (uint64_t i = 0; i < GROUP_NUM; i++) {
            s_doramclusteridclientshare[i] = new share * [TOP_K_NUM];
            s_out_clusteridclientshare[i] = new share * [TOP_K_NUM];
            s_doramgroupclusterid[i] = new share * [MAXIMUM_DORAM_CLUSTER_NUM];
        }
    }

    // GC read implementation
    void buildDoramReadCircuit(BooleanCircuit* bc, e_role role) {
        // if (role == SERVER) {
        //     // dprint("SERVER: Building DORAM read GC circuit");
        // }
        // else {
        //     // dprint("CLIENT: Building DORAM read GC circuit");
        // }

        // IMPORTANT: THE client will know only the dimensions of the dataset (constant group and cluster number), it will not know the actual dataset
        // This part deals with iterating through the datasets and labels to look for the matching id, when a matching id is found the key and the client 
        // index share is assigned to s_out_indcpaclusterdecryptkeys and s_out_clusterindexclientshare respectively.

        // CHecking if the random ids match, if they do, set them as outputs
        for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
            for (uint64_t doramclusternum = 0; doramclusternum < MAXIMUM_DORAM_CLUSTER_NUM; doramclusternum++) {
                for (uint64_t clusternum = 0; clusternum < TOP_K_NUM; clusternum++) {
                    // Checking the cluster id with the combined share cluster id if they match, 1 index-> cluster id 
                    share* clusternumeqout = bc->PutEQGate(s_doramclusteridclientshare[groupnum][clusternum], s_doramgroupclusterid[groupnum][doramclusternum]);

                    s_out_clusteridclientshare[groupnum][clusternum] = bc->PutMUXGate(s_out_clusteridclientshare[groupnum][clusternum],
                        s_doramgroupclusterid[groupnum][doramclusternum], bc->PutINVGate(clusternumeqout));
                }
            }
        }

        // Outputs the client cluster index share and the decrption key to the client
        for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
            for (uint64_t clusternum = 0; clusternum < TOP_K_NUM; clusternum++) {
                s_out_clusteridclientshare[groupnum][clusternum] = bc->PutOUTGate(s_out_clusteridclientshare[groupnum][clusternum], CLIENT);
            }
        }
    }

    // Here we take the real data from the server and client and convert it to shares (done seperately on each side)
    void settingServerAndClientShares(e_role role, Circuit* circ, uint64_t output_bitlen) {
        // if (role == SERVER) {
        //     // dprint("SERVER: Setting its shares (this will be done seperately on each end)");
        // }
        // else {
        //     // dprint("CLIENT: Setting its shares (this will be done seperately on each end)");
        // }

        // PRIVACY PROBLEM? - Here we are looping through the groups and then looping through the clusters of each group. Now one might think
        // why not hide the information regarding how many clusters are there in a group? by simply make one large cluster vector containing all the clusters 
        // for every group and tell the client just the number of clusters that are present? Actually this is not a problem because we are sending a constant
        // TOP-K number of clusters per group, thus leaks no information about the cluster number in each group ;)

        // The server part
        if (role == SERVER) {
            // Putting the server share for the id share
            // Setting the group and its cluster indices in the doram group-cluster index shares, this will be used to check for the matching cluster index 
            // ... in the GC, where the client will get all the items only from the top-k clusters of the doramdatabase that the server has without the 
            // ... server knowing anything about the clusters that were retreived
            for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {

                for (uint64_t clusternum = 0; clusternum < MAXIMUM_DORAM_CLUSTER_NUM; clusternum++) {
                    // Putting the group cluster ids of all the clusters
                    s_doramgroupclusterid[groupnum][clusternum] = circ->PutINGate(doramgroupclusterid.at(groupnum).at(clusternum), output_bitlen, SERVER);
                }

                for (uint64_t clusternum = 0; clusternum < TOP_K_NUM; clusternum++) {
                    // Putting dummy
                    s_doramclusteridclientshare[groupnum][clusternum] = circ->PutDummyINGate(output_bitlen);
                    // Initializing the client cluster share index with some initial value like 0, that will contain the top-k cluster 
                    // client share index which will be sent to the client after GC
                    s_out_clusteridclientshare[groupnum][clusternum] = circ->PutINGate((uint64_t)0, output_bitlen, SERVER);
                }
            }
        }
        // The client part
        else if (role == CLIENT) {
            // Putting the client share for the id share
            for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
                for (uint64_t clusternum = 0; clusternum < MAXIMUM_DORAM_CLUSTER_NUM; clusternum++) {
                    // Putting dummy
                    s_doramgroupclusterid[groupnum][clusternum] = circ->PutDummyINGate(output_bitlen);
                }
                // Putting the group and cluster indices (0,1,2,3..) so we can retriev the correct dataset array in the GC when the group and cluster 
                // indices from each client and server sides are combined, 0 -> groupid, 1-> clusterid
                for (uint64_t clusternum = 0; clusternum < TOP_K_NUM; clusternum++) {
                    // Putting the client share of the GC cluster id
                    s_doramclusteridclientshare[groupnum][clusternum] = circ->PutINGate(gctopkclusteridshare.at(groupnum).at(clusternum), output_bitlen, CLIENT);
                    // Putting dummy values
                    s_out_clusteridclientshare[groupnum][clusternum] = circ->PutDummyINGate(output_bitlen);
                }
            }
        }
    }

    void gcDoramRead(e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
        // if (role == SERVER) {
        //     // dprint("SERVER: Initializing GC Database masking");
        // }
        // else {
        //     // dprint("CLIENT: Initializing GC Database masking");
        // }

        // The usual variables for the GC config
        ABYParty* party = new ABYParty(role, address, port, seclv, output_bitlen, nthreads, mt_alg);
        std::vector<Sharing*>& sharings = party->GetSharings();
        Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

        // Initializing the global share variables
        initializeShareVariables(role);

        // Server and client setting their shares (this will be done seperately on each end)
        settingServerAndClientShares(role, circ, output_bitlen);

        // Building DORAM read GC circuit (server/client)
        buildDoramReadCircuit((BooleanCircuit*)circ, role);

        // dprint("Executing GC database masking circuit (server/client)");
        party->ExecCircuit();
        // dprint("Execute done");

        if (PRINT_NETWORK_USAGE_INFO) {
            iprint("Doram Read GC Info:");
            iprint("Total Depth - " + to_string(party->GetTotalDepth()));
            iprint("Total Gates - " + to_string(party->GetTotalGates()));
            iprint("Total Bytes Sent - " + to_string(party->GetSentData(P_TOTAL)));
            iprint("Total Bytes Received - " + to_string(party->GetReceivedData(P_TOTAL)));
            iprint("Timing - " + to_string(party->GetTiming(P_TOTAL)));
        }

        // The client get the cipher key and client cluster index share as the output
        // The client will not have any access over the doramdataset or the doramlabels as accessed in the for-loop, but will only know the 
        // size of each group, its clusters (which is constant) such that it can build an appropriate vector :D
        if (role == CLIENT) {
            // Converting the doramdataset shares to doramdataset
            clientclusteridshare.reserve(GROUP_NUM);
            for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {

                clientclusteridshare.push_back({ vector<uint64_t>{} });
                clientclusteridshare.at(groupnum).reserve(TOP_K_NUM);

                for (uint64_t clusterno = 0; clusterno < TOP_K_NUM; clusterno++) {
                    clientclusteridshare.at(groupnum).push_back(s_out_clusteridclientshare[groupnum][clusterno]->get_clear_value<uint64_t>());
                }
            }
        }
        // dprint("Client received masked dataset and datalabels");
    }

    std::thread threadGCDoramDatabaseRead(e_role role, const std::string& address, uint16_t port, seclvl seclv, uint64_t output_bitlen, uint64_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing) {
        return std::thread([=] { gcDoramRead(role, address, port, seclv, output_bitlen, nthreads, mt_alg, sharing); });
    }

};

// Initializes the GC as will be done at the server and client side
void gcDoramReadStart() {
    // dprint("Cluster kreyvium encryption key retrival with GC started");
    e_mt_gen_alg mt_alg = MT_OT;
    seclvl seclv = get_sec_lvl(128);
    GCDoramRead* gcserver = new GCDoramRead();
    GCDoramRead* gcclient = new GCDoramRead();
    thread threadclient = gcclient->threadGCDoramDatabaseRead(CLIENT, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
    thread threadserver = gcserver->threadGCDoramDatabaseRead(SERVER, "127.0.0.1", 8080, seclv, BIT_LENGTH, GC_THREADS, mt_alg, S_BOOL);
    threadclient.join();
    threadserver.join();
}

// Here we emulate that the server minus the clusters with its random share and encrypts the client shares
void encryptClientClusterShare() {
    // dprint("Server encrypting the clusters with Kreyvium");

    doramreadrandomshare.reserve(GROUP_NUM);
    seed.reserve(GROUP_NUM);

    for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
        doramreadrandomshare.push_back({ vector<vector<uint64_t>>{} });
        seed.push_back({ vector<uint64_t>{} });

        doramreadrandomshare.at(groupnum).reserve(MAXIMUM_DORAM_CLUSTER_NUM);
        seed.at(groupnum).reserve(MAXIMUM_DORAM_CLUSTER_NUM);

        for (uint64_t clusternum = 0; clusternum < MAXIMUM_DORAM_CLUSTER_NUM; clusternum++) {
            doramreadrandomshare.at(groupnum).push_back({ vector<uint64_t>{} });
            seed.at(groupnum).push_back(getRandomBitLengthSize());

            // Generating random kreyvium stream of size MAXIMUM_DORAM_CLUSTER_SIZE*64 bits by using the random seed as key
            doramreadrandomshare.at(groupnum).at(clusternum) = serverGeneratesDoramClusterRandomSharesFromSeed(MAXIMUM_DORAM_CLUSTER_SIZE,
                seed.at(groupnum).at(clusternum));

            for (uint64_t itemnum = 0; itemnum < MAXIMUM_DORAM_CLUSTER_SIZE; itemnum++) {

                // secret sharing using the output generated by the kreyvium key stream
                doramshufflelabels.at(groupnum).at(clusternum).at(itemnum) =
                    mathModuloSubtract(doramshufflelabels.at(groupnum).at(clusternum).at(itemnum),
                        doramreadrandomshare.at(groupnum).at(clusternum).at(itemnum), PLAIN_MODULUS);

                for (uint64_t dimension = 0; dimension < DIMENSION_NUM; dimension++) {
                    // secret sharing using the output generated by the kreyvium key stream
                    doramshuffledataset.at(groupnum).at(clusternum).at(dimension).at(itemnum) =
                        mathModuloSubtract(doramshuffledataset.at(groupnum).at(clusternum).at(dimension).at(itemnum),
                            doramreadrandomshare.at(groupnum).at(clusternum).at(itemnum), PLAIN_MODULUS);
                }
            }

        }
    }
    // Putting the seed sharing constant which is used to secret share the seed to the client (server master key)
    seedshareconst = getRandomBitLengthSize();
}

// The server sends all the encrypted shared data to the client
void sendSharesToClient() {

    // emulating sending of encrypted cluster dataset, labels and secret shared cluster seeds to the client
    clientgroupclusterid = doramgroupclusterid;
    clientclustershare = doramshuffledataset;
    clientclusterlabelshare = doramshufflelabels;

    if (PRINT_NETWORK_USAGE_INFO) {
        iprint("PIR info:");
        iprint("Server -> Client PIR Initialization - " + to_string(clientgroupclusterid.size() * clientgroupclusterid.at(0).size() * 8) + " Bytes");
        iprint("Server -> Client PIR Initialization - " + to_string(clientclustershare.size() * clientclustershare.at(0).size() *
            clientclustershare.at(0).at(0).size() * clientclustershare.at(0).at(0).at(0).size() * 8) + " Bytes");
        iprint("Server -> Client PIR Initialization - " + to_string(clientclusterlabelshare.size() * clientclusterlabelshare.at(0).size() *
            clientclusterlabelshare.at(0).at(0).size() * 8) + " Bytes");
    }

    clientseedshares.reserve(GROUP_NUM);

    for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
        clientseedshares.push_back({ vector<uint64_t>{} });
        clientseedshares.at(groupnum).reserve(doramshuffledataset.at(groupnum).size());

        for (uint64_t clusternum = 0; clusternum < doramshuffledataset.at(groupnum).size(); clusternum++) {
            // Adding the seed share const to the seeds that are sent to the client
            clientseedshares.at(groupnum).push_back(seed.at(groupnum).at(clusternum) ^ seedshareconst);
        }
    }

    if (PRINT_NETWORK_USAGE_INFO) {
        iprint("Server -> Client PIR Initialization - " + to_string(clientseedshares.size() * clientseedshares.at(0).size() * 8) + " Bytes");
    }
}

// Here the client checks if the client cluster id share is same as the cluster id share in clientclusteridshareindcpakeypair
// and then use the correct ind-cpa key to decrypt the cluster to get the client cluster share
void chooseTopClientClusterShares() {
    // dprint("Client after GC finding the correct key and decrypting the clusters with IND-CPA");

    uint64_t maskedclusteridx = 0;

    maskedclientdoramdataset.reserve(GROUP_NUM);
    clientseedtopkshares.reserve(GROUP_NUM);
    maskedclientdoramlabels.reserve(GROUP_NUM);

    for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
        maskedclientdoramdataset.push_back({ vector<vector<vector<uint64_t>>>{} });
        clientseedtopkshares.push_back({ vector<uint64_t>{} });
        maskedclientdoramlabels.push_back({ vector<vector<uint64_t>>{} });

        maskedclientdoramdataset.at(groupnum).reserve(TOP_K_NUM);
        clientseedtopkshares.at(groupnum).reserve(TOP_K_NUM);
        maskedclientdoramlabels.at(groupnum).reserve(TOP_K_NUM);

        for (uint64_t clusternum = 0; clusternum < clientclustershare.at(groupnum).size(); clusternum++) {

            // Matching the client id share and use the corresponding key to decrypt
            for (uint64_t index = 0; index < clientclusteridshare.at(groupnum).size(); index++) {

                if (clientgroupclusterid.at(groupnum).at(clusternum) == clientclusteridshare.at(groupnum).at(index)) {

                    clientseedtopkshares.at(groupnum).push_back(clientseedshares.at(groupnum).at(clusternum));

                    // adding a cluster
                    maskedclientdoramdataset.at(groupnum).push_back({ vector<vector<uint64_t>>{} });
                    maskedclientdoramlabels.at(groupnum).push_back({ vector<uint64_t>{} });

                    maskedclientdoramdataset.at(groupnum).at(maskedclusteridx) = clientclustershare.at(groupnum).at(clusternum);
                    maskedclientdoramlabels.at(groupnum).at(maskedclusteridx) = clientclusterlabelshare.at(groupnum).at(clusternum);

                    maskedclusteridx++;

                    break;
                }
            }
        }

        maskedclusteridx = 0;
    }
}

// Checking the server and the client doramread top k cluster ids
void gcDoramReadCheckTopkClusterIDs() {
    // dprint("Checking results of the GC TOP-K cluster ID retrival");
    iprint("--- CHECKING GC TOP-K CLUSTER ID RESULTS ---");
    for (uint64_t groupnum = 0; groupnum < GROUP_NUM; groupnum++) {
        iprint(" --- GROUP " + to_string(groupnum) + " ---");
        for (uint64_t clusterno = 0; clusterno < TOP_K_NUM; clusterno++) {
            iprint("   --- CLUSTER " + to_string(clusterno) + " ---");
            if (clusterno == 3) {
                iprint("    .");
                iprint("    .");
                break;
            }
            string s = "    ID : " + to_string(clientclusteridshare.at(groupnum).at(clusterno));
            iprint(s);
        }
    }
}
