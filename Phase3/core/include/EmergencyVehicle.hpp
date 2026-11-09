#pragma once
#include <string>
using namespace std;
#include "Vehicle.hpp"

// phase 3: name for the type number the evaluator types (1, 2 or 3)
string emergencyTypeName(int type);

//Emergency Vehicle class
class EmergencyVehicle : public Vehicle{
    string type;
    int priority;
    public:
        EmergencyVehicle(int id, string t, int p);
        void ShowEmergencyVehicle();
        void ShowVehicle(){
            ShowEmergencyVehicle();
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
        int getPriorityLvl(){
            return priority;
        }
};
