#include "../include/Simulator.hpp"

//foundation
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


//checks
bool Simulator::laneAvailable(int j){
    if(laneBusy(j)){
        log("[ERROR] " + name(j) + " is being cleared for an emergency vehicle. Try again after it has passed.");
        return false;
    }
    if(flow.laneFull(j)){
        log("[ERROR] The lane at " + name(j) + " is full (" + to_string(QUEUE_CAPACITY) + " vehicles).");
        return false;
    }
    return true;
}

bool Simulator::checkNewId(int id){
    if(id < 1 || id > MAX_VEHICLE_ID){
        log("[ERROR] Vehicle ID must be between 1 and " + to_string(MAX_VEHICLE_ID) + ".");
        return false;
    }
    if(findIndex(id) != -1){
        log("[ERROR] Vehicle ID " + to_string(id) + " is already in use by a vehicle that is still on the road.");
        return false;
    }
    if(freeSlot() == -1){
        log("[ERROR] The fleet is full (" + to_string(MAX_FLEET) + " vehicles on the road at the same time).");
        return false;
    }
    return true;
}

bool Simulator::isEmergencyId(int id){
    int i = findIndex(id);
    if(i == -1){
        return false;
    }
    return !fleet[i]->mustWait();       // only emergency vehicles do not have to wait
}

int Simulator::countParkedEmergency(int j, int skipID){
    int count = 0;
    for(int i = 0; i < flow.sideLength(j); i++){
        int id = flow.sideAt(j, i);
        if(id != skipID && isEmergencyId(id)){
            count++;
        }
    }
    return count;
}

int Simulator::firstNormalInLane(int j){
    for(int i = 0; i < flow.laneLength(j); i++){
        int id = flow.laneAt(j, i);
        if(!isEmergencyId(id)){
            return id;
        }
    }
    return -1;
}

string Simulator::routeText(SinglyLinkedList &route){
    string text = "";
    SinglyLinkedList::Node* p = route.getHead();     // p walks along the list, one node at a time
    while(p != nullptr){
        text = text + "J" + to_string(p->data);
        if(p->next != nullptr){
            text = text + " -> ";
        }
        p = p->next;
    }
    return text;
}


//Trip end and Reroute
void Simulator::prepareJunction(int j){
    int evID = fleet[activeIdx]->getID();
    string who = activeName();
    int waiting = flow.waitingExcept(j, evID);

    if(waiting == 0){
        // empty junction: no signal request, no evacuation, no overhead
        statBypass++;
        log("[BYPASS] " + name(j) + " is empty; " + who + " passed without signal request.");
        return;
    }

    flow.preempt(j);
    tookHold[j] = true;
    statPreempt++;
    log("[SIGNAL] " + name(j) + ": preempted green light granted for " + who + "; cross-traffic paused.");

    if(flow.evacuateToSide(j, evID)){
        statEvacuated = statEvacuated + waiting;
        // an emergency vehicle that is parked (not dispatched) is pulled over too, but it is not "normal"
        int parked = countParkedEmergency(j, evID);
        string text = to_string(waiting - parked) + " normal vehicle(s)";
        if(parked > 0){
            text = text + " and " + to_string(parked) + " parked emergency vehicle(s)";
        }
        log("[EVACUATION] " + name(j) + ": " + text + " pulled over to side lane. " + who + " passed to front.");
    }
    else{
        log("[EVACUATION] " + name(j) + ": side lane is too full, " + who + " uses the main lane.");
    }
}

void Simulator::cleanupJunction(int j){
    if(!tookHold[j]){
        return;
    }
    int back = flow.restoreFromSide(j);
    flow.release(j);
    tookHold[j] = false;
    if(back > 0){
        log("[EVACUATION] " + name(j) + ": " + to_string(back) + " vehicle(s) back in the main lane, same order.");
    }
    log("[SIGNAL] " + name(j) + " back to normal; cross-traffic released.");
}

void Simulator::releaseAllHolds(){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        cleanupJunction(j);
    }
}

void Simulator::stopTrip(string reason){
    log("[STOPPED] " + activeName() + " stopped at " + name(lastJunction) + ": " + reason + ". Request cancelled.");
    releaseAllHolds();                              // every hold goes back, whatever happened
    fleet[activeIdx]->getRoute().clear();
    routeCur = nullptr;                             // the nodes are deleted, so no pointer may still point at them
    dispatchActive = false;
}

void Simulator::finishTrip(){
    int id = fleet[activeIdx]->getID();
    int dest = routeCur->data;
    flow.laneRemove(dest, id);                      // it leaves the road network
    cleanupJunction(dest);
    log("[ARRIVED] " + activeName() + " reached " + name(dest) + ".");   // (needs the vehicle, so it comes BEFORE the delete)
    routeCur = nullptr;                             // routeCur points into the vehicle's route list: forget it first
    dispatchActive = false;
    lastTripArrived = true;
    removeVehicle(activeIdx);                       // delete the vehicle: its slot and its id are free again
    activeIdx = -1;
}

bool Simulator::reroute(int from){
    int dist[7] = {0};
    int prev[7] = {0};
    SinglyLinkedList &route = fleet[activeIdx]->getRoute();

    city.dijkstra(from, dist, prev);
    route.clear();              // this DELETES every node of the old route ...
    routeCur = nullptr;         // ... so routeCur must not keep pointing at them (dangling pointer)
    if(!city.buildRoute(activeReq.to, dist, prev, route)){
        log("[NO ROUTE] " + name(activeReq.to) + " cannot be reached from " + name(from) + " any more.");
        return false;
    }
    routeCur = route.getHead(); // the new list starts at the vehicle's current junction
    log("[REROUTE] New route: " + routeText(route) + " (distance " + to_string(dist[activeReq.to]) + ")");
    return true;
}
