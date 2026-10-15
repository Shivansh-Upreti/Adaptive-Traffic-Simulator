#pragma once
#include <iostream>
#include <string>
using namespace std;
#include "ArrayQueue.hpp"

const int NUM_JUNCTIONS = 6;
const int MODERATE_FROM = 3;    // 3 or more vehicles waiting = moderate traffic
const int HEAVY_FROM = 6;       // 6 or more vehicles waiting = heavy traffic
ArrayQueue junctionQ[NUM_JUNCTIONS];


bool validIndex(int j) {
    return j >= 1 && j <= NUM_JUNCTIONS;
}


void vehicleArrive(int j, int vehID) {
    if (!validIndex(j)) {
        cout << "Invalid junction ID\n";
        return;
    }

    junctionQ[j - 1].enqueue(vehID);
}


int vehicleDepart(int j) {
    if (!validIndex(j)) {
        cout << "Invalid junction ID\n";
        return -1;
    }

    if (junctionQ[j - 1].isempty()) {
        cout << "J" << j << " is empty\n";
        return -1;
    }

    return junctionQ[j - 1].dequeue();
}


int queueLength(int j) {
    if (!validIndex(j)) {
        cout << "Invalid junction ID\n";
        return -1;
    }

    return junctionQ[j - 1].size();
}


bool moveVehicle(int from, int to) {
    if (!validIndex(from) || !validIndex(to)) {
        cout << "Invalid junction ID\n";
        return false;
    }

    if (junctionQ[from - 1].isempty()) {
        cout << "No vehicle at J" << from << endl;
        return false;
    }

    if (junctionQ[to - 1].isFull()) {
        cout << "J" << to << " is full, vehicle waits\n";
        return false;
    }

    int v = junctionQ[from - 1].dequeue();
    junctionQ[to - 1].enqueue(v);

    cout << "Vehicle " << v << " moved J"
         << from << " -> J" << to << endl;

    return true;
}


// added in phase 2: same as moveVehicle but for an emergency vehicle.
// it does not wait for the vehicles ahead of it, remove() takes it out
// of the queue from wherever it is standing
bool moveEmergency(int from, int to, int vehID) {
    if (!validIndex(from) || !validIndex(to)) {
        cout << "Invalid junction ID\n";
        return false;
    }

    if (junctionQ[to - 1].isFull()) {
        cout << "J" << to << " is full, vehicle waits\n";
        return false;
    }

    if (!junctionQ[from - 1].remove(vehID)) {
        cout << "Vehicle " << vehID << " not found at J" << from << endl;
        return false;
    }

    junctionQ[to - 1].enqueue(vehID);

    cout << "Emergency vehicle " << vehID << " moved J"
         << from << " -> J" << to << endl;

    return true;
}


// added in phase 2: traffic density of a junction, decided from its queue length
string congestionLevel(int j) {
    if (!validIndex(j)) {
        return "INVALID";
    }

    int len = junctionQ[j - 1].size();
    if (len >= HEAVY_FROM) {
        return "HEAVY";
    }
    if (len >= MODERATE_FROM) {
        return "MODERATE";
    }
    return "LOW";
}


void showDensity() {
    for (int i = 1; i <= NUM_JUNCTIONS; i++) {
        cout << "J" << i << ": " << queueLength(i)
             << " waiting -> " << congestionLevel(i) << endl;
    }
}


void showAllQueues() {
    for (int i = 0; i < NUM_JUNCTIONS; i++) {
        cout << "J" << i + 1
             << " (length " << junctionQ[i].size() << "): ";

        if (junctionQ[i].isempty()) {
            cout << "-";
        } else {
            junctionQ[i].traverse();
        }
        cout << endl;
    }
}