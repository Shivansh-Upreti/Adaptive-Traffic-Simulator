#include <iostream>
using namespace std;
#include "include/TrafficFlow.hpp"

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

// empties a junction so the next test starts clean
void emptyJunction(int j){
    while(queueLength(j) > 0){
        vehicleDepart(j);
    }
}

int main(){
    cout<<"--- arrive and depart (FIFO) ---"<<endl;
    vehicleArrive(1, 10);
    vehicleArrive(1, 20);
    vehicleArrive(1, 30);
    check(queueLength(1) == 3, "3 vehicles waiting at J1");
    check(vehicleDepart(1) == 10, "first vehicle in leaves first");
    check(vehicleDepart(1) == 20, "second leaves second");
    check(queueLength(1) == 1, "1 vehicle left");
    emptyJunction(1);
    check(vehicleDepart(1) == -1, "departing from an empty junction gives -1");

    cout<<"\n--- bad junction numbers ---"<<endl;
    check(queueLength(0) == -1 && queueLength(7) == -1, "queueLength refuses J0 and J7");
    check(vehicleDepart(9) == -1, "vehicleDepart refuses J9");
    check(!moveVehicle(1, 9) && !moveVehicle(0, 2), "moveVehicle refuses bad junctions");

    cout<<"\n--- old queue bug (rear stuck at the end) ---"<<endl;
    for(int k = 0; k < 200; k++){
        vehicleArrive(2, k);
        vehicleDepart(2);
    }
    check(queueLength(2) == 0, "200 arrivals and departures, J2 is empty");
    vehicleArrive(2, 555);
    check(queueLength(2) == 1, "J2 still accepts a vehicle after 200 cycles");
    emptyJunction(2);

    cout<<"\n--- queue full (32) ---"<<endl;
    for(int k = 0; k < 40; k++){
        vehicleArrive(3, k);
    }
    check(queueLength(3) == 32, "J3 holds at most 32 vehicles");
    check(vehicleDepart(3) == 0, "oldest vehicle leaves first");
    vehicleArrive(3, 99);
    check(queueLength(3) == 32, "after one leaves, one more fits");
    check(!moveVehicle(4, 3), "moveVehicle from an empty junction fails");
    vehicleArrive(4, 77);
    check(!moveVehicle(4, 3), "moveVehicle into a full junction fails");
    check(queueLength(4) == 1, "vehicle stays at J4 when the move fails");
    emptyJunction(3);
    emptyJunction(4);

    cout<<"\n--- moving vehicles ---"<<endl;
    vehicleArrive(1, 101);
    vehicleArrive(1, 121);
    check(moveVehicle(1, 2), "move J1 -> J2 works");
    check(queueLength(1) == 1 && queueLength(2) == 1, "one vehicle left J1, one reached J2");
    check(vehicleDepart(2) == 101, "the vehicle at the front (101) was the one that moved");
    emptyJunction(1);

    cout<<"\n--- emergency vehicle skips the queue ---"<<endl;
    vehicleArrive(1, 101);
    vehicleArrive(1, 121);
    vehicleArrive(1, 102);          // ambulance is last in line
    check(moveEmergency(1, 5, 102), "ambulance moves out although it is last");
    check(queueLength(1) == 2 && queueLength(5) == 1, "the other two are still waiting at J1");
    check(vehicleDepart(1) == 101, "vehicles at J1 keep their order");
    check(!moveEmergency(1, 5, 999), "vehicle that is not in the queue cannot be moved");
    check(!moveEmergency(1, 9, 121), "emergency move to a bad junction fails");
    emptyJunction(1);
    emptyJunction(5);

    cout<<"\n--- traffic density ---"<<endl;
    check(congestionLevel(6) == "LOW", "0 waiting is LOW");
    for(int k = 0; k < 2; k++) vehicleArrive(6, k);
    check(congestionLevel(6) == "LOW", "2 waiting is LOW");
    vehicleArrive(6, 2);
    check(congestionLevel(6) == "MODERATE", "3 waiting is MODERATE");
    for(int k = 3; k < 5; k++) vehicleArrive(6, k);
    check(congestionLevel(6) == "MODERATE", "5 waiting is MODERATE");
    vehicleArrive(6, 5);
    check(congestionLevel(6) == "HEAVY", "6 waiting is HEAVY");
    check(congestionLevel(0) == "INVALID", "J0 is INVALID");
    emptyJunction(6);
    check(congestionLevel(6) == "LOW", "back to LOW when the queue is empty");

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
