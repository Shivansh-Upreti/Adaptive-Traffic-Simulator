#include "../include/ArrayStack.hpp"

void ArrayStack::push(int value){
    if (isFull()){
        return;
    }
    top++;
    data[top] = value;
}

int ArrayStack::pop(){
    if (isEmpty()){
        return -1;
    }
    int value = data[top];
    top--;
    return value;
}

int ArrayStack::getTop(){
    if (isEmpty()){
        return -1;
    }
    return data[top];
}
