#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "globalvarserver.cpp"
#include "globalvarclient.cpp"

using namespace std;

string pyNeighbourhoodReuse(uint64_t targetuserid, float lambda, int coloumnrange, int k)
{
    // dprint("Python neighbourhood reuse");
    string path = "../src/python/neighbourhoodreuse.py ../dataset/ratings.csv " + to_string(coloumnrange) + " " +
       to_string(targetuserid) + " " + to_string(lambda) + " " + to_string(k);
    // Use for the DP dataset
    // string path = "../src/python/neighbourhoodreuse_with_dp.py ../dataset/ratings.csv " + to_string(coloumnrange) + " " +
    //     to_string(targetuserid) + " " + to_string(0) + " " + to_string(k);
    return execPythonFile(path);
}

// Takes the data points and makes the minimum number of clusters required to acheive the maximum cluster size goal
string pyFindClusterCenterWithIds(string datasetpath, int clustercount, int coloumnrange, int maximumclustersize)
{
    // dprint("Python finding cluster centers");
    string path = "../src/python/findclustercenterswithids.py " + datasetpath + " " + to_string(clustercount) + " " + to_string(coloumnrange) + " " + to_string(maximumclustersize);
    return execPythonFile(path);
}

// Performs grouping of the nearby clusters in one group
string pyGroupClusterCenter(string datasetpath, int groupcount, int coloumnrange)
{
    // dprint("Python grouping the cluster centers");
    string path = "../src/python/groupclustercenters.py " + datasetpath + " " + to_string(groupcount) + " " + to_string(coloumnrange);
    return execPythonFile(path);
}

// Seperates the groups which qualifies to go to stash from the rest based on the MIN_GROUP_SIZE parameter
string pyStashPreprocessing(int mingroupsize)
{
    // dprint("Python performing stash preprocessing");
    string path = "../src/python/stashpreprocessing.py " + to_string(mingroupsize);
    return execPythonFile(path);
}

// Removes the decimals by multiplying with a magnitude
string pyNormalizeDatasetColoumn(string datasetpath, int coloumnindex, int magnitude)
{
    // dprint("Python normalizing dataset " + datasetpath);
    string path = "../src/python/normalizedatasetcoloumn.py ../dataset/" + datasetpath + " " + to_string(coloumnindex) + " " + to_string(magnitude);
    return execPythonFile(path);
}

// Makes a copy of the dataset with the IDs from the generated clusters
string pyMakeDatasetForPythonCheck()
{
    // dprint("Python copying all the datapoints from all the clusters and saving them into a single file");
    string path = "../src/python/crosscheckcopyclusterdata.py ../dataset/clusteroutput/clusters ../dataset/pythoncheck/ratings.csv";
    return execPythonFile(path);
}

// Make the cross check dataset and find the topk with python reference
string pyFindTopKPythonCheck(uint32_t ndimension, string datasetdirpath)
{
    // dprint("Python cross check for top-k");
    string path = "../src/python/crosscheckfindtopk.py " + to_string(ndimension) + " " + datasetdirpath + " ../dataset/pythoncheck/ratings.csv " + to_string(userpoint.at(0)) + " " + to_string(userpoint.at(1)) + " " + to_string(((float)userpoint.at(2) / 2)) + " " + to_string(TOP_K_NUM);
    return execPythonFile(path);
}

// NOT IN USE NOW
// Performs the Oblivious transfer to retrieve the labels of the data points
string pyOT(string labelfilepath, uint64_t choice, vector<uint64_t> labels)
{
    // dprint("Python 1 to N Oblivious Transfer");
    string path = "../src/ot/ot1n.py " + to_string(labels.size()) + " " + to_string(choice) + " " + labelfilepath;

    string result = execPythonFile(path);
    pprint("GC ID : " + to_string(choice) + ", OT label : " + result);
    return result;
}