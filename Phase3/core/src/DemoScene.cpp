#include "../include/DemoScene.hpp"

void TrafficFeeder::reset(unsigned int newSeed){
    seed = newSeed;
    nextId = 1000;
    target = 30;
}

int TrafficFeeder::random(int n){
    seed = seed * 1103515245u + 12345u;
    return (int)((seed >> 16) % (unsigned int)n);
}

int TrafficFeeder::freeId(RoadSim &world){
    for(int tries = 0; tries < 1000; tries++){
        nextId++;
        if(nextId > 1999){
            nextId = 1000;
        }
        if(!world.idInUse(nextId)){
            return nextId;
        }
    }
    return nextId;
}

bool TrafficFeeder::spawnRandomFrom(RoadSim &world, int from){
    int candidates[NUM_JUNCTIONS];
    int count = 0;
    for(int b = 1; b <= NUM_JUNCTIONS; b++){
        if(b != from && world.canReach(from, b)){
            candidates[count] = b;
            count++;
        }
    }
    if(count == 0){
        return false;           // this junction is cut off from all others: nothing to drive to
    }
    return world.spawnNormal(freeId(world), from, candidates[random(count)]);
}

void TrafficFeeder::feed(RoadSim &world){
    for(int k = 0; k < 2; k++){
        if(world.getAliveCount() >= target){
            return;
        }
        spawnRandom(world);
    }
}

int demoVehicleCount(int scenario){
    if(scenario == 5){
        return 4;
    }
    if(scenario == 1){
        return 3;
    }
    if(scenario == 8 || scenario == 9 || scenario == 10){
        return 1;
    }
    return 2;
}

string demoScenarioName(int scenario){
    switch(scenario){
        case 1: return "J4 Convergence: Ambulance, Fire Truck and Police car meet at J4";
        case 2: return "Ambulance tie-breaker: two Priority 1 ambulances meet at J4";
        case 3: return "Fire Truck tie-breaker: two Priority 2 fire trucks meet at J4";
        case 4: return "Police tie-breaker: two Priority 3 police cars meet at J4";
        case 5: return "Ultimate J4 bottleneck: Ambulance A, Ambulance B, Fire Truck and Police car meet at J4";
        case 6: return "Head-on road conflict: Ambulance J2->J4 against Ambulance J4->J2 on the same road";
        case 7: return "Simultaneous junction arrival: two Ambulances enter J4 from opposite sides, tie-breaker by arrival time";
        case 8: return "Road closed: J1 - J2 is closed, the Ambulance J1 -> J6 takes the Dijkstra detour J1 -> J3 -> J4 -> J6";
        case 9: return "Gridlock: an Ambulance overtakes a full lane J5 -> J6 down the center";
        case 10: return "No way through: every road into J6 closes, the Ambulance is stopped safely at J2";
        default: return "unknown scenario";
    }
}

