#pragma once
#include <iostream>
using namespace std;
#include "ArrayStack.hpp"

class RouteHistory {
    ArrayStack visited;

public:
    // call this every time the vehicle reaches a junction
    bool recordVisit(int junction);

    // -1 if nothing is recorded yet
    int currentJunction() {
        return visited.getTop();
    }

    // goes back one junction: removes the current one from the history
    // and returns the junction before it (the one the vehicle goes back to)
    // with 0 or 1 junction recorded there is nowhere to go back to, so -1
    int backtrack();

    int size() {
        return visited.size();
    }

    bool isEmpty() {
        return visited.isEmpty();
    }

    void clear();

    // latest junction first: J6 <- J2 <- J1
    // popping would destroy the history, so every junction is put
    // on a temporary stack while printing and then moved back
    void showNewestFirst();

    // first junction first: J1 -> J2 -> J6
    // moving everything to the temp stack reverses it, so popping
    // the temp stack gives the oldest junction first
    void showInOrder();
};
