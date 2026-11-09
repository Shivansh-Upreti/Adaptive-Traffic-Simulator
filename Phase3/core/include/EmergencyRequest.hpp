#pragma once
#include <string>
using namespace std;

// priority levels: the SMALLER the number, the MORE urgent the vehicle
const int PRIORITY_AMBULANCE = 1;
const int PRIORITY_FIRE_TRUCK = 2;
const int PRIORITY_POLICE = 3;
const int NORMAL_PRIORITY = 10;

//emergency request info
struct EmergencyRequest{
    int vehicleID;
    int priorityLVL;
    long long arrivalTime;
    string label;
    int from = 0;   //phase 2: junction where the vehicle starts
    int to = 0;     //phase 2: junction where it has to reach
};
