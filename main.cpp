/*#include "ArrayPriorityQueue.hpp"
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"*/

#include <iostream>
#include <string>
using namespace std;
//Junction consist of "ID" and its "name"
struct Junction{
    int junctionID;
    string name;
};

//Emergency Request Information
struct EmergencyRequest{
    int vehicleID;
    int priorityLVL;
    long long arrivalTime;
    string label;
};

//Emergency Comparison
struct EmergencyComparison{
    bool operator()(const EmergencyRequest& a, const EmergencyRequest& b){
        if(a.priorityLVL == b.priorityLVL){
            // Higher arrival time means later
            return a.arrivalTime > b.arrivalTime;
        }
        // Lower priority level means higher priority
        return a.priorityLVL < b.priorityLVL;
    }
};

//Vehicle class
class Vehicle{
    private:
        int vehID;
    public:
        //Vehicle class constructor
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

//Emergency Vehicle class to represent an emergency vehicle
class EmergencyVehicle : public Vehicle{
    string type;
    int priority;
    public:
        //EmergencyVehicle inherited class constructor
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

int main(){
    cout<<"\n=================================================="<<endl;
    cout<<"ADAPTIVE TRAFFIC SIGNAL AND EMERGENCY VEHICLE SIMULATOR --> PHASE 1" <<endl;
    cout<<"==================================================\n"<<endl;
    /*Vehicle car(101);
    car.ShowVehicle();
    cout<<"\n";
    EmergencyVehicle ambulance(102, "Ambulance", 1);
    ambulance.ShowEmergencyVehicle();*/

    //ARRAY OF JUNCTIONS:
    cout<<"STEP 1: ARRAY"<<endl;
    cout<<"--------------------------------------------------\n"<<endl;
    Junction junctions[6]={
        {1, "J1"},
        {2, "J2"},
        {3, "J3"},
        {4, "J4"},
        {5, "J5"},
        {6, "J6"}
    };
    cout<<"Junctions Information:\n"<< std::endl;
    for(int i=0; i<6; i++){
        cout<<"Junction ID: "<<junctions[i].junctionID<<", Name: "<<junctions[i].name<<endl;
    }
    cout<<"\n";

    //QUEUE OF VEHICLES:
    cout<<"STEP 2: QUEUE"<<endl;
    cout<<"--------------------------------------------------\n"<<endl;




    //PRIORITY QUEUE OF EMERGENCY VEHICLES:
    cout<<"STEP 3: PRIORITY QUEUE"<<endl;
    cout<<"--------------------------------------------------\n"<<endl;



    //LINKED LIST FOR ROUTE:
    cout<<"STEP 4: LINKED LIST"<<endl;
    cout<<"--------------------------------------------------\n"<<endl;



    //STACK FOR ROUTE HISTORY:
    cout<<"STEP 5: STACK"<<endl;
    cout<<"--------------------------------------------------\n"<<endl; 



    //CONCEPT OF OOP:
    cout<<"STEP 6: OOP"<<endl;
    cout<<"--------------------------------------------------\n"<<endl;

    Vehicle car(101);
    cout<<"Car Information:\n";
    car.ShowVehicle();
    cout<<"\n";
    EmergencyVehicle ambulance(102, "Ambulance", 2);
    ambulance.ShowEmergencyVehicle();
    cout<<"\n";

    cout<<"Inherited ID: "<<ambulance.getID()<<endl;
    cout<<"Priority Level: "<<ambulance.getPriorityLvl()<<endl;



}