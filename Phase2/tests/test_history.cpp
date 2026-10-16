#include <iostream>
using namespace std;
#include "include/RouteHistory.hpp"

int passed = 0;
int failed = 0;

void check(bool ok, const char* what){
    if(ok){
        cout<<"PASS: "<<what<<endl;
        passed++;
    }
    else{
        cout<<"FAIL: "<<what<<endl;
        failed++;
    }
}

int main(){
    cout<<"--- empty history ---"<<endl;
    RouteHistory h;
    check(h.isEmpty() && h.size() == 0, "new history is empty");
    check(h.currentJunction() == -1, "no current junction when empty");
    check(h.backtrack() == -1, "cannot backtrack when empty");
    h.showInOrder();
    h.showNewestFirst();

    cout<<"\n--- one junction ---"<<endl;
    h.recordVisit(1);
    check(h.size() == 1 && h.currentJunction() == 1, "J1 recorded and it is the current junction");
    check(h.backtrack() == -1, "cannot backtrack with only one junction");
    check(h.size() == 1 && h.currentJunction() == 1, "failed backtrack does not change the history");

    cout<<"\n--- normal trip ---"<<endl;
    h.recordVisit(2);
    h.recordVisit(4);
    h.recordVisit(6);
    check(h.size() == 4 && h.currentJunction() == 6, "4 junctions recorded, J6 is current");
    h.showInOrder();
    h.showNewestFirst();
    check(h.size() == 4 && h.currentJunction() == 6, "showing the history does not destroy it");
    h.showNewestFirst();
    h.showInOrder();
    check(h.size() == 4 && h.currentJunction() == 6, "still the same after showing again");

    cout<<"\n--- backtracking ---"<<endl;
    check(h.backtrack() == 4, "back from J6 to J4");
    check(h.currentJunction() == 4 && h.size() == 3, "J4 is current now, history has 3 junctions");
    check(h.backtrack() == 2, "back from J4 to J2");
    check(h.backtrack() == 1, "back from J2 to J1");
    check(h.backtrack() == -1, "cannot go back from the starting junction");
    check(h.currentJunction() == 1 && h.size() == 1, "history is just J1");
    h.recordVisit(3);
    check(h.currentJunction() == 3 && h.size() == 2, "vehicle can go forward again after backtracking");

    cout<<"\n--- clear ---"<<endl;
    h.clear();
    check(h.isEmpty() && h.currentJunction() == -1, "clear empties the history");

    cout<<"\n--- full history (stack holds 32) ---"<<endl;
    int stored = 0;
    for(int i = 0; i < 40; i++){
        if(h.recordVisit(1 + i % 6)){
            stored++;
        }
    }
    check(stored == 32 && h.size() == 32, "only 32 junctions fit, the rest are refused");
    h.showNewestFirst();
    check(h.size() == 32, "a full history survives being shown");

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
