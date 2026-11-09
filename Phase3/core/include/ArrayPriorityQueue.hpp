#pragma once
#include "EmergencyRequest.hpp"

const int PQ_CAPACITY = 10;

class ArrayPriorityQueue{
    EmergencyRequest arr[PQ_CAPACITY];
    int n;
    
    int findBest() const;

    public:
        ArrayPriorityQueue(){
            n = 0;
        }

        bool isEmpty() const {
            return n == 0;
        }

        bool isFull(){
            return n == PQ_CAPACITY;
        }

        int size(){
            return n;
        }

        // phase 3: returns false when the queue is full (phase 2 dropped the request silently)
        bool push(EmergencyRequest x);

        // phase 3: on an empty queue this returns a request with vehicleID -1
        // (phase 2 returned an uninitialised one)
        EmergencyRequest top() const;

        void pop();

        // phase 3: removes every request
        void clear(){
            n = 0;
        }

        // phase 3: true if this vehicle already has a request waiting
        bool containsVehicle(int vehicleID) const;

        // phase 3: i-th request in storage order (NOT priority order), vehicleID -1 if out of range
        EmergencyRequest getAt(int i);
};
