#pragma once
#include <iostream>
using namespace std;
#include "SinglyLinkedList.hpp"

//# 1: OOP #
//Vehicle class
class Vehicle{
    int vehID;
    SinglyLinkedList route;     //phase 2: the route this vehicle will follow
    public:
        //constructor
        Vehicle(int id){
            vehID = id;
        }
        //route is a linked list, a copy would free the same nodes twice
        Vehicle(const Vehicle&) = delete;
        //virtual so that the emergency vehicle can replace it
        virtual void ShowVehicle(){
            cout<<"Vehicle ID: "<<vehID<<endl;
        }
        //normal vehicles have to wait in the queue
        virtual bool mustWait(){
            return true;
        }
        int getID(){
            return vehID;
        }
        SinglyLinkedList& getRoute(){
            return route;
        }
        virtual ~Vehicle(){
            route.clear();
        }
};
