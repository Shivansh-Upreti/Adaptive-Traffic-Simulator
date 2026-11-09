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


// Priority rules
void RoadSim::commitEdge(int j, int n, int slot){
    road[j][n].activeEVCount++;
    if(road[j][n].activeEVCount == 1){
        log("[OVERTAKE] " + who(slot) + " enters " + name(j) + " -> " + name(n) + ": normal vehicles move to the shoulders, "
            + name(n) + " -> " + name(j) + " is frozen.");
    }
}

void RoadSim::setWait(int slot, int code, int j){
    Mover &m = movers[slot];
    if(m.waitCode == code){
        return;
    }
    m.waitCode = code;
    if(code == WAIT_BOX){
        statClearanceWaits++;
        log("[CLEARANCE] " + who(slot) + " has a green light at " + name(j) + " but the junction box is occupied: it waits at the line.");
    }
    else if(code == WAIT_YIELD){
        statYields++;
        int holder = box[j].reservedByID;
        int hs = findSlot(holder);
        string holderText = "a more important EV";
        if(hs != -1){
            holderText = who(hs) + " (priority " + to_string(movers[hs].priority) + ")";
        }
        log("[YIELD] " + who(slot) + " (priority " + to_string(m.priority) + ") waits at the line before " + name(j) + ": reserved by " + holderText + ".");
    }
    else if(code == WAIT_OPPOSITE){
        log("[YIELD] " + who(slot) + " waits at " + name(j) + ": another EV is on the opposite lane of the next road.");
    }
    else if(code == WAIT_EXIT){
        log("[WAIT] " + who(slot) + " waits at " + name(j) + ": the cell after the junction is taken.");
    }
}

bool RoadSim::outranks(int x, int y){
    if(movers[x].priority != movers[y].priority){
        return movers[x].priority < movers[y].priority;
    }
    return movers[x].arrival < movers[y].arrival;
}

bool RoadSim::outrankingEVOnSegment(int a, int b, int slot){
    if(headOnPass){
        return false;               // head-on pass mode: opposing EVs never block each other, both drive on (see isPassingHeadOn)
    }
    for(int i = 0; i < MAX_MOVERS; i++){
        if(i == slot || movers[i].veh == nullptr || !movers[i].isEV){
            continue;
        }
        bool there = false;
        if(movers[i].state == MV_ON_ROAD && movers[i].from == a && movers[i].to == b){
            there = true;
        }
        if(movers[i].state == MV_IN_BOX && movers[i].from == a && movers[i].cur != nullptr
           && movers[i].cur->next != nullptr && movers[i].cur->next->data == b){
            there = true;
        }
        if(there && outranks(i, slot)){
            return true;
        }
    }
    return false;
}

bool RoadSim::reservedByOpposingEV(int j, int n){
    if(!headOnPass){
        return false;
    }
    int holder = findSlot(box[j].reservedByID);
    return holder != -1 && movers[holder].state == MV_ON_ROAD && movers[holder].from == n && movers[holder].to == j;
}

int RoadSim::evEntryCode(int slot, int j){
    Mover &m = movers[slot];
    int a = m.from;                 // it stands at the end of the lane a -> j
    if(box[j].isReserved && box[j].reservedByID != m.veh->getID()){
        // Yield only to an EV that can really go BEFORE us. If the EV that reserved the junction is
        // behind us in the same center lane, it cannot pass us, so waiting for it would block both for ever
        int holder = findSlot(box[j].reservedByID);
        bool holderIsBehindUs = holder != -1 && movers[holder].state == MV_ON_ROAD
                                && movers[holder].from == m.from && movers[holder].to == m.to;
        if(!holderIsBehindUs){
            return WAIT_YIELD;
        }
    }
    if(outrankingEVOnSegment(j, a, slot)){
        return WAIT_OPPOSITE;       // a more important EV drives the opposite way on this road: this EV is frozen
    }
    if(m.cur->next == nullptr){
        return WAIT_NONE;           // j is the destination: no exit needed
    }
    int n = m.cur->next->data;
    if(road[j][n].evCell[0] != 0){
        return WAIT_EXIT;
    }
    if(outrankingEVOnSegment(n, j, slot)){
        return WAIT_OPPOSITE;       // a more important EV drives toward us on the next road
    }
    return WAIT_NONE;
}

