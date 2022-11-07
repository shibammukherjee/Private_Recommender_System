#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "security.cpp"

#include "../utils/globalvarserver.cpp"

using namespace std;

time_t timestart;
time_t timeend;

time_t timestartsub;
time_t timeendsub;

// Start the timer
void timerStart() {
    // dprint("Starting timer");
    time(&timestart);
}

// End the timer
void timerEnd(string title) {
    // dprint("Stopping timer");
    time(&timeend);
    pprintlf(title + to_string(timeend - timestart) + " seconds");
}

// Start the timer
void timerStartSub() {
    // dprint("Starting timer");
    time(&timestartsub);
}

// End the timer
void timerEndSub(string title) {
    // dprint("Stopping timer");
    time(&timeendsub);
    pprintlf(title + to_string(timeendsub - timestartsub) + " seconds");
}