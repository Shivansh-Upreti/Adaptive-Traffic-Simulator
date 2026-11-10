#pragma once
#include <string>
#include <iostream>
using namespace std;
#include "../../core/include/DemoScene.hpp"

// ======================================================================
// LiveView (phase 3, SFML version, "live traffic" mode)
// What the window needs to REMEMBER between two frames while it shows a RoadSim,
// and what every key does. There is no SFML in this file, so it can be tested without a window.
//
// RoadSim moves every vehicle by whole cells, once per tick. The window draws 60 frames per
// second, so it must SLIDE the vehicles smoothly from where they were (before the last tick)
// to where they are now. LiveView keeps that "before" picture (prev[]) and the fraction
// alpha (0.0 = just ticked, 1.0 = the next tick is due).
//
// It also eases the SIDEWAYS position of every vehicle (lateral[]): when an EV comes, the
// normal vehicles glide from their lane to the shoulder, and glide back afterwards.
// ======================================================================

const float LIVE_TICK_SECONDS = 0.28f;      // one tick of the engine lasts this long at speed 1.0
const float LANE_OFFSET = 9.0f;             // pixels from the road center to a normal lane
const float HEAD_ON_OFFSET = 7.0f;         // two EVs that pass each other: pixels from the road center to the side of each (its own right)
const float SHOULDER_OFFSET = 15.0f;        // pixels from the road center to the shoulder (outer edge)
const float LATERAL_EASE_SPEED = 45.0f;     // pixels per second: how fast a vehicle moves sideways
const int MAX_TICKS_PER_FRAME = 3;          // never run more than this per frame (the window must stay responsive)
const int BATCH_COUNT = 5;                  // the batch spawn of the user input mode creates this many normal vehicles
// the screens (a small state machine): the main simulation, the User Defined Input menu, and the three
// actions that need mouse clicks
const int UI_MAIN = 0;                      // main simulation (only the safe keys work)
const int UI_USER_MENU = 1;                 // "USER INPUT MODE - Select an action"
const int UI_SINGLE = 2;                    // action [1]: single custom vehicle (keys 1 / 2 / 3 / 9)
const int UI_BATCH = 3;                     // action [2]: batch of normal vehicles (right click spawns them)
const int UI_ROAD = 4;                      // action [3]: road closure (right click toggles the road)
const int OPPOSING_COUNT = 3;               // the 0 key of the user mode creates this many opposing normal vehicles

// where a vehicle was before the last tick
struct PosSnap {
    int id;                                 // 0 = nobody
    int state;
    int from;
    int to;
    int cell;
};

class LiveView{
    public:
        int startJ;                         // selected start junction (left click), 0 = none
        int destJ;                          // selected destination junction (right click), 0 = none
        bool paused;
        float speed;                        // 1.0 = normal
        bool heavyTraffic;                  // the feeder keeps adding vehicles
        bool demoActive;                    // a demo scene is running (it ends when its emergency vehicles are done)
        int uiState;                        // UI_MAIN, UI_USER_MENU, UI_SINGLE, UI_BATCH or UI_ROAD
        bool consoleLog;                    // true: the user actions are also written to the console (main.cpp turns it on)
        bool demoMenuOpen;                  // the demo sub-menu is on screen: the keys 1 - 7 choose a scenario
        int demoScenario;                   // the scenario that runs (or ran last), 1 - 7
        float tickTimer;                    // seconds since the last tick
        PosSnap prev[MAX_MOVERS];
        float lateral[MAX_MOVERS];          // sideways offset in pixels (positive = to the right of the driving direction)
        int lateralId[MAX_MOVERS];          // which vehicle that offset belongs to
        TrafficFeeder feeder;
        int nextEmergencyID;
        string checkText;

        LiveView();

        void reset();

        // mouse
        void selectStart(int j);
        void selectDest(int j);

        // the "before" picture
        // remembers where every vehicle is NOW (call it just before world.tick())
        void snapshot(RoadSim &w);

        // 0.0 right after a tick, 1.0 when the next tick is due
        float getAlpha();

        // keys

        // the next emergency vehicle id that no living vehicle uses (900 .. 999)
        int freeEmergencyId(RoadSim &w);

        // N: a normal vehicle at START. Its destination is RANDOM: any other junction that can be reached.
        // It does NOT copy the destination chosen for an emergency vehicle, so the background traffic flows on its own.
        void actionSpawnNormal(RoadSim &w);

        // 1 / 2 / 3: an Ambulance / Fire Truck / Police from START to DESTINATION
        void actionSpawnEmergency(RoadSim &w, int type);

        // O: OPPOSING traffic. Normal vehicles that start at the DESTINATION of the emergency vehicle and drive the
        // SAME roads BACKWARDS to its START, so they meet it head-on: center-overtake and the freeze of the opposite
        // lane can be shown on demand.
        // START and DESTINATION are taken from the selection (so O can even be pressed BEFORE the EV: the cars are
        // already on their way). Without a selection the emergency vehicle that was created last is used.
        void actionSpawnOpposing(RoadSim &w);

        // H: heavy traffic on / off
        void actionToggleHeavy(RoadSim &w);

        // C: close the road between START and DESTINATION, or open it again
        void actionToggleRoad(RoadSim &w);

        void actionSelfCheck(RoadSim &w);

        // ------User Defined Input mode (key U)------
        // Spawning, batches and road closures only work inside this mode, so nothing can happen by accident.
        void actionOpenUserMenu();

        // ESC: from an action back to the user menu, from the user menu back to the main simulation
        void actionUserBack();

        // a digit key inside the user mode. In the menu: 1 / 2 / 3 choose an action, 4 switches the heavy background traffic on / off.
        // Inside action [1]: 1 / 2 / 3 = Ambulance / Fire Truck / Police car, 0 = WITH opposing traffic, 9 = normal vehicle.
        void userInputKey(RoadSim &w, int digit);

        // a RIGHT click on a junction inside the user mode (destDone = the destination was just chosen)
        void userRightClick(RoadSim &w);

        // 5 normal vehicles at START, all driving to DESTINATION, created at once
        void actionBatchSpawn(RoadSim &w);

        // D: opens (or closes) the demo SUB-MENU. The next key 1 - 7 starts that scenario (see core/include/DemoScene.hpp)
        void actionToggleDemoMenu(RoadSim &w);

        // starts one demo scenario: 1 = J4 convergence, 2 / 3 / 4 = tie-breaker of two Ambulances / Fire Trucks / Police cars
        void actionDemo(RoadSim &w, int scenario);

        // ---------------- time ----------------
        // dt = seconds since the last call (already multiplied by the speed).
        // Runs as many engine ticks as the time allows (at most MAX_TICKS_PER_FRAME) and eases the sideways positions
        void update(RoadSim &w, float dt);

        // moves every vehicle sideways toward where it belongs: lane, shoulder or center
        void easeLateral(RoadSim &w, float dt);
};
