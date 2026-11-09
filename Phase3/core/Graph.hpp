#pragma once
#include <iostream>
using namespace std;
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"

const int NO_PATH = 999999;   // distance used for "cannot reach this junction"
// Travel TIME of a road (what the window shows next to every road). One tick of the live simulation is one second.
// A normal vehicle drives one cell (= one unit of road length) per tick, so an EMPTY road of length L takes
// L * SECONDS_PER_DISTANCE seconds. Every normal vehicle that stands still on the road adds WAIT_PENALTY_SECONDS.
const int SECONDS_PER_DISTANCE = 1;
const int WAIT_PENALTY_SECONDS = 1;
const int GRAPH_MAX_JUNCTIONS = 6;   // adjList[] and the dist[]/prev[] arrays of the callers have 7 slots (0 unused)

// one road leaving a junction
struct Edge {
    int to;
    int distance;
    Edge* next;
};

// road map as an adjacency list: every junction has its own linked list of roads
class Graph {
    Edge* adjList[7];       // adjList[i] = roads leaving junction i (index 0 is not used)
    int totalJunctions;

    // looks for the road from -> to, returns nullptr if there is none
    Edge* findEdge(int from, int to) {
        Edge* e = adjList[from];
        while(e != nullptr) {
            if(e->to == to) return e;
            e = e->next;
        }
        return nullptr;
    }

    void addEdge(int from, int to, int distance) {
        Edge* old = findEdge(from, to);
        if(old != nullptr) {        // road already exists, just change the distance
            old->distance = distance;
            return;
        }

        Edge* e = new Edge;
        e->to = to;
        e->distance = distance;
        e->next = nullptr;

        if(adjList[from] == nullptr) {
            adjList[from] = e;
        }
        else {
            Edge* last = adjList[from];
            while(last->next != nullptr) {
                last = last->next;
            }
            last->next = e;
        }
    }

    // phase 3: removes the one-way edge from -> to (returns false if there is none)
    // prev always points to the node just BEFORE e, so the list can be re-linked around e
    bool removeEdge(int from, int to) {
        Edge* prev = nullptr;           // nullptr while e is still the first node
        Edge* e = adjList[from];
        while(e != nullptr) {
            if(e->to == to) {
                if(prev == nullptr) {
                    adjList[from] = e->next;    // e was the first node: the head moves forward
                }
                else {
                    prev->next = e->next;       // the node before e now skips over e
                }
                delete e;                       // free the node we just unlinked
                return true;
            }
            prev = e;
            e = e->next;
        }
        return false;
    }

public:
    Graph(int n) {
        if(n > GRAPH_MAX_JUNCTIONS) n = GRAPH_MAX_JUNCTIONS;    // only 6 junctions are supported
        totalJunctions = n;
        for(int i = 0; i < 7; i++) {
            adjList[i] = nullptr;
        }
    }

    // phase 3: deletes every road. Each head pointer is set back to nullptr afterwards,
    // otherwise it would keep pointing at deleted memory (a dangling pointer)
    void clear() {
        for(int i = 0; i < 7; i++) {
            Edge* e = adjList[i];
            while(e != nullptr) {
                Edge* nextEdge = e->next;   // save next BEFORE the delete
                delete e;
                e = nextEdge;
            }
            adjList[i] = nullptr;
        }
    }

    ~Graph() {
        clear();
    }

    // a copy would share the same edges and free them twice, so no copying
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // the second limit is a fixed number, so even a wrong totalJunctions can never let an
    // index reach past the 7-slot arrays
    bool isValid(int j) {
        return j >= 1 && j <= totalJunctions && j <= GRAPH_MAX_JUNCTIONS;
    }

    // roads are two way, so the edge is added in both lists
    void addRoad(int j1, int j2, int distance) {
        if(!isValid(j1) || !isValid(j2) || j1 == j2 || distance <= 0) {
            cout << "Invalid road J" << j1 << " - J" << j2 << ", not added" << endl;
            return;
        }
        addEdge(j1, j2, distance);
        addEdge(j2, j1, distance);
    }

    bool hasRoad(int j1, int j2) {
        if(!isValid(j1) || !isValid(j2)) return false;
        return findEdge(j1, j2) != nullptr;
    }

    // phase 3: closes a road. it is two way, so the edge is removed from both lists
    // returns false if there was no such road
    bool removeRoad(int j1, int j2) {
        if(!hasRoad(j1, j2)) return false;
        removeEdge(j1, j2);
        removeEdge(j2, j1);
        return true;
    }

