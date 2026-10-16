#pragma once
#include <iostream>
using namespace std;
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"

const int NO_PATH = 999999;   // distance used for "cannot reach this junction"

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

public:
    Graph(int n) {
        if(n > 6) n = 6;    // only 6 junctions are supported
        totalJunctions = n;
        for(int i = 0; i < 7; i++) {
            adjList[i] = nullptr;
        }
    }

    ~Graph() {
        for(int i = 0; i < 7; i++) {
            Edge* e = adjList[i];
            while(e != nullptr) {
                Edge* nextEdge = e->next;
                delete e;
                e = nextEdge;
            }
        }
    }

    // a copy would share the same edges and free them twice, so no copying
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    bool isValid(int j) {
        return j >= 1 && j <= totalJunctions;
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