bool RoadSim::normalCanEnter(int slot, int j){
    Mover &m = movers[slot];
    int a = m.from;
    if(box[j].isReserved){
        return false;               // green wave: the junction belongs to an EV
    }
    if(road[j][a].activeEVCount > 0){
        return false;               // an EV drives on the opposite lane: this lane is frozen
    }
    if(road[a][j].activeEVCount > 0 && tickCount % 2 != 0){
        return false;               // an EV is on this lane: normal vehicles crawl (every second tick)
    }
    if(m.cur->next == nullptr){
        return true;
    }
    int n = m.cur->next->data;
    return road[j][n].cell[0] == 0;
}

void RoadSim::enterBox(int slot, int j){
    Mover &m = movers[slot];
    int a = m.from;                 // it comes from the lane a -> j
    int last = road[a][j].cells - 1;

    if(m.isEV){
        road[a][j].evCell[last] = 0;
        road[a][j].activeEVCount--;                     // it leaves segment a -> j ...
        if(m.cur->next != nullptr){
            commitEdge(j, m.cur->next->data, slot);     // ... and claims the next one
        }
    }
    else{
        road[a][j].cell[last] = 0;
    }

    box[j].isJunctionOccupied = true;
    box[j].occupantID = m.veh->getID();
    m.state = MV_IN_BOX;
    m.from = j;
    m.to = 0;
    m.cell = 0;
    m.waitTicks = 0;
    m.waitCode = WAIT_NONE;
}


//Junction box exits and entries
void RoadSim::phaseBoxExits(){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        if(!box[j].isJunctionOccupied){
            continue;
        }
        int slot = findSlot(box[j].occupantID);
        if(slot == -1){
            box[j].isJunctionOccupied = false;      // (cannot happen) never keep a box blocked by a vehicle that does not exist
            box[j].occupantID = 0;
            continue;
        }
        Mover &m = movers[slot];

        if(m.cur->next == nullptr){                 // j is the destination: the vehicle arrives and is deleted
            if(m.isEV){
                statArrivedEV++;
                log("[ARRIVED] " + who(slot) + " reached " + name(j) + ".");
            }
            else{
                statArrivedNormal++;
            }
            removeMover(slot);
            continue;
        }

        int n = m.cur->next->data;
        if(m.isEV){
            if(road[j][n].evCell[0] != 0){
                continue;                           // (cannot happen) it simply stays one more tick
            }
            road[j][n].evCell[0] = m.veh->getID();
        }
        else{
            if(road[j][n].cell[0] != 0){
                continue;                           // (cannot happen)
            }
            road[j][n].cell[0] = m.veh->getID();
        }
        box[j].isJunctionOccupied = false;
        box[j].occupantID = 0;
        m.state = MV_ON_ROAD;
        m.from = j;
        m.to = n;
        m.cell = 0;
        m.cur = m.cur->next;                        // cur now means: the junction it is heading to
        m.waitTicks = 0;
    }
}

