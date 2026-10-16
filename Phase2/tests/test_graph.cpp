#include <iostream>
using namespace std;
#include "include/Graph.hpp"

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

// same map that main.cpp uses
void buildMap(Graph &g){
    g.addRoad(1, 2, 5);
    g.addRoad(1, 3, 4);
    g.addRoad(2, 4, 4);
    g.addRoad(3, 4, 5);
    g.addRoad(3, 5, 5);
    g.addRoad(4, 5, 4);
    g.addRoad(2, 6, 6);
    g.addRoad(4, 6, 5);
    g.addRoad(5, 6, 7);
}

// reads the route list into an array, returns how many junctions it has
int routeToArray(SinglyLinkedList &route, int arr[]){
    int n = 0;
    SinglyLinkedList::Node* p = route.getHead();
    while(p != nullptr){
        arr[n] = p->data;
        n++;
        p = p->next;
    }
    return n;
}

int main(){
    cout<<"--- roads ---"<<endl;
    Graph g(6);
    buildMap(g);
    check(g.hasRoad(1, 2) && g.hasRoad(2, 1), "road is two way");
    check(!g.hasRoad(1, 6), "no road between J1 and J6");
    check(!g.hasRoad(0, 1) && !g.hasRoad(1, 7), "hasRoad is false for junctions that do not exist");
    g.addRoad(0, 1, 5);
    g.addRoad(1, 7, 5);
    g.addRoad(1, 1, 5);
    g.addRoad(1, 4, 0);
    check(!g.hasRoad(1, 4), "bad roads are rejected (junction 0, junction 7, loop, distance 0)");

    cout<<"\n--- dijkstra ---"<<endl;
    int dist[7] = {0};
    int prev[7] = {0};
    check(g.dijkstra(1, dist, prev), "dijkstra accepts J1 as source");
    check(dist[1] == 0, "distance to itself is 0");
    check(dist[2] == 5 && dist[3] == 4, "J2 = 5, J3 = 4");
    check(dist[4] == 9 && dist[5] == 9, "J4 = 9, J5 = 9");
    check(dist[6] == 11, "J6 = 11");
    check(prev[1] == -1, "source has no previous junction");
    check(!g.dijkstra(0, dist, prev) && !g.dijkstra(7, dist, prev), "bad source junction is refused");

    cout<<"\n--- route as linked list ---"<<endl;
    int arr[10];
    g.dijkstra(1, dist, prev);
    SinglyLinkedList r1;
    check(g.buildRoute(6, dist, prev, r1), "route J1 to J6 found");
    int n = routeToArray(r1, arr);
    check(n == 3 && arr[0] == 1 && arr[1] == 2 && arr[2] == 6, "route is J1 -> J2 -> J6 (source first)");

    g.dijkstra(5, dist, prev);
    SinglyLinkedList r2;
    g.buildRoute(1, dist, prev, r2);
    n = routeToArray(r2, arr);
    check(dist[1] == 9 && n == 3 && arr[0] == 5 && arr[1] == 3 && arr[2] == 1, "J5 to J1 = 9 through J3");

    SinglyLinkedList r3;
    g.buildRoute(5, dist, prev, r3);
    n = routeToArray(r3, arr);
    check(n == 1 && arr[0] == 5, "source to itself is a route with one junction");

    SinglyLinkedList r4;
    check(!g.buildRoute(9, dist, prev, r4) && r4.isEmpty(), "destination that does not exist gives no route");

    cout<<"\n--- unreachable ---"<<endl;
    Graph broken(6);
    broken.addRoad(1, 2, 5);
    broken.dijkstra(1, dist, prev);
    SinglyLinkedList r5;
    check(dist[6] == NO_PATH, "cut off junction has NO_PATH distance");
    check(!broken.buildRoute(6, dist, prev, r5) && r5.isEmpty(), "no route to a cut off junction");

    cout<<"\n--- BFS and DFS ---"<<endl;
    check(g.bfs(1) == 6, "BFS reaches all 6 junctions");
    check(broken.bfs(1) == 2, "BFS on the broken map reaches only J1 and J2");
    check(g.bfs(0) == 0, "BFS from a bad junction reaches nothing");
    g.dfs(1);
    broken.dfs(1);

    cout<<"\n--- other ---"<<endl;
    Graph tiny(9);
    tiny.addRoad(6, 5, 3);
    check(tiny.hasRoad(6, 5), "Graph(9) is cut down to 6 junctions and still works");
    tiny.addRoad(5, 6, 8);
    tiny.dijkstra(6, dist, prev);
    check(dist[5] == 8, "adding the same road again only changes its distance");

    r1.clear();
    r2.clear();
    r3.clear();
    r4.clear();
    r5.clear();

    cout<<"\n"<<passed<<" passed, "<<failed<<" failed"<<endl;
    return failed;
}