void buildMultiEVDemo(RoadSim &world, TrafficFeeder &feeder, int scenario){
    if(scenario < 1 || scenario > DEMO_SCENARIOS){
        scenario = 1;
    }
    int count = demoVehicleCount(scenario);
    int type[4] = {0, 0, 0, 0};
    int from[4] = {0, 0, 0, 0};
    int to[4] = {0, 0, 0, 0};
    int delay[4] = {0, 0, 0, 0};
    int id[4] = {DEMO_ID_1, DEMO_ID_2, DEMO_ID_3, DEMO_ID_4};
    int path[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};   // an explicit route (3 junctions): not used by the current scenes (all start and destination only)
    int pathLen[4] = {0, 0, 0, 0};
    if(scenario == 5){
        type[0] = PRIORITY_AMBULANCE;   from[0] = 2;  to[0] = 5;  delay[0] = 0;    // Ambulance A: J2 -> J4 -> J5
        type[1] = PRIORITY_AMBULANCE;   from[1] = 2;  to[1] = 5;  delay[1] = 1;    // Ambulance B: the SAME start and road, one tick later
        type[2] = PRIORITY_FIRE_TRUCK;  from[2] = 3;  to[2] = 6;  delay[2] = 0;    // Fire Truck: J3 -> J4 -> J6
        type[3] = PRIORITY_POLICE;      from[3] = 5;  to[3] = 2;  delay[3] = 0;    // Police car: J5 -> J4 -> J2
    }
    else if(scenario == 6){
        // head-on on ONE road: J2 -> J4 against J4 -> J2, both created in the same tick, so both enter the road together
        type[0] = PRIORITY_AMBULANCE;   from[0] = 2;  to[0] = 4;  delay[0] = 0;    // Ambulance A: J2 -> J4
        type[1] = PRIORITY_AMBULANCE;   from[1] = 4;  to[1] = 2;  delay[1] = 0;    // Ambulance B: J4 -> J2 (the same road, the other way)
    }
    else if(scenario == 7){
        type[0] = PRIORITY_AMBULANCE;   from[0] = 3;  to[0] = 6;  delay[0] = 0;    // Ambulance A: J3 -> J4 -> J6 (enters J4 from the J3 side)
        type[1] = PRIORITY_AMBULANCE;   from[1] = 6;  to[1] = 3;  delay[1] = 0;    // Ambulance B: J6 -> J4 -> J3 (enters J4 from the J6 side)
    }
    else if(scenario == 8){
        type[0] = PRIORITY_AMBULANCE;   from[0] = 1;  to[0] = 6;  delay[0] = 0;    // planned J1 -> J2 -> J6, then J1 - J2 closes
    }
    else if(scenario == 10){
        type[0] = PRIORITY_AMBULANCE;   from[0] = 1;  to[0] = 6;  delay[0] = 0;    // planned J1 -> J2 -> J6, then J6 is cut off
    }
    else if(scenario == 9){
        type[0] = PRIORITY_AMBULANCE;   from[0] = 5;  to[0] = 6;  delay[0] = 0;    // created after the jam has formed (below)
    }
    else if(scenario == 1){
        type[0] = PRIORITY_POLICE;      from[0] = 2;  to[0] = 3;       // J2 -> J4 -> J3   created FIRST, but least urgent
        type[1] = PRIORITY_AMBULANCE;   from[1] = 3;  to[1] = 6;       // J3 -> J4 -> J6
        type[2] = PRIORITY_FIRE_TRUCK;  from[2] = 5;  to[2] = 2;       // J5 -> J4 -> J2
    }
    else{
        int kind = PRIORITY_AMBULANCE;
        if(scenario == 3){
            kind = PRIORITY_FIRE_TRUCK;
        }
        if(scenario == 4){
            kind = PRIORITY_POLICE;
        }
        type[0] = kind;  from[0] = 3;  to[0] = 6;  delay[0] = 0;       // J3 -> J4 -> J6   the EARLIER arrival time
        type[1] = kind;  from[1] = 2;  to[1] = 5;  delay[1] = 1;       // J2 -> J4 -> J5   created one tick later
    }

    world.reset();
    world.setAnnounceTies(true);        // reset() turned it off: the demo wants the console proof
    world.setHeadOnPass(scenario == 6); // only scene 6 lets two EVs pass each other head-on
    feeder.reset(2026);
    feeder.setTarget(34);

    for(int t = 0; t < 40; t++){
        feeder.feed(world);
        world.tick();
    }

    if(scenario == 9){
        // the jam: 20 normal vehicles J5 -> J6 at once, then a few ticks so the lane fills and the line at J5 stays long
        for(int k = 0; k < DEMO_JAM_SIZE; k++){
            world.spawnNormal(DEMO_JAM_FIRST_ID + k, 5, 6);
        }
        for(int t = 0; t < DEMO_JAM_FILL_TICKS; t++){
            feeder.feed(world);
            world.tick();
        }
    }

    int lastDelay = 0;
    for(int k = 0; k < count; k++){
        if(delay[k] > lastDelay){
            lastDelay = delay[k];
        }
    }
    for(int t = 0; t <= lastDelay; t++){
        for(int k = 0; k < count; k++){
            if(delay[k] == t){
                if(pathLen[k] > 0){
                    world.spawn(id[k], type[k], from[k], to[k], path[k], pathLen[k]);      // the explicit route is checked road by road
                }
                else{
                    world.spawnEmergency(id[k], type[k], from[k], to[k]);
                }
            }
        }
        if(t < lastDelay){
            feeder.feed(world);
            world.tick();
        }
    }
    if(scenario == 8){
        world.closeRoad(1, 2);          // the Ambulance still waits at J1: it is re-planned at once
    }
    if(scenario == 10){
        world.closeRoad(2, 6);          // every road into J6 closes. The Ambulance's first road J1 - J2 is still open,
        world.closeRoad(4, 6);          // so it starts; at the stop line of J2 it finds no way left and is stopped
        world.closeRoad(5, 6);
    }
}
