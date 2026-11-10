// Tests for the SFML "live traffic" view logic (LiveView). They need NO window and NO SFML:
// time is driven with fake frames.
#include <iostream>
#include <string>
using namespace std;
#include "../include/LiveView.hpp"

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

// runs n fake frames of 'dt' seconds; returns the self-check problems
int frames(RoadSim &w, LiveView &v, int n, float dt){
    int problems = 0;
    for(int i = 0; i < n; i++){
        v.update(w, dt);
        problems = problems + w.selfCheck();
    }
    return problems;
}

int main(){
    cout<<"--- start values and selection ---"<<endl;
    {
        LiveView v;
        check(v.startJ == 0 && v.destJ == 0 && !v.paused && !v.heavyTraffic && v.speed == 1.0f, "a new view is empty, not paused, heavy traffic off");
        v.selectStart(0);
        v.selectStart(7);
        v.selectDest(-1);
        check(v.startJ == 0 && v.destJ == 0, "a click on no junction selects nothing");
        v.selectStart(3);
        v.selectDest(5);
        check(v.startJ == 3 && v.destJ == 5, "left click = start, right click = destination");
        v.heavyTraffic = true;
        v.reset();
        check(v.startJ == 0 && !v.heavyTraffic, "reset puts everything back");
    }

    cout<<"\n--- keys without a selection explain what to do ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.actionSpawnNormal(w);
        check(w.getAliveCount() == 0 && w.getRecentEvent(0).find("[INFO]") == 0, "N without a start junction: nothing created, a hint is logged");
        v.selectStart(1);
        v.actionSpawnEmergency(w, 1);
        check(w.getEVCount() == 0, "an emergency key without a destination creates nothing");
        v.actionToggleRoad(w);
        check(w.hasRoad(1, 2), "C without a destination changes nothing");
    }

    cout<<"\n--- spawning ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.selectStart(1);
        v.actionSpawnNormal(w);
        check(w.getAliveCount() == 1, "N with only a start creates a vehicle with a random destination");
        bool allDifferent = true;
        for(int k = 0; k < 20; k++){
            v.actionSpawnNormal(w);
        }
        check(w.getAliveCount() == 21, "20 more vehicles were created");
        v.selectDest(6);
        v.actionSpawnEmergency(w, 2);
        check(w.getEVCount() == 1, "a Fire Truck J1 -> J6 was created (key 2)");
        int id = v.freeEmergencyId(w);
        check(id >= 900 && id <= 999 && !w.idInUse(id), "emergency ids come from 900..999 and are never one that is in use");
        (void)allDifferent;
        check(w.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- time: ticks, speed limit and alpha ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.update(w, 0.1f);
        check(w.getTick() == 0, "0.1 seconds is not enough for a tick");
        v.update(w, 0.1f);
        v.update(w, 0.1f);
        check(w.getTick() == 1, "0.3 seconds gives one tick");
        float a = v.getAlpha();
        check(a >= 0.0f && a <= 1.0f, "alpha is always between 0 and 1");
        v.update(w, 5.0f);
        check(w.getTick() <= 1 + MAX_TICKS_PER_FRAME, "a very long frame runs at most 3 ticks");
        check(v.getAlpha() <= 1.0f, "and alpha is still at most 1");
        long long before = w.getTick();
        v.update(w, 0.0f);
        check(w.getTick() == before, "a frame of 0 seconds changes nothing");
    }

    cout<<"\n--- the 'before' picture used for smooth sliding ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        w.spawnNormal(5, 1, 2);
        v.update(w, LIVE_TICK_SECONDS + 0.01f);      // tick 1: the vehicle is placed on the road
        v.update(w, LIVE_TICK_SECONDS + 0.01f);      // tick 2: it drives one cell
        int s = w.slotOf(5);
        check(s != -1 && v.prev[s].id == 5, "the snapshot remembers the vehicle");
        check(v.prev[s].state == MV_ON_ROAD && v.prev[s].cell == w.getSlotCell(s) - 1, "and where it was one tick ago (one cell behind)");
        float a1 = v.getAlpha();
        v.update(w, 0.07f);                          // a quarter of a tick later
        float a2 = v.getAlpha();
        check(a2 - a1 > 0.23f && a2 - a1 < 0.27f, "a quarter of a tick of time moves alpha by about 0.25");
    }

    cout<<"\n--- sideways easing: lane -> shoulder -> lane ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        w.spawnNormal(10, 2, 6);                     // a normal car on the long road J2 -> J6 (12 cells)
        int problems = frames(w, v, 10, 0.1f);       // about 3 ticks
        int s = w.slotOf(10);
        check(s != -1 && v.lateral[s] == LANE_OFFSET, "a normal car with no EV near drives in its lane");
        w.spawnEmergency(50, 1, 2, 6);               // the ambulance follows on the same road
        float widest = LANE_OFFSET;
        bool evCentered = true;
        bool movedGradually = true;
        float last = v.lateral[s];
        for(int i = 0; i < 60 && w.slotOf(10) != -1; i++){
            v.update(w, 0.05f);
            problems = problems + w.selfCheck();
            s = w.slotOf(10);
            if(s == -1) break;
            if(v.lateral[s] > widest) widest = v.lateral[s];
            if(v.lateral[s] - last > LATERAL_EASE_SPEED * 0.05f + 0.01f) movedGradually = false;   // never jumps
            last = v.lateral[s];
            int es = w.slotOf(50);
            if(es != -1 && v.lateral[es] != 0.0f) evCentered = false;
        }
        check(widest > LANE_OFFSET + 3.0f, "the car moved toward the shoulder while the ambulance came");
        check(widest <= SHOULDER_OFFSET, "but never beyond the shoulder");
        check(movedGradually, "it glided sideways (no jumps)");
        check(evCentered, "the ambulance stayed in the center (lateral 0)");
        problems = problems + frames(w, v, 400, 0.1f);
        check(w.getAliveCount() == 0, "everything arrived afterwards");
        check(problems == 0, "self-check clean throughout");
    }
    {
        // a vehicle that is gone and a new one in the same slot must not inherit the old sideways position
        RoadSim w;
        LiveView v;
        w.spawnNormal(1, 1, 2);
        int problems = frames(w, v, 100, 0.1f);
        check(w.getAliveCount() == 0, "the first vehicle arrived and was deleted");
        w.spawnEmergency(900, 1, 3, 6);
        problems = problems + frames(w, v, 4, 0.1f);
        int s = w.slotOf(900);
        check(s != -1 && v.lateral[s] == 0.0f, "a new EV in a reused slot starts in the center, not where the old car was");
        check(problems == 0, "self-check clean");
    }

    cout<<"\n--- heavy traffic and the demo scene ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.actionToggleHeavy(w);
        check(v.heavyTraffic, "H switches heavy traffic on");
        int problems = frames(w, v, 300, 0.1f);
        check(w.getAliveCount() > 15, "after a while many vehicles are on the roads");
        check(w.getCreated() > 30, "and new ones keep coming");
        v.actionToggleHeavy(w);
        problems = problems + frames(w, v, 1500, 0.1f);
        check(w.getAliveCount() == 0 && problems == 0, "with heavy traffic off the roads empty out, self-check clean");
    }
    {
        RoadSim w;
        LiveView v;
        v.selectStart(2);
        v.actionDemo(w, 1);
        check(v.heavyTraffic && v.startJ == 0, "D starts the demo scene with heavy traffic and clears the selection");
        check(w.getEVCount() == 3 && w.getAliveCount() > 20, "three emergency vehicles and plenty of normal traffic");
        int problems = frames(w, v, 900, 0.1f);        // about 320 ticks
        check(w.getArrivedEV() == 3, "all three emergency vehicles arrived");
        check(w.getConflicts() >= 1 && w.getYields() >= 1 && w.getFrozenTicks() > 0, "with a conflict, a yield and frozen opposing traffic");
        check(problems == 0 && w.getTimeouts() == 0, "self-check clean on every frame, nobody gave up");
        v.actionSelfCheck(w);
        check(v.checkText == "Self-check: 0 problem(s)", "K reports 0 problems");
        v.actionDemo(w, 1);
        check(w.getEVCount() == 3 && w.selfCheck() == 0, "D again rebuilds the scene from scratch");
    }

    cout<<"\n--- roads ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.selectStart(2);
        v.selectDest(6);
        v.actionToggleRoad(w);
        check(!w.hasRoad(2, 6), "C closes the road");
        v.actionToggleRoad(w);
        check(w.hasRoad(2, 6), "C again opens it");
    }

    cout<<"\n--- mouse roles: START is where vehicles begin, DESTINATION is where they go ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.selectStart(2);               // left click
        v.selectDest(5);                // right click
        v.actionSpawnEmergency(w, PRIORITY_AMBULANCE);
        int ev = w.latestEVSlot();
        check(ev != -1 && w.getSlotOrigin(ev) == 2 && w.getSlotDestination(ev) == 5, "the EV starts at the LEFT-click junction and drives to the RIGHT-click junction");
        v.actionSpawnNormal(w);
        int found = -1;
        for(int i = 0; i < MAX_MOVERS; i++){
            if(w.isSlotUsed(i) && !w.isSlotEV(i)) found = i;
        }
        check(found != -1 && w.getSlotOrigin(found) == 2, "a normal vehicle also starts at START");
    }

    cout<<"\n--- normal vehicles choose their own destination ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.selectStart(1);
        v.selectDest(6);
        bool sameAsEV = true;           // stays true only if EVERY normal vehicle went to J6
        bool neverStart = true;
        int n = 0;
        for(int k = 0; k < 20; k++){
            v.actionSpawnNormal(w);
            frames(w, v, 3, 0.1f);
        }
        for(int i = 0; i < MAX_MOVERS; i++){
            if(w.isSlotUsed(i) && !w.isSlotEV(i)){
                n++;
                if(w.getSlotDestination(i) != 6) sameAsEV = false;
                if(w.getSlotDestination(i) == w.getSlotOrigin(i)) neverStart = false;
            }
        }
        check(n > 5, "several normal vehicles exist");
        check(!sameAsEV, "their destinations are NOT all the EV destination J6");
        check(neverStart, "no normal vehicle has its start as its destination");
    }

    cout<<"\n--- the demo scene: exactly 3 EVs, then it ends and can be started again ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.actionDemo(w, 1);
        check(v.demoActive && w.getEVCount() == 3, "D starts the demo with exactly 3 EVs");
        int made = w.getCreated();
        v.actionDemo(w, 1);
        check(w.getCreated() == made, "D while it runs does nothing");
        v.selectStart(1);
        v.selectDest(6);
        v.actionSpawnEmergency(w, PRIORITY_POLICE);
        check(w.getEVCount() == 3, "keys 1 2 3 create no extra EV while the demo runs");
        int problems = frames(w, v, 900, 0.1f);
        check(!v.demoActive && !v.heavyTraffic && w.getArrivedEV() == 3, "after the 3 EVs arrived the demo ended by itself, extra traffic is off");
        problems = problems + frames(w, v, 600, 0.1f);
        check(w.getArrivedEV() == 3 && w.getEVCount() == 0, "no new EV appears afterwards (no endless loop)");
        check(problems == 0, "self-check clean the whole time");
        v.actionDemo(w, 1);
        check(v.demoActive && w.getEVCount() == 3, "D can be triggered again by hand");
    }

    cout<<"\n--- the three kinds of EV (keys 1, 2, 3) ---"<<endl;
    {
        check(PRIORITY_AMBULANCE == 1 && PRIORITY_FIRE_TRUCK == 2 && PRIORITY_POLICE == 3, "key 1 = priority 1, key 2 = priority 2, key 3 = priority 3");
        check(emergencyTypeName(1) == "Ambulance" && emergencyTypeName(2) == "Fire Truck" && emergencyTypeName(3) == "Police", "names: Ambulance, Fire Truck, Police");
        RoadSim w;
        LiveView v;
        v.selectStart(1);
        v.selectDest(6);
        v.actionSpawnEmergency(w, PRIORITY_POLICE);
        int ev = w.latestEVSlot();
        check(ev != -1 && w.getSlotPriority(ev) == 3 && w.getSlotLabel(ev) == "Police", "key 3 creates a Police car of priority 3");
        v.actionSpawnEmergency(w, PRIORITY_FIRE_TRUCK);
        ev = w.latestEVSlot();
        check(ev != -1 && w.getSlotPriority(ev) == 2 && w.getSlotLabel(ev) == "Fire Truck", "key 2 creates a Fire Truck of priority 2");
    }

    cout<<"\n--- O: opposing traffic ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.actionSpawnOpposing(w);
        check(w.getAliveCount() == 0, "O with nothing selected and no EV creates nothing");
        v.selectStart(1);
        v.selectDest(6);
        v.actionSpawnEmergency(w, PRIORITY_AMBULANCE);
        v.actionSpawnOpposing(w);
        int count = 0;
        bool backwards = true;
        for(int i = 0; i < MAX_MOVERS; i++){
            if(w.isSlotUsed(i) && !w.isSlotEV(i)){
                count++;
                if(w.getSlotOrigin(i) != 6 || w.getSlotDestination(i) != 1) backwards = false;
            }
        }
        check(count > 0 && count <= OPPOSING_COUNT, "O creates opposing normal vehicles");
        check(backwards, "they start at the EV destination J6 and drive to the EV start J1");
        check(w.canReach(1, 6) && !w.canReach(0, 6), "canReach answers without side effects");
        int problems = frames(w, v, 600, 0.1f);
        check(problems == 0 && w.getArrivedEV() == 1, "head-on traffic: self-check clean and the EV arrives");
    }

    cout<<"\n--- O without a selection uses the latest EV ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.selectStart(2);
        v.selectDest(5);
        v.actionSpawnEmergency(w, PRIORITY_FIRE_TRUCK);
        v.reset();                      // selection forgotten, EV still on the road
        v.actionSpawnOpposing(w);
        int count = 0;
        bool ok = true;
        for(int i = 0; i < MAX_MOVERS; i++){
            if(w.isSlotUsed(i) && !w.isSlotEV(i)){
                count++;
                if(w.getSlotOrigin(i) != 5 || w.getSlotDestination(i) != 2) ok = false;
            }
        }
        check(count > 0 && ok, "the opposing vehicles use the route of the latest EV");
    }

    cout<<"\n--- User Defined Input mode (key U): a state machine ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.selectStart(1);
        v.selectDest(6);
        v.userInputKey(w, 1);
        v.userRightClick(w);
        check(v.uiState == UI_MAIN && w.getAliveCount() == 0 && w.hasRoad(1, 2), "in the main simulation the digits and right clicks create and change nothing");
        v.actionOpenUserMenu();
        check(v.uiState == UI_USER_MENU, "U opens the user menu");
        v.userInputKey(w, 9);
        check(w.getAliveCount() == 0, "in the menu a 9 does nothing yet");
        v.userInputKey(w, 1);
        check(v.uiState == UI_SINGLE, "[1] switches to single custom vehicle");
        v.userInputKey(w, 1);
        check(w.getEVCount() == 1 && w.getSlotPriority(w.latestEVSlot()) == 1, "then key 1 creates an Ambulance (priority 1)");
        check(w.getAliveCount() == 1, "key 1 creates only the EV, no opposing traffic");
        v.userInputKey(w, 0);
        int opp = 0;
        bool backwards = true;
        for(int i = 0; i < MAX_MOVERS; i++){
            if(w.isSlotUsed(i) && !w.isSlotEV(i)){
                opp++;
                if(w.getSlotOrigin(i) != 6 || w.getSlotDestination(i) != 1) backwards = false;
            }
        }
        check(opp >= 1 && opp <= OPPOSING_COUNT && backwards, "key 0 creates normal vehicles J6 -> J1 (opposing traffic, Dest -> Start)");
        v.userInputKey(w, 2);
        v.userInputKey(w, 3);
        check(w.getEVCount() == 3, "keys 2 and 3 create a Fire Truck and a Police car");
        int before = w.getAliveCount();
        int evsBefore = w.getEVCount();
        v.userInputKey(w, 9);
        check(w.getAliveCount() == before + 1 && w.getEVCount() == evsBefore, "key 9 creates exactly ONE normal vehicle");
        v.actionUserBack();
        check(v.uiState == UI_USER_MENU, "ESC goes back to the user menu");
        v.actionUserBack();
        check(v.uiState == UI_MAIN, "ESC again returns to the main simulation");
    }
    {
        RoadSim w;
        LiveView v;
        v.actionOpenUserMenu();
        v.userInputKey(w, 2);
        check(v.uiState == UI_BATCH, "[2] switches to batch spawn");
        v.selectStart(2);
        v.selectDest(5);
        v.userRightClick(w);
        int n = 0;
        bool ok = true;
        for(int i = 0; i < MAX_MOVERS; i++){
            if(w.isSlotUsed(i)){
                n++;
                if(w.getSlotOrigin(i) != 2 || w.getSlotDestination(i) != 5) ok = false;
            }
        }
        check(n == BATCH_COUNT && ok, "the right click creates 5 normal vehicles J2 -> J5 at once");
        check(w.selfCheck() == 0, "self-check clean after the batch");
    }
    {
        RoadSim w;
        LiveView v;
        v.actionOpenUserMenu();
        v.userInputKey(w, 3);
        check(v.uiState == UI_ROAD, "[3] switches to road closure");
        v.selectStart(2);
        v.selectDest(6);
        v.userRightClick(w);
        check(!w.hasRoad(2, 6), "the right click closes the road J2-J6 (Graph::removeRoad)");
        v.userRightClick(w);
        check(w.hasRoad(2, 6), "the next right click opens it again");
    }
    {
        RoadSim w;
        LiveView v;
        v.selectStart(1);
        v.selectDest(6);
        v.actionSpawnEmergency(w, PRIORITY_AMBULANCE);
        v.actionOpenUserMenu();
        bool before = v.heavyTraffic;
        v.userInputKey(w, 4);
        check(v.heavyTraffic != before && v.uiState == UI_USER_MENU, "[4] switches the heavy background traffic on and stays in the menu");
        v.userInputKey(w, 4);
        check(v.heavyTraffic == before, "[4] again switches it off");
        v.heavyTraffic = true;
        v.reset();
        check(v.uiState == UI_MAIN, "R (reset) returns to the main simulation");
    }

    cout<<"\n--- scenario 6: the two ambulances move to opposite sides of the road ---"<<endl;
    {
        RoadSim w;
        LiveView v;
        v.actionDemo(w, 6);
        bool opposite = false;
        bool a = false;
        bool b = false;
        float la = 0.0f;
        float lb = 0.0f;
        for(int i = 0; i < 900 && !opposite; i++){
            v.update(w, 0.1f);
            int sa = w.slotOf(901);
            int sb = w.slotOf(902);
            if(sa != -1 && sb != -1 && w.isPassingHeadOn(sa) && w.isPassingHeadOn(sb)){
                a = true;
                b = true;
                la = v.lateral[sa];
                lb = v.lateral[sb];
                if(la == HEAD_ON_OFFSET && lb == HEAD_ON_OFFSET) opposite = true;     // each one on ITS OWN right = opposite sides of the road
            }
        }
        check(a && b && opposite, "while they pass, both ambulances are drawn HEAD_ON_OFFSET to their own right (opposite sides)");
        frames(w, v, 900, 0.1f);
        check(!v.demoActive && w.getArrivedEV() == 2, "the demo ends by itself");
        v.actionDemo(w, 2);
        bool normalAgain = true;
        for(int i = 0; i < 40; i++){
            v.update(w, 0.1f);
            for(int k = 0; k < MAX_MOVERS; k++){
                if(w.isSlotUsed(k) && w.isPassingHeadOn(k)) normalAgain = false;
            }
        }
        check(normalAgain, "head-on pass mode is switched off for the other scenes");
    }

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
