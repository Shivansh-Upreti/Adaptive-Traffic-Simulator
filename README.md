# Adaptive Traffic Signal & Emergency Vehicle Simulator

A C++ simulation that manages traffic flow at city junctions while dynamically prioritizing emergency vehicles. The project is built in phases, starting from core data structures and Object-Oriented design, moving to graph-based routing and density-aware signal logic, and extending to a live traffic engine with graphical visualization.

---

## Phase 1 (Completed)

Foundational Object-Oriented design and core data structures for the simulation environment.

- **Object-Oriented Design:** A `Vehicle` base class with an `EmergencyVehicle` derived class, which differentiates regular traffic from ambulances and fire engines.
- **Junction Structs:** `Junction` structs map out the basic traffic grid using fixed arrays.
- **Array Queues:** Regular lane traffic at each junction is managed with array-based queues.
- **Array Priority Queues:** Emergency vehicles bypass regular traffic through a priority queue with a custom comparator (`EmergencyComparison`). Vehicles are ordered by severity level, and ties are broken by arrival time.
- **Linked Lists:** Singly linked lists track active vehicle routes dynamically.
- **Stacks:** Array-based stacks store the history of traversed junctions for route logging and backtracking.

---

## Phase 2 (Completed)

Graph-based city modeling, emergency routing, and density-aware signal control.

- **Graph Road Network:** The city grid is modeled as a graph using an **Adjacency List** representation.
- **Graph Algorithms:**
  - **BFS** for level-order traversal and reachability.
  - **DFS** for exploring the network and its connectivity.
  - **Dijkstra's Algorithm** for shortest-path emergency routing.
- **Density-Aware Traffic Logic:** Signal behavior adapts to congestion based on queue length thresholds, classified as `LOW`, `MODERATE`, or `HEAVY`.

---

## Phase 3 (In Progress)

Real-time simulation and visualization, currently under development.

- **Live Traffic Engine (RoadSim):** A real-time traffic engine that updates vehicle movement on each simulation tick.
- **SFML Graphical Visualization:** A graphical front end built with [SFML](https://www.sfml-dev.org/) to render roads, junctions, signals, and vehicles.
- **"Green Wave" Emergency Preemption:** When an emergency vehicle is on its route, upcoming junctions along its path are reserved so signals turn green ahead of it.
- **Center-Overtake Mechanism:** Normal vehicles move to the road shoulder so the emergency vehicle can pass through the center of the road.

---

## How to Compile and Run

**Requirements:** a C++17-compatible compiler (e.g., `g++`).

Compile all source files together:

```bash
g++ -std=c++17 main.cpp Vehicle.cpp EmergencyVehicle.cpp Junction.cpp TrafficSignal.cpp TrafficManager.cpp Graph.cpp Simulation.cpp -o traffic_sim
```

Run:

```bash
./traffic_sim
```

On Windows:

```bash
traffic_sim.exe
```

**With SFML (Phase 3 visualization):**

```bash
g++ -std=c++17 main.cpp Vehicle.cpp EmergencyVehicle.cpp Junction.cpp TrafficSignal.cpp TrafficManager.cpp Graph.cpp Simulation.cpp -o traffic_sim -lsfml-graphics -lsfml-window -lsfml-system
```

**Using CMake:**

```bash
mkdir build && cd build
cmake ..
cmake --build .
```
