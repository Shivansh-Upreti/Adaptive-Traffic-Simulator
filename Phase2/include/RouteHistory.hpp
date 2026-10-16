#pragma once
#include <iostream>
using namespace std;
#include "ArrayStack.hpp"

// keeps the history of junctions a vehicle has actually passed
// the stack top is always the junction where the vehicle is right now
class RouteHistory {
    ArrayStack visited;

public:
    // call this every time the vehicle reaches a junction
    bool recordVisit(int junction) {
        if(visited.isFull()) {
            cout << "History is full, J" << junction << " not recorded" << endl;
            return false;
        }
        visited.push(junction);
        return true;
    }

    // -1 if nothing is recorded yet
    int currentJunction() {
        return visited.getTop();
    }

    // goes back one junction: removes the current one from the history
    // and returns the junction before it (the one the vehicle goes back to)
    // with 0 or 1 junction recorded there is nowhere to go back to, so -1
    int backtrack() {
        if(visited.size() < 2) {
            cout << "Cannot backtrack, there is no earlier junction" << endl;
            return -1;
        }
        visited.pop();
        return visited.getTop();
    }

    int size() {
        return visited.size();
    }

    bool isEmpty() {
        return visited.isEmpty();
    }

    void clear() {
        while(!visited.isEmpty()) {
            visited.pop();
        }
    }

    // latest junction first: J6 <- J2 <- J1
    // popping would destroy the history, so every junction is put
    // on a temporary stack while printing and then moved back
    void showNewestFirst() {
        if(visited.isEmpty()) {
            cout << "No route history yet" << endl;
            return;
        }
        ArrayStack temp;
        while(!visited.isEmpty()) {
            int j = visited.pop();
            cout << "J" << j;
            if(!visited.isEmpty()) {
                cout << " <- ";
            }
            temp.push(j);
        }
        while(!temp.isEmpty()) {
            visited.push(temp.pop());
        }
        cout << endl;
    }

    // first junction first: J1 -> J2 -> J6
    // moving everything to the temp stack reverses it, so popping
    // the temp stack gives the oldest junction first
    void showInOrder() {
        if(visited.isEmpty()) {
            cout << "No route history yet" << endl;
            return;
        }
        ArrayStack temp;
        while(!visited.isEmpty()) {
            temp.push(visited.pop());
        }
        while(!temp.isEmpty()) {
            int j = temp.pop();
            cout << "J" << j;
            if(!temp.isEmpty()) {
                cout << " -> ";
            }
            visited.push(j);
        }
        cout << endl;
    }
};
