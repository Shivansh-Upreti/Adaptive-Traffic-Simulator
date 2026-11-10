// Tests for the SFML view logic (ViewModel). They need NO window and NO SFML:
// the animation timeline is driven with fake "frames" of 0.1 seconds.
#include <iostream>
#include <string>
using namespace std;
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

// runs fake frames until the vehicle is no longer shown. returns how many roads it drove
// (a road = one switch to VM_TRAVEL). gives up after 2000 frames
int runUntilIdle(Simulator &sim, ViewModel &vm){
    int travels = 0;
    int last = vm.state;
    for(int frame = 0; frame < 2000; frame++){
        vm.update(sim, 0.1f);
        if(vm.state == VM_TRAVEL && last != VM_TRAVEL){
            travels++;
        }
        last = vm.state;
        if(vm.state == VM_IDLE && !sim.isDispatchActive()){
            break;
        }
    }
    return travels;
}

// runs fake frames until the vehicle waits at junction j. false if that never happens
bool runUntilAt(Simulator &sim, ViewModel &vm, int j){
    for(int frame = 0; frame < 2000; frame++){
        vm.update(sim, 0.1f);
        if(vm.state == VM_DWELL && sim.getCurrentJunction() == j){
            return true;
        }
        if(vm.state == VM_IDLE){
            return false;
        }
    }
    return false;
}

