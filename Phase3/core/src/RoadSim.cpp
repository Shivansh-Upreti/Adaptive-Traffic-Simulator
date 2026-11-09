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

SinglyLinkedList::Node* RoadSim::upcomingStart(int slot){
    Mover &m = movers[slot];
    if(m.cur == nullptr){
        return nullptr;
    }
    if(m.state == MV_IN_BOX){
        return m.cur->next;         // it is inside junction cur already, the junctions AFTER it are upcoming
    }
    return m.cur;                   // it is heading to cur (or waiting at it)
}



//Removing vehicles, routes
void RoadSim::removeMover(int slot){
    Mover &m = movers[slot];
    if(m.veh == nullptr){
        return;
    }
    int id = m.veh->getID();

    if(m.state == MV_ON_ROAD){
        if(m.isEV){
            road[m.from][m.to].evCell[m.cell] = 0;
            road[m.from][m.to].activeEVCount--;
        }
        else{
            road[m.from][m.to].cell[m.cell] = 0;
        }
    }
    else if(m.state == MV_IN_BOX){
        box[m.from].isJunctionOccupied = false;
        box[m.from].occupantID = 0;
        if(m.isEV && m.cur != nullptr && m.cur->next != nullptr){
            road[m.from][m.cur->next->data].activeEVCount--;    // it had claimed the next segment
        }
    }
    else if(m.state == MV_QUEUED){
        spawnQ[m.from].remove(id);
    }

    if(m.isEV){                     // a vehicle that is gone can not keep junctions reserved
        for(int j = 1; j <= NUM_JUNCTIONS; j++){
            if(box[j].reservedByID == id){
                box[j].isReserved = false;
                box[j].reservedByID = 0;
            }
        }
    }

    m.cur = nullptr;                // cur points into the route list, which the vehicle owns: forget it first
    delete m.veh;                   // frees the vehicle and, through its destructor, its route list
    statDeleted++;
    clearMover(slot);               // the slot is free and no pointer to the deleted vehicle is left
}

bool RoadSim::ensureRouteOpen(int slot){
    Mover &m = movers[slot];
    if(m.cur == nullptr || m.cur->next == nullptr){
        return true;                // nothing ahead (it is at its destination)
    }
    int here = m.cur->data;
    int next = m.cur->next->data;
    if(city.hasRoad(here, next)){
        return true;
    }

    int dist[7] = {0};
    int prev[7] = {0};
    city.dijkstra(here, dist, prev);
    SinglyLinkedList &route = m.veh->getRoute();
    route.clear();                  // this DELETES every node of the old route ...
    m.cur = nullptr;                // ... so cur must not keep pointing at them (dangling pointer)
    if(!city.buildRoute(m.destination, dist, prev, route)){
        if(m.isEV){
            log("[STOPPED] " + who(slot) + " at " + name(here) + ": no way to " + name(m.destination) + " any more. Removed.");
        }
        statStranded++;
        removeMover(slot);
        return false;
    }
    m.cur = route.getHead();        // the new route starts at the junction the vehicle is at
    statReroutes++;
    if(m.isEV){
        log("[REROUTE] " + who(slot) + " at " + name(here) + ": the road ahead is closed, new route found.");
    }
    return true;
}


//Emergency Order and green wave
int RoadSim::orderEVs(int order[]){
    ArrayPriorityQueue pq;
    for(int i = 0; i < MAX_MOVERS; i++){
        if(movers[i].veh != nullptr && movers[i].isEV){
            EmergencyRequest r = {movers[i].veh->getID(), movers[i].priority, movers[i].arrival, movers[i].label, movers[i].from, movers[i].destination};
            pq.push(r);
        }
    }
    int n = 0;
    while(!pq.isEmpty()){
        EmergencyRequest r = pq.top();      // smallest priority number first, earlier arrival on a tie
        pq.pop();
        order[n] = findSlot(r.vehicleID);
        n++;
    }
    return n;
}

void RoadSim::updateReservations(){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        box[j].isReserved = false;
        box[j].reservedByID = 0;
    }

    int order[MAX_ACTIVE_EV + 2];
    int n = orderEVs(order);
    for(int k = 0; k < n; k++){
        int slot = order[k];
        Mover &m = movers[slot];
        int mask = 0;

        SinglyLinkedList::Node* p = upcomingStart(slot);     // p walks along the route list ...
        int count = 0;
        while(p != nullptr && count < GREEN_WAVE_AHEAD){     // ... for at most 2 nodes
            int j = p->data;
            if(!box[j].isReserved){
                box[j].isReserved = true;
                box[j].reservedByID = m.veh->getID();
                mask = mask | (1 << j);
            }
            p = p->next;
            count++;
        }

        int newlyReserved = mask & (~m.reservedMask);        // junctions that were not reserved by this EV last tick
        if(newlyReserved != 0){
            string list = "";
            for(int j = 1; j <= NUM_JUNCTIONS; j++){
                if(newlyReserved & (1 << j)){
                    list = list + name(j) + " ";
                }
            }
            log("[GREEN WAVE] " + who(slot) + " reserves " + list + "- cross-traffic is locked there.");
        }
        m.reservedMask = mask;
    }
}