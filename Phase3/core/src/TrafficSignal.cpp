#include "../include/TrafficSignal.hpp"

bool TrafficSignal::release(){
    if(emergencyHolds == 0){
        return false;
    }
    emergencyHolds--;
    return true;
}

