#include "../include/ViewModel.hpp"

void ViewModel::reset(){
    startJ = 0;
    destJ = 0;
    state = VM_IDLE;
    timer = 0.0f;
    travelFrom = 0;
    travelTo = 0;
    travelVeh = -1;
    travelPriority = 0;
    travelT = 0.0f;
    paused = false;
    speed = 1.0f;
    nextNormalID = 101;
    nextEmergencyID = 901;
    checkText = "";
}

void ViewModel::selectStart(int j){
    if(validIndex(j)){
        startJ = j;
    }
}

void ViewModel::selectDest(int j){
    if(validIndex(j)){
        destJ = j;
    }
}

void ViewModel::actionSpawnNormal(Simulator &sim){
    if(startJ == 0){
        sim.addNote("[INFO] Left-click a junction first: that is where the vehicle starts.");
        return;
    }
    if(sim.spawnNormal(nextNormalID, startJ)){
        nextNormalID++;                 // the id is used up only if the vehicle was really created
    }
}

void ViewModel::actionSpawnEmergency(Simulator &sim, int type){
    if(startJ == 0 || destJ == 0){
        sim.addNote("[INFO] Left-click the START junction and right-click the DESTINATION junction first.");
        return;
    }
    if(sim.spawnEmergency(nextEmergencyID, type, startJ, destJ)){
        nextEmergencyID++;
    }
}

void ViewModel::actionDispatch(Simulator &sim){
    if(state != VM_IDLE || sim.isDispatchActive()){
        sim.addNote("[INFO] An emergency vehicle is already on the road.");
        return;
    }
    if(sim.startDispatch() == DISPATCH_STARTED){
        state = VM_DWELL;
        timer = 0.0f;
    }
}

void ViewModel::actionMoveNormal(Simulator &sim){
    if(startJ == 0 || destJ == 0){
        sim.addNote("[INFO] Select a START and a DESTINATION junction first.");
        return;
    }
    sim.moveNormal(startJ, destJ);
}

void ViewModel::actionLeave(Simulator &sim){
    if(startJ == 0){
        sim.addNote("[INFO] Left-click a junction first.");
        return;
    }
    sim.departNormal(startJ);
}

void ViewModel::actionToggleRoad(Simulator &sim){
    if(startJ == 0 || destJ == 0){
        sim.addNote("[INFO] Select a START and a DESTINATION junction first.");
        return;
    }
    if(sim.hasRoad(startJ, destJ)){
        sim.closeRoad(startJ, destJ);
    }
    else{
        sim.openRoad(startJ, destJ, DEFAULT_ROAD_LENGTH);
    }
}

void ViewModel::actionSelfCheck(Simulator &sim){
    int problems = sim.selfCheck();
    checkText = "Self-check: " + to_string(problems) + " problem(s)";
}

void ViewModel::actionDemo(Simulator &sim){
    int normalAt[8] = {1, 1, 2, 2, 2, 4, 3, 3};
    for(int i = 0; i < 8; i++){
        if(sim.spawnNormal(nextNormalID, normalAt[i])){
            nextNormalID++;
        }
    }
    int type[3] = {3, 2, 1};            // police, fire truck, ambulance: the LAST one is the most urgent
    int from[3] = {5, 3, 1};
    int to[3] = {1, 6, 6};
    for(int i = 0; i < 3; i++){
        if(sim.spawnEmergency(nextEmergencyID, type[i], from[i], to[i])){
            nextEmergencyID++;
        }
    }
    startJ = 1;
    destJ = 6;
}

void ViewModel::update(Simulator &sim, float dt){
    if(state == VM_IDLE){
        if(sim.isDispatchActive()){     // a trip exists that this timeline does not know yet
            state = VM_DWELL;
            timer = 0.0f;
        }
        return;
    }

    if(state == VM_DWELL){
        if(!sim.isDispatchActive()){
            state = VM_IDLE;
            return;
        }
        timer = timer + dt;
        if(timer >= DWELL_TIME){
            stepOnce(sim);
        }
        return;
    }

    // VM_TRAVEL
    travelT = travelT + dt / TRAVEL_TIME;
    if(travelT >= 1.0f){
        travelT = 1.0f;
        timer = 0.0f;
        if(sim.isDispatchActive()){
            state = VM_DWELL;
        }
        else{
            state = VM_IDLE;
        }
    }
}

void ViewModel::stepOnce(Simulator &sim){
    int fromJ = sim.getCurrentJunction();
    int toJ = sim.getNextJunction();
    int evID = sim.getActiveVehicleID();
    int evPriority = sim.getActivePriority();

    sim.stepDispatch();

    bool moved;
    int arrivedAt = toJ;
    if(sim.isDispatchActive()){
        arrivedAt = sim.getCurrentJunction();
        moved = (arrivedAt != fromJ);
    }
    else{
        moved = sim.wasLastTripArrived() && toJ != 0;
    }

    timer = 0.0f;
    if(moved){
        state = VM_TRAVEL;
        travelFrom = fromJ;
        travelTo = arrivedAt;
        travelVeh = evID;
        travelPriority = evPriority;
        travelT = 0.0f;
    }
    else if(!sim.isDispatchActive()){
        state = VM_IDLE;                // the trip stopped without a move
    }
    // else: the road was closed and the trip was re-routed. The vehicle waits one more moment.
}
