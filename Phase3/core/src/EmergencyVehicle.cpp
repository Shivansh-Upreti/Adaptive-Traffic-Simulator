#include "../include/EmergencyVehicle.hpp"

string emergencyTypeName(int type){
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

EmergencyVehicle::EmergencyVehicle(int id, string t, int p) : Vehicle(id){
    type = t;
    priority = p;
}

void EmergencyVehicle::ShowEmergencyVehicle(){
    cout<<"Emergency Vehicle ID: "<<getID()<<endl;
    cout<<"Type: "<<type<<endl;
    cout<<"Priority: "<<priority<<endl;
}
