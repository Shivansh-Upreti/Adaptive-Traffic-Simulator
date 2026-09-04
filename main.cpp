#include <iostream>
#include <string>
using namespace std;

//junction node
struct Junction{
    int junctionID;
    string name;
};

//emergency request info
struct EmergencyRequest{
    int vehicleID;
    int priorityLVL;
    long long arrivalTime;
    string label;
};

//# 1: OOP #
//Vehicle class
class Vehicle{
    int vehID;
    public:
        //constructor
        Vehicle(int id){
            vehID = id;
        }
        void ShowVehicle(){
            cout<<"Vehicle ID: "<<vehID<<endl;
        }
        int getID(){
            return vehID;
        }
};

//Emergency Vehicle class
class EmergencyVehicle : public Vehicle{
    string type;
    int priority;
    public:
        //constructor
        EmergencyVehicle(int id, string t, int p): Vehicle(id){
            type = t;
            priority = p;
        }
        void ShowEmergencyVehicle(){
            cout<<"Emergency Vehicle ID: "<<getID()<<endl;
            cout<<"Type: "<<type<<endl;
            cout<<"Priority: "<<priority<<endl;
        }
        string getType(){
            return type;
        }
        int getPriorityLvl(){
            return priority;
        }


};

#include "ArrayPriorityQueue.hpp"
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"

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
    return 0;

}