#pragma once

class ArrayPriorityQueue{
    EmergencyRequest arr[10];
    int n;
    public:
        ArrayPriorityQueue(){
            n = 0;
        }

        bool isEmpty(){
            return n == 0;
        }

        bool isFull(){
            return n == 10;
        }

    
        int size(){
            return n;
        }

        void push(EmergencyRequest x){
            if (isFull()){
                return;
            }
            arr[n] = x;
            n++;
        }

        EmergencyRequest top(){
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
            return arr[pos];
        }

        void pop(){
            if (isEmpty()){
                return;
            }

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

            for (int i = pos; i < n - 1; i++){
                arr[i] = arr[i + 1];
            }
            n--;
        }
};