#include <iostream>
#include <string>
using namespace std;

//#1: OOP#
#include "Vehicle.hpp"
#include "EmergencyVehicle.hpp"
#include "EmergencyRequest.hpp"

//junction node
struct Junction{
    int junctionID;
    string name;
};

#include "ArrayPriorityQueue.hpp"
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"
#include "Graph.hpp"
#include "TrafficFlow.hpp"
#include "RouteHistory.hpp"

//finds a vehicle in the fleet using its id
Vehicle* findVehicle(Vehicle* fleet[], int n, int id){
    for(int i=0; i<n; i++){
        if(fleet[i]->getID() == id){
            return fleet[i];
        }
    }
    return nullptr;
}

//drives a vehicle along its own route (linked list), one road at a time,
//and records every junction in its route history
void driveEmergency(Graph &city, Vehicle &v, RouteHistory &history){
    int vehID = v.getID();
    SinglyLinkedList::Node* cur = v.getRoute().getHead();

    //vehicle is already standing in the queue of its first junction
    history.recordVisit(cur->data);

    while(cur->next != nullptr){
        int from = cur->data;
        int to = cur->next->data;

        if(!city.hasRoad(from, to)){
            cout<<"No road between J"<<from<<" and J"<<to<<", vehicle stopped!"<<endl;
            return;
        }
        moveEmergency(from, to, vehID);
        history.recordVisit(to);
        cur = cur->next;
    }

    //reached the destination, so it leaves the network
    junctionQ[cur->data - 1].remove(vehID);
    cout<<"Vehicle "<<vehID<<" reached J"<<cur->data<<endl;
}


