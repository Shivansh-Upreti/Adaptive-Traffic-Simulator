#include "../include/LiveView.hpp"

LiveView::LiveView(){
    consoleLog = false;                 // not part of reset(): the R key must not switch the console logs off
    reset();
}

void LiveView::reset(){
    startJ = 0;
    destJ = 0;
    paused = false;
    speed = 1.0f;
    heavyTraffic = false;
    demoActive = false;
    demoMenuOpen = false;
    uiState = UI_MAIN;
    demoScenario = 0;
    tickTimer = 0.0f;
    feeder.reset(2026);
    nextEmergencyID = 900;
    checkText = "";
    for(int i = 0; i < MAX_MOVERS; i++){
        prev[i].id = 0;
        prev[i].state = MV_FREE;
        prev[i].from = 0;
        prev[i].to = 0;
        prev[i].cell = 0;
        lateral[i] = LANE_OFFSET;
        lateralId[i] = 0;
    }
}

void LiveView::selectStart(int j){
    if(validIndex(j)){
        startJ = j;
    }
}

void LiveView::selectDest(int j){
    if(validIndex(j)){
        destJ = j;
    }
}

void LiveView::snapshot(RoadSim &w){
    for(int i = 0; i < MAX_MOVERS; i++){
        prev[i].id = w.getSlotID(i);
        prev[i].state = w.getSlotState(i);
        prev[i].from = w.getSlotFrom(i);
        prev[i].to = w.getSlotTo(i);
        prev[i].cell = w.getSlotCell(i);
    }
    tickTimer = 0.0f;
}

float LiveView::getAlpha(){
    float a = tickTimer / LIVE_TICK_SECONDS;
    if(a < 0.0f) a = 0.0f;
    if(a > 1.0f) a = 1.0f;
    return a;
}

int LiveView::freeEmergencyId(RoadSim &w){
    for(int tries = 0; tries < 200; tries++){
        nextEmergencyID++;
        if(nextEmergencyID > 999){
            nextEmergencyID = 900;
        }
        if(!w.idInUse(nextEmergencyID)){
            return nextEmergencyID;
        }
    }
    return nextEmergencyID;
}

void LiveView::actionSpawnNormal(RoadSim &w){
    if(startJ == 0){
        w.addNote("[INFO] Left-click a junction first: that is where the vehicle starts.");
        return;
    }
    bool anywhere = false;
    for(int b = 1; b <= NUM_JUNCTIONS; b++){
        if(b != startJ && w.canReach(startJ, b)){
            anywhere = true;
        }
    }
    if(!anywhere){
        w.addNote("[ERROR] J" + to_string(startJ) + " is cut off: no junction can be reached from there.");
        return;
    }
    feeder.spawnRandomFrom(w, startJ);
}

void LiveView::actionSpawnEmergency(RoadSim &w, int type){
    if(demoActive){
        w.addNote("[INFO] The demo scene is running with its 3 emergency vehicles. Wait until they have arrived (R resets).");
        return;
    }
    if(startJ == 0 || destJ == 0){
        w.addNote("[INFO] Left-click the START junction and right-click the DESTINATION junction first.");
        return;
    }
    w.spawnEmergency(freeEmergencyId(w), type, startJ, destJ);
}

void LiveView::actionSpawnOpposing(RoadSim &w){
    int from = startJ;
    int to = destJ;
    if(from == 0 || to == 0 || from == to){
        int ev = w.latestEVSlot();
        if(ev == -1){
            w.addNote("[INFO] Select START and DESTINATION (left / right click) or create an emergency vehicle first, then press O.");
            return;
        }
        from = w.getSlotOrigin(ev);
        to = w.getSlotDestination(ev);
    }
    int made = 0;
    for(int k = 0; k < OPPOSING_COUNT; k++){
        if(!w.spawnOpposing(feeder.freeId(w), from, to)){
            break;
        }
        made++;
    }
    if(made > 0){
        w.addNote("[OPPOSING] " + to_string(made) + " normal vehicles start at J" + to_string(to) + " and drive back to J"
                  + to_string(from) + " on the roads of the emergency vehicle.");
    }
}

void LiveView::actionToggleHeavy(RoadSim &w){
    heavyTraffic = !heavyTraffic;
    if(heavyTraffic){
        w.addNote("[INFO] Heavy traffic ON: vehicles keep entering the roads.");
    }
    else{
        w.addNote("[INFO] Heavy traffic OFF: the roads empty out.");
    }
}

