#pragma once
#include <iostream>
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
        Vehicle& operator=(const Vehicle&) = delete;   //no copying, every vehicle is unique and has its own route

        virtual void ShowVehicle(){
            cout<<"Vehicle ID: "<<vehID<<endl;
        }
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
