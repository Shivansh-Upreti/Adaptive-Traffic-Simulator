#include "../include/Simulator.hpp"

void Simulator::log(string msg){
    eventLog[logTotal % LOG_SIZE] = msg;
    logTotal++;
}

void Simulator::buildMap(){
    city.addRoad(1, 2, 5);
    city.addRoad(1, 3, 4);
    city.addRoad(2, 4, 4);
    city.addRoad(3, 4, 5);
    city.addRoad(3, 5, 5);
    city.addRoad(4, 5, 4);
    city.addRoad(2, 6, 6);
    city.addRoad(4, 6, 5);
    city.addRoad(5, 6, 7);
}

int Simulator::findIndex(int id){
    for(int i = 0; i < MAX_FLEET; i++){
        if(fleet[i] != nullptr && fleet[i]->getID() == id){
            return i;
        }
    }
    return -1;
}

int Simulator::freeSlot(){
    for(int i = 0; i < MAX_FLEET; i++){
        if(fleet[i] == nullptr){
            return i;
        }
    }
    return -1;
}

void Simulator::removeVehicle(int idx){
    if(idx < 0 || idx >= MAX_FLEET || fleet[idx] == nullptr){
        return;
    }
    delete fleet[idx];          // frees the vehicle (and its route list)
    fleet[idx] = nullptr;       // the slot must never keep pointing at deleted memory
    fleetCount--;
}

int Simulator::countInLanes(int id){
    int count = 0;
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        for(int k = 0; k < flow.laneLength(j); k++){
            if(flow.laneAt(j, k) == id){
                count++;
            }
        }
        for(int k = 0; k < flow.sideLength(j); k++){
            if(flow.sideAt(j, k) == id){
                count++;
            }
        }
    }
    return count;
}