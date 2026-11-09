#pragma once
#include <iostream>
using namespace std;

const int QUEUE_CAPACITY = 32;

// simple FIFO queue stored in an array (unchanged from phase 2 except the
// two small helper functions at the bottom)
class ArrayQueue {
    int queue[QUEUE_CAPACITY];
    int front;
    int rear;
    public:
        ArrayQueue(){
            rear=-1;
            front=0;
        }

        bool isempty(){
            return rear<front;
        }

        bool isFull(){
            return size() == QUEUE_CAPACITY;
        }

        void enqueue(int x){
            if(isFull()){
                cout<<"Queue is Full"<<endl;
                return;
            }

            // rear reached the end of the array but there is space at the front
            // (vehicles already left), so shift everyone back to index 0 first
            if(rear == QUEUE_CAPACITY - 1){
                int count = size();
                for(int i = 0; i < count; i++){
                    queue[i] = queue[front + i];
                }
                front = 0;
                rear = count - 1;
            }
            rear++;
            queue[rear]=x;
        }

        int dequeue(){
            if (isempty()){
                cout<<"queue is empty"<<endl;
                return -1;
            }
            int x=queue[front];
            front++;
            return x;
        }

        int getFront(){
            if (isempty()){
                cout<<"queue is empty"<<endl;
                return -1;
            }
            return queue[front];
        }

        int size(){
            return rear - front + 1;
        }

        void traverse(){
            if(isempty()){
                cout<<"Queue is empty"<<endl;
                return;
            }
            for(int i=front; i<=rear; i++){
                cout<<queue[i]<<", ";
            }
        }

        bool remove(int x){
            if (isempty()){
                return false;
            }
            int pos = -1;

            // Find the vehicle
            for (int i = front; i <= rear; i++){
                if (queue[i] == x){
                    pos = i;
                    break;
                }
            }

            // Vehicle not found
            if (pos == -1){
                return false;
            }

            // Shift elements left
            for (int i = pos; i < rear; i++){
                queue[i] = queue[i + 1];
            }
            rear--;
            return true;
        }

        // ---- added in phase 3 ----

        // the i-th vehicle counted from the front (0 = front), -1 if i is out of range
        int at(int i){
            if(i < 0 || i >= size()){
                return -1;
            }
            return queue[front + i];
        }

        // true if x is somewhere in the queue
        bool contains(int x){
            for(int i = front; i <= rear; i++){
                if(queue[i] == x){
                    return true;
                }
            }
            return false;
        }
        
        void clear(){
            front = 0;
            rear = -1;
        }
};
