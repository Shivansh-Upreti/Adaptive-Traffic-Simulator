#pragma once
#include <iostream>
using namespace std;

const int QUEUE_CAPACITY = 32;

class ArrayQueue {
    int queue[QUEUE_CAPACITY];
    int front;
    int rear;
    public:
        ArrayQueue();

        bool isempty(){
            return rear<front;
        }

        bool isFull(){
            return size() == QUEUE_CAPACITY;
        }

        void enqueue(int x);

        int dequeue();

        int getFront();

        int size(){
            return rear - front + 1;
        }

        void traverse();

        bool remove(int x);

        //added in phase 3

        // the i-th vehicle counted from the front (0 = front), -1 if i is out of range
        int at(int i);

        // true if x is somewhere in the queue
        bool contains(int x);

        // empties the queue
        void clear();
};