void LiveView::actionToggleRoad(RoadSim &w){
    if(startJ == 0 || destJ == 0){
        w.addNote("[INFO] Select a START and a DESTINATION junction first.");
        return;
    }
    if(w.hasRoad(startJ, destJ)){
        w.closeRoad(startJ, destJ);
    }
    else{
        w.openRoad(startJ, destJ, 6);
    }
}

void LiveView::actionSelfCheck(RoadSim &w){
    int problems = w.selfCheck();
    checkText = "Self-check: " + to_string(problems) + " problem(s)";
}

void LiveView::actionOpenUserMenu(){
    demoMenuOpen = false;
    uiState = UI_USER_MENU;
    if(consoleLog){
        cout<<"[UI] User Defined Input Mode: select an action (1 - 4, ESC = back)."<<endl;
    }
}

void LiveView::actionUserBack(){
    if(uiState == UI_USER_MENU){
        uiState = UI_MAIN;
        if(consoleLog){
            cout<<"[UI] Back to the main simulation."<<endl;
        }
    }
    else if(uiState != UI_MAIN){
        uiState = UI_USER_MENU;
    }
}

void LiveView::userInputKey(RoadSim &w, int digit){
    switch(uiState){
        case UI_USER_MENU:
            if(digit == 1){
                uiState = UI_SINGLE;
            }
            else if(digit == 2){
                uiState = UI_BATCH;
            }
            else if(digit == 3){
                uiState = UI_ROAD;
            }
            else if(digit == 4){
                actionToggleHeavy(w);               // background heavy traffic on / off (the menu stays open)
                if(consoleLog){
                    if(heavyTraffic){
                        cout<<"[ADAPTIVE CONTROL] Heavy background traffic ON: random vehicles keep entering the network."<<endl;
                    }
                    else{
                        cout<<"[ADAPTIVE CONTROL] Heavy background traffic OFF: no new random vehicles, the roads drain."<<endl;
                    }
                }
            }
            break;
        case UI_SINGLE:
            if(digit >= 1 && digit <= 3){
                int before = w.getCreated();
                int from = startJ;
                int to = destJ;
                actionSpawnEmergency(w, digit);
                if(consoleLog && w.getCreated() > before){
                    cout<<"[SPAWN] "<<emergencyTypeName(digit)<<" (priority "<<digit<<") J"<<from<<" -> J"<<to<<" created."<<endl;
                }
            }
            else if(digit == 0){
                // 0: OPPOSING traffic. Normal vehicles start at DESTINATION and drive the same roads BACKWARDS to START,
                // so the Center-Overtake and the evacuation of the opposite lane start at once
                int before = w.getCreated();
                int from = startJ;
                int to = destJ;
                actionSpawnOpposing(w);
                if(consoleLog && w.getCreated() > before){
                    cout<<"[OPPOSING] "<<(w.getCreated() - before)<<" normal vehicles"<<endl;
                    cout<<"           drive the EV route backwards (J"<<to<<" -> J"<<from<<"): Center-Overtake and lane evacuation can be observed."<<endl;
                }
            }
            else if(digit == 9){
                int before = w.getCreated();
                int from = startJ;
                actionSpawnNormal(w);
                if(consoleLog && w.getCreated() > before){
                    cout<<"[SPAWN] Normal vehicle at J"<<from<<" with its own random destination created."<<endl;
                }
            }
            break;
        default:
            break;
    }
}

void LiveView::userRightClick(RoadSim &w){
    if(uiState == UI_BATCH){
        actionBatchSpawn(w);
    }
    else if(uiState == UI_ROAD){
        actionToggleRoad(w);
        if(consoleLog && startJ != 0 && destJ != 0){
            if(w.hasRoad(startJ, destJ)){
                cout<<"[REROUTE] Road J"<<startJ<<"-J"<<destJ<<" opened (Graph::addRoad). Shortest paths are recomputed with Dijkstra."<<endl;
            }
            else{
                cout<<"[REROUTE] Road J"<<startJ<<"-J"<<destJ<<" closed (Graph::removeRoad). Vehicles on it recalculate their routes with Dijkstra."<<endl;
            }
        }
    }
}

