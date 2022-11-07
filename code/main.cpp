#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "src/bfv/bfv.cpp"

#include "src/utils/config.cpp"
#include "src/utils/filehandler.cpp"
#include "src/utils/globalvarclient.cpp"
#include "src/utils/globalvarserver.cpp"
#include "src/utils/print.cpp"
#include "src/utils/pythonfunc.cpp"
#include "src/utils/timer.cpp"
#include "src/utils/vectors.cpp"
#include "src/utils/convert.cpp"

#include "src/clientside/client.cpp"
#include "src/serverside/server.cpp"

#include "src/gc/doramdatabaseread.cpp"
#include "src/gc/doramreadclusterstopk.cpp"
#include "src/gc/shareddistancetopk.cpp"
#include "src/gc/stashplusdoramdatapointtopk.cpp"
#include "src/gc/kreyviumencrypt.cpp"

#include "src/process/doramclusterpointstopk.cpp"
#include "src/process/doramread.cpp"
#include "src/process/group.cpp"
#include "src/process/stash.cpp"
#include "src/process/stashplusdoramtopk.cpp"

#include "src/doram/initdoram.cpp"

#include "src/kreyvium/kreyviumplain.cpp"

using namespace std;

int main(int argc, char** argv) {
    DEBUGMODE = false;
    DATAMODE = true;

    REDUCE_DATASET = false;
    REDUCE_DATASET_AND_EXIT = false;
    PROCESS_DATASET = false;
    PROCESS_DATASET_AND_EXIT = false;

    pprint(readTextFile("../welcome.txt", false));

    pprint("Usage Example: ./main .");
    pprint("Runs with the original dataset");
    pprint("Usage Example: ./main");
    pprint("Runs with the reduced dataset");
    pprint("");

    if (argc > 1) {
        ORIGINAL_MODE = 1;
        pprint("## Running with Original Dataset ##");
    }
    else {
        REDUCED_MODE = 1;
        pprint("## Running with Reduced Dataset ##");
    }
    pprint("");

    pprint(" ##### NORMALIZATION, MAKING OPTIMIZED CLUSTERS, GROUPS AND STASH "
        "##### ");

    if (REDUCED_MODE) {

        if (REDUCE_DATASET == true) {
            timerStart();
            pprint(" ##### DATASET REDUCTION USING NEIGHBOURHOOD REUSE ##### ");
            pprint(pyNeighbourhoodReuse(CLIENTID, 0.5, 3, 3));
            timerEnd("Dataset Reduction Time: ");
        }
        // IF we want to just reduce the dataset
        if (REDUCE_DATASET_AND_EXIT == true) {
            return 0;
        }

        if (PROCESS_DATASET == true) {
            timerStart();
            // ######## SERVER ########
            // ---- Server normalises the dataset single coloumn ----
            pyNormalizeDatasetColoumn(
                "ratings_reduced.csv", 2,
                NORMALIZATION_MAGNITUDE);
            // ---- Server finds cluster centers, makes the minimum number of clusters
            // required as per the maximum cluster size constrain and give IDS ----
            pprint(pyFindClusterCenterWithIds(
                "../dataset/ratings_reduced_normalized.csv", MAX_CLUSTER_NUM,
                DIMENSION_NUM,
                MAX_CLUSTER_SIZE));
            // ---- Server puts all the nearby clusters in one group ----
            pyGroupClusterCenter("../dataset/clusteroutput/clustercenters.csv",
                MAX_GROUP_NUM, DIMENSION_NUM);
            // ---- Stash preprocessing where we determine if the group and its items
            // qualify to be a group based on minimum group size or do they go to stash
            // ----
            pprint(pyStashPreprocessing(MIN_GROUP_SIZE));
            // ---- Copy the cluster datapoint data to a new ratings.csv file, but now
            // it has the datapoint IDs so we can cross verify the results with the
            // python implemenation ----
            pyMakeDatasetForPythonCheck();

            timerEnd("Preprocessing Time: ");
        }
        // If we want to just process the dataset
        if (PROCESS_DATASET_AND_EXIT == true) {
            return 0;
        }


    }

    if (ORIGINAL_MODE) {

        // Full Dataset
        POLY_MODULUS_DEGREE = 32768;
        MAX_CLUSTER_NUM = 3000;
        MAX_CLUSTER_SIZE = 50;
        MAX_GROUP_NUM = 30;
        MIN_GROUP_SIZE = 150;

        // // 80000
        // POLY_MODULUS_DEGREE = 16384;
        // MAX_CLUSTER_NUM = 3000;
        // MAX_CLUSTER_SIZE = 50;
        // MAX_GROUP_NUM = 30;
        // MIN_GROUP_SIZE = 107;

        // // 60000
        // POLY_MODULUS_DEGREE = 16384;
        // MAX_CLUSTER_NUM = 3000;
        // MAX_CLUSTER_SIZE = 50;
        // MAX_GROUP_NUM = 30;
        // MIN_GROUP_SIZE = 110;

        // 40000
        // POLY_MODULUS_DEGREE = 16384;
        // MAX_CLUSTER_NUM = 3000;
        // MAX_CLUSTER_SIZE = 50;
        // MAX_GROUP_NUM = 30;
        // MIN_GROUP_SIZE = 115;


        if (PROCESS_DATASET == true) {

            timerStart();
            // ######## SERVER ########
            // ---- Server normalises the dataset single coloumn ----
            pyNormalizeDatasetColoumn(
                "ratings.csv", 2,
                NORMALIZATION_MAGNITUDE);
            // ---- Server finds cluster centers, makes the minimum number of clusters
            // required as per the maximum cluster size constrain and give IDS ----
            pprint(pyFindClusterCenterWithIds(
                "../dataset/ratings_normalized.csv", MAX_CLUSTER_NUM,
                DIMENSION_NUM,
                MAX_CLUSTER_SIZE));
            // ---- Server puts all the nearby clusters in one group ----
            pyGroupClusterCenter("../dataset/clusteroutput/clustercenters.csv",
                MAX_GROUP_NUM, DIMENSION_NUM);
            // ---- Stash preprocessing where we determine if the group and its items
            // qualify to be a group based on minimum group size or do they go to stash
            // ----
            pprint(pyStashPreprocessing(MIN_GROUP_SIZE));
            // ---- Copy the cluster datapoint data to a new ratings.csv file, but now
            // it has the datapoint IDs so we can cross verify the results with the
            // python implemenation ----
            pyMakeDatasetForPythonCheck();

            timerEnd("Preprocessing Time: ");
        }
        // If we want to just process the dataset
        if (PROCESS_DATASET_AND_EXIT == true) {
            return 0;
        }


    }

    // ---- Used to measure the overall time (excluding the above preprocessing
    // since it is one time) ----
    auto overall_time_start = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

    // ---- Server parses the stash ----
    stashdataset = serverGeneratesStash();
    // ---- Server parses the stash labels ----
    stashlabels = serverGetsStashLabels();
    // ---- Server parses the clusters points from the groups and sets the
    // GROUP_NUM ----
    allgroupclusterdataset = serverGetsClustersFromGroups();
    // ---- Server parses the clusters labels from the groups ----
    allgroupclusterlabels = serverGetsClustersLabelsFromGroups();
    // ---- Server put the clusters (and its containing points) which qualify to
    // be in the groups in the DORAM set by initializing the DORAM -----
    initDoram();

    pprint(" ##### INITIALIZING AHE ##### ");

    // ######## SERVER + CLIENT (SEPERATELY) ########
    // ---- Server sets the PLAIN_MODULUS according to the BIT_LENGTH which is
    // need to initialize BFV ----
    serverSetsPlainModulusWithBitLength();
    // need to make the kreyvium plain and GC work
    serverSetsKreyviumAdjustedBitLength();
    // ---- Server and Client initialize and share BFV params ----
    initializeBFV();

    // ######## CLIENT ########
    // ---- Client sets the query points ----
    clientSetNormalizedQueryPoint(CLIENTID, 60, 10);

    //  ---- Client enrcypts the query points and sends to server ----
    clientEncryptPoints();

    // ######## SERVER + CLIENT (SEPERATELY) ########
    // ---- After the AHE initialization, for optimisation part in GC, we reduce
    // the bit length ----
    serverReduceBitLengthWithBitsToReduce();
    // ---- After the AHE initialization, for optimisation part in GC, we reduce
    // the plain modulus ----
    serverSetsPlainModulusWithBitLength();

    pprintll(" ##### INITIALIZING AHE ENDED -> STASH PROCESS STARTED ##### ");

    // ######## SERVER + CLIENT (MIX FUNCTIONS) ########
    timerStart();
    // ---- Takes care of the stash part of the process ----
    if (!processStash(100)) {
        return 0;
    }
    timerEnd(" Stash Time Taken: ");

    // IF THERE ARE NO GROUPS; SKIP THE GROUP PROCESS
    if (GROUP_NUM == 0) {
        pprintll(" ##### STASH PROCESS ENDED -> SKIPPING GROUP PROCESS ##### ");

    } else {
        pprintll(" ##### STASH PROCESS ENDED -> GROUP PROCESS STARTED ##### ");

        // ######## SERVER + CLIENT (MIX FUNCTIONS) ########
        timerStart();
        // ---- Takes care of the group part of the process ----
        if (!processGroup(0)) {
            return 0;
        }
        timerEnd(" Group Time Taken: ");

        pprintll(" ##### GROUP PROCESS ENDED -> DORAM READ STARTED ##### ");

        // ######## SERVER + CLIENT (MIX FUNCTIONS) ########
        timerStart();
        // Performs the required GC to retrive the doram dataset secret share of all
        // the datapoints and labels of all the top-k clusters from all the groups
        processDoramRead();
        timerEnd(" Doram Read Time Taken: ");

        pprintll(
            " ##### DORAM READ ENDED -> DORAM CLUSTER POINTS TOP-K STARTED ##### ");

        // ######## SERVER + CLIENT (MIX FUNCTIONS) ########
        timerStart();
        // ---- First performs the distance calculation of all the datapoints from the
        // top-k clusters we got above, then perform top-k of all the datapoints from
        // all
        // ... the clusters with GC ----
        processDoramClusterPointsTopK();
        timerEnd(" Doram Top K Cluster Points Time Taken: ");

        pprintll(" ##### DORAM CLUSTER POINTS TOP-K ENDED -> (DORAM CLUSTER POINTS) "
            "+ (STASH POINTS) TOP-K STARTED ##### ");

        // ######## SERVER + CLIENT (MIX FUNCTIONS) ########
        timerStart();
        // ---- This gives the top-k of the (stash + doram topk datapoints) ----
        processStashPlusDoramTopk();
        timerEnd(" Stash + Doram Top K Time Taken: ");

        pprintlf(" ##### (DORAM CLUSTER POINTS) + (STASH POINTS) TOP-K ENDED ##### ");
    }

    // ---- Measuring overall time ends ----
    auto overall_time_end = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    pprintlf("Overall Time Taken: " +
        to_string(overall_time_end - overall_time_start) + " milliseconds");

    // ######## PERFORMING PYTHON CHECK ########
    pprintlf(" ##### CROSS CHECKING REDUCED DATASET WITH PYTHON ##### ");
    // ---- Here we make the crosschecking reduced dataset and the cross verifiy
    // our results with the simple python top-k implementation which gives out
    // ... the ids of the top-k results ----
    pprintlf(pyFindTopKPythonCheck(3, "../dataset/ratings_reduced.csv"));

    pprintlf(" ##### CROSS CHECKING ORIGINAL DATASET WITH PYTHON ##### ");
    // ---- Here we make the crosschecking original dataset and the cross verifiy
    // our results with the reduced implementation ----
    pprintlf(pyFindTopKPythonCheck(3, "../dataset/ratings.csv"));

    pprintll("Good Bye ;)");
}