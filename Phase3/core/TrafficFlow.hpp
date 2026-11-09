#pragma once
#include <iostream>
#include <string>
using namespace std;
#include "ArrayQueue.hpp"
#include "TrafficSignal.hpp"

const int NUM_JUNCTIONS = 6;
const int MODERATE_FROM = 3;    // 3 or more vehicles waiting = moderate traffic
const int HEAVY_FROM = 6;       // 6 or more vehicles waiting = heavy traffic

// junction numbers are 1..6
inline bool validIndex(int j) {
    return j >= 1 && j <= NUM_JUNCTIONS;
}

// TrafficFlow = the lanes and signals of all junctions.
// phase 2 kept them in GLOBAL arrays. That meant two Simulators would have shared
// (and wrecked) the same lanes. Now the arrays are private members, so every
// object has its own lanes. The function names are the same as in phase 2.
//
// Every function that takes a junction number checks it first (validIndex), so a bad
// number like 0 or 7 can never reach an array. The caller gets false / -1 / 0 instead.

class TrafficFlow {
    ArrayQueue mainLane[NUM_JUNCTIONS];             // the main lane of every junction (index = junction - 1)
    ArrayQueue sideLane[NUM_JUNCTIONS];             // phase 3: the pull-over lane of every junction
    TrafficSignal signal[NUM_JUNCTIONS];            // phase 3: the signal of every junction

public:
    // phase 3: returns true only if the vehicle really joined the lane
    // (phase 2 returned nothing, so a full lane lost the vehicle without anyone noticing)
    bool vehicleArrive(int j, int vehID) {
        if (!validIndex(j)) {
            cout << "Invalid junction ID\n";
            return false;
        }

        if (mainLane[j - 1].isFull()) {
            cout << "J" << j << " lane is full\n";
            return false;
        }

        mainLane[j - 1].enqueue(vehID);
        return true;
    }


    int vehicleDepart(int j) {
        if (!validIndex(j)) {
            cout << "Invalid junction ID\n";
            return -1;
        }

        if (mainLane[j - 1].isempty()) {
            cout << "J" << j << " is empty\n";
            return -1;
        }

        return mainLane[j - 1].dequeue();
    }


    int queueLength(int j) {
        if (!validIndex(j)) {
            cout << "Invalid junction ID\n";
            return -1;
        }

        return mainLane[j - 1].size();
    }


    bool moveVehicle(int from, int to) {
        if (!validIndex(from) || !validIndex(to)) {
            cout << "Invalid junction ID\n";
            return false;
        }

        if (mainLane[from - 1].isempty()) {
            cout << "No vehicle at J" << from << endl;
            return false;
        }

        if (mainLane[to - 1].isFull()) {
            cout << "J" << to << " is full, vehicle waits\n";
            return false;
        }

        int v = mainLane[from - 1].dequeue();
        mainLane[to - 1].enqueue(v);

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

        if (mainLane[to - 1].isFull()) {
            cout << "J" << to << " is full, vehicle waits\n";
            return false;
        }

        if (!mainLane[from - 1].remove(vehID)) {
            cout << "Vehicle " << vehID << " not found at J" << from << endl;
            return false;
        }

        mainLane[to - 1].enqueue(vehID);

        cout << "Emergency vehicle " << vehID << " moved J"
             << from << " -> J" << to << endl;

        return true;
    }


    // added in phase 2: traffic density of a junction, decided from its queue length
    string congestionLevel(int j) {
        if (!validIndex(j)) {
            return "INVALID";
        }

        int len = mainLane[j - 1].size();
        if (len >= HEAVY_FROM) {
            return "HEAVY";
        }
        if (len >= MODERATE_FROM) {
            return "MODERATE";
        }
        return "LOW";
    }


    // phase 3: reading the lanes (silent: no printing)

    int laneLength(int j) {
        if (!validIndex(j)) return 0;
        return mainLane[j - 1].size();
    }