    // phase 3: length of the road, 0 if there is no road
    int roadLength(int j1, int j2) {
        if(!isValid(j1) || !isValid(j2)) return 0;
        Edge* e = findEdge(j1, j2);
        if(e == nullptr) return 0;
        return e->distance;
    }

    // phase 3: estimated TRAVEL TIME of the road j1 -> j2 in seconds, 0 if there is no road.
    //   time = base time (length * SECONDS_PER_DISTANCE)
    //        + WAIT_PENALTY_SECONDS for every normal vehicle that is waiting on the road (waitingVehicles)
    //        + extraDelay: seconds that the caller adds for a preemption (an emergency vehicle slows or freezes the road)
    // The routing (dijkstra) still uses the plain length, so the routes of the demos do not change.
    int getTravelTime(int j1, int j2, int waitingVehicles, int extraDelay) {
        int length = roadLength(j1, j2);
        if(length == 0) return 0;
        if(waitingVehicles < 0) waitingVehicles = 0;
        if(extraDelay < 0) extraDelay = 0;
        return length * SECONDS_PER_DISTANCE + waitingVehicles * WAIT_PENALTY_SECONDS + extraDelay;
    }

    void showRoads() {
        for(int i = 1; i <= totalJunctions; i++) {
            cout << "J" << i << " : ";
            Edge* e = adjList[i];
            while(e != nullptr) {
                cout << "J" << e->to << "(" << e->distance << ") ";
                e = e->next;
            }
            cout << endl;
        }
    }

    // BFS: visits the nearest junctions first, uses the queue
    // returns how many junctions it managed to reach
    int bfs(int src) {
        if(!isValid(src)) return 0;
        bool visited[7] = {false};
        ArrayQueue q;
        int count = 0;

        q.enqueue(src);
        visited[src] = true;
        cout << "BFS from J" << src << ": ";
        while(!q.isempty()) {
            int u = q.dequeue();
            cout << "J" << u << " ";
            count++;
            for(Edge* e = adjList[u]; e != nullptr; e = e->next) {
                if(!visited[e->to]) {
                    visited[e->to] = true;
                    q.enqueue(e->to);
                }
            }
        }
        cout << endl;
        return count;
    }

    // DFS: goes as deep as it can first (recursion does the work of a stack)
    void dfsVisit(int u, bool visited[]) {
        visited[u] = true;
        cout << "J" << u << " ";
        for(Edge* e = adjList[u]; e != nullptr; e = e->next) {
            if(!visited[e->to]) {
                dfsVisit(e->to, visited);
            }
        }
    }

    void dfs(int src) {
        if(!isValid(src)) return;
        bool visited[7] = {false};
        cout << "DFS from J" << src << ": ";
        dfsVisit(src, visited);
        cout << endl;
    }

    // shortest distance from src to every junction
    // dist[] = distance, prev[] = junction we came from (-1 for src and unreachable ones)
    bool dijkstra(int src, int dist[], int prev[]) {
        if(!isValid(src)) return false;

        bool visited[7] = {false};
        for(int i = 1; i <= totalJunctions; i++) {
            dist[i] = NO_PATH;
            prev[i] = -1;
        }
        dist[src] = 0;

        for(int count = 1; count <= totalJunctions; count++) {
            // pick the closest junction we have not finished yet
            int u = -1, minDist = NO_PATH;
            for(int i = 1; i <= totalJunctions; i++) {
                if(!visited[i] && dist[i] < minDist) {
                    minDist = dist[i];
                    u = i;
                }
            }
            if(u == -1) break;
            visited[u] = true;

            // check every road leaving u, maybe it gives a shorter way
            for(Edge* e = adjList[u]; e != nullptr; e = e->next) {
                if(!visited[e->to]) {
                    int newDist = dist[u] + e->distance;
                    if(newDist < dist[e->to]) {
                        dist[e->to] = newDist;
                        prev[e->to] = u;
                    }
                }
            }
        }
        return true;
    }

    // prev[] gives the route backwards (dest -> src), so first push it on the
    // stack and then pop it into the linked list, that way the list is src -> dest
    bool buildRoute(int dest, int dist[], int prev[], SinglyLinkedList &route) {
        if(!isValid(dest) || dist[dest] == NO_PATH) return false;

        ArrayStack temp;
        int at = dest;
        while(at != -1) {
            temp.push(at);
            at = prev[at];
        }
        while(!temp.isEmpty()) {
            route.pushBack(temp.pop());
        }
        return true;
    }
};
