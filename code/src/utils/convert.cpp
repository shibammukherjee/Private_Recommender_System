#pragma once

#include <vector>
#include <cstdint>

using namespace std;

// Converts uint64 to a "stream" of uint8 vector
// Used for stream cipher kryvium
vector<uint8_t> uint64_to_uint8_vec(uint64_t in) {
    vector<uint8_t> out;

    out.reserve(8);
    out.push_back(in);
    out.push_back(in >> 8);
    out.push_back(in >> 16);
    out.push_back(in >> 24);
    out.push_back(in >> 32);
    out.push_back(in >> 40);
    out.push_back(in >> 48);
    out.push_back(in >> 56);


    return out;
}

// Converts a "stream" of uint8 vector to uint64
// Used for stream cipher kryvium
uint64_t uint8_vec_to_uint64(vector<uint8_t> in) {

    return (uint64_t(in.at(7)) << 56) |
        (uint64_t(in.at(6)) << 48) |
        (uint64_t(in.at(5)) << 40) |
        (uint64_t(in.at(4)) << 32) |
        (uint64_t(in.at(3)) << 24) |
        (uint64_t(in.at(2)) << 16) |
        (uint64_t(in.at(1)) << 8) |
        (uint64_t(in.at(0)));
}

// Converts uint64 vector to a "stream" of uint8 vector
// Used for stream cipher kryvium
vector<uint8_t> uint64_vec_to_uint8_vec(vector<uint64_t> in) {
    vector<uint8_t> out;

    size_t itemcount = in.size();
    out.reserve(8 * itemcount);

    for (size_t itemnum = 0; itemnum < itemcount; itemnum++) {

        out.push_back(in.at(itemnum));
        out.push_back(in.at(itemnum) >> 8);
        out.push_back(in.at(itemnum) >> 16);
        out.push_back(in.at(itemnum) >> 24);
        out.push_back(in.at(itemnum) >> 32);
        out.push_back(in.at(itemnum) >> 40);
        out.push_back(in.at(itemnum) >> 48);
        out.push_back(in.at(itemnum) >> 56);

    }

    return out;
}

// Converts a "stream" of uint8 vector  to uint64 vector
// Used for stream cipher kryvium
vector<uint64_t> uint8_vec_to_uint64_vec(vector<uint8_t> in) {

    size_t out_size = in.size() / 8;
    vector<uint64_t> out;
    out.reserve(out_size);

    size_t out_idx = 0;
    for (size_t i = 0; i < out_size; i++) {

        out.push_back((uint64_t(in[out_idx + 7]) << 56) |
            (uint64_t(in.at(out_idx + 6)) << 48) |
            (uint64_t(in.at(out_idx + 5)) << 40) |
            (uint64_t(in.at(out_idx + 4)) << 32) |
            (uint64_t(in.at(out_idx + 3)) << 24) |
            (uint64_t(in.at(out_idx + 2)) << 16) |
            (uint64_t(in.at(out_idx + 1)) << 8) |
            (uint64_t(in.at(out_idx))));

        out_idx += 8;
    }

    return out;
}

// Converts uint64 vector vector to a "stream" of uint8 vector
// Used for stream cipher kryvium
vector<uint8_t> uint64_vec_vec_to_uint8_vec(vector<vector<uint64_t>> in) {

    vector<uint8_t> out;
    out.reserve(8 * in.size() * in.at(0).size());

    for (size_t dimensionnum = 0; dimensionnum < in.size(); dimensionnum++) {

        for (size_t itemnum = 0; itemnum < in.at(0).size(); itemnum++) {

            out.push_back(in.at(dimensionnum).at(itemnum));
            out.push_back(in.at(dimensionnum).at(itemnum) >> 8);
            out.push_back(in.at(dimensionnum).at(itemnum) >> 16);
            out.push_back(in.at(dimensionnum).at(itemnum) >> 24);
            out.push_back(in.at(dimensionnum).at(itemnum) >> 32);
            out.push_back(in.at(dimensionnum).at(itemnum) >> 40);
            out.push_back(in.at(dimensionnum).at(itemnum) >> 48);
            out.push_back(in.at(dimensionnum).at(itemnum) >> 56);

        }

    }

    return out;
}

// Converts a "stream" of uint8 vectors  to uint64 vectors
// Used for stream cipher kryvium
vector<vector<uint64_t>> uint8_vec_to_uint64_vec_vec(vector<uint8_t> in, uint64_t firstdimsize, uint64_t seconddimsize) {

    vector<vector<uint64_t>> out;
    out.reserve(firstdimsize);
    for (size_t i = 0; i < firstdimsize; i++) {
        out.push_back({ vector<uint64_t>{} });
        out.at(i).reserve(seconddimsize);
    }

    size_t out_idx = 0;
    for (size_t dimensionnum = 0; dimensionnum < firstdimsize; dimensionnum++) {
        for (size_t itemnum = 0; itemnum < seconddimsize; itemnum++) {

            out.at(dimensionnum).push_back((uint64_t(in.at(out_idx + 7)) << 56) |
                (uint64_t(in.at(out_idx + 6)) << 48) |
                (uint64_t(in.at(out_idx + 5)) << 40) |
                (uint64_t(in.at(out_idx + 4)) << 32) |
                (uint64_t(in.at(out_idx + 3)) << 24) |
                (uint64_t(in.at(out_idx + 2)) << 16) |
                (uint64_t(in.at(out_idx + 1)) << 8) |
                (uint64_t(in.at(out_idx))));

            out_idx += 8;

        }
    }

    return out;
}

// 2048 -> 00000000 00100000 00000000 00000000 00000000 00000000 00000000 00000000
vector<bool> uint8_vec_to_bool(const std::vector<uint8_t>& in) {
    vector<bool> k(in.size() * 8);
    for (size_t i = 0; i < in.size() * 8; i++) {
        if (i / 8 >= in.size()) {
            return k;
        }
        k[i] = ((in[i / 8] >> (i % 8)) & 1);
    }
    return k;
}