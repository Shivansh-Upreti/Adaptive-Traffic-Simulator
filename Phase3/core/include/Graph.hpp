#pragma once
#include <iostream>
using namespace std;
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"
#include "CityMap.hpp"          // NUM_JUNCTIONS, JUNCTION_SLOTS

const int NO_PATH = 999999;   // distance used for "cannot reach this junction"
// Travel TIME of a road (what the window shows next to every road). One tick of the live simulation is one second.
// A normal vehicle drives one cell (= one unit of road length) per tick, so an EMPTY road of length L takes
// L * SECONDS_PER_DISTANCE seconds. Every normal vehicle that stands still on the road adds WAIT_PENALTY_SECONDS.
const int SECONDS_PER_DISTANCE = 1;
const int WAIT_PENALTY_SECONDS = 1;

// one road leaving a junction
struct Edge {
    int to;
    int distance;
    Edge* next;
};

// road map as an adjacency list: every junction has its own linked list of roads
class Graph {
    Edge* adjList[JUNCTION_SLOTS];       // adjList[i] = roads leaving junction i (index 0 is not used)
    int totalJunctions;

    // looks for the road from -> to, returns nullptr if there is none
    Edge* findEdge(int from, int to);

    void addEdge(int from, int to, int distance);

    // phase 3: removes the one-way edge from -> to (returns false if there is none)
    // prev always points to the node just BEFORE e, so the list can be re-linked around e
    bool removeEdge(int from, int to);

public:
    Graph(int n);

    // phase 3: deletes every road. Each head pointer is set back to nullptr afterwards,
    // otherwise it would keep pointing at deleted memory (a dangling pointer)
    void clear();

    ~Graph() {
        clear();
    }

    // a copy would share the same edges and free them twice, so no copying
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // the second limit is a fixed number, so even a wrong totalJunctions can never let an
    // index reach past the 7-slot arrays
    bool isValid(int j) {
        return j >= 1 && j <= totalJunctions && j <= NUM_JUNCTIONS;
    }

    // roads are two way, so the edge is added in both lists
    void addRoad(int j1, int j2, int distance);

    bool hasRoad(int j1, int j2);

    // phase 3: closes a road. it is two way, so the edge is removed from both lists
    // returns false if there was no such road
    bool removeRoad(int j1, int j2);

    // phase 3: length of the road, 0 if there is no road
    int roadLength(int j1, int j2);

    // phase 3: estimated TRAVEL TIME of the road j1 -> j2 in seconds, 0 if there is no road.
    //   time = base time (length * SECONDS_PER_DISTANCE)
    //        + WAIT_PENALTY_SECONDS for every normal vehicle that is waiting on the road (waitingVehicles)
    //        + extraDelay: seconds that the caller adds for a preemption (an emergency vehicle slows or freezes the road)
    // The routing (dijkstra) still uses the plain length, so the routes of the demos do not change.
    int getTravelTime(int j1, int j2, int waitingVehicles, int extraDelay);


    // BFS: visits the nearest junctions first, uses the queue
    // returns how many junctions it managed to reach
    int bfs(int src);

    // DFS: goes as deep as it can first (recursion does the work of a stack)
    void dfsVisit(int u, bool visited[]);

    void dfs(int src);

    // shortest distance from src to every junction
    // dist[] = distance, prev[] = junction we came from (-1 for src and unreachable ones)
    bool dijkstra(int src, int dist[], int prev[]);

    // prev[] gives the route backwards (dest -> src), so first push it on the
    // stack and then pop it into the linked list, that way the list is src -> dest
    bool buildRoute(int dest, int dist[], int prev[], SinglyLinkedList &route);
};
