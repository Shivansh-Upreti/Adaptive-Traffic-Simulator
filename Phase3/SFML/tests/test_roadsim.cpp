// Tests for RoadSim, the live-traffic engine: vehicles that drive along the roads.
// Every scenario runs the engine's self-check after EVERY tick.
#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>
using namespace std;
#include "../../core/include/DemoScene.hpp"

int passed = 0;
int failed = 0;

void check(bool ok, const char* what){
    if(ok){
        cout<<"PASS: "<<what<<endl;
        passed++;
    }
    else{
        cout<<"FAIL: "<<what<<endl;
        failed++;
    }
}

// runs n ticks and returns the number of self-check problems found (0 = clean)
int run(RoadSim &w, int n){
    int problems = 0;
    for(int t = 0; t < n; t++){
        w.tick();
        problems = problems + w.selfCheck();
    }
    return problems;
}

// runs until no vehicle is left (at most maxTicks). returns the ticks used, or -1 if vehicles were still there
int drain(RoadSim &w, int maxTicks, int &problems){
    for(int t = 1; t <= maxTicks; t++){
        w.tick();
        problems = problems + w.selfCheck();
        if(w.getAliveCount() == 0){
            return t;
        }
    }
    return -1;
}

// the index-th field (0 = first) of a line of the CSV file
string csvField(const string &line, int index){
    int start = 0;
    for(int k = 0; k < index; k++){
        start = (int)line.find(",", start) + 1;
    }
    int end = (int)line.find(",", start);
    if(end == -1){
        end = (int)line.size();
    }
    return line.substr(start, end - start);
}

