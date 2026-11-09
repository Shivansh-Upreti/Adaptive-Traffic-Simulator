#pragma once
#include "RoadSim.hpp"

// ======================================================================
// Helpers for the live-traffic demo (used by main.cpp file).
//
// TrafficFeeder keeps a steady stream of normal vehicles on the roads: every tick it adds
// vehicles between random junctions until about 'target' vehicles are on the road.
// A vehicle that arrives is DELETED by RoadSim, so thousands of vehicles can be created
// over time although only MAX_MOVERS (60) exist at once.
// ======================================================================
class TrafficFeeder{
    unsigned int seed;
    int nextId;                         // ids 1000 .. 1999, used in turn
    int target;                         // how many vehicles to keep on the road

    public:
        TrafficFeeder(){
            reset(2026);
        }

        void reset(unsigned int newSeed);

        void setTarget(int t){
            target = t;
        }

        int getTarget(){
            return target;
        }

        // a small random number generator (the same numbers every run, so the demo is repeatable)
        int random(int n);

        // the next id that no living vehicle uses
        int freeId(RoadSim &world);

        // A normal vehicle that starts at 'from' and drives to a RANDOM destination of its own:
        // any junction except 'from' that can be reached on the roads that are open right now.
        // (Normal vehicles never share the destination of an emergency vehicle by accident.)
        bool spawnRandomFrom(RoadSim &world, int from);

        // adds one normal vehicle at a random junction, driving to a random destination
        bool spawnRandom(RoadSim &world){
            return spawnRandomFrom(world, 1 + random(NUM_JUNCTIONS));
        }

        // call once per tick: up to 2 new vehicles, until 'target' vehicles are on the road
        void feed(RoadSim &world);
};

//=======================================================================================================================
// The demo scenes (a sub-menu: the user picks one of seven)
//   1  J4 CONVERGENCE   Ambulance (1), Fire Truck (2) and Police car (3) start at the SAME moment from three different
//                       junctions and all three reach the stop lines of J4 together. The priority queue sorts them:
//                       1st Ambulance, 2nd Fire Truck, 3rd Police car (the Police car was even created FIRST)
//   2  AMBULANCE TIE    two Ambulances (both priority 1), the second one is created one tick later
//   3  FIRE TRUCK TIE   two Fire Trucks (both priority 2), the second one is created one tick later
//   4  POLICE TIE       two Police cars (both priority 3), the second one is created one tick later
//   5  J4 BOTTLENECK    FOUR EVs: Ambulance A and Ambulance B (priority 1) start at the SAME junction, B one tick later, and
//                       drive the same road behind each other; a Fire Truck and a Police car come from other junctions.
//                       Passing order at J4: Ambulance A, Ambulance B, Fire Truck, Police car
//   6  HEAD-ON ROAD     Ambulance A drives J2 -> J4 and Ambulance B drives J4 -> J2 on the SAME road, entering it in the same tick.
//                       Head-on pass mode (RoadSim::setHeadOnPass) lets BOTH keep driving: the road's activeEVCount sees both,
//                       the normal vehicles of that road move to the shoulders, and the window draws each ambulance on its
//                       own side of the road, so they pass each other without clipping
//   7  JUNCTION ARRIVAL Ambulance A (J3 -> J4 -> J6) and Ambulance B (J6 -> J4 -> J3) enter J4 from OPPOSITE sides in the same
//                       tick. Equal priority: the earlier arrival time decides who gets the junction first
// In scenes 2 - 4 the equal priority is decided by the EARLIER ARRIVAL TIME (ArrayPriorityQueue, unchanged from Phase 2).
// The routes below were found with a timing search (the traffic is the same every run), so they REALLY meet at J4.
// Scenes 1 - 4 have at most three emergency vehicles, scene 5 has four. Their ids are always 901, 902, 903 and 904.
//=======================================================================================================================

const int DEMO_SCENARIOS = 7;
const int DEMO_ID_1 = 901;
const int DEMO_ID_2 = 902;
const int DEMO_ID_3 = 903;
const int DEMO_ID_4 = 904;                  // only scene 5 uses a fourth vehicle

// how many emergency vehicles a scenario creates (3 for scenario 1, 4 for scenario 5, otherwise 2: scenes 2, 3, 4, 6 and 7)
int demoVehicleCount(int scenario);

string demoScenarioName(int scenario);

// true when none of the demo emergency vehicles is on the road any more (they all arrived)
inline bool demoSceneFinished(RoadSim &world){
    return !world.idInUse(DEMO_ID_1) && !world.idInUse(DEMO_ID_2) && !world.idInUse(DEMO_ID_3) && !world.idInUse(DEMO_ID_4);
}

// Builds one demo scene inside 'world'.
//   1. background traffic: the feeder fills the roads (40 ticks), every vehicle with its OWN random destination
//   2. the emergency vehicles of the chosen scenario (tables below; 'delay' = ticks after the first spawn)
// The caller keeps calling feeder.feed() and world.tick() until demoSceneFinished(), then stops the feeder.
void buildMultiEVDemo(RoadSim &world, TrafficFeeder &feeder, int scenario = 1);