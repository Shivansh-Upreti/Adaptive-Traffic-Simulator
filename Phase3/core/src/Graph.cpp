#include "../include/Graph.hpp"

Edge* Graph::findEdge(int from, int to){
    Edge* e = adjList[from];
    while(e != nullptr) {
        if(e->to == to) return e;
        e = e->next;
    }
    return nullptr;
}

void Graph::addEdge(int from, int to, int distance){
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

bool Graph::removeEdge(int from, int to){
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

Graph::Graph(int n){
    if(n > NUM_JUNCTIONS) n = NUM_JUNCTIONS;        // the arrays are sized for NUM_JUNCTIONS (CityMap.hpp)
    totalJunctions = n;
    for(int i = 0; i < JUNCTION_SLOTS; i++) {
        adjList[i] = nullptr;
    }
}

void Graph::clear(){
    for(int i = 0; i < JUNCTION_SLOTS; i++) {
        Edge* e = adjList[i];
        while(e != nullptr) {
            Edge* nextEdge = e->next;   // save next BEFORE the delete
            delete e;
            e = nextEdge;
        }
        adjList[i] = nullptr;
    }
}

void Graph::addRoad(int j1, int j2, int distance){
    if(!isValid(j1) || !isValid(j2) || j1 == j2 || distance <= 0) {
        cout << "Invalid road J" << j1 << " - J" << j2 << ", not added" << endl;
        return;
    }
    addEdge(j1, j2, distance);
    addEdge(j2, j1, distance);
}

bool Graph::hasRoad(int j1, int j2){
    if(!isValid(j1) || !isValid(j2)) return false;
    return findEdge(j1, j2) != nullptr;
}

bool Graph::removeRoad(int j1, int j2){
    if(!hasRoad(j1, j2)) return false;
    removeEdge(j1, j2);
    removeEdge(j2, j1);
    return true;
}

int Graph::roadLength(int j1, int j2){
    if(!isValid(j1) || !isValid(j2)) return 0;
    Edge* e = findEdge(j1, j2);
    if(e == nullptr) return 0;
    return e->distance;
}

int Graph::getTravelTime(int j1, int j2, int waitingVehicles, int extraDelay){
    int length = roadLength(j1, j2);
    if(length == 0) return 0;
    if(waitingVehicles < 0) waitingVehicles = 0;
    if(extraDelay < 0) extraDelay = 0;
    return length * SECONDS_PER_DISTANCE + waitingVehicles * WAIT_PENALTY_SECONDS + extraDelay;
}


int Graph::bfs(int src){
    if(!isValid(src)) return 0;
    bool visited[JUNCTION_SLOTS] = {false};
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

void Graph::dfsVisit(int u, bool visited[]){
    visited[u] = true;
    cout << "J" << u << " ";
    for(Edge* e = adjList[u]; e != nullptr; e = e->next) {
        if(!visited[e->to]) {
            dfsVisit(e->to, visited);
        }
    }
}

void Graph::dfs(int src){
    if(!isValid(src)) return;
    bool visited[JUNCTION_SLOTS] = {false};
    cout << "DFS from J" << src << ": ";
    dfsVisit(src, visited);
    cout << endl;
}

bool Graph::dijkstra(int src, int dist[], int prev[]){
    if(!isValid(src)) return false;

    bool visited[JUNCTION_SLOTS] = {false};
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

bool Graph::buildRoute(int dest, int dist[], int prev[], SinglyLinkedList &route){
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