    // i-th vehicle of the main lane counted from the front (0 = front), -1 if there is none
    int laneAt(int j, int i) {
        if (!validIndex(j)) return -1;
        return mainLane[j - 1].at(i);
    }

    bool laneContains(int j, int vehID) {
        if (!validIndex(j)) return false;
        return mainLane[j - 1].contains(vehID);
    }

    bool laneFull(int j) {
        if (!validIndex(j)) return false;
        return mainLane[j - 1].isFull();
    }

    // takes a vehicle out of the main lane from wherever it stands. false if it is not there
    bool laneRemove(int j, int vehID) {
        if (!validIndex(j)) return false;
        return mainLane[j - 1].remove(vehID);
    }

    int sideLength(int j) {
        if (!validIndex(j)) return 0;
        return sideLane[j - 1].size();
    }

    int sideAt(int j, int i) {
        if (!validIndex(j)) return -1;
        return sideLane[j - 1].at(i);
    }


    // phase 3: signals

    void preempt(int j) {
        if (validIndex(j)) {
            signal[j - 1].preempt();
        }
    }

    // false if there was no hold to give back (or the junction does not exist)
    bool release(int j) {
        if (!validIndex(j)) return false;
        return signal[j - 1].release();
    }

    bool isPreempted(int j) {
        if (!validIndex(j)) return false;
        return signal[j - 1].isPreempted();
    }

    int holdCount(int j) {
        if (!validIndex(j)) return 0;
        return signal[j - 1].getHolds();
    }

    const char* signalName(int j) {
        if (!validIndex(j)) return "INVALID";
        return signal[j - 1].getName();
    }


    // phase 3: two-lane evacuation

    // how many vehicles wait in the main lane of junction j, not counting vehicle skipID
    // (the emergency vehicle must not count itself as "traffic")
    int waitingExcept(int j, int skipID) {
        if (!validIndex(j)) {
            return 0;
        }
        int count = mainLane[j - 1].size();
        if (mainLane[j - 1].contains(skipID)) {
            count--;
        }
        return count;
    }

    // every vehicle except keepID pulls over from the main lane into the side lane.
    // they are taken out from the front one by one, so they keep their order in the side lane.
    // the check comes FIRST: if the side lane cannot hold them all, nothing is moved at all
    bool evacuateToSide(int j, int keepID) {
        if (!validIndex(j)) {
            return false;
        }
        ArrayQueue &lane = mainLane[j - 1];
        ArrayQueue &side = sideLane[j - 1];

        int total = lane.size();
        int toMove = waitingExcept(j, keepID);
        if (side.size() + toMove > QUEUE_CAPACITY) {
            return false;
        }

        bool keepFound = false;
        for (int i = 0; i < total; i++) {
            int id = lane.dequeue();
            if (id == keepID) {
                keepFound = true;       // the emergency vehicle stays on the road
            } else {
                side.enqueue(id);       // everyone else pulls over
            }
        }
        if (keepFound) {
            lane.enqueue(keepID);       // it is the only vehicle left, so it is at the front
        }
        return true;
    }

    // the pulled-over vehicles drive back onto the main lane in the same order.
    // (a queue gives the order back unchanged, a stack would have reversed it)
    // returns how many vehicles went back
    int restoreFromSide(int j) {
        if (!validIndex(j)) {
            return 0;
        }
        ArrayQueue &lane = mainLane[j - 1];
        ArrayQueue &side = sideLane[j - 1];

        int moved = 0;
        while (!side.isempty() && !lane.isFull()) {
            lane.enqueue(side.dequeue());
            moved++;
        }
        return moved;
    }

    // empties every lane and puts every signal back to NORMAL
    void reset() {
        for (int i = 0; i < NUM_JUNCTIONS; i++) {
            mainLane[i].clear();
            sideLane[i].clear();
            signal[i].reset();
        }
    }
};
