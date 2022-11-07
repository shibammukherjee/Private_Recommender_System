#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "security.cpp"

#include "../utils/globalvarserver.cpp"

using namespace std;

// A simple check if a partiular value exists in a vector
bool checkValueExistinVector(vector<uint64_t> vec, uint64_t value) {
    // dprint("Checking if value exists in vector");
    if (find(vec.begin(), vec.end(), value) != vec.end()) {
        return true;    // value exists
    }
    return false;
}

// Multiplies the values of the vector coloumn by the given magnitude
void normalizeVectorSingleColoumn(vector<uint64_t>& vec, uint64_t coloumnindex, int normalization_magnitute) {
    // dprint("Normalizing vector");
    vec.at(coloumnindex) = vec.at(coloumnindex) * normalization_magnitute;
}

// Permutes the vector randomly with seed taken from getRandomData()
void permuteDatasetandLabels(vector<vector<uint64_t>>& d, vector<uint64_t>& l) {
    // dprint("Permuting dataset points and labels");

    uint64_t datasetrowsize = d.at(0).size();
    uint64_t dimensionsize = d.size();

    vector<uint64_t> index;
    index.reserve(datasetrowsize);

    vector<uint64_t> labels_temp;
    labels_temp.reserve(datasetrowsize);
    // Fill the random index with any coloumn number (say, 0)
    for (uint64_t i = 0; i < datasetrowsize; i++) {
        while (true) {
            uint64_t random = getRandom32WithModulo(datasetrowsize);
            if (!checkValueExistinVector(index, random)) {
                index.push_back(random);
                break;
            }
        }
        labels_temp.push_back(l.at(index.at(i)));
    }

    // Initialize dataset_temp and labels_temp
    vector<vector<uint64_t>> dataset_temp;
    dataset_temp.reserve(dimensionsize);
    for (uint64_t i = 0; i < dimensionsize; i++) {
        dataset_temp.push_back({ vector<uint64_t>{} });
        dataset_temp.at(i).reserve(index.size());
        // Fill the temp dataset
        for (uint64_t j = 0; j < index.size(); j++) {
            dataset_temp.at(i).push_back(d.at(i).at(index.at(j)));
        }
    }

    d = dataset_temp;
    l = labels_temp;
}

// Flattens a 3 dimension vector to 1 dimension 
vector<uint64_t> flattenVector(vector<vector<vector<uint64_t>>> v) {
    vector<uint64_t> fv;
    for (uint64_t i = 0; i < v.size(); i++) {
        for (uint64_t j = 0; j < v.at(i).size(); j++) {
            for (uint64_t k = 0; k < v.at(i).at(j).size(); k++) {
                fv.push_back(v.at(i).at(j).at(k));
            }
        }
    }
    return fv;
}

// Clears the old vector data, here it has been used when we are reusing the variables and the functions for the stash and the group process
void clearOldResultVectors() {
    // dprint("Clearing old vectors");

    // serverside
    randomservershareplusdatapointdimensions.clear();
    id.clear();
    uniformrandomshuffleindexlist.clear();
    stashrandomidxor.clear();
    stashrandomdistancesubstract.clear();
    stashdataset.clear();
    stashlabels.clear();
    cipherdistanceshare.clear();

    // clientside
    vectordistance.clear();
    gcoutputdistanceshares.clear();
    gcoutputidshares.clear();
}