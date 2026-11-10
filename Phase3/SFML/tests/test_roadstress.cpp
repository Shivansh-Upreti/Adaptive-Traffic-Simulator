// Stress test for the live-traffic engine (RoadSim): thousands of random operations, many of them
// illegal. After EVERY operation the engine's self-check must find 0 problems. Besides, the test
//   - makes sure the roads never LOCK UP: after the chaos, every vehicle must drain away,
//   - watches every new and delete (leaks, double deletes, dangling pointers).
// BUILD NOTE (MinGW): link with -static (see MemTracker.hpp).
#include "MemTracker.hpp"
#include <iostream>
#include <string>
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

unsigned int seedValue = 1;
int rnd(int n){                         // 0 .. n-1, the same numbers every run
    seedValue = seedValue * 1103515245u + 12345u;
    return (int)((seedValue >> 16) % (unsigned int)n);
}

int main(){
    cout<<"start"<<endl;                // warm up cout before tracking
    tracking = true;

    // ------------------------------------------------------------
    cout<<"\n--- 1: random operations (120 runs x 500 operations) ---"<<endl;
    long ops = 0;
    int badRun = 0;
    int badOp = 0;
    const char* lastOp = "";
    int notDrained = 0;
    long sumArrivedEV = 0, sumReroutes = 0, sumConflicts = 0, sumYields = 0, sumClearance = 0, sumFrozen = 0, sumStranded = 0, sumTimeouts = 0;
    // the roads of the starting map: NUM_ROADS, ROAD_FROM, ROAD_TO (CityMap.hpp)
    for(int run = 1; run <= 120 && badRun == 0; run++){
        seedValue = (unsigned int)run;
        RoadSim w;
        for(int i = 0; i < 500; i++){
            int op = rnd(30);
            if(op < 8){
                w.spawnNormal(1 + rnd(90), rnd(8), rnd(8));              lastOp = "spawnNormal";
            }
            else if(op < 11){
                w.spawnEmergency(900 + rnd(14), rnd(5), rnd(8), rnd(8)); lastOp = "spawnEmergency";
            }
            else if(op < 25){
                int n = 1 + rnd(4);
                for(int t = 0; t < n; t++){
                    w.tick();
                }
                lastOp = "tick";
            }
            else if(op == 25){
                int k = rnd(NUM_ROADS);
                w.closeRoad(ROAD_FROM[k], ROAD_TO[k]);                      lastOp = "closeRoad";
            }
            else if(op == 26){
                int k = rnd(NUM_ROADS);
                w.openRoad(ROAD_FROM[k], ROAD_TO[k], 1 + rnd(12));          lastOp = "openRoad";
            }
            else if(op == 27 && rnd(10) == 0){
                w.reset();                                                lastOp = "reset";
            }
            else{
                // read-only questions with out-of-range arguments must be safe
                w.getSlotRouteJunction(rnd(MAX_MOVERS + 6) - 3, rnd(9) - 2);
                w.isOnShoulder(rnd(MAX_MOVERS + 6) - 3);
                w.isFrozen(rnd(MAX_MOVERS + 6) - 3);
                w.getLaneCell(rnd(9) - 1, rnd(9) - 1, rnd(50) - 5);
                w.getCenterCell(rnd(9) - 1, rnd(9) - 1, rnd(50) - 5);
                w.getRecentEvent(rnd(16) - 3);
                lastOp = "queries";
            }
            ops++;
            if(w.selfCheck() != 0){
                badRun = run;
                badOp = i;
                break;
            }
        }
        if(badRun == 0){
            // LIVENESS: stop adding vehicles. Everything that is still alive must be gone soon
            // (arrived, or removed because it was stranded or gave up). A lock-up would show here.
            int problems = 0;
            bool drained = false;
            for(int t = 0; t < 3000; t++){
                w.tick();
                if(w.selfCheck() != 0){ problems++; break; }
                if(w.getAliveCount() == 0){ drained = true; break; }
            }
            if(problems != 0){ badRun = run; badOp = 9999; lastOp = "drain"; }
            if(!drained && problems == 0){
                notDrained++;
                cout<<"run "<<run<<" did not drain: alive "<<w.getAliveCount()<<" EVs "<<w.getEVCount()<<endl;
            }
            sumArrivedEV += w.getArrivedEV();
            sumReroutes += w.getReroutes();
            sumConflicts += w.getConflicts();
            sumYields += w.getYields();
            sumClearance += w.getClearanceWaits();
            sumFrozen += w.getFrozenTicks();
            sumStranded += w.getStranded();
            sumTimeouts += w.getTimeouts();
        }
    }
    if(badRun != 0){
        cout<<"first bad run: "<<badRun<<", operation "<<badOp<<" ("<<lastOp<<")"<<endl;
    }
    cout<<ops<<" operations done"<<endl;
    cout<<"reached: EV arrivals "<<sumArrivedEV<<", reroutes "<<sumReroutes<<", conflicts "<<sumConflicts<<", yields "<<sumYields
        <<", clearance waits "<<sumClearance<<", frozen vehicle-ticks "<<sumFrozen<<", stranded "<<sumStranded<<", gave up "<<sumTimeouts<<endl;
    check(badRun == 0, "the self-check found 0 problems after every random operation and every drain tick");
    check(notDrained == 0, "after the chaos every vehicle drained away: the roads never locked up");
    check(sumArrivedEV > 100 && sumReroutes > 20 && sumConflicts > 5 && sumYields > 5 && sumClearance > 5 && sumFrozen > 200,
          "the random runs really reached arrivals, reroutes, conflicts, yields, clearance waits and frozen traffic");

    // ------------------------------------------------------------
    cout<<"\n--- 2: 20,000 ticks of heavy traffic with emergency vehicles coming and going ---"<<endl;
    {
        RoadSim w;
        TrafficFeeder f;
        f.setTarget(50);
        seedValue = 777;
        int problems = 0;
        int maxAlive = 0;
        int nextEV = 900;
        for(int t = 0; t < 20000; t++){
            f.feed(w);
            if(rnd(40) == 0){                                  // an emergency vehicle every ~40 ticks
                int a = 1 + rnd(6);
                int b = 1 + rnd(5);
                if(b >= a) b++;
                w.spawnEmergency(nextEV, 1 + rnd(3), a, b);
                nextEV++;
                if(nextEV > 999) nextEV = 900;
            }
            if(rnd(2500) == 0){ w.closeRoad(ROAD_FROM[rnd(NUM_ROADS)], ROAD_TO[0] ); }   // an occasional closed road ...
            if(rnd(1200) == 0){ int k = rnd(NUM_ROADS); w.openRoad(ROAD_FROM[k], ROAD_TO[k], 4 + rnd(4)); }   // ... and reopened ones
            w.tick();
            if(w.selfCheck() != 0){ problems++; break; }
            if(w.getAliveCount() > maxAlive) maxAlive = w.getAliveCount();
        }
        cout<<"created "<<w.getCreated()<<", deleted "<<w.getDeleted()<<", arrived normal "<<w.getArrivedNormal()<<", arrived EV "<<w.getArrivedEV()
            <<", max alive "<<maxAlive<<", gave up "<<w.getTimeouts()<<", stranded "<<w.getStranded()<<endl;
        check(problems == 0, "20,000 ticks: the self-check was clean on every single tick");
        check(w.getCreated() > 3000 && w.getCreated() > 20 * 60 / 2, "thousands of vehicles were created although only 60 slots exist");
        check(maxAlive <= MAX_MOVERS, "never more than 60 vehicles alive");
        check(w.getTimeouts() == 0, "nobody had to give up: the roads never jammed");
    }

    // ------------------------------------------------------------
    cout<<"\n--- 2b: the simulation is destroyed (or reset) while vehicles are in lanes, boxes and waiting lines ---"<<endl;
    {
        int problems = 0;
        for(int round = 0; round < 60; round++){
            RoadSim w;
            TrafficFeeder f;
            buildMultiEVDemo(w, f);
            seedValue = (unsigned int)(5000 + round);
            int stopAt = 1 + rnd(120);
            for(int t = 0; t < stopAt; t++){
                f.feed(w);
                w.tick();
                if(w.selfCheck() != 0){ problems++; break; }
            }
            if(round % 3 == 0){
                w.reset();                                      // sometimes a reset first ...
                problems = problems + w.selfCheck();
                w.spawnNormal(1, 1, 6);
                w.tick();
            }
            // ... and the destructor runs with vehicles still alive
        }
        check(problems == 0, "60 simulations were destroyed or reset at random moments of the demo scene");
    }

    // ------------------------------------------------------------
    tracking = false;
    cout<<"\n--- 3: memory ---"<<endl;
    cout<<"allocations: "<<allocCount<<", frees: "<<freeCount<<", still alive: "<<aliveCount<<", bad frees: "<<badFrees<<endl;
    check(allocCount > 5000, "the test really allocated memory (it is not empty)");
    check(badFrees == 0, "no pointer was deleted twice (or deleted without being allocated)");
    check(aliveCount == 0 && allocCount == freeCount, "every new has a delete: nothing leaked");

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
