#include "../include/ArrayQueue.hpp"

ArrayQueue::ArrayQueue(){
    rear=-1;
    front=0;
}

void ArrayQueue::enqueue(int x){
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

int ArrayQueue::dequeue(){
    if (isempty()){
        cout<<"queue is empty"<<endl;
        return -1;
    }
    int x=queue[front];
    front++;
    return x;
}

int ArrayQueue::getFront(){
    if (isempty()){
        cout<<"queue is empty"<<endl;
        return -1;
    }
    return queue[front];
}

void ArrayQueue::traverse(){
    if(isempty()){
        cout<<"Queue is empty"<<endl;
        return;
    }
    for(int i=front; i<=rear; i++){
        cout<<queue[i]<<", ";
    }
}

bool ArrayQueue::remove(int x){
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

    for (int i = pos; i < rear; i++){
        queue[i] = queue[i + 1];
    }
    rear--;
    return true;
}

int ArrayQueue::at(int i){
    if(i < 0 || i >= size()){
        return -1;
    }
    return queue[front + i];
}

bool ArrayQueue::contains(int x){
    for(int i = front; i <= rear; i++){
        if(queue[i] == x){
            return true;
        }
    }
    return false;
}

void ArrayQueue::clear(){
    front = 0;
    rear = -1;
}
