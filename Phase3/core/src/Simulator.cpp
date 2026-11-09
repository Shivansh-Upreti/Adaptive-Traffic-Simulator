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


//Constructor and Reset
Simulator::Simulator() : city(NUM_JUNCTIONS){
    // reset() looks at these first, so they need a start value
    for(int i = 0; i < MAX_FLEET; i++){
        fleet[i] = nullptr;
    }
    fleetCount = 0;
    dispatchActive = false;
    reset();
}

void Simulator::reset(){
    if(dispatchActive){
        stopTrip("the simulation was reset");   // gives every signal hold back, clears the route
    }
    for(int i = 0; i < MAX_FLEET; i++){
        if(fleet[i] != nullptr){
            delete fleet[i];    // one delete for every new. Vehicle has a virtual destructor,
            fleet[i] = nullptr; // so an EmergencyVehicle is destroyed correctly too
        }
    }
    fleetCount = 0;

    flow.reset();
    emergencyQ.clear();
    city.clear();               // delete every road ...
    buildMap();                 // ... and build the starting map again
    history.clear();

    clockTick = 0;
    for(int k = 0; k < LOG_SIZE; k++){
        eventLog[k] = "";
    }
    logTotal = 0;

    dispatchActive = false;
    lastTripArrived = false;
    activeIdx = -1;
    activeReq.vehicleID = -1;
    activeReq.priorityLVL = 0;
    activeReq.arrivalTime = 0;
    activeReq.label = "";
    routeCur = nullptr;
    lastJunction = 0;
    for(int j = 0; j <= NUM_JUNCTIONS; j++){
        tookHold[j] = false;
    }
    statBypass = 0;
    statPreempt = 0;
    statEvacuated = 0;
}


//Spawning
bool Simulator::spawnNormal(int id, int j){
    if(!checkNewId(id)){
        return false;
    }
    if(!validIndex(j)){
        log("[ERROR] Junction must be between 1 and " + to_string(NUM_JUNCTIONS) + ".");
        return false;
    }
    if(!laneAvailable(j)){
        return false;
    }

    Vehicle* v = new Vehicle(id);       // new vehicle on the heap
    fleet[freeSlot()] = v;              // the first empty slot remembers the address so it can be deleted later
    fleetCount++;                       // (checkNewId made sure that a free slot exists)
    flow.vehicleArrive(j, id);          // cannot fail, the lane was checked above
    log("[SPAWN] Vehicle " + to_string(id) + " (priority " + to_string(NORMAL_PRIORITY) + ") waits at " + name(j) + ".");
    return true;
}

bool Simulator::spawnEmergency(int id, int type, int from, int to){
    if(!checkNewId(id)){
        return false;
    }
    if(type < PRIORITY_AMBULANCE || type > PRIORITY_POLICE){
        log("[ERROR] Emergency type must be 1 (Ambulance), 2 (Fire Truck) or 3 (Police).");
        return false;
    }
    if(!validIndex(from) || !validIndex(to)){
        log("[ERROR] Junctions must be between 1 and " + to_string(NUM_JUNCTIONS) + ".");
        return false;
    }
    if(from == to){
        log("[ERROR] Start and destination are the same junction.");
        return false;
    }
    if(emergencyQ.isFull()){
        log("[ERROR] " + to_string(PQ_CAPACITY) + " emergency requests are already waiting. Dispatch one first.");
        return false;
    }
    if(!laneAvailable(from)){
        return false;
    }

    string label = emergencyTypeName(type);
    EmergencyVehicle* ev = new EmergencyVehicle(id, label, type);
    EmergencyRequest req = {id, type, clockTick, label, from, to};
    clockTick++;
    emergencyQ.push(req);               // cannot fail, fullness was checked above
    fleet[freeSlot()] = ev;             // an EmergencyVehicle* fits into a Vehicle* (inheritance)
    fleetCount++;
    flow.vehicleArrive(from, id);       // cannot fail, the lane was checked above
    log("[SPAWN] " + label + " " + to_string(id) + " (priority " + to_string(type) + ") waits at " + name(from)
        + ", destination " + name(to) + ". Request queued.");
    return true;
}


//Emergency Trip
int Simulator::startDispatch(){
    if(dispatchActive){
        log("[ERROR] An emergency vehicle is already on the road.");
        return DISPATCH_FAILED;
    }
    if(emergencyQ.isEmpty()){
        log("[INFO] No emergency request is waiting.");
        return DISPATCH_NOTHING;
    }

    EmergencyRequest req = emergencyQ.top();    // lowest priority number, earliest arrival on a tie
    emergencyQ.pop();
    int idx = findIndex(req.vehicleID);
    if(idx == -1){
        log("[ERROR] The vehicle of this request is not on the road. Request dropped.");
        return DISPATCH_FAILED;
    }

    log("[DISPATCH] " + req.label + " " + to_string(req.vehicleID) + " (priority " + to_string(req.priorityLVL) + "): "
        + name(req.from) + " -> " + name(req.to));

    int dist[7] = {0};
    int prev[7] = {0};
    SinglyLinkedList &route = fleet[idx]->getRoute();   // a reference: the list stays inside the vehicle
    route.clear();
    city.dijkstra(req.from, dist, prev);
    if(!city.buildRoute(req.to, dist, prev, route)){
        log("[NO ROUTE] " + name(req.to) + " cannot be reached from " + name(req.from)
            + ". Request cancelled, the vehicle stays at " + name(req.from) + ".");
        return DISPATCH_FAILED;
    }

    activeIdx = idx;
    activeReq = req;
    dispatchActive = true;
    lastTripArrived = false;
    routeCur = route.getHead();                         // the vehicle stands on the first node
    lastJunction = req.from;
    history.clear();
    history.recordVisit(req.from);
    for(int j = 0; j <= NUM_JUNCTIONS; j++){
        tookHold[j] = false;
    }

    log("[ROUTE] " + routeText(route) + " (distance " + to_string(dist[req.to]) + ")");
    prepareJunction(req.from);                          // the start junction is handled like any other
    return DISPATCH_STARTED;
}

