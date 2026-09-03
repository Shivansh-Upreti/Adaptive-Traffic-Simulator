#pragma once
#include <stdexcept>

template <class T>
class ArrayQueue
{
private:
    T queue[32];
    int front;
    int rear;
public:
    ArrayQueue()
    {
        rear=-1;
        front=0;
    }
    bool isempty()
    {
        return rear<front;
    }
    bool isFull()
    {
        return rear == 31;
    }
    void enqueue(T x)
    {
        if(isFull())
        {
            cout<<"Queue is Full"<<endl;
            return;
        }
        rear++;
        queue[rear]=x;
    }
    T dequeue()
    {
        if (isempty())
        {
            cout<<"queue is empty"<<endl;
            return T();
        }

        T x=queue[front];
        front++;

        return x;
    }
    T getFront()
    {
        if (isempty())
        {
            cout<<"queue is empty"<<endl;
            return T();
        }
        return queue[front];
    }
    int size()
    {
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
};