int main(){
    cout<<"--- start values and selection ---"<<endl;
    {
        ViewModel vm;
        check(vm.startJ == 0 && vm.destJ == 0 && vm.state == VM_IDLE && !vm.paused, "a new view model is empty and idle");
        check(vm.nextNormalID == 101 && vm.nextEmergencyID == 901, "ids start at 101 and 901");
        vm.selectStart(0);
        vm.selectStart(7);
        vm.selectDest(-1);
        check(vm.startJ == 0 && vm.destJ == 0, "a click on no junction (0, 7, -1) selects nothing");
        vm.selectStart(3);
        vm.selectDest(5);
        check(vm.startJ == 3 && vm.destJ == 5, "left click sets the start, right click the destination");
        vm.nextNormalID = 500;
        vm.state = VM_TRAVEL;
        vm.reset();
        check(vm.startJ == 0 && vm.nextNormalID == 101 && vm.state == VM_IDLE, "reset puts everything back");
    }

    cout<<"\n--- keys without a selection explain what to do ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        vm.actionSpawnNormal(sim);
        check(sim.getFleetCount() == 0 && sim.getRecentEvent(0).find("[INFO]") == 0, "N without a start junction: nothing created, a hint is logged");
        vm.selectStart(1);
        vm.actionSpawnEmergency(sim, 1);
        check(sim.getFleetCount() == 0 && sim.getPendingCount() == 0, "emergency key without a destination: nothing created");
        vm.actionMoveNormal(sim);
        vm.actionToggleRoad(sim);
        check(sim.hasRoad(1, 2), "M and C without a destination change nothing");
        ViewModel vm2;
        vm2.actionLeave(sim);
        check(sim.getRecentEvent(0).find("[INFO]") == 0, "L without a start junction logs a hint");
    }

    cout<<"\n--- spawning uses the next free id, a refused spawn does not use one up ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        vm.selectStart(1);
        vm.actionSpawnNormal(sim);
        vm.actionSpawnNormal(sim);
        check(sim.idInUse(101) && sim.idInUse(102) && vm.nextNormalID == 103, "two vehicles: ids 101 and 102");
        for(int k = 0; k < 40; k++){
            vm.actionSpawnNormal(sim);
        }
        check(sim.getLaneLength(1) == 32, "the lane at J1 holds 32");
        // 2 vehicles + 30 more fill the lane = ids 101..132 are used, so the next free id is 133
        // (the other 10 spawns were refused and must NOT have used up ids 133..142)
        check(vm.nextNormalID == 133, "refused spawns (lane full) did not use up ids");
        vm.selectDest(6);
        vm.actionSpawnEmergency(sim, 1);
        check(sim.getPendingCount() == 0 && vm.nextEmergencyID == 901, "no emergency vehicle can start in a full lane, id 901 is not used up");
        vm.selectStart(2);
        vm.actionSpawnEmergency(sim, 1);
        check(sim.getPendingCount() == 1 && vm.nextEmergencyID == 902, "ambulance 901 created at J2");
    }

    cout<<"\n--- a whole trip on the timeline ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        vm.selectStart(1);
        vm.selectDest(6);
        vm.actionSpawnEmergency(sim, 1);
        vm.actionDispatch(sim);
        check(vm.state == VM_DWELL && sim.isDispatchActive(), "dispatch: the vehicle waits at its start junction first");

        vm.actionDispatch(sim);
        check(sim.getRecentEvent(0).find("already on the road") != string::npos, "a second dispatch is refused with a message");

        vm.update(sim, 0.1f);
        check(vm.state == VM_DWELL, "before DWELL_TIME nothing moves");

        int travels = runUntilIdle(sim, vm);
        check(travels == 2, "J1 -> J2 -> J6 is two roads");
        check(sim.wasLastTripArrived() && !sim.isDispatchActive(), "the ambulance arrived");
        check(vm.state == VM_IDLE, "the timeline is idle again");
        check(vm.travelVeh == 901, "the driving vehicle was 901");
        vm.actionSelfCheck(sim);
        check(vm.checkText == "Self-check: 0 problem(s)", "self-check text says 0 problems");
    }

    cout<<"\n--- driving animation values ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        vm.selectStart(1);
        vm.selectDest(6);
        vm.actionSpawnEmergency(sim, 1);
        vm.actionDispatch(sim);
        vm.update(sim, DWELL_TIME + 0.01f);          // one jump over the waiting time: the vehicle starts driving
        check(vm.state == VM_TRAVEL && vm.travelFrom == 1 && vm.travelTo == 2 && vm.travelT == 0.0f,
              "after the waiting time the vehicle starts on road J1 -> J2 at 0.0");
        vm.update(sim, TRAVEL_TIME / 2.0f);
        check(vm.travelT > 0.49f && vm.travelT < 0.51f, "half the travel time = halfway");
        vm.update(sim, TRAVEL_TIME);
        check(vm.state == VM_DWELL && vm.travelT == 1.0f, "travelT never goes above 1.0, then the vehicle waits at J2");
        runUntilIdle(sim, vm);
    }

    cout<<"\n--- a road closed while the vehicle is at J2: reroute on screen ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        vm.selectStart(1);
        vm.selectDest(6);
        vm.actionSpawnEmergency(sim, 1);
        vm.actionDispatch(sim);
        check(runUntilAt(sim, vm, 2), "the vehicle reaches J2");
        vm.selectStart(2);
        vm.selectDest(6);
        vm.actionToggleRoad(sim);
        check(!sim.hasRoad(2, 6), "C closes the road J2 - J6");
        int travels = runUntilIdle(sim, vm);
        check(travels == 2, "two more roads: J2 -> J4 -> J6");
        check(sim.wasLastTripArrived(), "the ambulance still arrived");
        vm.actionToggleRoad(sim);
        check(sim.hasRoad(2, 6), "C again opens the road");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- a trip that cannot go on ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        vm.selectStart(1);
        vm.selectDest(6);
        vm.actionSpawnEmergency(sim, 1);
        sim.spawnNormal(50, 2);
        vm.actionDispatch(sim);
        runUntilAt(sim, vm, 2);
        sim.closeRoad(2, 6);
        sim.closeRoad(4, 6);
        sim.closeRoad(5, 6);
        int travels = runUntilIdle(sim, vm);
        check(travels == 0, "no further road was driven");
        check(!sim.isDispatchActive() && !sim.wasLastTripArrived() && vm.state == VM_IDLE, "the trip stopped and the timeline is idle");
        check(sim.getSideLength(2) == 0 && !sim.isPreempted(2), "signal released, side lane empty");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- the demo scene dispatches in priority order ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        vm.actionDemo(sim);
        check(sim.getFleetCount() == 11 && sim.getPendingCount() == 3, "demo: 8 normal vehicles and 3 emergency requests");
        check(vm.startJ == 1 && vm.destJ == 6, "demo selects J1 and J6");
        int expected[3] = {903, 902, 901};           // ambulance, fire truck, police
        bool orderOk = true;
        bool allArrived = true;
        for(int i = 0; i < 3; i++){
            vm.actionDispatch(sim);
            if(sim.getActiveVehicleID() != expected[i]) orderOk = false;
            runUntilIdle(sim, vm);
            if(!sim.wasLastTripArrived()) allArrived = false;
        }
        check(orderOk, "order: ambulance 903, fire truck 902, police 901");
        check(allArrived, "all three arrived");
        check(sim.getPreemptCount() > 0 && sim.getEvacuatedCount() > 0, "the busy junctions caused signal requests and evacuations");
        check(sim.selfCheck() == 0, "self-check is clean");
    }

    cout<<"\n--- the timeline notices a trip it did not start ---"<<endl;
    {
        Simulator sim;
        ViewModel vm;
        sim.spawnEmergency(1, 1, 1, 6);
        sim.startDispatch();
        vm.update(sim, 0.1f);
        check(vm.state == VM_DWELL, "an active trip puts an idle timeline into VM_DWELL");
        runUntilIdle(sim, vm);
    }

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