bool Simulator::stepDispatch(){
    if(!dispatchActive){
        return false;
    }
    if(routeCur == nullptr){
        stopTrip("the route was lost");
        return false;
    }
    if(routeCur->next == nullptr){
        finishTrip();
        return false;
    }

    int id = fleet[activeIdx]->getID();
    int from = routeCur->data;
    SinglyLinkedList::Node* nxt = routeCur->next;       // the node of the next junction
    int to = nxt->data;

    // a road can be closed after the route was planned
    if(!city.hasRoad(from, to)){
        log("[BLOCKED] The road " + name(from) + " - " + name(to) + " is closed. Looking for another route...");
        if(!reroute(from)){
            stopTrip("no other route to " + name(activeReq.to));
            return false;
        }
        return true;        // the move itself is tried again with the new route
    }

    // the NEXT junction turns green first (or is skipped if empty) ...
    prepareJunction(to);

    // ... and only then the vehicle moves. all checks first, so a failure changes nothing
    bool canMove = !flow.laneFull(to) && flow.laneContains(from, id);
    if(!canMove){
        cleanupJunction(to);
        stopTrip("it cannot enter " + name(to));
        return false;
    }
    flow.laneRemove(from, id);
    if(!flow.vehicleArrive(to, id)){            // cannot fail (checked above), but a vehicle must never vanish
        flow.vehicleArrive(from, id);
        cleanupJunction(to);
        stopTrip("it cannot enter " + name(to));
        return false;
    }
    history.recordVisit(to);
    routeCur = nxt;                                     // routeCur walks forward along the list
    lastJunction = to;
    log("[MOVE] " + activeName() + ": " + name(from) + " -> " + name(to));
    cleanupJunction(from);                              // it has left 'from': signal back, vehicles return

    if(routeCur->next == nullptr){
        finishTrip();
        return false;
    }
    return true;
}


//Normal traffic and roads
bool Simulator::moveNormal(int from, int to){
    if(!validIndex(from) || !validIndex(to)){
        log("[ERROR] Junctions must be between 1 and " + to_string(NUM_JUNCTIONS) + ".");
        return false;
    }
    if(from == to){
        log("[ERROR] Start and destination are the same junction.");
        return false;
    }
    if(!city.hasRoad(from, to)){
        log("[ERROR] There is no road between " + name(from) + " and " + name(to) + ".");
        return false;
    }
    if(flow.isPreempted(from)){
        log("[SIGNAL] " + name(from) + " is green for an emergency vehicle; normal traffic is paused.");
        return false;
    }
    int id = firstNormalInLane(from);
    if(id == -1){
        log("[ERROR] No normal vehicle is waiting at " + name(from) + ".");
        return false;
    }
    if(!laneAvailable(to)){
        return false;
    }
    flow.laneRemove(from, id);
    if(!flow.vehicleArrive(to, id)){            // cannot fail (checked above), but a vehicle must never vanish
        flow.vehicleArrive(from, id);
        log("[ERROR] Vehicle " + to_string(id) + " could not enter " + name(to) + ".");
        return false;
    }
    log("[MOVE] Vehicle " + to_string(id) + ": " + name(from) + " -> " + name(to));
    return true;
}

bool Simulator::departNormal(int j){
    if(!validIndex(j)){
        log("[ERROR] Junction must be between 1 and " + to_string(NUM_JUNCTIONS) + ".");
        return false;
    }
    if(flow.isPreempted(j)){
        log("[SIGNAL] " + name(j) + " is green for an emergency vehicle; normal traffic is paused.");
        return false;
    }
    int id = firstNormalInLane(j);
    if(id == -1){
        log("[ERROR] No normal vehicle is waiting at " + name(j) + ".");
        return false;
    }
    flow.laneRemove(j, id);
    removeVehicle(findIndex(id));               // it left: delete it so its slot and id are free again
    log("[LEAVE] Vehicle " + to_string(id) + " left the network from " + name(j) + ".");
    return true;
}

//Roads
bool Simulator::closeRoad(int a, int b){
    if(!city.removeRoad(a, b)){
        log("[ERROR] There is no road between " + name(a) + " and " + name(b) + " to close.");
        return false;
    }
    log("[ROAD] The road " + name(a) + " - " + name(b) + " is now CLOSED.");
    return true;
}

bool Simulator::openRoad(int a, int b, int length){
    if(!city.isValid(a) || !city.isValid(b) || a == b){
        log("[ERROR] A road needs two different junctions between 1 and " + to_string(NUM_JUNCTIONS) + ".");
        return false;
    }
    if(length < 1 || length > MAX_ROAD_LENGTH){
        log("[ERROR] Road length must be between 1 and " + to_string(MAX_ROAD_LENGTH) + ".");
        return false;
    }
    city.addRoad(a, b, length);
    log("[ROAD] The road " + name(a) + " - " + name(b) + " is now OPEN (length " + to_string(length) + ").");
    return true;
}

