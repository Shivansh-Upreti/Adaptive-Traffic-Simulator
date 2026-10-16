#pragma once
#include <string>
using namespace std;
#include "Vehicle.hpp"

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
        //overrides Vehicle::ShowVehicle
        void ShowVehicle(){
            ShowEmergencyVehicle();
        }
        //emergency vehicles skip the queue
        bool mustWait(){
            return false;
        }
        string getType(){
            return type;
        }
        int getPriorityLvl(){
            return priority;
        }


};
