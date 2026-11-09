#include "../include/ArrayPriorityQueue.hpp"

int ArrayPriorityQueue::findBest() const{
    int pos = 0;
    for (int i = 1; i < n; i++){
        if (arr[i].priorityLVL < arr[pos].priorityLVL){
            pos = i;
        }
        else if (arr[i].priorityLVL == arr[pos].priorityLVL){
            if (arr[i].arrivalTime < arr[pos].arrivalTime){
                pos = i;
            }
        }
    }
    return pos;
}

bool ArrayPriorityQueue::push(EmergencyRequest x){
    if (isFull()){
        return false;
    }
    arr[n] = x;
    n++;
    return true;
}

EmergencyRequest ArrayPriorityQueue::top() const{
    if (isEmpty()){
        EmergencyRequest none = {-1, 0, 0, "", 0, 0};
        return none;
    }
    return arr[findBest()];
}

void ArrayPriorityQueue::pop(){
    if (isEmpty()){
        return;
    }

    int pos = findBest();

    // close the gap by shifting everything after pos one step left
    for (int i = pos; i < n - 1; i++){
        arr[i] = arr[i + 1];
    }
    n--;
}

bool ArrayPriorityQueue::containsVehicle(int vehicleID) const{
    for (int i = 0; i < n; i++){
        if (arr[i].vehicleID == vehicleID){
            return true;
        }
    }
    return false;
}

EmergencyRequest ArrayPriorityQueue::getAt(int i){
    if (i < 0 || i >= n){
        EmergencyRequest none = {-1, 0, 0, "", 0, 0};
        return none;
    }
    return arr[i];
}
