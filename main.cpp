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

//comparison
struct EmergencyComparison{
    bool operator()(const EmergencyRequest& a, const EmergencyRequest& b){
        if(a.priorityLVL == b.priorityLVL){

            return a.arrivalTime > b.arrivalTime;
        }

        return a.priorityLVL < b.priorityLVL;
    }
};

//Vehicle class
class Vehicle{
    private:
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
            //ShowVehicle();
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
/*#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"*/

int main(){
    cout<<"\n=================================================="<<endl;
    cout<<"ADAPTIVE TRAFFIC SIGNAL AND EMERGENCY VEHICLE SIMULATOR"<<endl;
    cout<<"==================================================\n"<<endl;

    Vehicle car(101);
    Vehicle bike(121);
    Vehicle bus(223);
    EmergencyVehicle ambu(102, "Ambulance", 1);
    EmergencyVehicle fireT(202, "Fire Truck", 2);
    EmergencyVehicle ambu2(221, "Ambulance", 1);

    /*Vehicle car(101);
    car.ShowVehicle();
    cout<<"\n";
    EmergencyVehicle ambulance(102, "Ambulance", 1);
    ambulance.ShowEmergencyVehicle();*/

//JUNCTIONS:
    cout<<"# 1: ARRAY #"<<endl;
    
    Junction junctions[6]={
        {1, "J1"},
        {2, "J2"},
        {3, "J3"},
        {4, "J4"},
        {5, "J5"},
        {6, "J6"}
    };
    cout<<"Junctions Info. :"<<endl;
    for(int i=0; i<6; i++){
        cout<<"Junction ID:"<<junctions[i].junctionID;
        cout<<" , Name:"<<junctions[i].name<<endl;
    }
    cout<<"\n";
    cout<<"------------------------------------\n"<<endl;

//QUEUE of all vehicles at J:
    cout<<"# 2: QUEUE #\n"<<endl;
    
    cout<<"------------------------------------\n"<<endl;

//QUEUE of emergency vehicles:
    cout<<"# 3: PRIORITY QUEUE #\n"<<endl;
    
    cout<<"Emergency Vehicles Coming!!\n"<<endl;
    ArrayPriorityQueue EmergencyQ;
    EmergencyRequest r1 = {ambu.getID(), ambu.getPriorityLvl(), 1000, ambu.getType()};
    EmergencyRequest r2 = {fireT.getID(), fireT.getPriorityLvl(), 2000, fireT.getType()};
    EmergencyRequest r3 = {ambu2.getID(), ambu2.getPriorityLvl(), 1500, ambu2.getType()};

    //emergency requests pushed into P.Queue
    EmergencyQ.push(r1);EmergencyQ.push(r2);EmergencyQ.push(r3);
    cout<<"Emergency Requests Added!!\n\n"<<endl;

    cout<<"Emergency Vehicles Going!!!\n"<<endl;
    while(!EmergencyQ.isEmpty()){
        EmergencyRequest topReq = EmergencyQ.top();
        cout<<"Vehicle ID: "<<topReq.vehicleID<<", Priority Level: "<<topReq.priorityLVL<<", Arrival Time: "<<topReq.arrivalTime<<", Label: "<<topReq.label<<endl;
        EmergencyQ.pop();
    }

    cout<<"\n";
    cout<<"------------------------------------\n"<<endl;

//ROUTE:
    cout<<"#4: LINKED LIST #\n"<<endl;
   


    cout<<"------------------------------------\n"<<endl;


//ROUTE HISTORY:
    cout<<"# 5: STACK #\n"<<endl; 


    cout<<"------------------------------------\n"<<endl;


//CONCEPT OF OOP:
    cout<<"# 6: OOP #"<<endl;

    cout<<"===================="<<endl;
    cout<<"Vehicle Info."<<endl;
    cout<<"====================\n"<<endl;
    cout<<"Car: ";
    car.ShowVehicle();
    cout<<"\n";

    cout<<"Bike: ";
    bike.ShowVehicle();
    cout<<"\n";

    cout<<"Bus: ";
    bus.ShowVehicle();
    cout<<"\n";

    //Emergency Vehicle(EV)
    cout<<"EV: ";
    ambu.ShowEmergencyVehicle();
    cout<<"\n";

    cout<<"EV: ";
    fireT.ShowEmergencyVehicle();
    cout<<"\n";

    cout<<"EV: ";
    ambu2.ShowEmergencyVehicle();
    cout<<"\n";



}