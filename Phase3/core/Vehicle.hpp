#pragma once
using namespace std;
#include "SinglyLinkedList.hpp"
#include "EmergencyRequest.hpp"

//# 1: OOP #
//Vehicle class
class Vehicle{
    int vehID;
    SinglyLinkedList route;
    public:
        Vehicle(int id){
            vehID = id;
        }
        Vehicle(const Vehicle&) = delete;
        Vehicle& operator=(const Vehicle&) = delete;    // phase 3: no copying in any form

        virtual bool mustWait(){
            return true;
        }
        // phase 3: normal vehicles have the lowest priority (EmergencyVehicle overrides this)
        virtual int getPriority(){
            return NORMAL_PRIORITY;
        }
        int getID(){
            return vehID;
        }
        SinglyLinkedList& getRoute(){
            return route;
        }
        virtual ~Vehicle(){
        }
};
