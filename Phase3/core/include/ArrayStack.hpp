#pragma once

class ArrayStack{
    int data[32];
    int top;
    public:
        ArrayStack(){
            top = -1;
        }

        bool isEmpty(){
            return top == -1;
        }

        bool isFull(){
            return top == 31;
        }

        int size(){
            return top + 1;
        }

        void push(int value);

        int pop();

        int getTop();
};