int main(){
    cout<<"--- spawning: every check happens before anything is created ---"<<endl;
    {
        RoadSim w;
        check(!w.spawnNormal(0, 1, 2) && !w.spawnNormal(10000, 1, 2), "vehicle id 0 and 10000 are refused");
        check(!w.spawnNormal(1, 0, 2) && !w.spawnNormal(1, 1, 7), "junction 0 and 7 are refused");
        check(!w.spawnNormal(1, 3, 3), "start equal to destination is refused");
        check(!w.spawnEmergency(2, 0, 1, 2) && !w.spawnEmergency(2, 4, 1, 2), "emergency type 0 and 4 are refused");
        check(w.getAliveCount() == 0 && w.getCreated() == 0, "refused spawns created nothing");
        check(w.spawnNormal(1, 1, 2), "a valid vehicle is accepted");
        check(!w.spawnNormal(1, 2, 3), "the same id again is refused");
        w.closeRoad(2, 6);
        w.closeRoad(4, 6);
        w.closeRoad(5, 6);
        check(!w.spawnNormal(3, 1, 6), "a destination that cannot be reached is refused");
        check(w.getAliveCount() == 1 && w.getCreated() - w.getDeleted() == 1, "and it left nothing behind (created - deleted = alive)");
        check(w.selfCheck() == 0, "self-check is clean");
    }
    {
        RoadSim w;
        bool all = true;
        for(int k = 0; k < 32; k++){
            if(!w.spawnNormal(100 + k, 1, 2 + (k % 5))) all = false;
        }
        check(all, "32 vehicles can wait at one junction");
        check(!w.spawnNormal(200, 1, 2), "the 33rd is refused (waiting line full)");
        RoadSim w2;
        int ok = 0;
        for(int k = 0; k < 70; k++){
            if(w2.spawnNormal(100 + k, 1 + (k % 6), 1 + ((k + 1) % 6))) ok++;
        }
        check(ok == MAX_MOVERS && w2.getAliveCount() == MAX_MOVERS, "at most 60 vehicles are alive at the same time");
        RoadSim w3;
        int evOk = 0;
        for(int k = 0; k < 12; k++){
            if(w3.spawnEmergency(900 + k, 1 + (k % 3), 1 + (k % 6), 1 + ((k + 2) % 6))) evOk++;
        }
        check(evOk == MAX_ACTIVE_EV, "at most 8 emergency vehicles at the same time");
        check(w3.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- driving, arriving and DELETING (fleet slots are reused) ---"<<endl;
    {
        RoadSim w;
        w.spawnNormal(1, 1, 2);
        int problems = 0;
        int ticks = drain(w, 100, problems);
        check(ticks > 0 && problems == 0, "one vehicle J1 -> J2 arrives and the self-check stayed clean");
        check(w.getArrivedNormal() == 1 && w.getAliveCount() == 0, "it arrived and is gone");
        check(w.getCreated() == 1 && w.getDeleted() == 1, "it was created with new and deleted: 1 and 1");

        RoadSim a;
        a.spawnNormal(1, 1, 6);
        a.spawnEmergency(2, 1, 1, 6);
        int normalDone = -1, evDone = -1;
        for(int t = 1; t <= 200 && (normalDone == -1 || evDone == -1); t++){
            a.tick();
            if(evDone == -1 && !a.idInUse(2)) evDone = t;
            if(normalDone == -1 && !a.idInUse(1)) normalDone = t;
        }
        check(evDone > 0 && normalDone > 0 && evDone < normalDone, "the emergency vehicle is faster than a normal vehicle on the same trip");
    }
    {
        RoadSim w;
        int id = 1;
        int problems = 0;
        for(int round = 0; round < 500; round++){
            int a = 1 + (round % 6);
            int b = 1 + ((round + 2 + (round / 6) % 3) % 6);
            if(a == b) b = 1 + (a % 6);
            w.spawnNormal(id, a, b);
            id++;
            if(id > 9999) id = 1;
            problems = problems + run(w, 3);
        }
        int ticks = drain(w, 500, problems);
        check(ticks > 0 && problems == 0, "500 vehicles were sent one after the other and everything drained, self-check clean");
        check(w.getCreated() == 500 && w.getDeleted() == 500, "500 created and 500 deleted: far more vehicles than the 60 slots");
        check(w.getTimeouts() == 0, "nobody had to give up");
    }

    cout<<"\n--- JUNCTION CLEARANCE: an EV with a green light still needs an empty box ---"<<endl;
    {
        // The rule: nobody enters a box that was occupied when the tick began. We watch 600 busy ticks
        // and the demo scene, and compare every tick with the box state of the tick before.
        RoadSim w;
        TrafficFeeder f;
        f.setTarget(45);
        buildMultiEVDemo(w, f);
        int problems = 0;
        bool noEntryIntoOccupied = true;
        bool waitWasJustified = true;
        bool someoneWaited = false;
        for(int t = 0; t < 600; t++){
            int occupiedBefore[NUM_JUNCTIONS + 1];
            int occupantBefore[NUM_JUNCTIONS + 1];
            for(int j = 0; j <= NUM_JUNCTIONS; j++){
                occupiedBefore[j] = w.isJunctionOccupied(j) ? 1 : 0;
                occupantBefore[j] = w.getBoxOccupant(j);
            }
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int j = 1; j <= NUM_JUNCTIONS; j++){
                int occ = w.getBoxOccupant(j);
                if(occ != 0 && occ != occupantBefore[j] && occupiedBefore[j] == 1){
                    noEntryIntoOccupied = false;         // somebody drove into a box that was not empty
                }
            }
            for(int s = 0; s < MAX_MOVERS; s++){
                if(w.isSlotEV(s) && w.getSlotWaitCode(s) == WAIT_BOX && w.getSlotState(s) == MV_ON_ROAD){
                    someoneWaited = true;
                    if(occupiedBefore[w.getSlotTo(s)] == 0) waitWasJustified = false;
                }
            }
        }
        check(problems == 0, "600 busy ticks with the demo scene: the self-check is clean on every tick");
        check(noEntryIntoOccupied, "nobody (not even an EV) ever entered a junction box that was occupied when the tick began");
        check(w.getClearanceWaits() >= 1 && someoneWaited, "an EV with a green light really had to wait for an occupied box");
        check(waitWasJustified, "and whenever an EV waited for the box, the box really had been occupied");
    }
    {
        // two vehicles can never be in one box: scan many busy ticks
        RoadSim w;
        TrafficFeeder f;
        f.setTarget(45);
        bool ok = true;
        int problems = 0;
        for(int t = 0; t < 600; t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int j = 1; j <= NUM_JUNCTIONS; j++){
                int inside = 0;
                for(int s = 0; s < MAX_MOVERS; s++){
                    if(w.getSlotState(s) == MV_IN_BOX && w.getSlotFrom(s) == j) inside++;
                }
                if(inside > 1) ok = false;
                if(w.isJunctionOccupied(j) && inside != 1) ok = false;
            }
        }
        check(ok && problems == 0, "600 busy ticks: never more than one vehicle inside any junction box");
    }

    cout<<"\n--- BIDIRECTIONAL LANES: an EV on J2 -> J4 freezes the traffic on J4 -> J2 ---"<<endl;
    {
        RoadSim w;
        w.spawnNormal(10, 4, 2);                    // X drives J4 -> J2
        int problems = run(w, 4);
        int xs = w.slotOf(10);
        int cellBefore = w.getSlotCell(xs);
        check(w.getSlotState(xs) == MV_ON_ROAD && w.getSlotFrom(xs) == 4 && w.getSlotTo(xs) == 2, "X is driving on J4 -> J2");
        check(!w.isFrozen(xs) && !w.isOnShoulder(xs), "with no EV near, X is not frozen and not on the shoulder");

        w.spawnEmergency(50, 1, 2, 4);              // the ambulance drives J2 -> J4: head-on with X
        bool everFrozen = false;
        bool cellConstant = true;
        bool shoulder = true;
        int frozenTicks = 0;
        int frozenCell = -1;
        for(int t = 0; t < 40; t++){
            w.tick();
            problems = problems + w.selfCheck();
            xs = w.slotOf(10);
            if(xs == -1) break;
            if(w.getActiveEVCount(2, 4) > 0 && w.getSlotState(xs) == MV_ON_ROAD && w.getSlotFrom(xs) == 4){
                if(w.isFrozen(xs)){
                    everFrozen = true;
                    frozenTicks++;
                    if(frozenCell == -1) frozenCell = w.getSlotCell(xs);
                    else if(w.getSlotCell(xs) != frozenCell) cellConstant = false;
                }
                if(!w.isOnShoulder(xs)) shoulder = false;
            }
        }
        check(everFrozen && frozenTicks >= 2, "X was frozen for several ticks while the ambulance was on J2 -> J4");
        check(cellConstant, "X did not move at all while frozen (no head-on collision possible)");
        check(shoulder, "X was on the shoulder during that time");
        check(frozenCell >= cellBefore, "(and it had not moved backwards)");
        int ticks = drain(w, 200, problems);
        check(ticks > 0 && problems == 0, "afterwards X and the ambulance both arrive, self-check clean");
        check(w.getFrozenTicks() >= 2, "the engine counted the frozen vehicle-ticks");
    }
    {
        // Two EVs SWAP roads through the same junction: A drives J2 -> J4 -> J5 and B drives J5 -> J4 -> J2.
        // Each one needs the road the other one is on. With a plain "wait for the other" rule they would
        // wait for each other for ever. The rule: the MORE IMPORTANT EV drives down the center, the other
        // one freezes on the shoulder until it has passed.
        for(int experiment = 0; experiment < 2; experiment++){
            int prioA = (experiment == 0) ? 3 : 1;
            int prioB = (experiment == 0) ? 1 : 3;
            RoadSim w;
            w.spawnEmergency(60, prioA, 2, 5);
            w.spawnEmergency(61, prioB, 5, 2);
            int hi = (prioA < prioB) ? 60 : 61;
            int lo = (hi == 60) ? 61 : 60;
            int problems = 0;
            bool loFrozenSometime = false;
            bool loOnShoulderWhileFrozen = true;
            bool hiNeverFrozen = true;
            bool loStoodStill = true;
            int doneAt = -1;
            for(int t = 1; t <= 200 && doneAt == -1; t++){
                int loSlot = w.slotOf(lo);
                int loCellBefore = (loSlot != -1) ? w.getSlotCell(loSlot) : -1;
                int loStateBefore = (loSlot != -1) ? w.getSlotState(loSlot) : -1;
                w.tick();
                problems = problems + w.selfCheck();
                int hiSlot = w.slotOf(hi);
                loSlot = w.slotOf(lo);
                if(hiSlot != -1 && w.isFrozen(hiSlot)) hiNeverFrozen = false;
                if(loSlot != -1 && w.isFrozen(loSlot)){
                    loFrozenSometime = true;
                    if(!w.isOnShoulder(loSlot)) loOnShoulderWhileFrozen = false;
                    // a frozen vehicle that was already on the road must not have advanced
                    if(loStateBefore == MV_ON_ROAD && w.getSlotState(loSlot) == MV_ON_ROAD && w.getSlotCell(loSlot) != loCellBefore) loStoodStill = false;
                }
                if(w.getAliveCount() == 0) doneAt = t;
            }
            check(doneAt > 0, experiment == 0 ? "road swap (A=3, B=1): no deadlock, both EVs arrive" : "road swap (A=1, B=3): no deadlock, both EVs arrive");
            check(problems == 0, "road swap: the self-check was clean on every tick");
            check(hiNeverFrozen, "the more important EV was never frozen");
            check(loFrozenSometime && loOnShoulderWhileFrozen, "the less important EV froze on the shoulder while the other passed");
            check(loStoodStill, "and it really stood still while frozen (no head-on possible)");
        }
    }

    {
        // The less important EV L is in the MIDDLE of the long road J2 -> J6 when the more important EV H
        // starts at J6 and drives toward it. L must freeze on the shoulder at once, H passes in the center.
        RoadSim w;
        w.spawnEmergency(70, 3, 2, 6);                          // L: police, J2 -> J6
        int problems = run(w, 4);
        int ls = w.slotOf(70);
        check(w.getSlotState(ls) == MV_ON_ROAD && w.getSlotFrom(ls) == 2 && w.getSlotTo(ls) == 6, "L is driving on the long road J2 -> J6");
        w.spawnEmergency(71, 1, 6, 2);                          // H: ambulance, J6 -> J2, head-on with L
        bool frozenSeen = false;
        bool stoodStill = true;
        bool onShoulder = true;
        bool hiPassedFast = false;
        int frozenCell = -1;
        int firstFrozenTick = -1;
        for(int t = 1; t <= 200 && w.getAliveCount() > 0; t++){
            w.tick();
            problems = problems + w.selfCheck();
            ls = w.slotOf(70);
            if(ls != -1 && w.getSlotState(ls) == MV_ON_ROAD && w.isFrozen(ls)){
                if(!frozenSeen){
                    frozenSeen = true;
                    firstFrozenTick = t;
                    frozenCell = w.getSlotCell(ls);
                }
                if(w.getSlotCell(ls) != frozenCell) stoodStill = false;
                if(!w.isOnShoulder(ls)) onShoulder = false;
            }
            if(ls != -1 && frozenSeen && !w.isFrozen(ls) && w.getSlotCell(ls) > frozenCell) hiPassedFast = true;   // it drives on again afterwards
        }
        check(frozenSeen && firstFrozenTick <= 3, "L froze within 3 ticks of the ambulance starting toward it");
        check(stoodStill && onShoulder, "while frozen L did not move and was on the shoulder");
        check(hiPassedFast || w.getArrivedEV() == 2, "afterwards L drove on and arrived");
        check(w.getArrivedEV() == 2 && problems == 0, "both EVs arrived and the self-check was clean on every tick");
    }
    {
        // two EVs of EQUAL priority swap roads: the one that was created first is the more important one
        RoadSim w;
        w.spawnEmergency(60, 2, 2, 5);                          // created first
        w.spawnEmergency(61, 2, 5, 2);                          // created second
        int problems = 0;
        bool secondFroze = false;
        bool firstFroze = false;
        int doneAt = -1;
        for(int t = 1; t <= 200 && doneAt == -1; t++){
            w.tick();
            problems = problems + w.selfCheck();
            int s60 = w.slotOf(60);
            int s61 = w.slotOf(61);
            if(s60 != -1 && w.isFrozen(s60)) firstFroze = true;
            if(s61 != -1 && w.isFrozen(s61)) secondFroze = true;
            if(w.getAliveCount() == 0) doneAt = t;
        }
        check(doneAt > 0 && problems == 0, "equal priority road swap: no deadlock, both arrive, self-check clean");
        check(secondFroze && !firstFroze, "the EV that was created first passed, the other one froze");
    }

    {
        // DEADLOCK found by the stress test: a LESS important EV ahead, a MORE important EV right behind it
        // in the same center lane. The ahead EV must not wait for the one behind it (which cannot pass it).
        RoadSim w;
        w.spawnEmergency(40, 3, 2, 4);                          // police first, J2 -> J4
        int problems = run(w, 3);
        w.spawnEmergency(41, 1, 2, 4);                          // ambulance behind it, same lane, same destination
        int ticks = drain(w, 200, problems);
        check(ticks > 0 && w.getArrivedEV() == 2, "less important EV ahead, more important behind: both arrive (no deadlock)");
        check(problems == 0, "self-check clean");
    }
    {
        // the same problem in the waiting line of a junction: the less important EV is first in line
        RoadSim w;
        w.spawnEmergency(42, 3, 2, 5);                          // police, first in the line at J2
        w.spawnEmergency(43, 1, 2, 4);                          // ambulance, second in the line
        w.tick();
        int ambulance = w.slotOf(43);
        int police = w.slotOf(42);
        check(w.getSlotState(ambulance) == MV_ON_ROAD, "the more important EV leaves the line first, although it was second");
        check(w.getSlotState(police) == MV_QUEUED || w.getSlotState(police) == MV_ON_ROAD, "(and the police car is still in the line or follows)");
        int problems = 0;
        int ticks = drain(w, 200, problems);
        check(ticks > 0 && w.getArrivedEV() == 2 && problems == 0, "both arrive, self-check clean");
    }

    cout<<"\n--- CENTER-OVERTAKE: same-direction cars move to the shoulder and crawl ---"<<endl;
    {
        RoadSim w;
        w.spawnNormal(20, 2, 4);                    // Y drives J2 -> J4, the same way the EV will go
        int problems = run(w, 4);
        int ys = w.slotOf(20);
        w.spawnEmergency(51, 1, 2, 4);
        bool onShoulder = true;
        int ticksOnSegment = 0;
        int movedCells = 0;
        int lastCell = w.getSlotCell(ys);
        for(int t = 0; t < 20; t++){
            w.tick();
            problems = problems + w.selfCheck();
            ys = w.slotOf(20);
            if(ys == -1 || w.getSlotState(ys) != MV_ON_ROAD) break;
            if(w.getActiveEVCount(2, 4) > 0 && w.getSlotFrom(ys) == 2 && w.getSlotTo(ys) == 4){
                ticksOnSegment++;
                if(!w.isOnShoulder(ys)) onShoulder = false;
                if(w.getSlotCell(ys) > lastCell) movedCells += w.getSlotCell(ys) - lastCell;
            }
            lastCell = w.getSlotCell(ys);
        }
        check(ticksOnSegment >= 2 && onShoulder, "while the EV was on the segment, Y was on the shoulder");
        check(movedCells * 2 <= ticksOnSegment + 1, "Y crawled: at most one cell every second tick");
        int ticks = drain(w, 200, problems);
        check(ticks > 0 && problems == 0, "Y keeps moving and arrives afterwards");
    }

    cout<<"\n--- GREEN WAVE: the next 1-2 junctions are reserved, cross-traffic is locked ---"<<endl;
    {
        RoadSim w;
        w.spawnEmergency(70, 1, 1, 6);              // route J1 -> J2 -> J6
        w.tick();
        check(w.isReserved(1) && w.getReservedBy(1) == 70, "the start junction J1 is reserved for the EV");
        check(w.isReserved(2) && w.getReservedBy(2) == 70, "the next junction J2 is reserved too");
        check(!w.isReserved(6), "J6 is three junctions away: not reserved yet");
        check(!w.isReserved(4) && !w.isReserved(3), "junctions that are not on the route are not reserved");
        bool sawJ6 = false;
        int problems = 0;
        for(int t = 0; t < 60 && w.getAliveCount() > 0; t++){
            w.tick();
            problems = problems + w.selfCheck();
            if(w.isReserved(6) && w.getReservedBy(6) == 70) sawJ6 = true;
        }
        check(sawJ6, "J6 got reserved when the EV came within two junctions");
        check(problems == 0, "self-check clean");
        check(!w.isReserved(1) && !w.isReserved(2) && !w.isReserved(6), "after the EV has arrived nothing is reserved any more");
    }
    {
        // no normal vehicle may ENTER a reserved junction (it may only be inside if it got in before the reservation)
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f);
        bool ok = true;
        int problems = 0;
        int prevOccupant[NUM_JUNCTIONS + 1];
        for(int j = 0; j <= NUM_JUNCTIONS; j++) prevOccupant[j] = w.getBoxOccupant(j);
        for(int t = 0; t < 200; t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int j = 1; j <= NUM_JUNCTIONS; j++){
                int occ = w.getBoxOccupant(j);
                if(occ != 0 && occ != prevOccupant[j]){          // a NEW vehicle entered the box this tick
                    int s = w.slotOf(occ);
                    if(s != -1 && !w.isSlotEV(s) && w.isReserved(j)) ok = false;
                }
                prevOccupant[j] = occ;
            }
        }
        check(ok, "200 ticks of heavy traffic: no normal vehicle entered a junction while an EV had it reserved");
        check(problems == 0, "self-check clean");
    }

    cout<<"\n--- MULTI-EV CONFLICTS: the priority queue decides ---"<<endl;
    {
        // Two EVs for the same junction J4: id 81 drives J2 -> J4 -> J5, id 82 drives J3 -> J4 -> J6.
        // 'offset' shifts their start times (negative: 82 starts first), so that for some offsets
        // they reach the stop lines of J4 in the same tick.
        int experimentsWithConflict = 0;
        bool winnerRight = true;
        bool allArrived = true;
        int problems = 0;
        int priorityA[3] = {1, 3, 2};             // priority of 81 in the three experiments
        int priorityB[3] = {2, 1, 2};             // priority of 82
        for(int experiment = 0; experiment < 3; experiment++){
            int conflictsHere = 0;
            for(int offset = -6; offset <= 6; offset++){
                RoadSim w;
                int t81 = (offset < 0) ? -offset : 0;
                int t82 = (offset > 0) ? offset : 0;
                int firstIn4 = 0;
                for(int t = 0; t < 140; t++){
                    if(t == t81) w.spawnEmergency(81, priorityA[experiment], 2, 5);
                    if(t == t82) w.spawnEmergency(82, priorityB[experiment], 3, 6);
                    w.tick();
                    problems = problems + w.selfCheck();
                    int occ = w.getBoxOccupant(4);
                    if(firstIn4 == 0 && (occ == 81 || occ == 82)) firstIn4 = occ;
                }
                if(w.getArrivedEV() != 2) allArrived = false;
                if(w.getConflicts() > 0){
                    conflictsHere++;
                    // who must win? smaller priority number; on a tie the one that was created first
                    int expected;
                    if(priorityA[experiment] < priorityB[experiment]) expected = 81;
                    else if(priorityB[experiment] < priorityA[experiment]) expected = 82;
                    else expected = (t81 <= t82) ? 81 : 82;
                    if(firstIn4 != expected) winnerRight = false;
                }
            }
            if(conflictsHere > 0) experimentsWithConflict++;        // an experiment without any conflict would prove nothing
        }
        check(experimentsWithConflict == 3, "in each of the 3 experiments the two EVs really arrived at J4 together for some start offset");
        check(winnerRight, "every time, the EV with the smaller priority number entered J4 first (equal priority: the one created first)");
        check(allArrived && problems == 0, "all 39 runs: both EVs arrived and the self-check was clean");
    }
    {
        // the demo scene: the Fire Truck (2) and the Ambulance (1) meet; the Police (3) is least important
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f);
        int problems = 0;
        for(int t = 0; t < 300; t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
        }
        check(w.getArrivedEV() == 3, "demo scene: all three EVs arrived");
        check(w.getConflicts() >= 1 && w.getYields() >= 1, "demo scene: a priority conflict happened and the loser yielded");
        check(w.getFrozenTicks() > 0, "demo scene: opposing traffic was frozen for the EVs");
        check(problems == 0 && w.getTimeouts() == 0, "demo scene: clean self-check on every tick, nobody gave up");
    }

    cout<<"\n--- closed roads: reroute, strand, open again ---"<<endl;
    {
        RoadSim w;
        w.spawnEmergency(90, 1, 1, 6);               // J1 -> J2 -> J6
        int problems = 0;
        for(int t = 0; t < 60; t++){
            w.tick();
            problems = problems + w.selfCheck();
            int s = w.slotOf(90);
            if(s != -1 && w.getSlotState(s) == MV_ON_ROAD && w.getSlotFrom(s) == 1 && w.getSlotTo(s) == 2) break;
        }
        check(w.closeRoad(2, 6), "the road J2 - J6 is closed while the EV is on J1 -> J2");
        int ticks = drain(w, 200, problems);
        check(ticks > 0 && w.getArrivedEV() == 1, "the EV found another way (through J4) and arrived");
        check(w.getReroutes() >= 1 && problems == 0, "it rerouted, and the self-check stayed clean");
    }
    {
        RoadSim w;
        w.spawnNormal(91, 1, 6);
        int problems = run(w, 3);
        w.closeRoad(2, 6);
        w.closeRoad(4, 6);
        w.closeRoad(5, 6);
        int ticks = drain(w, 300, problems);
        check(ticks > 0 && problems == 0, "J6 is cut off: the vehicle is removed instead of blocking the road for ever");
        check(w.getStranded() == 1 && w.getCreated() == w.getDeleted(), "it was counted as stranded and deleted");
    }
    {
        RoadSim w;
        w.closeRoad(1, 2);
        w.closeRoad(1, 3);
        check(!w.spawnNormal(1, 1, 5), "J1 is cut off: a trip from J1 is refused");
        check(w.openRoad(1, 3, 4) && w.spawnNormal(1, 1, 5), "after J1 - J3 is opened again, the trip is possible");
        int problems = 0;
        int ticks = drain(w, 300, problems);
        check(ticks > 0 && problems == 0 && w.getArrivedNormal() == 1, "and the vehicle arrives");
        check(!w.openRoad(1, 1, 5) && !w.openRoad(0, 2, 5) && !w.openRoad(1, 2, 0) && !w.openRoad(1, 2, 21), "bad roads are refused");
    }

    cout<<"\n--- reset ---"<<endl;
    {
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f);
        for(int t = 0; t < 8; t++){
            f.feed(w);
            w.tick();
        }
        check(w.getAliveCount() > 10 && w.getEVCount() > 0, "before: many vehicles, EVs on the road");
        w.closeRoad(2, 4);
        w.reset();
        check(w.getAliveCount() == 0 && w.getEVCount() == 0, "after reset: nothing alive");
        check(w.getCreated() == 0 && w.getDeleted() == 0 && w.getTick() == 0, "after reset: counters are 0");
        check(w.hasRoad(2, 4), "after reset: the closed road is back");
        bool lanesEmpty = true;
        for(int a = 1; a <= NUM_JUNCTIONS; a++){
            for(int b = 1; b <= NUM_JUNCTIONS; b++){
                for(int c = 0; c < w.getCells(a, b); c++){
                    if(w.getLaneCell(a, b, c) != 0 || w.getCenterCell(a, b, c) != 0) lanesEmpty = false;
                }
                if(w.getActiveEVCount(a, b) != 0) lanesEmpty = false;
            }
        }
        for(int j = 1; j <= NUM_JUNCTIONS; j++){
            if(w.isJunctionOccupied(j) || w.isReserved(j) || w.getSpawnQueueLength(j) != 0) lanesEmpty = false;
        }
        check(lanesEmpty, "after reset: all lanes, boxes, reservations and waiting lines are empty");
        check(w.selfCheck() == 0, "after reset: self-check clean");
        check(w.spawnNormal(1, 1, 2), "after reset: id 1 can be used again");
        w.reset();
        w.reset();
        check(w.getAliveCount() == 0, "reset twice in a row is harmless");
    }

    cout<<"\n--- bad numbers never reach an array ---"<<endl;
    {
        RoadSim w;
        bool ok = true;
        int bad[5] = {-1, 0, 7, 8, 1000};
        for(int i = 0; i < 5; i++){
            int j = bad[i];
            if(w.getCells(j, 1) != 0 || w.getCells(1, j) != 0) ok = false;
            if(w.getActiveEVCount(j, 1) != 0 || w.getLaneCell(j, 1, 0) != 0 || w.getCenterCell(1, j, 0) != 0) ok = false;
            if(w.isJunctionOccupied(j) || w.isReserved(j) || w.getReservedBy(j) != 0 || w.getBoxOccupant(j) != 0) ok = false;
            if(w.getSpawnQueueLength(j) != 0 || w.hasRoad(j, 1)) ok = false;
        }
        for(int s = -3; s < MAX_MOVERS + 3; s++){
            if(s < 0 || s >= MAX_MOVERS){
                if(w.isSlotUsed(s) || w.getSlotID(s) != 0 || w.isOnShoulder(s) || w.isFrozen(s) || w.getSlotRouteJunction(s, 0) != 0) ok = false;
            }
        }
        check(ok, "junction numbers -1, 0, 7, 8, 1000 and bad slot numbers are refused by every getter");
        check(w.getLaneCell(1, 2, -1) == 0 && w.getLaneCell(1, 2, 999) == 0, "bad cell numbers give 0");
        check(w.getRecentEvent(-1) == "" && w.getRecentEvent(99) == "", "bad event numbers give empty text");
    }

    cout<<"\n--- the self-check really finds damage ---"<<endl;
    {
        // spawn a vehicle, then check that the checker notices when its bookkeeping is wrong.
        // (The engine does not let us break its state, so we only confirm the clean state and counters here;
        //  test_roadstress mutates the engine to prove the checker catches real bugs.)
        RoadSim w;
        w.spawnNormal(1, 1, 2);
        w.spawnEmergency(2, 1, 3, 6);
        int problems = run(w, 15);
        check(problems == 0 && w.selfCheck() == 0, "a busy but healthy state has 0 problems");
        check(w.getCreated() - w.getDeleted() == w.getAliveCount(), "created - deleted = alive");
    }

    cout<<"\n--- demo scenarios: J4 convergence and the three tie-breakers ---"<<endl;
    {
        // scenario 1: the queue order at J4 must be 1st Ambulance, 2nd Fire Truck, 3rd Police car
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, 1);
        bool orderSeen = false;
        int problems = 0;
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int k = 0; k < 6; k++){
                string e = w.getRecentEvent(k);
                if(e.find("[QUEUE ORDER] J4: 1st Ambulance") == 0 && e.find("2nd Fire Truck") != string::npos && e.find("3rd Police") != string::npos) orderSeen = true;
            }
        }
        check(orderSeen, "scenario 1: [QUEUE ORDER] J4 = 1st Ambulance, 2nd Fire Truck, 3rd Police car");
        check(demoVehicleCount(1) == 3 && w.getArrivedEV() == 3 && demoSceneFinished(w) && problems == 0, "scenario 1: exactly 3 EVs, all arrive, self-check clean");
        check(w.getTieBreaks() == 0, "scenario 1: no tie (three different priorities)");
    }
    for(int sc = 2; sc <= 4; sc++){
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, sc);
        bool rightWinner = false;
        int problems = 0;
        string want = "Two Priority " + to_string(sc - 1) + " vehicles met at J4. Vehicle ID 901 passed first";
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int k = 0; k < 6; k++){
                string e = w.getRecentEvent(k);
                if(e.find("[TIE-BREAKER PROVED]") == 0 && e.find(want) != string::npos && e.find("Vehicle ID 902") != string::npos) rightWinner = true;
            }
        }
        check(rightWinner && w.getTieBreaks() == 1, ("scenario " + to_string(sc) + ": one tie-break at J4, the earlier vehicle 901 wins").c_str());
        check(demoVehicleCount(sc) == 2 && w.getArrivedEV() == 2 && problems == 0, ("scenario " + to_string(sc) + ": 2 EVs arrive, self-check clean").c_str());
    }

    cout<<"\n--- scenario 5: the ultimate J4 bottleneck (4 EVs, no leaks) ---"<<endl;
    {
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, 5);
        string order = "";                  // the ids in the order they entered J4
        int lastNo = 0;
        bool threeWaiting = false;
        int problems = 0;
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int k = 7; k >= 0; k--){
                string e = w.getRecentEvent(k);
                if(e.find("[PASS J4] no. ") == 0 && atoi(e.c_str() + 14) == lastNo + 1){
                    lastNo++;
                    size_t p = e.find("enters");
                    size_t q = e.find(": ") + 2;
                    while(e[q] < '0' || e[q] > '9') q++;
                    order = order + e.substr(q, 3) + " ";
                    (void)p;
                }
                if(e.find("[QUEUE ORDER] J4") == 0 && e.find("3rd") != string::npos) threeWaiting = true;
            }
        }
        check(demoVehicleCount(5) == 4 && w.getArrivedEV() == 4 && demoSceneFinished(w), "scenario 5: four EVs, all arrive");
        check(order.find("901 902 903 904") == 0, "J4 is passed in the order Ambulance A (901), Ambulance B (902), Fire Truck (903), Police car (904)");
        check(threeWaiting, "three EVs of different roads really waited at J4 together");
        check(problems == 0 && w.getTimeouts() == 0, "self-check clean on every tick, nobody gave up");
        for(int t = 0; t < 3000 && w.getAliveCount() > 0; t++) w.tick();
        check(w.getAliveCount() == 0 && w.getCreated() == w.getDeleted(), "everything drains away: created == deleted (no leak)");
    }

    cout<<"\n--- scenarios 6 and 7: head-on on one road, simultaneous arrival at J4 ---"<<endl;
    {
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, 6);
        bool bothOnRoad = false;            // the road J2 -> J4 and the road J4 -> J2 each see one EV at the same time
        bool bothPassing = false;           // and both are marked as passing head-on (the window draws them on their own sides)
        int problems = 0;
        int sa = w.slotOf(901);
        int sb = w.slotOf(902);
        bool routesOk = (sa != -1 && sb != -1 && w.getSlotRouteJunction(sa, 0) == 2 && w.getSlotRouteJunction(sa, 1) == 4
                         && w.getSlotRouteJunction(sb, 0) == 4 && w.getSlotRouteJunction(sb, 1) == 2);
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            if(w.getActiveEVCount(2, 4) == 1 && w.getActiveEVCount(4, 2) == 1){
                bothOnRoad = true;
                sa = w.slotOf(901);
                sb = w.slotOf(902);
                if(sa != -1 && sb != -1 && w.isPassingHeadOn(sa) && w.isPassingHeadOn(sb) && !w.isFrozen(sa) && !w.isFrozen(sb)) bothPassing = true;
            }
        }
        check(routesOk, "scenario 6: A is routed J2 -> J4, B is routed J4 -> J2");
        check(bothOnRoad && bothPassing, "scenario 6: activeEVCount sees one EV on each side of the road, both keep driving (nobody is frozen)");
        check(demoVehicleCount(6) == 2 && w.getArrivedEV() == 2 && w.getYields() == 0 && demoSceneFinished(w), "scenario 6: both arrive without any yield");
        check(problems == 0 && w.getTimeouts() == 0, "scenario 6: self-check clean on every tick");
        check(!w.isPassingHeadOn(0), "head-on pass is only reported for living EVs on a road");
    }
    {
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, 7);
        bool rightWinner = false;
        string order = "";
        int lastNo = 0;
        int problems = 0;
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int k = 7; k >= 0; k--){
                string e = w.getRecentEvent(k);
                if(e.find("[TIE-BREAKER PROVED]") == 0 && e.find("met at J4. Vehicle ID 901 passed first") != string::npos) rightWinner = true;
                if(e.find("[PASS J4] no. ") == 0 && atoi(e.c_str() + 14) == lastNo + 1){
                    lastNo++;
                    size_t q = e.find(": ") + 2;
                    while(e[q] < '0' || e[q] > '9') q++;
                    order = order + e.substr(q, 3) + " ";
                }
            }
        }
        check(rightWinner && w.getTieBreaks() == 1, "scenario 7: two Priority 1 ambulances from opposite sides tie at J4, 901 wins by arrival time");
        check(order.find("901 902") == 0, "scenario 7: J4 is entered by 901 first, then 902");
        check(w.getArrivedEV() == 2 && problems == 0, "scenario 7: both arrive, self-check clean");
    }

    cout<<"\n--- road labels: estimated travel time in seconds ---"<<endl;
    {
        Graph g(NUM_JUNCTIONS);
        g.addRoad(1, 2, 9);
        check(g.getTravelTime(1, 2, 0, 0) == 9 * SECONDS_PER_DISTANCE, "Graph::getTravelTime of an empty road = its length in seconds");
        check(g.getTravelTime(1, 2, 3, 2) == 9 + 3 * WAIT_PENALTY_SECONDS + 2, "every waiting vehicle and the preemption delay are added");
        check(g.getTravelTime(1, 3, 5, 5) == 0, "no road: 0");
        RoadSim w;
        bool same = true;
        for(int a = 1; a <= NUM_JUNCTIONS; a++){
            for(int b = 1; b <= NUM_JUNCTIONS; b++){
                if(w.hasRoad(a, b) && w.edgeTravelSeconds(a, b) != w.roadLength(a, b)) same = false;
            }
        }
        check(same, "on empty roads the label equals the road length (J1-J2: 9 s)");
        w.closeRoad(1, 2);
        check(w.edgeTravelSeconds(1, 2) == 0 && w.edgeTravelSeconds(2, 1) == 0, "a closed road has no travel time");
        w.openRoad(1, 2, 9);
        w.spawnEmergency(901, PRIORITY_AMBULANCE, 1, 2);
        bool seen = false;
        for(int t = 0; t < 40 && !seen; t++){
            w.tick();
            if(w.getActiveEVCount(1, 2) > 0){
                seen = true;
                check(w.edgePreempted(1, 2) && w.edgePreempted(2, 1), "an EV on the road pre-empts it (both directions)");
                check(w.edgeTravelSeconds(1, 2) == 9 + 9, "the lane of the EV crawls: base time + base time (18 s)");
                check(w.edgeTravelSeconds(2, 1) == 9 + 5, "the opposite lane is frozen until the EV has passed: 9 + ceil(9 / 2) = 14 s");
            }
        }
        check(seen, "the EV reached the road");
        RoadSim w2;
        TrafficFeeder f2;
        f2.setTarget(45);
        bool slower = false;
        bool neverFaster = true;
        for(int t = 0; t < 300; t++){
            f2.feed(w2);
            w2.tick();
            for(int a = 1; a <= NUM_JUNCTIONS; a++){
                for(int b = 1; b <= NUM_JUNCTIONS; b++){
                    if(!w2.hasRoad(a, b)) continue;
                    int s = w2.edgeTravelSeconds(a, b);
                    if(s < w2.roadLength(a, b)) neverFaster = false;
                    if(s > w2.roadLength(a, b)) slower = true;
                }
            }
        }
        check(neverFaster, "the label is never shorter than the empty road");
        check(slower, "under heavy traffic the label of a congested road becomes longer than its base time");
    }

    cout<<"\n--- scenario 8: shortest path blocked, Dijkstra finds the detour ---"<<endl;
    {
        Graph g(NUM_JUNCTIONS);
        g.addRoad(1, 2, 5);
        g.addRoad(2, 6, 6);
        g.addRoad(1, 3, 4);
        g.addRoad(3, 4, 5);
        g.addRoad(4, 6, 5);
        int dist[7] = {0};
        int prev[7] = {0};
        g.dijkstra(1, dist, prev);
        check(dist[6] == 11, "Graph: the shortest way J1 -> J6 is 11 (through J2)");
        g.removeRoad(1, 2);
        g.dijkstra(1, dist, prev);
        check(dist[6] == 14, "after the road J1 - J2 is removed, Dijkstra returns the detour (14) and never the closed road");

        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, 8);
        int s = w.slotOf(901);
        bool detour = (s != -1 && w.getSlotRouteJunction(s, 0) == 1 && w.getSlotRouteJunction(s, 1) == 3
                       && w.getSlotRouteJunction(s, 2) == 4 && w.getSlotRouteJunction(s, 3) == 6);
        check(demoVehicleCount(8) == 1 && !w.hasRoad(1, 2) && detour, "scenario 8: the road J1 - J2 is closed and the route of the Ambulance is now J1 -> J3 -> J4 -> J6");
        check(w.getReroutes() >= 1, "the re-route is counted");
        int problems = 0;
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
        }
        check(w.getArrivedEV() == 1 && problems == 0 && w.getTimeouts() == 0, "the Ambulance arrives over the detour, self-check clean");
    }

    cout<<"\n--- scenario 9: extreme gridlock, center-overtake ---"<<endl;
    {
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, 9);
        int ev = w.slotOf(901);
        int occupiedWhenEVEnters = 0;
        int onShoulderMax = 0;
        int queueMax = 0;
        bool evAlwaysMoved = true;
        int lastCell = -1;
        int problems = 0;
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            ev = w.slotOf(901);
            if(w.getSpawnQueueLength(5) > queueMax) queueMax = w.getSpawnQueueLength(5);
            if(ev != -1 && w.getSlotState(ev) == MV_ON_ROAD && w.getSlotFrom(ev) == 5){
                int occ = 0;
                int sh = 0;
                for(int c = 0; c < w.getCells(5, 6); c++){
                    if(w.getLaneCell(5, 6, c) != 0) occ++;
                }
                for(int s = 0; s < MAX_MOVERS; s++){
                    if(w.isSlotUsed(s) && !w.isSlotEV(s) && w.getSlotState(s) == MV_ON_ROAD && w.getSlotFrom(s) == 5 && w.getSlotTo(s) == 6 && w.isOnShoulder(s)) sh++;
                }
                if(occupiedWhenEVEnters == 0) occupiedWhenEVEnters = occ;
                if(sh > onShoulderMax) onShoulderMax = sh;
                if(w.getSlotCell(ev) <= lastCell) evAlwaysMoved = false;
                lastCell = w.getSlotCell(ev);
            }
        }
        check(occupiedWhenEVEnters >= 6, "scenario 9: at least 6 normal vehicles stand in the lane when the Ambulance enters the road");
        check(queueMax >= 6, "scenario 9: the waiting line at J5 reaches 6 vehicles or more");
        check(onShoulderMax >= 6, "scenario 9: all of them are on the shoulder while the Ambulance passes (center-overtake)");
        check(evAlwaysMoved, "scenario 9: the Ambulance never stands still on the road");
        check(w.getArrivedEV() == 1 && problems == 0 && w.getTimeouts() == 0, "scenario 9: it arrives, self-check clean on every tick (no clipping)");
    }

    cout<<"\n--- scenario 10: no way through, the Ambulance is stopped safely ---"<<endl;
    {
        RoadSim w;
        TrafficFeeder f;
        buildMultiEVDemo(w, f, 10);
        check(demoVehicleCount(10) == 1 && !w.hasRoad(2, 6) && !w.hasRoad(4, 6) && !w.hasRoad(5, 6), "scenario 10: all three roads into J6 are closed");
        check(w.idInUse(901), "scenario 10: the Ambulance has started (its first road J1 - J2 is open)");
        bool stoppedAtJ2 = false;
        int problems = 0;
        for(int t = 0; t < 300 && !demoSceneFinished(w); t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
            for(int k = 0; k < ROAD_LOG_SIZE; k++){
                if(w.getRecentEvent(k).find("[STOPPED] Ambulance 901 at J2: no way to J6") == 0) stoppedAtJ2 = true;
            }
        }
        check(stoppedAtJ2, "scenario 10: [STOPPED] names the Ambulance, J2 and J6");
        check(demoSceneFinished(w) && w.getArrivedEV() == 0 && w.getStranded() >= 1, "scenario 10: it is removed (counted as stranded), not stuck on the road");
        for(int t = 0; t < 200; t++){
            f.feed(w);
            w.tick();
            problems = problems + w.selfCheck();
        }
        check(problems == 0 && w.getTimeouts() == 0, "scenario 10: no crash, self-check clean, the traffic keeps flowing for 200 more ticks");
    }

    cout<<"\n--- CSV audit log of the EV trips ---"<<endl;
    {
        string file = "audit_test_tmp.csv";
        remove(file.c_str());                       // start without the file
        RoadSim w;
        w.setAuditLog(true);
        w.setAuditFile(file);
        w.spawnEmergency(901, PRIORITY_AMBULANCE, 1, 6);
        w.spawnEmergency(902, PRIORITY_POLICE, 6, 3);
        for(int t = 0; t < 200 && w.getArrivedEV() < 2; t++) w.tick();
        check(w.getArrivedEV() == 2, "two EV trips finished");

        string line[5];
        int lines = 0;
        ifstream in(file.c_str());
        check(in.is_open(), "the CSV file was created");
        while(lines < 5 && getline(in, line[lines])){
            lines++;
        }
        in.close();
        check(lines == 3, "header + one row per finished EV trip (3 lines)");
        check(line[0] == "tick,vehicle_id,priority,start,destination,path,Time Taken (Ticks)", "the header row is there exactly once (first line)");
        bool durationOk = true;                      // both trips were created in tick 0: time taken = the tick of the row
        for(int r = 1; r <= 2; r++){
            string tickText = line[r].substr(0, line[r].find(","));
            string takenText = line[r].substr(line[r].rfind(",") + 1);
            if(tickText != takenText || atoi(takenText.c_str()) < 1) durationOk = false;
        }
        check(durationOk, "the last column (Time Taken in ticks) = arrival tick - creation tick");
        check(line[1].find(",902,3,J6,J3,J6->") != string::npos && csvField(line[1], 5).rfind("->J3") == csvField(line[1], 5).size() - 4, "row 1 (the trip that finished first): id 902, priority 3 (Police), J6 -> J3");
        check(line[2].find(",901,1,J1,J6,J1->") != string::npos && csvField(line[2], 5).rfind("->J6") == csvField(line[2], 5).size() - 4, "row 2: id 901, priority 1 (Ambulance), J1 -> J6, a path from J1 to J6");

        // the path is the one that was REALLY driven: scene 8 re-routes the Ambulance around the closed road J1 - J2
        RoadSim w2;
        TrafficFeeder f2;
        remove(file.c_str());
        buildMultiEVDemo(w2, f2, 8);
        w2.setAuditLog(true);
        w2.setAuditFile(file);
        for(int t = 0; t < 300 && !demoSceneFinished(w2); t++){
            f2.feed(w2);
            w2.tick();
        }
        ifstream in2(file.c_str());
        string header, row;
        getline(in2, header);
        getline(in2, row);
        in2.close();
        check(row.find(",901,1,J1,J6,J1->J3->J4->J6") != string::npos, "the logged path is the detour J1->J3->J4->J6, not the original route");
        remove(file.c_str());

        // a file that cannot be opened: the function reports it, nothing crashes, the simulation goes on
        RoadSim w3;
        w3.setAuditLog(true);
        w3.setAuditFile("no_such_folder_xyz/audit.csv");
        w3.spawnEmergency(903, PRIORITY_AMBULANCE, 1, 3);
        for(int t = 0; t < 100 && w3.getArrivedEV() < 1; t++) w3.tick();
        check(w3.getArrivedEV() == 1 && w3.selfCheck() == 0, "an unwritable file does not stop the trip (error is reported)");
        check(!appendEVAuditRow("no_such_folder_xyz/audit.csv", 1, 1, 1, 1, 2, "J1->J2", 1), "appendEVAuditRow returns false when the file cannot be opened");

        // off by default: nothing is written
        RoadSim w4;
        w4.setAuditFile(file);
        w4.spawnEmergency(904, PRIORITY_AMBULANCE, 1, 3);
        for(int t = 0; t < 100 && w4.getArrivedEV() < 1; t++) w4.tick();
        ifstream in4(file.c_str());
        check(!in4.is_open(), "the audit log is off by default: no file is created");
    }

    cout<<"\n--- audit log after a reset (file emptied) ---"<<endl;
    {
        string file = "audit_wipe_tmp.csv";
        remove(file.c_str());
        check(appendEVAuditRow(file, 5, 901, 1, 1, 6, "J1->J6", 4), "a first row is written");
        ofstream wipe(file.c_str(), ios::trunc);    // what the R key does
        wipe.close();
        check(appendEVAuditRow(file, 9, 902, 2, 2, 5, "J2->J5", 7), "a row after the wipe is written");
        string l[4];
        int n = 0;
        ifstream in(file.c_str());
        while(n < 4 && getline(in, l[n])) n++;
        in.close();
        check(n == 2 && l[0] == "tick,vehicle_id,priority,start,destination,path,Time Taken (Ticks)" && l[1] == "9,902,2,J2,J5,J2->J5,7", "after the wipe only the header and the new row are in the file");
        remove(file.c_str());
    }

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
