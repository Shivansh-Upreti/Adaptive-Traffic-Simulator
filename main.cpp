/*#include "ArrayPriorityQueue.hpp"
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"*/

#include <iostream>
#include <string>

class Vehicle{
    private:
        int vehID;
    public:
        Vehicle(int id){
            vehID = id;
        }
        void ShowVehicle(){
            std::cout<<"Vehicle ID: "<<vehID<<std::endl;
        }
        int getID(){
            return vehID;
        }
};
class EmergencyVehicle : public Vehicle{
    std::string type;
    int priority;
    public:
        EmergencyVehicle(int id, std::string t, int p): Vehicle(id){
            type = t;
            priority = p;
        }
        void ShowEmergencyVehicle(){
            ShowVehicle();
            std::cout<<"Emergency Vehicle ID: "<<getID()<<std::endl;
            std::cout<<"Type: "<<type<<std::endl;
            std::cout<<"Priority: "<<priority<<std::endl;
        }
};

int main(){
    Vehicle car(101);
    car.ShowVehicle();
    EmergencyVehicle ambulance(102, "Ambulance", 1);
    ambulance.ShowEmergencyVehicle();
}