void RoadSim::phaseBoxEntries(){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        // 1. who stands at a stop line (the last cell of a lane) in front of junction j?
        int evSlots[NUM_JUNCTIONS + 1];
        int normSlots[NUM_JUNCTIONS + 1];
        int nEv = 0;
        int nNorm = 0;
        for(int a = 1; a <= NUM_JUNCTIONS; a++){
            int cells = road[a][j].cells;
            if(cells == 0){
                continue;
            }
            int evId = road[a][j].evCell[cells - 1];
            if(evId != 0){
                int s = findSlot(evId);
                if(s != -1 && ensureRouteOpen(s)){
                    evSlots[nEv] = s;
                    nEv++;
                }
            }
            int id = road[a][j].cell[cells - 1];
            if(id != 0){
                int s = findSlot(id);
                if(s != -1 && ensureRouteOpen(s)){
                    normSlots[nNorm] = s;
                    nNorm++;
                }
            }
        }

        // 1b. CONFLICT: two or more EVs stand at stop lines of this junction. The custom priority queue
        //     ranks them: smaller priority number first, earlier arrival on a tie
        if(nEv >= 2){
            ArrayPriorityQueue ranking;
            for(int k = 0; k < nEv; k++){
                Mover &m = movers[evSlots[k]];
                EmergencyRequest r = {m.veh->getID(), m.priority, m.arrival, m.label, m.from, m.destination};
                ranking.push(r);
            }
            EmergencyRequest first = ranking.top();
            ranking.pop();
            EmergencyRequest second = ranking.top();
            ranking.pop();
            EmergencyRequest third = {0, 0, 0, "", 0, 0};      // none, unless a third EV is waiting too
            if(!ranking.isEmpty()){
                third = ranking.top();
            }
            int key = first.vehicleID * 10000 + second.vehicleID;
            if(conflictKey[j] != key){              // write it into the log once, not every tick
                conflictKey[j] = key;
                statConflicts++;
                log("[CONFLICT] " + name(j) + ": " + first.label + " " + to_string(first.vehicleID) + " (priority " + to_string(first.priorityLVL)
                    + ") and " + second.label + " " + to_string(second.vehicleID) + " (priority " + to_string(second.priorityLVL)
                    + ") arrive together. " + first.label + " " + to_string(first.vehicleID) + " goes first, the other waits at the line.");
                // the same priority: the queue decided by the EARLIER ARRIVAL TIME (ArrayPriorityQueue, unchanged from Phase 2)
                if(first.priorityLVL == second.priorityLVL){
                    statTieBreaks++;
                    string proof = "[TIE-BREAKER PROVED] Two Priority " + to_string(first.priorityLVL) + " vehicles met at " + name(j)
                        + ". Vehicle ID " + to_string(first.vehicleID) + " passed first due to earlier arrival time (arrival no. "
                        + to_string(first.arrivalTime) + " against no. " + to_string(second.arrivalTime) + " for Vehicle ID " + to_string(second.vehicleID) + ").";
                    log(proof);
                    if(announceTies){
                        cout<<proof<<endl;
                    }
                }
            }
            // the whole queue order, as the priority queue pops it: 1st, 2nd and (if there is one) 3rd
            long long order = (long long)first.vehicleID * 1000000 + (long long)second.vehicleID * 1000 + third.vehicleID;
            if(orderKey[j] != order){
                orderKey[j] = order;
                string line = "[QUEUE ORDER] " + name(j) + ": 1st " + first.label + " " + to_string(first.vehicleID) + " (priority " + to_string(first.priorityLVL)
                    + "), 2nd " + second.label + " " + to_string(second.vehicleID) + " (priority " + to_string(second.priorityLVL) + ")";
                if(third.vehicleID != 0){
                    line = line + ", 3rd " + third.label + " " + to_string(third.vehicleID) + " (priority " + to_string(third.priorityLVL) + ")";
                }
                log(line);
                if(announceTies){
                    cout<<line<<endl;
                }
            }
        }
        else{
            conflictKey[j] = 0;
            orderKey[j] = 0;
        }

        // 2. JUNCTION CLEARANCE: nobody enters while the box was occupied at the start of the tick,
        //    not even an EV with a green light. (A vehicle that is leaving needs this tick to get out.)
        if(!boxFreeAtStart[j] || box[j].isJunctionOccupied){
            for(int k = 0; k < nEv; k++){
                setWait(evSlots[k], WAIT_BOX, j);
            }
            continue;
        }

        // 3. emergency vehicles first. The custom priority queue picks the winner
        ArrayPriorityQueue pq;
        int eligible = 0;
        for(int k = 0; k < nEv; k++){
            int code = evEntryCode(evSlots[k], j);
            if(code == WAIT_NONE){
                Mover &m = movers[evSlots[k]];
                EmergencyRequest r = {m.veh->getID(), m.priority, m.arrival, m.label, m.from, m.destination};
                pq.push(r);
                eligible++;
            }
            else{
                setWait(evSlots[k], code, j);
            }
        }
        if(eligible > 0){
            EmergencyRequest best = pq.top();           // the smallest priority number wins (earlier arrival on a tie)
            if(announceTies){                           // demo only: number the EVs in the order they pass the junction
                passCount[j]++;
                string line = "[PASS " + name(j) + "] no. " + to_string(passCount[j]) + ": " + best.label + " " + to_string(best.vehicleID)
                    + " (priority " + to_string(best.priorityLVL) + ", arrival no. " + to_string(best.arrivalTime) + ") enters " + name(j);
                log(line);
                cout<<line<<endl;
            }
            enterBox(findSlot(best.vehicleID), j);
            continue;
        }

        // 4. normal vehicles: the one that has waited longest goes first
        int bestNorm = -1;
        for(int k = 0; k < nNorm; k++){
            if(normalCanEnter(normSlots[k], j)){
                if(bestNorm == -1 || movers[normSlots[k]].waitTicks > movers[bestNorm].waitTicks){
                    bestNorm = normSlots[k];
                }
            }
        }
        if(bestNorm != -1){
            enterBox(bestNorm, j);
        }
    }
}


