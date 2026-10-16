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
    cout<<"# 2: ARRAY #"<<endl;
    
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

//QUEUE of all vehicles at J:
    cout<<"# 3: QUEUE #\n"<<endl;
    
    ArrayQueue vehicleQ;
    vehicleQ.enqueue(car.getID());
    vehicleQ.enqueue(bike.getID());
    vehicleQ.enqueue(ambu.getID());
    vehicleQ.enqueue(fireT.getID());
    vehicleQ.enqueue(bus.getID());
    vehicleQ.enqueue(ambu2.getID());
    cout<<"Vehicles At Junction:"<<endl;
    cout<<"Size: "<<vehicleQ.size()<<endl;
    cout<<"[ ";
    vehicleQ.traverse();
    cout<<"]"<<endl;

    cout<<"\n\n";
    cout<<"------------------------------------\n"<<endl;

//QUEUE of emergency vehicles:
   cout<<"# 4: PRIORITY QUEUE #\n"<<endl;
    
    cout<<"Emergency Vehicles Coming!!\n"<<endl;
    ArrayPriorityQueue EmergencyQ;
    EmergencyRequest r1 = {ambu.getID(), ambu.getPriorityLvl(), 1000, ambu.getType()};
    EmergencyRequest r2 = {fireT.getID(), fireT.getPriorityLvl(), 2000, fireT.getType()};
    EmergencyRequest r3 = {ambu2.getID(), ambu2.getPriorityLvl(), 1500, ambu2.getType()};

    //emergency requests pushed into P.Queue
    EmergencyQ.push(r1);EmergencyQ.push(r2);EmergencyQ.push(r3);
    cout<<"Emergency Requests Added!!"<<endl;

    cout<<"\n\n";
    cout<<"------------------------------------\n"<<endl;

//Vehicle Going from J
    cout<<"============================"<<endl;
    cout<<"VEHICLES GOING FROM JUNCTION"<<endl;
    cout<<"============================\n"<<endl;

    cout<<"Emergency Vehicle Going From Junction:"<<endl;
    while(!EmergencyQ.isEmpty()){
        EmergencyRequest topRequest = EmergencyQ.top();
        cout<<"Vehicle ID: "<<topRequest.vehicleID<<endl;
        cout<<"Type: "<<topRequest.label<<endl;
        cout<<"Priority Level: "<<topRequest.priorityLVL<<endl;
        cout<<"Arrival Time: "<<topRequest.arrivalTime<<endl;

        cout<<"\n";
        EmergencyQ.pop();
        vehicleQ.remove(topRequest.vehicleID);
    }
    cout<<"\n\n";
    cout<<"Normal Vehicles Going From Junction:"<<endl;
    while(!vehicleQ.isempty()){
        cout<<"Vehicle ID: "<<vehicleQ.dequeue()<<endl;
        cout<<"\n";
    }

    cout<<"\n\n";
    cout<<"------------------------------------\n"<<endl;

//ROUTE:
    cout<<"#5: LINKED LIST #\n"<<endl;
   
     SinglyLinkedList route;

    route.pushBack(junctions[0].junctionID);
    route.pushBack(junctions[2].junctionID);
    route.pushBack(junctions[4].junctionID);
    route.pushBack(junctions[5].junctionID);

    cout<<"Planned Route for EV:"<<endl;
    route.traverse();

    
    cout<<"\n\n";
    cout<<"------------------------------------\n"<<endl;


//ROUTE HISTORY:
    cout<<"# 6: STACK #\n"<<endl; 

    ArrayStack routeHist;

    cout<<"Route History of EV:"<<endl;
    SinglyLinkedList::Node* point=route.getHead();
    while(point!=nullptr){
        routeHist.push(point->data);
        point=point->next;
    }

    while(!routeHist.isEmpty()){
        int junctionID=routeHist.pop();
        cout<<junctions[junctionID-1].name<<" ";
        if(!routeHist.isEmpty()){
            cout<<" <-  ";
        }
    }

    cout<<"\n\n";
    cout<<"------------------------------------\n"<<endl;


//ROAD NETWORK:
    cout<<"# 7: GRAPH #\n"<<endl; 

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
    cout<<"# 8: TRAFFIC FLOW AT JUNCTIONS #\n"<<endl;

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
    cout<<"# 9: EMERGENCY DISPATCH #\n"<<endl;

    return 0;
}