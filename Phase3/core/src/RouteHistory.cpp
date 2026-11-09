#include "../include/RouteHistory.hpp"

bool RouteHistory::recordVisit(int junction){
    if(visited.isFull()) {
        cout << "History is full, J" << junction << " not recorded" << endl;
        return false;
    }
    visited.push(junction);
    return true;
}

int RouteHistory::backtrack(){
    if(visited.size() < 2) {
        cout << "Cannot backtrack, there is no earlier junction" << endl;
        return -1;
    }
    visited.pop();
    return visited.getTop();
}

void RouteHistory::clear(){
    while(!visited.isEmpty()) {
        visited.pop();
    }
}

void RouteHistory::showNewestFirst(){
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

void RouteHistory::showInOrder(){
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
