#include "../include/TrafficFlow.hpp"

bool TrafficFlow::vehicleArrive(int j, int vehID){
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

int TrafficFlow::vehicleDepart(int j){
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

int TrafficFlow::queueLength(int j){
    if (!validIndex(j)) {
        cout << "Invalid junction ID\n";
        return -1;
    }

    return mainLane[j - 1].size();
}

bool TrafficFlow::moveVehicle(int from, int to){
    if (!validIndex(from) || !validIndex(to)) {
        cout << "Invalid junction ID\n";
        return false;
    }

    if (from == to) {
        cout << "Source and destination are the same\n";
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

bool TrafficFlow::moveEmergency(int from, int to, int vehID){
    if (!validIndex(from) || !validIndex(to)) {
        cout << "Invalid junction ID\n";
        return false;
    }

    if (from == to) {
        cout << "Source and destination are the same\n";
        return false;
    }

    if (!mainLane[from - 1].contains(vehID)) {
        cout << "Vehicle " << vehID<< " not found at J" << from << endl;
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

string TrafficFlow::congestionLevel(int j){
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

void TrafficFlow::showDensity(){
    for (int i = 1; i <= NUM_JUNCTIONS; i++) {
        cout << "J" << i << ": " << queueLength(i)
             << " waiting -> " << congestionLevel(i) << endl;
    }
}

void TrafficFlow::showAllQueues(){
    for (int i = 0; i < NUM_JUNCTIONS; i++) {
        cout << "J" << i + 1
             << " (length " << mainLane[i].size() << "): ";

        if (mainLane[i].isempty()) {
            cout << "-";
        } else {
            mainLane[i].traverse();
        }
        cout << endl;
    }
}

int TrafficFlow::laneLength(int j){
    if (!validIndex(j)) return 0;
    return mainLane[j - 1].size();
}

int TrafficFlow::laneAt(int j, int i){
    if (!validIndex(j)) return -1;
    return mainLane[j - 1].at(i);
}

bool TrafficFlow::laneContains(int j, int vehID){
    if (!validIndex(j)) return false;
    return mainLane[j - 1].contains(vehID);
}

bool TrafficFlow::laneFull(int j){
    if (!validIndex(j)) return false;
    return mainLane[j - 1].isFull();
}

bool TrafficFlow::laneRemove(int j, int vehID){
    if (!validIndex(j)) return false;
    return mainLane[j - 1].remove(vehID);
}

int TrafficFlow::sideLength(int j){
    if (!validIndex(j)) return 0;
    return sideLane[j - 1].size();
}

int TrafficFlow::sideAt(int j, int i){
    if (!validIndex(j)) return -1;
    return sideLane[j - 1].at(i);
}

void TrafficFlow::preempt(int j){
    if (validIndex(j)) {
        signal[j - 1].preempt();
    }
}

bool TrafficFlow::release(int j){
    if (!validIndex(j)) return false;
    return signal[j - 1].release();
}

bool TrafficFlow::isPreempted(int j){
    if (!validIndex(j)) return false;
    return signal[j - 1].isPreempted();
}

int TrafficFlow::holdCount(int j){
    if (!validIndex(j)) return 0;
    return signal[j - 1].getHolds();
}


int TrafficFlow::waitingExcept(int j, int skipID){
    if (!validIndex(j)) {
        return 0;
    }
    int count = mainLane[j - 1].size();
    if (mainLane[j - 1].contains(skipID)) {
        count--;
    }
    return count;
}

bool TrafficFlow::evacuateToSide(int j, int keepID){
    if (!validIndex(j)) {
        return false;
    }
    ArrayQueue &lane = mainLane[j - 1];
    ArrayQueue &side = sideLane[j - 1];

    int total = lane.size();

    int toMove = waitingExcept(j, keepID);

    // Check capacity BEFORE moving any vehicles.
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

int TrafficFlow::restoreFromSide(int j){
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

void TrafficFlow::reset(){
    for (int i = 0; i < NUM_JUNCTIONS; i++) {
        mainLane[i].clear();
        sideLane[i].clear();
        signal[i].reset();
    }
}
