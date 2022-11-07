#pragma once
#include <iostream>
#include <vector>
#include <string>

using namespace std;


bool DEBUGMODE = true;
bool DATAMODE = true;
bool ERRORMODE = true;
bool WARNINGMODE = true;

// normal print
void pprint(string str) {
    cout << str << endl;
}
void pprint(string str, int i) {
    cout << str << " " << to_string(i) << endl;
}
void pprint(int i) {
    cout << i << endl;
}
void pprint(uint64_t i) {
    cout << i << endl;
}
// print with 1 line in front and 2 in the back
void pprintll(string str) {
    cout << endl << str << endl << endl;
}
// print with extra line in the beginnig
void pprintlf(string str) {
    cout << endl << str << endl;
}
// Print with an extra line in the end
void pprintlb(string str) {
    cout << str << endl << endl;
}
// Print without new line, just a single space
void pprintwnl(string str) {
    cout << str;
}

// print error
void eprint(string msg, string func, int line) {
    if (ERRORMODE) { cout << "ERROR: " << msg << "::" << func << "::" << to_string(line) << endl; }
}
// print warning
void wprint(string str) {
    if (WARNINGMODE) { cout << "WARNING: " << str << endl; }
}
// // print debug
void dprint(string str) {
    if (DEBUGMODE) { cout << "DEBUG: " << str << endl; }
}

// print data (i makes no sense I KNOW (maybe information :D), but d is for debug)
void iprint(string str) {
    if (DATAMODE) { cout << str << endl; }
}
void iprintlf(string str) {
    cout << endl << str << endl;
}
void iprint(string str, int i) {
    if (DATAMODE) { cout << str << " " << to_string(i) << endl; }
}
void iprint(int i) {
    if (DATAMODE) { cout << i << endl; }
}
void iprint(uint64_t i) {
    if (DATAMODE) { cout << i << endl; }
}

// Set DEBUGMODE
void setDebugMode(bool b) {
    DEBUGMODE = b;
}
// Set ERRORMODE
void setErrorMode(bool b) {
    ERRORMODE = b;
}
// Ser WARNINGMODE
void setWarningMode(bool b) {
    WARNINGMODE = b;
}