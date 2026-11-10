// Stress test: thousands of random operations, many of them illegal (junction 0 or 7,
// used ids, closed roads, full lanes ...). After EVERY operation the engine's self-check
// must find 0 problems. The test also watches every new and delete:
//   - leak:        a pointer that was made with new and never deleted
//   - double free: a delete of a pointer that is not (or no longer) alive
// (AddressSanitizer and valgrind are not available with this MinGW, so we track by hand.)
// BUILD NOTE (MinGW): link this test with -static (CMake does it). The C++ library .dll cannot see the
// counting new/delete below, so without -static strings are missed and optimised builds give wrong counts.
#include <cstdlib>
#include <cstring>
#include <new>
#include <iostream>
#include <string>
using namespace std;

// ------------------------------------------------------------------
// our own new / delete: they behave like the normal ones, but write down every pointer
// ------------------------------------------------------------------
const int MAX_TRACKED = 60000;
static void* alivePtr[MAX_TRACKED];     // every pointer that new gave out and delete has not taken back
static size_t aliveSize[MAX_TRACKED];   // and how many bytes each one has
static int aliveCount = 0;
static long allocCount = 0;
static long freeCount = 0;
static long badFrees = 0;               // deletes of a pointer that was not alive
static bool tracking = false;           // only the code between start and stop is watched

void* operator new(size_t size){
    void* p = malloc(size);
    if(p == nullptr){
        throw bad_alloc();
    }
    if(tracking){
        allocCount++;
        if(aliveCount < MAX_TRACKED){
            alivePtr[aliveCount] = p;
            aliveSize[aliveCount] = size;
            aliveCount++;
        }
    }
    return p;
}

// DANGLING POINTER DETECTOR: while tracking, a deleted block is filled with 0xDD and is
// never given back to the system. A pointer that still points at deleted memory (a
// dangling pointer) then reads 0xDDDDDDDD instead of its old value, which makes the
// bug show up at once (wrong numbers, crash) instead of "working by luck".
static void forget(void* p){
    if(p == nullptr){
        return;
    }
    if(tracking){
        for(int i = aliveCount - 1; i >= 0; i--){       // newest pointers are most likely, so search from the end
            if(alivePtr[i] == p){
                memset(p, 0xDD, aliveSize[i]);          // poison the block, but do NOT free() it
                alivePtr[i] = alivePtr[aliveCount - 1]; // fill the hole with the last entry
                aliveSize[i] = aliveSize[aliveCount - 1];
                aliveCount--;
                freeCount++;
                return;
            }
        }
        badFrees++;                                     // not alive: double delete (or never allocated while tracking)
        return;                                         // do NOT free it again
    }
    free(p);
}
void operator delete(void* p) noexcept{ forget(p); }
void operator delete(void* p, size_t) noexcept{ forget(p); }

#include "../../core/include/Simulator.hpp"
#include "../include/ViewModel.hpp"

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

// a small random number generator, so every run is the same
unsigned int seedValue = 1;
int rnd(int n){                         // 0 .. n-1
    seedValue = seedValue * 1103515245u + 12345u;
    return (int)((seedValue >> 16) % (unsigned int)n);
}

// one random operation on the engine. returns a short name so a failure can be reproduced
// (the roads of the starting map come from CityMap.hpp: NUM_ROADS, ROAD_FROM, ROAD_TO)

const char* randomEngineOp(Simulator &sim){
    // Closing a REAL road while a trip is running is what makes reroutes and stopped trips
    // happen. Pure random junction pairs almost never hit a real road, so help a little.
    if(sim.isDispatchActive() && rnd(3) == 0){
        int k = rnd(NUM_ROADS);
        sim.closeRoad(ROAD_FROM[k], ROAD_TO[k]);
        return "closeRoad (during a trip)";
    }
    if(!sim.isDispatchActive() && rnd(9) == 0){
        int k = rnd(NUM_ROADS);
        sim.openRoad(ROAD_FROM[k], ROAD_TO[k], 1 + rnd(9));
        return "openRoad (between trips)";
    }

    if(rnd(120) == 0){
        sim.reset();                    // a fresh start in the middle of anything (the R key)
        return "reset";
    }

    int op = rnd(17);
    if(op <= 2){ sim.spawnNormal(1 + rnd(70), rnd(8)); return "spawnNormal"; }
    if(op == 3){ sim.spawnEmergency(1 + rnd(70), rnd(5), rnd(8), rnd(8)); return "spawnEmergency"; }
    if(op <= 5){ sim.startDispatch(); return "startDispatch"; }
    if(op <= 9){ sim.stepDispatch(); return "stepDispatch"; }
    if(op <= 11){ sim.moveNormal(rnd(8), rnd(8)); return "moveNormal"; }
    if(op == 12){ sim.departNormal(rnd(8)); return "departNormal"; }
    if(op == 13){ sim.closeRoad(rnd(8), rnd(8)); return "closeRoad"; }
    if(op == 14){ sim.openRoad(rnd(8), rnd(8), rnd(103) - 1); return "openRoad"; }
    // read-only questions with out-of-range arguments must be safe as well
    sim.getRouteJunction(rnd(12) - 3);
    sim.getRecentEvent(rnd(14) - 3);
    sim.getLaneVehicle(rnd(9) - 1, rnd(45) - 4);
    sim.getSideVehicle(rnd(9) - 1, rnd(45) - 4);
    sim.getLaneLength(rnd(9) - 1);
    sim.isPreempted(rnd(9) - 1);
    return "queries";
}

