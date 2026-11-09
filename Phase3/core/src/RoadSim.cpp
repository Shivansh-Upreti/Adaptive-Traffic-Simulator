#include "../include/RoadSim.hpp"

//Foundation
void RoadSim::log(string msg){
    eventLog[logTotal % ROAD_LOG_SIZE] = msg;
    logTotal++;
}

int RoadSim::findSlot(int id){
    for(int i = 0; i < MAX_MOVERS; i++){
        if(movers[i].veh != nullptr && movers[i].veh->getID() == id){
            return i;
        }
    }
    return -1;
}

int RoadSim::freeMoverSlot(){
    for(int i = 0; i < MAX_MOVERS; i++){
        if(movers[i].veh == nullptr){
            return i;
        }
    }
    return -1;
}

int RoadSim::countEVs(){
    int count = 0;
    for(int i = 0; i < MAX_MOVERS; i++){
        if(movers[i].veh != nullptr && movers[i].isEV){
            count++;
        }
    }
    return count;
}

int RoadSim::cellsForLength(int length){
    int c = length * 2;
    if(c < 6){
        c = 6;
    }
    if(c > MAX_CELLS){
        c = MAX_CELLS;
    }
    return c;
}

void RoadSim::ensureLane(int a, int b, int length){
    if(road[a][b].cells == 0){
        road[a][b].cells = cellsForLength(length);
    }
    if(road[b][a].cells == 0){
        road[b][a].cells = road[a][b].cells;
    }
}

void RoadSim::buildMap(){
    int fromList[9] = {1, 1, 2, 3, 3, 4, 2, 4, 5};
    int toList[9]   = {2, 3, 4, 4, 5, 5, 6, 6, 6};
    int lenList[9]  = {5, 4, 4, 5, 5, 4, 6, 5, 7};
    for(int i = 0; i < 9; i++){
        city.addRoad(fromList[i], toList[i], lenList[i]);
        ensureLane(fromList[i], toList[i], lenList[i]);
    }
}

void RoadSim::clearMover(int i){
    movers[i].veh = nullptr;
    movers[i].state = MV_FREE;
    movers[i].from = 0;
    movers[i].to = 0;
    movers[i].cell = 0;
    movers[i].cur = nullptr;
    movers[i].destination = 0;
    movers[i].origin = 0;
    movers[i].isEV = false;
    movers[i].priority = NORMAL_PRIORITY;
    movers[i].arrival = 0;
    movers[i].label = "";
    movers[i].waitTicks = 0;
    movers[i].waitCode = WAIT_NONE;
    movers[i].reservedMask = 0;
    movers[i].lastMoveTick = -1;
}