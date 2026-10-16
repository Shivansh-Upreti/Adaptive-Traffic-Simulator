#pragma once
#include <string>
using namespace std;

//emergency request info
struct EmergencyRequest{
    int vehicleID;
    int priorityLVL;
    long long arrivalTime;
    string label;
    int from = 0;   //phase 2: junction where the vehicle starts
    int to = 0;     //phase 2: junction where it has to reach
};