//Lane moves, spawns, patience
void RoadSim::phaseLaneMoves(){
    for(int a = 1; a <= NUM_JUNCTIONS; a++){
        for(int b = 1; b <= NUM_JUNCTIONS; b++){
            RoadSegment &r = road[a][b];
            if(r.cells == 0){
                continue;
            }

            // ---- normal lane ----
            bool frozen = road[b][a].activeEVCount > 0;             // an EV drives the opposite way
            bool crawl = r.activeEVCount > 0;                       // an EV drives on this segment
            bool mayMove = !frozen && !(crawl && tickCount % 2 != 0);
            for(int c = r.cells - 1; c >= 0; c--){                  // from the front of the lane to the back
                int id = r.cell[c];
                if(id == 0){
                    continue;
                }
                int slot = findSlot(id);
                if(slot == -1){
                    continue;
                }
                if(mayMove && c < r.cells - 1 && r.cell[c + 1] == 0){
                    r.cell[c + 1] = id;                             // one cell forward
                    r.cell[c] = 0;
                    movers[slot].cell = c + 1;
                    movers[slot].waitTicks = 0;
                    movers[slot].lastMoveTick = tickCount;
                }
                else{
                    movers[slot].waitTicks++;
                    if(frozen){
                        statFrozenTicks++;
                    }
                }
            }

            // ---- center lane: emergency vehicles, two cells per tick ----
            // (an EV only stands still for a MORE IMPORTANT EV that drives the opposite way on this road)
            for(int step = 0; step < EV_CELLS_PER_TICK; step++){
                for(int c = r.cells - 2; c >= 0; c--){
                    int id = r.evCell[c];
                    if(id != 0 && r.evCell[c + 1] == 0){
                        int slot = findSlot(id);
                        if(slot != -1 && outrankingEVOnSegment(b, a, slot)){
                            continue;                               // frozen: the more important EV passes first
                        }
                        r.evCell[c + 1] = id;
                        r.evCell[c] = 0;
                        if(slot != -1){
                            movers[slot].cell = c + 1;
                            movers[slot].waitCode = WAIT_NONE;
                            movers[slot].lastMoveTick = tickCount;
                        }
                    }
                }
            }
        }
    }
}

void RoadSim::phaseSpawns(){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        if(spawnQ[j].isempty()){
            continue;
        }
        // an emergency vehicle in the line goes first, and the MOST IMPORTANT one of them
        // (not simply the first: a less important EV in front must not block a more important one behind it)
        int pickId = spawnQ[j].at(0);
        int bestEV = -1;
        for(int i = 0; i < spawnQ[j].size(); i++){
            int s = findSlot(spawnQ[j].at(i));
            if(s != -1 && movers[s].isEV){
                if(bestEV == -1 || outranks(s, bestEV)){
                    bestEV = s;
                }
            }
        }
        if(bestEV != -1){
            pickId = movers[bestEV].veh->getID();
        }
        int slot = findSlot(pickId);
        if(slot == -1){
            spawnQ[j].remove(pickId);
            continue;
        }
        if(!ensureRouteOpen(slot)){
            continue;                               // it was removed
        }
        Mover &m = movers[slot];
        int n = m.cur->next->data;                  // a spawned vehicle always has a first road (start != destination)

        bool canPlace = true;
        if(m.isEV){
            // the same rules as at a stop line, and the reason is written into the log
            int code = WAIT_NONE;
            if(box[j].isJunctionOccupied){
                code = WAIT_BOX;
            }
            else if(box[j].isReserved && box[j].reservedByID != m.veh->getID() && !reservedByOpposingEV(j, n)){
                code = WAIT_YIELD;
            }
            else if(road[j][n].evCell[0] != 0){
                code = WAIT_EXIT;
            }
            else if(outrankingEVOnSegment(n, j, slot)){
                code = WAIT_OPPOSITE;
            }
            if(code != WAIT_NONE){
                setWait(slot, code, j);
                canPlace = false;
            }
        }
        else{
            if(box[j].isJunctionOccupied || box[j].isReserved || road[j][n].cell[0] != 0 || road[n][j].activeEVCount > 0){
                canPlace = false;
            }
        }
        if(!canPlace){
            m.waitTicks++;
            continue;
        }

        spawnQ[j].remove(pickId);
        m.state = MV_ON_ROAD;
        m.from = j;
        m.to = n;
        m.cell = 0;
        m.cur = m.cur->next;                        // heading to n
        m.waitTicks = 0;
        m.waitCode = WAIT_NONE;                     // it is driving now: forget why it waited in the line
        if(m.isEV){
            road[j][n].evCell[0] = pickId;
            commitEdge(j, n, slot);
        }
        else{
            road[j][n].cell[0] = pickId;
        }
    }
}

void RoadSim::phasePatience(){
    for(int i = 0; i < MAX_MOVERS; i++){
        if(movers[i].veh != nullptr && !movers[i].isEV && movers[i].waitTicks > PATIENCE_LIMIT){
            statTimeouts++;
            removeMover(i);
        }
    }
}