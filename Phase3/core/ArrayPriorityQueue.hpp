#pragma once
#include "EmergencyRequest.hpp"

const int PQ_CAPACITY = 10;

class ArrayPriorityQueue{
    EmergencyRequest arr[PQ_CAPACITY];
    int n;
    int findBest(){
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

    public:
        ArrayPriorityQueue(){
            n = 0;
        }

        bool isEmpty(){
            return n == 0;
        }

        bool isFull(){
            return n == PQ_CAPACITY;
        }

        int size(){
            return n;
        }

        // phase 3: returns false when the queue is full (phase 2 dropped the request silently)
        bool push(EmergencyRequest x){
            if (isFull()){
                return false;
            }
            arr[n] = x;
            n++;
            return true;
        }

        // phase 3: on an empty queue this returns a request with vehicleID -1
        // (phase 2 returned an uninitialised one)
        EmergencyRequest top(){
            if (isEmpty()){
                EmergencyRequest none = {-1, 0, 0, "", 0, 0};
                return none;
            }
            return arr[findBest()];
        }

        void pop(){
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

        // phase 3: removes every request
        void clear(){
            n = 0;
        }

        // phase 3: true if this vehicle already has a request waiting
        bool containsVehicle(int vehicleID){
            for (int i = 0; i < n; i++){
                if (arr[i].vehicleID == vehicleID){
                    return true;
                }
            }
            return false;
        }

        // phase 3: i-th request in storage order (NOT priority order), vehicleID -1 if out of range
        EmergencyRequest getAt(int i){
            if (i < 0 || i >= n){
                EmergencyRequest none = {-1, 0, 0, "", 0, 0};
                return none;
            }
            return arr[i];
        }
};
