// Tests for the new phase 3 features: priority order, empty-junction bypass,
// signal preemption, two-lane evacuation, rerouting, closed roads and the self-check.
#include <iostream>
#include <string>
using namespace std;
#include "../../core/include/Simulator.hpp"

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

// runs the active trip until it ends
void runTrip(Simulator &sim){
    int guard = 0;
    while(sim.stepDispatch() && guard < 50){
        guard++;
    }
}

// number of vehicles standing in main lanes
int totalInLanes(Simulator &sim){
    int total = 0;
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        total = total + sim.getLaneLength(j) + sim.getSideLength(j);
    }
    return total;
}

bool anyHold(Simulator &sim){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        if(sim.isPreempted(j)) return true;
        if(sim.getSideLength(j) > 0) return true;
    }
    return false;
}

int main(){
    cout<<"--- TrafficSignal counter ---"<<endl;
    {
        TrafficSignal s;
        check(!s.release(), "release without a hold is refused (counter never goes below 0)");
        check(s.allowsNormalTraffic(), "a new signal lets normal traffic go");
        s.preempt();
        s.preempt();
        check(s.getHolds() == 2 && !s.allowsNormalTraffic(), "two emergency vehicles: two holds, normal traffic paused");
        check(s.release() && s.isPreempted(), "one release: still held by the other vehicle");
        check(s.release() && !s.isPreempted() && s.allowsNormalTraffic(), "second release: back to normal");
    }

    cout<<"\n--- Graph::removeRoad (first, middle, last edge of a list) ---"<<endl;
    {
        Graph g(6);
        g.addRoad(1, 2, 5);
        g.addRoad(2, 4, 4);
        g.addRoad(2, 6, 6);          // J2's list is now: J1, J4, J6
        check(g.removeRoad(2, 4), "remove the MIDDLE road of J2's list");
        check(!g.hasRoad(2, 4) && !g.hasRoad(4, 2), "the road is gone in both directions");
        check(g.hasRoad(2, 1) && g.hasRoad(2, 6), "the other roads of J2 are still there");
        check(g.removeRoad(2, 6), "remove the LAST road of J2's list");
        check(g.removeRoad(2, 1), "remove the FIRST (and now only) road of J2's list");
        check(!g.removeRoad(2, 1), "closing a road that is already closed gives false");
        check(!g.removeRoad(0, 1) && !g.removeRoad(1, 7), "bad junction numbers give false");
        g.addRoad(2, 6, 9);
        check(g.roadLength(2, 6) == 9 && g.roadLength(6, 2) == 9, "a road can be opened again");
        check(g.roadLength(1, 6) == 0, "roadLength is 0 where there is no road");
        int dist[7] = {0};
        int prev[7] = {0};
        g.dijkstra(1, dist, prev);
        check(dist[2] == NO_PATH, "after closing, J2 is cut off from J1 (dijkstra sees it)");
    }

    cout<<"\n--- spawn checks: nothing is half-created ---"<<endl;
    {
        Simulator sim;
        check(!sim.spawnNormal(0, 1) && !sim.spawnNormal(10000, 1), "vehicle id 0 and 10000 are refused");
        check(!sim.spawnNormal(101, 0) && !sim.spawnNormal(101, 7), "junction 0 and 7 are refused");
        check(sim.getFleetCount() == 0, "refused spawns created nothing");
        check(sim.spawnNormal(101, 1), "a valid vehicle is created");
        check(!sim.spawnNormal(101, 2), "the same id again is refused");
        check(!sim.spawnEmergency(102, 0, 1, 6) && !sim.spawnEmergency(102, 4, 1, 6), "emergency type 0 and 4 are refused");
        check(!sim.spawnEmergency(102, 1, 3, 3), "start equal to destination is refused");
        check(!sim.spawnEmergency(102, 1, 1, 7), "destination J7 is refused");
        check(sim.getFleetCount() == 1 && sim.getPendingCount() == 0, "refused emergency spawns created nothing");
        check(sim.spawnEmergency(102, 1, 1, 6), "a valid emergency vehicle is created");
        check(!sim.spawnEmergency(102, 1, 1, 6), "an emergency vehicle with a used id is refused");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- lane full (32) and fleet full (40) ---"<<endl;
    {
        Simulator sim;
        bool allOk = true;
        for(int k = 0; k < 32; k++){
            if(!sim.spawnNormal(1000 + k, 3)) allOk = false;
        }
        check(allOk && sim.getLaneLength(3) == 32, "32 vehicles fit at J3");
        check(!sim.spawnNormal(2000, 3), "the 33rd vehicle at J3 is refused");
        check(!sim.spawnEmergency(2001, 1, 3, 6), "an emergency vehicle cannot start in a full lane");
        check(sim.getFleetCount() == 32 && sim.getPendingCount() == 0, "nothing was half-created");
        for(int k = 0; k < 8; k++){
            sim.spawnNormal(3000 + k, 4);
        }
        check(sim.getFleetCount() == 40, "the fleet holds 40 vehicles");
        check(!sim.spawnNormal(4000, 5), "the 41st vehicle is refused (fleet full)");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- the fleet-full bug: a vehicle that leaves or arrives gives its slot back ---"<<endl;
    {
        Simulator sim;
        for(int k = 0; k < 32; k++){
            sim.spawnNormal(1000 + k, 3);
        }
        for(int k = 0; k < 8; k++){
            sim.spawnNormal(3000 + k, 4);
        }
        check(sim.getFleetCount() == 40 && !sim.spawnNormal(4000, 5), "40 vehicles are on the road and the 41st is refused");

        check(sim.departNormal(3), "the first vehicle at J3 leaves the network");
        check(sim.getFleetCount() == 39, "it was deleted: 39 vehicles are left");
        check(!sim.idInUse(1000), "its id is free again");
        check(sim.spawnNormal(4000, 5), "now a new vehicle fits (this is what failed before)");
        check(sim.spawnNormal(1000, 5) == false, "(40 on the road again, so the next one is refused)");
        sim.departNormal(3);
        check(sim.spawnNormal(1000, 5), "the id 1000 of the vehicle that left can be used again");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- continuous running: 400 trips with only 40 slots ---"<<endl;
    {
        Simulator sim;
        for(int k = 0; k < 6; k++){
            sim.spawnNormal(100 + k, 1 + (k % 6));          // 6 vehicles that stay
        }
        int baseline = sim.getFleetCount();
        bool allArrived = true;
        bool alwaysBack = true;
        int maxSeen = 0;
        for(int trip = 0; trip < 400; trip++){
            int from = 1 + (trip % 6);
            int to = 1 + ((trip + 3) % 6);
            sim.spawnEmergency(900 + (trip % 10), 1 + (trip % 3), from, to);
            if(sim.getFleetCount() > maxSeen) maxSeen = sim.getFleetCount();
            sim.startDispatch();
            runTrip(sim);
            if(!sim.wasLastTripArrived()) allArrived = false;
            if(sim.getFleetCount() != baseline) alwaysBack = false;
        }
        check(allArrived, "all 400 emergency trips arrived");
        check(alwaysBack, "after every arrival the vehicle count was back to the 6 that stay");
        check(maxSeen == baseline + 1, "never more than 7 vehicles at a time, although 406 were created in total");
        check(sim.selfCheck() == 0 && !anyHold(sim), "self-check is clean, nothing held");
        for(int k = 0; k < 6; k++){
            sim.departNormal(1 + (k % 6));
        }
        check(sim.getFleetCount() == 0, "when the last vehicles leave, the fleet is empty");
    }

    cout<<"\n--- priority order: 1 before 2 before 3, same level = earlier arrival ---"<<endl;
    {
        Simulator sim;
        sim.spawnEmergency(301, 3, 1, 6);    // police,    arrives first
        sim.spawnEmergency(302, 2, 1, 6);    // fire truck
        sim.spawnEmergency(303, 1, 1, 6);    // ambulance
        sim.spawnEmergency(304, 1, 1, 6);    // ambulance, arrives after 303
        int expected[4] = {303, 304, 302, 301};
        bool orderOk = true;
        bool allArrived = true;
        for(int i = 0; i < 4; i++){
            if(sim.startDispatch() != DISPATCH_STARTED) { orderOk = false; break; }
            if(sim.getActiveVehicleID() != expected[i]) orderOk = false;
            runTrip(sim);
            if(!sim.wasLastTripArrived()) allArrived = false;
        }
        check(orderOk, "dispatch order is 303, 304 (tie: earlier first), 302, 301");
        check(allArrived, "all four vehicles arrived");
        check(sim.startDispatch() == DISPATCH_NOTHING, "dispatch with no request waiting is handled");
        check(sim.selfCheck() == 0 && !anyHold(sim), "self-check clean, no signal held, no side lane in use");
    }

    cout<<"\n--- request queue full (10) ---"<<endl;
    {
        Simulator sim;
        bool allOk = true;
        for(int k = 0; k < 10; k++){
            if(!sim.spawnEmergency(400 + k, 1, 1 + (k % 5), 6)) allOk = false;
        }
        check(allOk && sim.getPendingCount() == 10, "10 requests are accepted");
        check(!sim.spawnEmergency(500, 1, 1, 6), "the 11th request is refused");
        check(sim.getFleetCount() == 10 && totalInLanes(sim) == 10, "the refused spawn left no vehicle behind");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- empty junctions: bypass, no signal request ---"<<endl;
    {
        Simulator sim;
        sim.spawnEmergency(501, 1, 1, 6);
        check(sim.startDispatch() == DISPATCH_STARTED, "dispatch starts");
        runTrip(sim);
        check(sim.wasLastTripArrived(), "the ambulance arrived");
        check(sim.getBypassCount() == 3, "J1, J2 and J6 were all empty: 3 bypasses");
        check(sim.getPreemptCount() == 0 && sim.getEvacuatedCount() == 0, "no signal request and no evacuation");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- signal preemption and two-lane evacuation (step by step) ---"<<endl;
    {
        Simulator sim;
        sim.spawnNormal(101, 2);
        sim.spawnNormal(121, 2);
        sim.spawnNormal(131, 2);
        sim.spawnNormal(151, 1);
        sim.spawnEmergency(601, 1, 1, 6);       // route J1 -> J2 -> J6

        check(sim.startDispatch() == DISPATCH_STARTED, "dispatch starts");
        check(sim.isPreempted(1), "J1 had waiting traffic: signal preempted");
        check(sim.getSideLength(1) == 1 && sim.getLaneLength(1) == 1 && sim.getLaneVehicle(1, 0) == 601,
              "J1: the normal vehicle pulled over, the ambulance is alone at the front");

        check(sim.stepDispatch(), "step 1: J1 -> J2");
        check(!sim.isPreempted(1) && sim.getSideLength(1) == 0 && sim.getLaneVehicle(1, 0) == 151,
              "J1 is back to normal and vehicle 151 is back in its lane");
        check(sim.isPreempted(2) && sim.getSideLength(2) == 3, "J2: signal preempted, 3 vehicles pulled over");
        check(sim.getLaneLength(2) == 1 && sim.getLaneVehicle(2, 0) == 601, "J2: the ambulance is at the front");
        check(sim.getCurrentJunction() == 2 && sim.getNextJunction() == 6, "the engine reports J2 -> J6");
        check(sim.getRouteJunction(0) == 2 && sim.getRouteJunction(1) == 6 && sim.getRouteJunction(2) == 0,
              "the remaining route is J2, J6 and then nothing");
        check(sim.getRouteJunction(-1) == 0, "a negative route position gives 0");

        check(!sim.moveNormal(2, 4), "normal traffic at J2 is refused while the signal is held");
        check(!sim.departNormal(2), "a normal vehicle cannot leave J2 either");
        check(sim.getSideLength(2) == 3, "the refusals changed nothing");

        check(!sim.stepDispatch(), "step 2: J2 -> J6, and the trip is over");
        check(sim.wasLastTripArrived() && !sim.isDispatchActive(), "the ambulance arrived");
        check(sim.getLaneLength(2) == 3 && sim.getLaneVehicle(2, 0) == 101 && sim.getLaneVehicle(2, 1) == 121
              && sim.getLaneVehicle(2, 2) == 131, "the pulled-over vehicles came back in the SAME order (101, 121, 131)");
        check(!anyHold(sim), "no signal held, no side lane in use");
        check(sim.getPreemptCount() == 2 && sim.getBypassCount() == 1 && sim.getEvacuatedCount() == 4,
              "2 signal requests (J1, J2), 1 bypass (J6), 4 vehicles pulled over");
        check(sim.moveNormal(2, 4), "normal traffic moves again once the ambulance has passed");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- a road closed in the middle of the trip: reroute ---"<<endl;
    {
        Simulator sim;
        sim.spawnEmergency(701, 1, 1, 6);
        sim.startDispatch();
        check(sim.stepDispatch() && sim.getCurrentJunction() == 2, "the ambulance is at J2");
        check(sim.closeRoad(2, 6), "road J2 - J6 is closed");
        check(!sim.closeRoad(2, 6), "closing it again is refused");
        check(sim.stepDispatch() && sim.isDispatchActive() && sim.getCurrentJunction() == 2,
              "the blocked step found a new route and the trip goes on");
        check(sim.getNextJunction() == 4, "new route goes through J4 (J2 -> J4 -> J6)");
        runTrip(sim);
        check(sim.wasLastTripArrived(), "the ambulance still arrived");
        check(sim.selfCheck() == 0 && !anyHold(sim), "self-check clean, nothing held");
    }

    cout<<"\n--- no route at the start ---"<<endl;
    {
        Simulator sim;
        sim.closeRoad(2, 6);
        sim.closeRoad(4, 6);
        sim.closeRoad(5, 6);
        sim.spawnEmergency(801, 1, 1, 6);
        check(sim.startDispatch() == DISPATCH_FAILED, "dispatch to a cut-off junction fails cleanly");
        check(!sim.isDispatchActive() && !sim.stepDispatch(), "no trip is running");
        check(sim.getLaneLength(1) == 1, "the vehicle stays where it was");
        check(sim.selfCheck() == 0 && !anyHold(sim), "self-check clean, nothing held");
        check(sim.openRoad(2, 6, 7), "the road is opened again");
        sim.spawnEmergency(802, 1, 1, 6);
        check(sim.startDispatch() == DISPATCH_STARTED, "a new request can be dispatched now");
        runTrip(sim);
        check(sim.wasLastTripArrived(), "and it arrives");
        check(!sim.openRoad(1, 1, 5) && !sim.openRoad(1, 7, 5) && !sim.openRoad(1, 2, 0) && !sim.openRoad(1, 2, 100),
              "bad roads are refused (same junction, J7, length 0, length 100)");
    }

    cout<<"\n--- a trip that gets stuck in the middle gives everything back ---"<<endl;
    {
        Simulator sim;
        sim.spawnNormal(111, 2);
        sim.spawnEmergency(901, 1, 1, 6);
        sim.startDispatch();
        sim.stepDispatch();                          // ambulance at J2, vehicle 111 pulled over
        check(sim.isPreempted(2) && sim.getSideLength(2) == 1, "before: J2 held, 111 in the side lane");
        sim.closeRoad(2, 6);
        sim.closeRoad(4, 6);
        sim.closeRoad(5, 6);
        check(!sim.stepDispatch(), "the trip stops: no way to J6");
        check(!sim.isDispatchActive() && !sim.wasLastTripArrived(), "the trip is over, without arriving");
        check(!sim.isPreempted(2) && sim.getSideLength(2) == 0, "J2 signal released, side lane empty");
        check(sim.getLaneLength(2) == 2, "vehicle 111 and the parked ambulance are both in the J2 lane");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- normal vehicles ---"<<endl;
    {
        Simulator sim;
        sim.spawnNormal(11, 1);
        check(!sim.moveNormal(1, 6), "no road J1 - J6: refused");
        check(sim.moveNormal(1, 2), "J1 -> J2 works");
        check(!sim.moveNormal(1, 2), "nothing left at J1: refused");
        check(!sim.moveNormal(1, 1) && !sim.moveNormal(0, 2) && !sim.moveNormal(1, 7), "bad junctions refused");

        sim.spawnEmergency(21, 1, 3, 6);             // parked emergency vehicle, not dispatched
        sim.spawnNormal(31, 3);                      // lane J3 is now [21, 31]
        check(sim.moveNormal(3, 4), "a normal vehicle behind a parked ambulance can still move");
        check(sim.getLaneLength(3) == 1 && sim.getLaneVehicle(3, 0) == 21, "the parked ambulance did not move");
        check(!sim.departNormal(3), "only the parked ambulance is left at J3: no normal vehicle to leave");
        check(sim.departNormal(4), "the vehicle at J4 leaves the network");
        check(sim.selfCheck() == 0, "self-check is clean");

        for(int k = 0; k < 32; k++){
            sim.spawnNormal(100 + k, 5);
        }
        sim.spawnNormal(13, 4);                      // a normal vehicle waits at J4, and J4 - J5 is a road
        check(sim.getLaneLength(5) == 32, "J5 is full");
        check(!sim.moveNormal(4, 5), "moving into the full lane of J5 is refused");
        check(sim.getLaneLength(5) == 32 && sim.getLaneLength(4) == 1, "the vehicle stayed at J4, J5 did not change");
    }

    cout<<"\n--- the self-check really finds problems ---"<<endl;
    {
        Simulator sim;
        TrafficFlow &t = sim.getTraffic();           // the lanes themselves: we damage them on purpose
        sim.spawnNormal(5, 1);
        check(sim.selfCheck() == 0, "clean state: 0 problems");
        t.vehicleArrive(3, 999);                     // a lane entry for a vehicle that does not exist
        check(sim.selfCheck() >= 1, "an unknown vehicle id in a lane is detected");
        t.laneRemove(3, 999);
        t.vehicleArrive(4, 5);                       // vehicle 5 in two lanes at once
        check(sim.selfCheck() >= 1, "a vehicle standing in two lanes is detected");
        t.laneRemove(4, 5);
        t.preempt(5);                                // a signal held with no emergency vehicle on the road
        check(sim.selfCheck() >= 1, "a signal held for nobody is detected");
        t.release(5);
        t.evacuateToSide(1, -1);                     // vehicle 5 pulled into a side lane with no trip running
        check(sim.selfCheck() >= 1, "pulled-over vehicles with no trip running are detected");
        t.restoreFromSide(1);
        check(sim.selfCheck() == 0, "after repairing, 0 problems again");
    }

    cout<<"\n--- two simulators at the same time do not disturb each other (phase 2 style globals did) ---"<<endl;
    {
        Simulator a;
        a.spawnNormal(1, 2);
        a.spawnNormal(2, 2);
        {
            Simulator b;                             // with global lanes this used to empty A's lanes
            check(a.getLaneLength(2) == 2, "creating a second simulator leaves the first one's lanes alone");
            check(b.getLaneLength(2) == 0 && b.getFleetCount() == 0, "the second simulator starts empty");
            check(b.spawnNormal(1, 3), "the same vehicle id can be used in the second simulator");
            check(a.getLaneLength(3) == 0 && b.getLaneLength(3) == 1, "a vehicle of B does not appear in A");
            b.spawnNormal(7, 1);
            b.spawnEmergency(9, 1, 1, 6);
            b.startDispatch();
            check(a.getPendingCount() == 0 && !a.isDispatchActive() && b.isDispatchActive(), "a trip in B does not touch A");
            check(b.isPreempted(1) && !a.isPreempted(1), "the signal of B is held, the signal of A is not");
        }
        check(a.getLaneLength(2) == 2 && a.selfCheck() == 0, "after B is destroyed, A is unchanged and its self-check is clean");
    }

    cout<<"\n--- reset(): a fresh start without deleting the simulator ---"<<endl;
    {
        Simulator sim;
        sim.spawnNormal(101, 2);
        sim.spawnNormal(102, 2);
        sim.spawnEmergency(901, 1, 1, 6);
        sim.spawnEmergency(902, 2, 3, 6);
        sim.closeRoad(4, 6);
        sim.startDispatch();
        sim.stepDispatch();                          // stops half way: a hold, pulled-over vehicles, a route list
        check(sim.isDispatchActive() && sim.isPreempted(2), "before: a trip is running and J2 is held");

        sim.reset();
        check(!sim.isDispatchActive() && sim.getFleetCount() == 0 && sim.getPendingCount() == 0, "after reset: no trip, no vehicles, no requests");
        check(!anyHold(sim) && totalInLanes(sim) == 0, "after reset: no signal held, all lanes empty");
        check(sim.hasRoad(4, 6) && sim.roadLength(4, 6) == 5, "after reset: the closed road is back (starting map)");
        check(sim.getEventCount() == 0 && sim.getRecentEvent(0) == "", "after reset: the event log is empty");
        check(sim.getPreemptCount() == 0 && sim.getBypassCount() == 0 && sim.getEvacuatedCount() == 0, "after reset: the counters are 0");
        check(sim.getCurrentJunction() == 0 && sim.getActiveVehicleID() == -1 && sim.getRouteJunction(0) == 0, "after reset: nothing is active");
        check(sim.selfCheck() == 0, "after reset: self-check is clean");

        check(sim.spawnNormal(101, 2), "after reset: id 101 can be used again");
        check(sim.spawnEmergency(901, 1, 1, 6), "after reset: a new emergency vehicle can be created");
        check(sim.startDispatch() == DISPATCH_STARTED, "after reset: a trip can start");
        runTrip(sim);
        check(sim.wasLastTripArrived() && sim.selfCheck() == 0, "after reset: the trip arrives and the self-check is clean");
        sim.reset();
        sim.reset();                                 // twice in a row must be harmless
        check(sim.getFleetCount() == 0 && sim.selfCheck() == 0, "reset twice in a row is harmless");
    }

    cout<<"\n--- Graph::clear and ArrayPriorityQueue::clear ---"<<endl;
    {
        Graph g(6);
        g.addRoad(1, 2, 5);
        g.addRoad(2, 3, 5);
        g.clear();
        check(!g.hasRoad(1, 2) && !g.hasRoad(2, 3), "clear removes every road");
        g.addRoad(1, 2, 7);
        check(g.hasRoad(2, 1) && g.roadLength(1, 2) == 7, "roads can be added again after clear");
        g.clear();
        g.clear();                                   // twice in a row must be harmless (no dangling head pointers)
        check(!g.hasRoad(1, 2), "clear twice in a row is harmless");

        ArrayPriorityQueue q;
        EmergencyRequest r = {1, 1, 0, "A", 1, 2};
        q.push(r);
        q.push(r);
        q.clear();
        check(q.isEmpty() && q.top().vehicleID == -1, "clear empties the priority queue");
    }

    cout<<"\n--- the stop message names the right junction ---"<<endl;
    {
        Simulator sim;
        sim.spawnEmergency(901, 1, 1, 6);
        sim.startDispatch();
        sim.stepDispatch();                          // at J2
        sim.closeRoad(2, 6);
        sim.closeRoad(4, 6);
        sim.closeRoad(5, 6);
        sim.stepDispatch();                          // blocked, no other route: stopped
        bool named = false;
        for(int k = 0; k < LOG_SIZE; k++){
            string e = sim.getRecentEvent(k);
            if(e.find("[STOPPED]") == 0 && e.find("stopped at J2") != string::npos) named = true;
        }
        check(named, "[STOPPED] says the vehicle stopped at J2");
    }

    cout<<"\n--- TrafficFlow refuses bad junction numbers everywhere (no array can be reached) ---"<<endl;
    {
        TrafficFlow f;
        bool allRefused = true;
        int bad[5] = {-1, 0, 7, 8, 1000};
        for(int i = 0; i < 5; i++){
            int j = bad[i];
            if(f.vehicleArrive(j, 1)) allRefused = false;
            if(f.laneLength(j) != 0 || f.sideLength(j) != 0) allRefused = false;
            if(f.laneAt(j, 0) != -1 || f.sideAt(j, 0) != -1) allRefused = false;
            if(f.laneContains(j, 1) || f.laneFull(j) || f.laneRemove(j, 1)) allRefused = false;
            if(f.isPreempted(j) || f.release(j) || f.holdCount(j) != 0) allRefused = false;
            if(f.evacuateToSide(j, 1) || f.restoreFromSide(j) != 0 || f.waitingExcept(j, 1) != 0) allRefused = false;
            f.preempt(j);
            if(f.congestionLevel(j) != "INVALID") allRefused = false;
        }
        check(allRefused, "junction numbers -1, 0, 7, 8 and 1000 are refused by every function and change nothing");
        bool untouched = true;
        for(int j = 1; j <= NUM_JUNCTIONS; j++){
            if(f.laneLength(j) != 0 || f.holdCount(j) != 0) untouched = false;
        }
        check(untouched, "and all six real junctions are still empty and unheld");
    }

    cout<<"\n--- event log (used by the SFML window) ---"<<endl;
    {
        Simulator sim;
        int before = sim.getEventCount();
        sim.spawnNormal(1, 1);
        sim.spawnNormal(1, 1);                       // refused, but still logged
        check(sim.getEventCount() == before + 2, "every action is logged, refused ones too");
        check(sim.getRecentEvent(0).find("[ERROR]") == 0, "newest event first");
        check(sim.getRecentEvent(1).find("[SPAWN]") == 0, "then the one before it");
        check(sim.getRecentEvent(50) == "", "asking for an event that does not exist gives empty text");
        sim.addNote("[INFO] hello");
        check(sim.getRecentEvent(0) == "[INFO] hello", "addNote writes a message into the event log");
        check(sim.getRouteJunction(0) == 0, "with no trip running there is no route");
    }

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
