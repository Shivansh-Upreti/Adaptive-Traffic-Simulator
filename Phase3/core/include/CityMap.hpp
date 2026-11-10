#pragma once

// ======================================================================
// The city map: the ONE place that says how many junctions there are and which roads exist.
// The project keeps a fixed map of 6 junctions on purpose (a simple, readable network).
// Both engines (RoadSim and Simulator) build their roads from the table below, so the
// LIVE and LAB modes can never drift apart, and every array is sized from NUM_JUNCTIONS.
//
//        J1 --5-- J2 --6-- J6
//        |        |       / |
//        4        4     5   7
//        |        |   /     |
//        J3 --5-- J4 --4-- J5
//         \_______5_______/            (J3 - J5, length 5)
// ======================================================================
const int NUM_JUNCTIONS = 6;
const int JUNCTION_SLOTS = NUM_JUNCTIONS + 1;   // arrays indexed by junction number: slot 0 is not used

const int NUM_ROADS = 9;                        // every road is two-way
const int ROAD_FROM[NUM_ROADS]   = {1, 1, 2, 3, 3, 4, 2, 4, 5};
const int ROAD_TO[NUM_ROADS]     = {2, 3, 4, 4, 5, 5, 6, 6, 6};
const int ROAD_LENGTH[NUM_ROADS] = {5, 4, 4, 5, 5, 4, 6, 5, 7};