int main(){
    cout<<"start"<<endl;                // warm up cout before tracking
    tracking = true;

    // ------------------------------------------------------------
    cout<<"\n--- 1: random operations on the engine (300 runs x 300 operations) ---"<<endl;
    int badRun = 0;
    const char* lastOp = "";
    int badOpNumber = 0;
    long opsDone = 0;
    // coverage: how often did the random runs really reach the interesting situations?
    long seenArrived = 0, seenReroute = 0, seenStopped = 0, seenBlocked = 0, seenNoRoute = 0, seenSignal = 0, seenEvac = 0, seenBypass = 0;
    for(int run = 1; run <= 300 && badRun == 0; run++){
        seedValue = (unsigned int)run;
        Simulator sim;
        for(int i = 0; i < 300; i++){
            int eventsBefore = sim.getEventCount();
            lastOp = randomEngineOp(sim);
            opsDone++;

            int fresh = sim.getEventCount() - eventsBefore;     // events written by this operation (at most 8 are kept)
            if(fresh > LOG_SIZE) fresh = LOG_SIZE;
            for(int k = 0; k < fresh; k++){
                string e = sim.getRecentEvent(k);
                if(e.find("[ARRIVED]") == 0) seenArrived++;
                if(e.find("[REROUTE]") == 0) seenReroute++;
                if(e.find("[STOPPED]") == 0) seenStopped++;
                if(e.find("[BLOCKED]") == 0) seenBlocked++;
                if(e.find("[NO ROUTE]") == 0) seenNoRoute++;
                if(e.find("[SIGNAL] J") == 0 && e.find("preempted") != string::npos) seenSignal++;
                if(e.find("[EVACUATION]") == 0 && e.find("pulled over") != string::npos) seenEvac++;
                if(e.find("[BYPASS]") == 0) seenBypass++;
            }

            if(sim.selfCheck() != 0){
                badRun = run;
                badOpNumber = i;
                break;
            }
        }
    }
    if(badRun != 0){
        cout<<"first bad run: "<<badRun<<", operation number "<<badOpNumber<<" ("<<lastOp<<")"<<endl;
    }
    cout<<opsDone<<" operations done"<<endl;
    cout<<"reached: arrivals "<<seenArrived<<", reroutes "<<seenReroute<<", stopped trips "<<seenStopped<<", blocked roads "<<seenBlocked
        <<", no-route "<<seenNoRoute<<", signal holds "<<seenSignal<<", evacuations "<<seenEvac<<", bypasses "<<seenBypass<<endl;
    check(badRun == 0, "the self-check found 0 problems after every one of the random operations");
    check(seenArrived > 50 && seenReroute > 10 && seenStopped > 10 && seenSignal > 50 && seenEvac > 50 && seenBypass > 50,
          "the random runs really reached arrivals, reroutes, stopped trips, signal holds, evacuations and bypasses");

    // ------------------------------------------------------------
    cout<<"\n--- 2: random key presses and frames on the view logic (200 runs x 300 steps) ---"<<endl;
    int badView = 0;
    int badViewStep = 0;
    const char* viewProblem = "";
    long viewSteps = 0;
    for(int run = 1; run <= 200 && badView == 0; run++){
        seedValue = (unsigned int)(1000 + run);
        Simulator sim;
        ViewModel vm;
        for(int i = 0; i < 300; i++){
            int op = rnd(16);
            if(op == 0) vm.selectStart(rnd(9) - 1);
            else if(op == 1) vm.selectDest(rnd(9) - 1);
            else if(op == 2) vm.actionSpawnNormal(sim);
            else if(op == 3) vm.actionSpawnEmergency(sim, rnd(5));
            else if(op == 4) vm.actionDispatch(sim);
            else if(op == 5) vm.actionMoveNormal(sim);
            else if(op == 6) vm.actionLeave(sim);
            else if(op == 7) vm.actionToggleRoad(sim);
            else if(op == 8 && rnd(12) == 0) vm.actionDemo(sim);
            else if(op == 9 && rnd(10) == 0){
                sim.reset();            // the R key: the simulator and the window forget everything together
                vm.reset();
            }
            else if(op == 9) vm.actionSelfCheck(sim);
            else{
                int frames = 1 + rnd(4);
                for(int f = 0; f < frames; f++){
                    vm.update(sim, (float)rnd(700) / 1000.0f);      // 0.0 .. 0.7 seconds
                }
            }
            viewSteps++;

            viewProblem = "";
            if(sim.selfCheck() != 0) viewProblem = "self-check";
            else if(vm.state < VM_IDLE || vm.state > VM_TRAVEL) viewProblem = "state out of range";
            else if(vm.travelT < 0.0f || vm.travelT > 1.0f) viewProblem = "travelT outside 0..1";
            else if(vm.state == VM_TRAVEL && (!validIndex(vm.travelFrom) || !validIndex(vm.travelTo))) viewProblem = "travelling on a road that does not exist";
            else if(vm.state != VM_IDLE && !sim.isDispatchActive() && vm.state == VM_DWELL) viewProblem = "waiting at a junction with no trip";
            if(viewProblem[0] != '\0'){
                badView = run;
                badViewStep = i;
                break;
            }
        }
    }
    if(badView != 0){
        cout<<"first bad run: "<<badView<<", step "<<badViewStep<<": "<<viewProblem<<endl;
    }
    cout<<viewSteps<<" steps done"<<endl;
    check(badView == 0, "view logic stayed consistent after every random key press and frame");

    // ------------------------------------------------------------
    tracking = false;
    cout<<"\n--- 3: memory ---"<<endl;
    cout<<"allocations: "<<allocCount<<", frees: "<<freeCount<<", still alive: "<<aliveCount<<", bad frees: "<<badFrees<<endl;
    check(allocCount > 1000, "the test really allocated memory (it is not empty)");
    check(badFrees == 0, "no pointer was deleted twice (or deleted without being allocated)");
    check(aliveCount == 0 && allocCount == freeCount, "every new has a delete: nothing leaked");

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
