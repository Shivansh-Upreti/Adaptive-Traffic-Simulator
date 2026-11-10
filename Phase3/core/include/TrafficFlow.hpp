#pragma once
#include <iostream>
#include <string>
using namespace std;
#include "ArrayQueue.hpp"
#include "TrafficSignal.hpp"

#include "CityMap.hpp"          // NUM_JUNCTIONS
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
    bool vehicleArrive(int j, int vehID);


    int vehicleDepart(int j);


    int queueLength(int j);


    bool moveVehicle(int from, int to);


    // added in phase 2: same as moveVehicle but for an emergency vehicle.
    // it does not wait for the vehicles ahead of it, remove() takes it out
    // of the queue from wherever it is standing
    bool moveEmergency(int from, int to, int vehID);


    // added in phase 2: traffic density of a junction, decided from its queue length
    string congestionLevel(int j);


    void showDensity();


    void showAllQueues();


    // phase 3: reading the lanes (silent: no printing)

    int laneLength(int j);

    // i-th vehicle of the main lane counted from the front (0 = front), -1 if there is none
    int laneAt(int j, int i);

    bool laneContains(int j, int vehID);

    bool laneFull(int j);

    // takes a vehicle out of the main lane from wherever it stands. false if it is not there
    bool laneRemove(int j, int vehID);

    int sideLength(int j);

    int sideAt(int j, int i);


    // phase 3: signals

    void preempt(int j);

    // false if there was no hold to give back (or the junction does not exist)
    bool release(int j);

    bool isPreempted(int j);

    int holdCount(int j);



    // phase 3: two-lane evacuation 

    // how many vehicles wait in the main lane of junction j, not counting vehicle skipID
    // (the emergency vehicle must not count itself as "traffic")
    int waitingExcept(int j, int skipID);

    // every vehicle except keepID pulls over from the main lane into the side lane.
    // they are taken out from the front one by one, so they keep their order in the side lane.
    // the check comes FIRST: if the side lane cannot hold them all, nothing is moved at all
    bool evacuateToSide(int j, int keepID);

    // the pulled-over vehicles drive back onto the main lane in the same order.
    // (a queue gives the order back unchanged, a stack would have reversed it)
    // returns how many vehicles went back
    int restoreFromSide(int j);

    // empties every lane and puts every signal back to NORMAL
    void reset();
};
