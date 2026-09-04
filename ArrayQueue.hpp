#pragma once 

class ArrayQueue { 
    int queue[32]; 
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
            return rear == 31; 
        }

        void enqueue(int x){ 
            if(isFull()){
                cout<<"Queue is Full"<<endl; 
                return; 
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
};