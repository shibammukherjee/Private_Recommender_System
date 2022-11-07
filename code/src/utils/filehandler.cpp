#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <algorithm>

#include "print.cpp"

using namespace std;

// Reads a text file and returns the string
string readTextFile(string path, bool errmsg) {
    string text, data;
    ifstream file(path);

    // dprint("Reading file");
    if (file.is_open()) {
        while (getline(file, text)) {
            data += text + '\n';
        }
        file.close();
    }
    else {
        if (errmsg) {
            eprint("File couldn't be opened::" + path, __FUNCTION__, __LINE__);
        }
        return "error";
    }
    return data;
}

// Parse csv file
// Return vector structure - (num of dimensions, num of elements)
vector<vector<uint64_t>> parseCSVDataset(string str, uint64_t dimensions) {
    // dprint("Parsing CSV Dataset");
    vector<vector<uint64_t>> dataset;
    for (uint64_t i = 0; i < dimensions; i++) {
        dataset.push_back({ vector<uint64_t>{} });
    }
    uint64_t start_index = 0;
    for (uint64_t i = 0; i < str.length(); i++) {
        if (str[i] == '\n') {
            uint64_t line_start_index = start_index;
            string digitstring = "";
            uint64_t comma_number = 0;
            for (uint64_t j = line_start_index; j <= i; j++) {
                if (str[j] == ',' || str[j] == '\n') {
                    dataset.at(comma_number).push_back(stoull(digitstring));
                    comma_number++;
                    digitstring = "";

                    if (comma_number == dimensions) {
                        break;
                    }
                    continue;
                }
                digitstring += str[j];
            }
            start_index = i + 1;
        }
    }
    return dataset;
}

// Parse csv file
vector<uint64_t> parseCSVLabels(string str) {
    // dprint("Parsing CSV Labels");
    vector<uint64_t> labelset;
    for (uint64_t i = 0; i < str.length(); i++) {
        if (str[i] == '\n') {
            string label = "";
            for (uint64_t j = i - 1; j != 0; j--) {
                if (str[j] == ',') {
                    reverse(label.begin(), label.end());
                    labelset.push_back(stoull(label));
                    break;
                }
                label += str[j];
            }
        }
    }

    return labelset;
}

// Write csv single coloumn
void writeCSVStringSingleColoumn(string pathtowrite, vector<uint64_t> data) {
    // dprint("Writing to CSV file");
    ofstream file;
    file.open(pathtowrite);
    for (uint64_t i = 0; i < data.size(); i++) {
        file << data.at(i) << "\n";
    }
    file.close();
}

// Execute python3 file in shell
string execPythonFile(string path) {
    // dprint("Executing Python File");
    array<char, 128> buffer;
    string result;
    string s = "python3 " + path;
    const char* cmd = s.c_str();

    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        eprint("popen() failed", __FUNCTION__, __LINE__);
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result.substr(0, result.length() - 1);
}