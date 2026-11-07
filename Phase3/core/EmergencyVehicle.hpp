#pragma once
#include <string>
using namespace std;
#include "Vehicle.hpp"

// phase 3: name for the type number the evaluator types (1, 2 or 3)
inline string emergencyTypeName(int type){
    if(type == PRIORITY_AMBULANCE){
        return "Ambulance";
    }
    if(type == PRIORITY_FIRE_TRUCK){
        return "Fire Truck";
    }
    if(type == PRIORITY_POLICE){
        return "Police";
    }
    return "Unknown";
}

class EmergencyVehicle : public Vehicle{
    string type;
    int priority;
    public:
        EmergencyVehicle(int id, string t, int p): Vehicle(id){
            type = t;
            priority = p;
        }
        bool mustWait(){
            return false;
        }
        int getPriority(){
            return priority;
        }
        string getType(){
            return type;
        }
};