void LiveView::actionBatchSpawn(RoadSim &w){
    if(startJ == 0 || destJ == 0 || startJ == destJ){
        w.addNote("[INFO] Left-click the START and right-click a different DESTINATION junction first.");
        return;
    }
    if(!w.canReach(startJ, destJ)){
        w.addNote("[ERROR] J" + to_string(destJ) + " cannot be reached from J" + to_string(startJ) + ".");
        return;
    }
    int made = 0;
    for(int k = 0; k < BATCH_COUNT; k++){
        if(w.spawnNormal(feeder.freeId(w), startJ, destJ)){
            made++;
        }
    }
    if(made > 0){
        w.addNote("[ADAPTIVE CONTROL] Heavy traffic: " + to_string(made) + " vehicles J" + to_string(startJ) + " -> J" + to_string(destJ) + ".");
        if(consoleLog){
            cout<<"[ADAPTIVE CONTROL] Heavy traffic detected: "<<made<<" vehicles queued at J"<<startJ<<" heading to J"<<destJ
                <<". Spawn queue, junction boxes and signals are re-evaluated every tick."<<endl;
        }
    }
}

void LiveView::actionToggleDemoMenu(RoadSim &w){
    if(demoActive){
        w.addNote("[INFO] A demo scene is running. It ends by itself when its emergency vehicles have arrived (R resets).");
        demoMenuOpen = false;
        return;
    }
    demoMenuOpen = !demoMenuOpen;
}

void LiveView::actionDemo(RoadSim &w, int scenario){
    demoMenuOpen = false;
    if(demoActive){
        w.addNote("[INFO] The demo scene is already running. It ends by itself when its emergency vehicles have arrived.");
        return;
    }
    if(scenario < 1 || scenario > DEMO_SCENARIOS){
        return;
    }
    buildMultiEVDemo(w, feeder, scenario);   // see core/include/DemoScene.hpp: never more than 3 emergency vehicles
    demoScenario = scenario;
    heavyTraffic = true;                // background traffic while the scene runs ...
    demoActive = true;                  // ... until its emergency vehicles are done
    startJ = 0;
    destJ = 0;
    snapshot(w);                        // so nothing slides in from the old picture
    w.addNote("[INFO] Demo " + to_string(scenario) + ": " + demoScenarioName(scenario) + ". It ends by itself; press D to choose again.");
}

void LiveView::update(RoadSim &w, float dt){
    tickTimer = tickTimer + dt;
    int ticksDone = 0;
    while(tickTimer >= LIVE_TICK_SECONDS && ticksDone < MAX_TICKS_PER_FRAME){
        float rest = tickTimer - LIVE_TICK_SECONDS;
        snapshot(w);                    // (sets tickTimer to 0)
        if(heavyTraffic){
            feeder.feed(w);
        }
        w.tick();
        tickTimer = rest;
        ticksDone++;
    }
    if(tickTimer >= LIVE_TICK_SECONDS){
        tickTimer = 0.0f;               // we are behind (a very long frame): drop the lost time, do not catch up for ever
    }
    if(demoActive && demoSceneFinished(w)){
        demoActive = false;             // the scene is over: no extra traffic any more, D can be pressed again
        heavyTraffic = false;
        w.setAnnounceTies(false);       // the console proof belongs to the demo only
        w.setHeadOnPass(false);
        w.addNote("[INFO] Demo scene finished: the 3 emergency vehicles are done. The extra traffic stops. Press D to run it again.");
    }
    easeLateral(w, dt);
}

void LiveView::easeLateral(RoadSim &w, float dt){
    for(int i = 0; i < MAX_MOVERS; i++){
        if(!w.isSlotUsed(i)){
            lateralId[i] = 0;
            continue;
        }
        float target = LANE_OFFSET;                     // normal vehicle: its lane
        if(w.isSlotEV(i)){
            target = 0.0f;                              // EV: the center
        }
        if(w.isOnShoulder(i)){
            target = SHOULDER_OFFSET;                   // gives way: the outer edge
        }
        if(w.isPassingHeadOn(i)){
            target = HEAD_ON_OFFSET;                    // head-on EVs: each one moves to its own side of the road
        }
        if(lateralId[i] != w.getSlotID(i)){
            lateralId[i] = w.getSlotID(i);              // a new vehicle in this slot: start at its target
            lateral[i] = target;
            continue;
        }
        float step = LATERAL_EASE_SPEED * dt;
        if(lateral[i] < target){
            lateral[i] = lateral[i] + step;
            if(lateral[i] > target) lateral[i] = target;
        }
        else if(lateral[i] > target){
            lateral[i] = lateral[i] - step;
            if(lateral[i] < target) lateral[i] = target;
        }
    }
}