int main(){
    cout<<"\n======================================================="<<endl;
    cout<<"ADAPTIVE TRAFFIC SIGNAL AND EMERGENCY VEHICLE SIMULATOR"<<endl;
    cout<<"=======================================================\n"<<endl;

    Vehicle car(101);
    Vehicle bike(121);
    Vehicle bus(223);
    EmergencyVehicle ambu(102, "Ambulance", 1);
    EmergencyVehicle fireT(202, "Fire Truck", 2);
    EmergencyVehicle ambu2(221, "Ambulance", 1);
    Vehicle* fleet[6] = {&car, &bike, &bus, &ambu, &fireT, &ambu2};

//JUNCTIONS:
    cout<<"# ARRAY #"<<endl;
    
    Junction junctions[6]={ {1, "J1"},
                            {2, "J2"},
                            {3, "J3"},
                            {4, "J4"},
                            {5, "J5"},
                            {6, "J6"}};     

    cout<<"Junctions Info. :"<<endl;
    for(int i=0; i<6; i++){
        cout<<"Junction ID:"<<junctions[i].junctionID;
        cout<<" , Name:"<<junctions[i].name<<endl;
    }
    cout<<"\n";
    cout<<"------------------------------------\n"<<endl;

//ROAD NETWORK:
    cout<<"# GRAPH #\n"<<endl; 

    Graph city(6);
    city.addRoad(1, 2, 5);
    city.addRoad(1, 3, 4);
    city.addRoad(2, 4, 4);
    city.addRoad(3, 4, 5);
    city.addRoad(3, 5, 5);
    city.addRoad(4, 5, 4);
    city.addRoad(2, 6, 6);
    city.addRoad(4, 6, 5);
    city.addRoad(5, 6, 7);
    city.addRoad(7, 2, 5);     //J7 does not exist, this should get rejected

    cout<<"\nRoads (distance in brackets):"<<endl;
    city.showRoads();

    cout<<"\n";
    int reached=city.bfs(1);
    city.dfs(1);
    if(reached==6){
        cout<<"All 6 junctions can be reached from J1"<<endl;
    }

    cout<<"\n------------------------------------\n"<<endl;

//Vehicle routing:
    cout<<"# TRAFFIC FLOW AT JUNCTIONS #\n"<<endl;

    vehicleArrive(1, car.getID());
    vehicleArrive(1, bike.getID());
    vehicleArrive(3, bus.getID());
    cout<<"Normal vehicles have arrived:"<<endl;
    showAllQueues();

    cout<<"\nCar 101 wants to go J1 -> J2"<<endl;
    if(city.hasRoad(1, 2)){
        moveVehicle(1, 2);
    }
    cout<<"Bike 121 wants to go J1 -> J6"<<endl;
    if(!city.hasRoad(1, 6)){
        cout<<"There is no direct road from J1 to J6, bike keeps waiting"<<endl;
    }

    cout<<"\nCar 101 leaves from J2"<<endl;
    int left = vehicleDepart(2);
    cout<<"Vehicle "<<left<<" left the network"<<endl;
    cout<<"Trying to depart from empty J5:"<<endl;
    vehicleDepart(5);
    cout<<"Length of J1 queue: "<<queueLength(1)<<endl;

    cout<<"\nQueues now:"<<endl;
    showAllQueues();

    cout<<"\nTraffic density:"<<endl;
    showDensity();

    cout<<"\nRush hour, 7 vehicles arrive at J4:"<<endl;
    for(int id = 301; id <= 307; id++){
        vehicleArrive(4, id);
    }
    cout<<"J4 has "<<queueLength(4)<<" waiting -> "<<congestionLevel(4)<<endl;
    for(int i = 0; i < 7; i++){
        vehicleDepart(4);       //rush hour is over
    }
    cout<<"After they leave, J4 is "<<congestionLevel(4)<<endl;

    cout<<"\n------------------------------------\n"<<endl;

//Emergency Vehicle routing:
    cout<<"# EMERGENCY DISPATCH #\n"<<endl;

    ArrayPriorityQueue EmergencyQ;
    EmergencyRequest r1 = {ambu.getID(),  ambu.getPriorityLvl(),  1000, ambu.getType(),  1, 6};
    EmergencyRequest r2 = {fireT.getID(), fireT.getPriorityLvl(), 2000, fireT.getType(), 3, 6};
    EmergencyRequest r3 = {ambu2.getID(), ambu2.getPriorityLvl(), 1500, ambu2.getType(), 5, 1};
    EmergencyQ.push(r1);
    EmergencyQ.push(r2);
    EmergencyQ.push(r3);

    while(!EmergencyQ.isEmpty()){
        EmergencyRequest req=EmergencyQ.top();
        EmergencyQ.pop();

        cout<<"-----------------------"<<endl;
        Vehicle* v = findVehicle(fleet,6,req.vehicleID);
        v->ShowVehicle();
        if(v->mustWait()){
            cout<<"This vehicle waits in the queue"<<endl;
        }
        else{
            cout<<"This vehicle skips the queue"<<endl;
        }
        cout<<"Going from J"<<req.from<<" to J"<<req.to<<endl;

        //shortest path,then put it into a linked list
        int dist[7] = {0};
        int prev[7] = {0};
        SinglyLinkedList& path = v->getRoute();
        city.dijkstra(req.from,dist,prev);
        if(!city.buildRoute(req.to,dist,prev,path)){
            cout<<"No route found!"<<endl;
            continue;
        }
        cout<<"Shortest distance: "<<dist[req.to]<<endl;
        cout<<"Route (stored in the vehicle): ";
        path.traverse();
        cout<<endl;

        cout<<"\nQueues before the run:"<<endl;
        RouteHistory history;
        vehicleArrive(req.from,req.vehicleID);     //joins the back of the queue
        showAllQueues();

        cout<<"\nDriving:"<<endl;
        driveEmergency(city,*v,history);

        cout<<"\nRoute History (latest first): ";
        history.showNewestFirst();
        cout<<endl;
        path.clear();
    }

    cout<<"-----------------------"<<endl;
    cout<<"Queues at the end:"<<endl;
    showAllQueues();

    cout<<"\n------------------------------------\n"<<endl;

//Route History and Backtracking:
    cout<<"# ROUTE HISTORY AND BACKTRACKING #\n"<<endl;

    RouteHistory trip;
    cout<<"History before any trip:"<<endl;
    trip.showNewestFirst();
    cout<<"Backtrack with nothing recorded:"<<endl;
    trip.backtrack();

    cout<<"\nAmbulance 102 starts at J1 and goes towards J6 through J4"<<endl;
    vehicleArrive(1,ambu.getID());
    trip.recordVisit(1);
    cout<<"Only J1 recorded, try to backtrack:"<<endl;
    trip.backtrack();

    moveEmergency(1,2,ambu.getID());
    trip.recordVisit(2);
    moveEmergency(2,4,ambu.getID());
    trip.recordVisit(4);

    cout<<"\nHistory in order:     ";
    trip.showInOrder();
    cout<<"History latest first: ";
    trip.showNewestFirst();
    cout<<"Ambulance is at J"<<trip.currentJunction()<<endl;
    showAllQueues();
    //pretend the road ahead of J4 is blocked, so it goes back the way it came
    cout<<"\nRoad ahead is blocked, ambulance goes back:"<<endl;
    while(trip.size()>1){
        int from = trip.currentJunction();
        int back = trip.backtrack();
        moveEmergency(from, back, ambu.getID());
        cout<<"History now: ";
        trip.showNewestFirst();
    }
    cout<<"Backtrack again at the start:"<<endl;
    trip.backtrack();

    junctionQ[0].remove(ambu.getID());      //ambulance is taken off the road
    cout<<"\nQueues after the ambulance is removed:"<<endl;
    showAllQueues();

    cout<<"\n------------------------------------\n"<<endl;

//UNREACHABLE CASE
    cout<<"# NO ROUTE CASE #\n"<<endl;

    Graph broken(6);
    broken.addRoad(1, 2, 5);        //only one road, J6 is cut off
    int d[7] = {0};
    int p[7] = {0};
    SinglyLinkedList none;
    broken.dijkstra(1, d, p);
    cout<<"Ambulance from J1 to J6 on the broken map:"<<endl;
    if(!broken.buildRoute(6, d, p, none)){
        cout<<"No route found, J6 cannot be reached"<<endl;
    }

    cout<<"\n------------------------------------\n"<<endl;

    return 0;
}