// Memory-leak test without any extra tool.
// AddressSanitizer and valgrind are not available with this MinGW, so we count by hand:
// every "new" adds 1 to allocCount, every "delete" adds 1 to freeCount.
// After a whole scenario both numbers must be equal.
// BUILD NOTE (MinGW): link this test with -static (CMake does it). The C++ library .dll cannot see the
// counting new/delete below, so without -static strings are missed and optimised builds give wrong counts.
#include <cstdlib>
#include <new>
#include <iostream>
using namespace std;

// GCC sees "free() on memory from operator new" once it inlines our counting new/delete into the
// code under test. That is a false alarm here (our new really uses malloc), so it is switched off
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"

static long allocCount = 0;
static long freeCount = 0;

// our own new/delete: the same as the normal ones, but they count
void* operator new(size_t size){
    allocCount++;
    void* p = malloc(size);
    if(p == nullptr){
        throw bad_alloc();
    }
    return p;
}
void operator delete(void* p) noexcept{
    if(p != nullptr){
        freeCount++;
        free(p);
    }
}
void operator delete(void* p, size_t) noexcept{
    if(p != nullptr){
        freeCount++;
        free(p);
    }
}

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

void runTrip(Simulator &sim){
    int guard = 0;
    while(sim.stepDispatch() && guard < 50){
        guard++;
    }
}

int main(){
    cout<<"start"<<endl;          // warm up cout before counting

    cout<<"\n--- scenario 1: normal use, trips, closed road, reroute ---"<<endl;
    long a1 = allocCount;
    long f1 = freeCount;
    {
        Simulator sim;
        for(int k = 0; k < 10; k++){
            sim.spawnNormal(100 + k, 1 + (k % 6));
        }
        sim.spawnEmergency(201, 1, 1, 6);
        sim.spawnEmergency(202, 2, 3, 5);
        sim.spawnEmergency(203, 3, 6, 1);
        sim.startDispatch();
        runTrip(sim);
        sim.startDispatch();
        sim.stepDispatch();
        sim.closeRoad(3, 5);
        sim.closeRoad(4, 5);
        runTrip(sim);
        sim.startDispatch();
        runTrip(sim);
        sim.openRoad(3, 5, 5);
        sim.moveNormal(1, 2);
        sim.departNormal(2);
    }
    long made1 = allocCount - a1;
    long freed1 = freeCount - f1;
    cout<<"allocations: "<<made1<<", frees: "<<freed1<<endl;
    check(made1 > 20, "the scenario really allocated memory (the test is not empty)");
    check(made1 == freed1, "scenario 1: every new has a delete");

    cout<<"\n--- scenario 2: the simulation is closed in the MIDDLE of a trip ---"<<endl;
    long a2 = allocCount;
    long f2 = freeCount;
    {
        Simulator sim;
        sim.spawnNormal(1, 1);
        sim.spawnNormal(2, 2);
        sim.spawnEmergency(11, 1, 1, 6);
        sim.startDispatch();
        sim.stepDispatch();                  // stops half way: route list and signal hold still exist
        // sim goes out of scope here with the trip still running
    }
    long made2 = allocCount - a2;
    long freed2 = freeCount - f2;
    cout<<"allocations: "<<made2<<", frees: "<<freed2<<endl;
    check(made2 > 5 && made2 == freed2, "scenario 2: closing in the middle of a trip leaks nothing");

    cout<<"\n--- scenario 3: refused spawns (full queues) leak nothing ---"<<endl;
    long a3 = allocCount;
    long f3 = freeCount;
    {
        Simulator sim;
        for(int k = 0; k < 15; k++){
            sim.spawnEmergency(300 + k, 1, 1 + (k % 5), 6);    // the 11th to 15th are refused
        }
        for(int k = 0; k < 40; k++){
            sim.spawnNormal(500 + k, 6);                       // lane and fleet limits are hit
        }
    }
    long made3 = allocCount - a3;
    long freed3 = freeCount - f3;
    cout<<"allocations: "<<made3<<", frees: "<<freed3<<endl;
    check(made3 > 10 && made3 == freed3, "scenario 3: refused spawns leak nothing");

    cout<<"\n--- scenario 4: one Graph with roads added and removed ---"<<endl;
    long a4 = allocCount;
    long f4 = freeCount;
    {
        Graph g(6);
        for(int k = 1; k < 6; k++){
            g.addRoad(k, k + 1, 3);
        }
        g.removeRoad(1, 2);
        g.removeRoad(3, 4);
        g.removeRoad(5, 6);
        g.addRoad(1, 2, 7);
    }
    long made4 = allocCount - a4;
    long freed4 = freeCount - f4;
    cout<<"allocations: "<<made4<<", frees: "<<freed4<<endl;
    check(made4 > 5 && made4 == freed4, "scenario 4: removeRoad frees the edge it removes, the destructor frees the rest");

    cout<<"\n--- scenario 5: reset() in the middle of a trip, then use the simulator again ---"<<endl;
    long a5 = allocCount;
    long f5 = freeCount;
    {
        Simulator sim;
        for(int round = 0; round < 3; round++){
            sim.spawnNormal(1, 1);
            sim.spawnNormal(2, 2);
            sim.spawnEmergency(11, 1, 1, 6);
            sim.startDispatch();
            sim.stepDispatch();              // half way: route list, signal hold and pulled-over vehicles exist
            sim.closeRoad(2, 6);
            sim.reset();                     // everything must be freed here, and the map comes back
        }
        sim.spawnEmergency(12, 1, 1, 6);
        sim.startDispatch();
        runTrip(sim);
        sim.reset();
    }
    long made5 = allocCount - a5;
    long freed5 = freeCount - f5;
    cout<<"allocations: "<<made5<<", frees: "<<freed5<<endl;
    check(made5 > 20 && made5 == freed5, "scenario 5: reset() and the destructor free everything, however often they run");

    cout<<"\n--- scenario 6: two simulators at the same time ---"<<endl;
    long a6 = allocCount;
    long f6 = freeCount;
    {
        Simulator a;
        Simulator b;
        a.spawnEmergency(1, 1, 1, 6);
        b.spawnEmergency(1, 2, 3, 6);
        a.startDispatch();
        b.startDispatch();
        a.stepDispatch();
        b.stepDispatch();
        // both are destroyed in the middle of their trips
    }
    long made6 = allocCount - a6;
    long freed6 = freeCount - f6;
    cout<<"allocations: "<<made6<<", frees: "<<freed6<<endl;
    check(made6 > 10 && made6 == freed6, "scenario 6: two simulators destroyed mid-trip leak nothing");

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
