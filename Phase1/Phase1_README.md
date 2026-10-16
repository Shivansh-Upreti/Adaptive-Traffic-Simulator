# Adaptive Traffic Signal & Emergency Vehicle Simulator

This project is a C++ based simulation designed to manage traffic flow at junctions while dynamically prioritizing emergency vehicles. 

Currently in **Phase 1**, the project establishes the foundational Object-Oriented design and core data structures required for the simulation environment.

## Current Implementation (Phase 1)
* **Object-Oriented Programming (OOP):** Utilizes classes and inheritance to differentiate standard `Vehicle` objects from `EmergencyVehicle` objects.
* **Arrays & Structs:** Implements `Junction` structs to map out the basic traffic grid using fixed arrays.
* **Queues:** Implementing standard queues to manage and simulate regular lane traffic at each junction.
* **Priority Queues & Custom Comparators:** Uses priority queues integrated with an `EmergencyComparison` structure to allow ambulances and fire engines to bypass standard lane traffic. It prioritizes emergencies by severity level and breaks ties based on arrival time.
* **Linked Lists:** Designed to track active vehicle routes dynamically.
* **Stacks:** Maintained to store a history of traversed junctions for route logging and backtracking.

## Roadmap (Phase 2)
* **Graphs & Shortest Path Algorithms:** Modeling the entire city grid as a graph network to dynamically calculate the most efficient, shortest path for emergency routing.

## How to Compile and Run
```bash
g++ main.cpp -o traffic_sim
./traffic_sim
