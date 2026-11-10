#pragma once
#include <iostream>
#include <string>
using namespace std;
#include "EmergencyRequest.hpp"
#include "Vehicle.hpp"
#include "EmergencyVehicle.hpp"
#include "ArrayPriorityQueue.hpp"
#include "ArrayQueue.hpp"
#include "SinglyLinkedList.hpp"
#include "Graph.hpp"
#include "TrafficFlow.hpp"      // only for NUM_JUNCTIONS and validIndex()
#include "AuditLog.hpp"         // CSV log of every finished EV trip

// ======================================================================
// RoadSim (phase 3, "live traffic") -- vehicles that DRIVE ALONG THE ROADS.
//
// The Simulator class ("lab") moves an emergency vehicle from junction to junction in
// one jump. RoadSim is a second engine in which EVERY vehicle drives along the roads, one
// small step per tick(), so the things that can go wrong on a real road can be shown:
//
//   * ROAD SEGMENT   one direction of one road = road[from][to]. It is a row of CELLS
//                    (a vehicle id in each cell, 0 = empty). There is a second row, the
//                    CENTER lane, which only emergency vehicles (EV) use.
//                    road[a][b] and road[b][a] are the two opposite directions.
//   * JUNCTION BOX   the crossing in the middle of a junction. Only ONE vehicle can be
//                    inside (isJunctionOccupied), so vehicles can never drive through
//                    each other. An EV with a green light still waits for a free box.
//   * GREEN WAVE     every tick each EV reserves the next 1-2 junctions of its route
//                    (isReserved). Normal vehicles may not enter a reserved junction, so
//                    the cross-traffic is locked BEFORE the EV arrives.
//   * CENTER-OVERTAKE while an EV is on a segment (activeEVCount > 0), the normal vehicles
//                    on it move to the shoulders and crawl, and the EV drives down the
//                    cleared center. Vehicles on the OPPOSITE segment freeze.
//   * EV CONFLICTS   two EVs that want the same junction: the custom priority queue
//                    decides. Smaller priority number wins, earlier arrival on a tie.
//                    The other EV waits at the stop line.
//   * MEMORY         every vehicle is made with new when it is spawned and DELETED when
//                    it arrives, so a slot can be used again for ever.
//
// Data structures from earlier phases that are reused here: Graph (map + Dijkstra),
// SinglyLinkedList (the route of every vehicle), ArrayPriorityQueue (EV order and
// conflicts), ArrayQueue (the waiting line at a junction), ArrayStack (inside buildRoute).
// ======================================================================

const int MAX_CELLS = 40;               // most cells in one lane
const int MAX_MOVERS = 60;              // most vehicles alive at the same time
const int MAX_ACTIVE_EV = 8;            // most emergency vehicles at the same time (the priority queue holds 10)
const int EV_CELLS_PER_TICK = 2;        // an EV moves 2 cells per tick, a normal vehicle 1
const int GREEN_WAVE_AHEAD = 2;         // how many upcoming junctions an EV reserves
const int PATIENCE_LIMIT = 300;         // a normal vehicle that cannot move for this many ticks gives up
const int ROAD_LOG_SIZE = 12;           // the last 12 events are remembered
const int MAX_ROAD_ID = 9999;

// ADAPTIVE SIGNALS (key G). Off: the normal vehicle that waited longest enters a junction (the old rule).
// On: each junction gives green to its busiest approach, for longer when that lane is denser
// (the LOW / MODERATE / HEAVY levels of TrafficFlow). Emergency vehicles always go first.
const int GREEN_TICKS_LOW = 4;          // 0-2 vehicles in the lane
const int GREEN_TICKS_MODERATE = 8;     // 3-5 vehicles
const int GREEN_TICKS_HEAVY = 12;       // 6 or more vehicles
const int MAX_RED_TICKS = 40;           // a vehicle that waited this long gets green next, so no road waits for ever

// where a vehicle is
const int MV_FREE = 0;                  // this slot of movers[] is empty
const int MV_QUEUED = 1;                // waiting in the spawn queue of a junction for room on the road
const int MV_ON_ROAD = 2;               // in a cell of a lane (normal) or of the center lane (EV)
const int MV_IN_BOX = 3;                // inside the junction box

// why an emergency vehicle is waiting (for the event log)
const int WAIT_NONE = 0;
const int WAIT_BOX = 1;                 // junction box is occupied
const int WAIT_YIELD = 2;               // the junction is reserved by an EV with higher priority
const int WAIT_OPPOSITE = 3;            // an EV is driving on the opposite segment
const int WAIT_EXIT = 4;                // the first cell after the junction is taken

// ONE direction of ONE road: road[from][to]
struct RoadSegment {
    int cells;                          // number of cells, 0 = this road has never existed
    int cell[MAX_CELLS];                // normal lane: vehicle id per cell, 0 = empty. cell 0 is next to the start junction
    int evCell[MAX_CELLS];              // center lane (emergency vehicles only)
    int activeEVCount;                  // emergency vehicles on this segment, or in the box about to enter it
};

// the physical crossing of a junction
struct JunctionBox {
    bool isJunctionOccupied;            // a vehicle is inside the box right now
    int occupantID;                     // which one (0 = nobody)
    bool isReserved;                    // an EV has reserved this junction (green wave)
    int reservedByID;                   // which EV (0 = nobody). A plain true/false is not enough when TWO EVs compete
};

// everything one vehicle needs to drive. movers[i].veh == nullptr means the slot is free
struct Mover {
    Vehicle* veh;                       // made with new, deleted when the vehicle arrives
    int state;                          // MV_FREE, MV_QUEUED, MV_ON_ROAD, MV_IN_BOX
    int from;                           // lane start / the junction where it waits or crosses
    int to;                             // lane end (while MV_ON_ROAD)
    int cell;                           // position in the lane (0 = start)
    SinglyLinkedList::Node* cur;        // node of the route: the junction it is IN, or the one it is HEADING to
    int destination;
    int origin;                         // the junction where the trip started
    bool isEV;
    int priority;                       // 1 / 2 / 3 for Ambulance / Fire Truck / Police, 10 for normal vehicles
    long long arrival;                  // arrival order (breaks ties between equal priorities)
    string label;                       // "Ambulance" ... or "Vehicle"
    int waitTicks;                      // ticks without moving
    int waitCode;                       // WAIT_... (EV only)
    int reservedMask;                   // bit j set = this EV holds junction j reserved
    long long lastMoveTick;             // the last tick in which it drove forward inside a lane (-1 = never)
    long long spawnTick;                // the tick in which it was created (time taken = arrival tick - spawnTick)
    string drivenPath;                  // the junctions REALLY passed, e.g. "J1->J3->J4" (a re-route changes it)
};

class RoadSim{
    Graph city;
    RoadSegment road[NUM_JUNCTIONS + 1][NUM_JUNCTIONS + 1];     // index 0 is not used
    JunctionBox box[NUM_JUNCTIONS + 1];
    Mover movers[MAX_MOVERS];
    ArrayQueue spawnQ[NUM_JUNCTIONS + 1];       // ids of vehicles that wait for room at a junction
    bool boxFreeAtStart[NUM_JUNCTIONS + 1];     // was the box empty when this tick began?
    int passCount[NUM_JUNCTIONS + 1];           // how many EVs have entered this junction (numbers the [PASS] lines of a demo)
    long long orderKey[NUM_JUNCTIONS + 1];      // the order of the waiting EVs last written to the log (so it is written only when it changes)
    int conflictKey[NUM_JUNCTIONS + 1];         // which two EVs were in conflict at this junction (so the log is written only once)

    long long tickCount;
    long long arrivalClock;
    string eventLog[ROAD_LOG_SIZE];
    int logTotal;

    // statistics (shown on screen, checked by the tests)
    int statCreated;                    // vehicles made with new
    int statDeleted;                    // vehicles deleted
    int statArrivedNormal;
    int statArrivedEV;
    int statConflicts;                  // two EVs wanted the same junction in the same tick
    int statTieBreaks;                  // conflicts between two EVs of the SAME priority (decided by the earlier arrival)
    bool headOnPass;                    // true: two EVs that meet head-on on a road BOTH keep driving (each on its own side). Only scene 6 of the demo turns it on
    bool announceTies;                  // true: a tie-break is also written to the console with cout (the demo scene turns it on)
    int statYields;                     // an EV had to wait for a higher-priority EV
    int statClearanceWaits;             // an EV had to wait for an occupied junction box
    int statFrozenTicks;                // vehicle-ticks that normal vehicles stood still for an EV
    int statReroutes;
    int statStranded;                   // vehicles removed because no route was left
    int statTimeouts;                   // normal vehicles that gave up waiting
    int statMaxAlive;
    long long statNormalTripTicks;      // sum of the trip times of the normal vehicles that arrived

    // settings: they survive reset()
    bool auditOn;                       // write a CSV row for every finished EV trip
    string auditFile;
    bool adaptiveSignals;

    // adaptive signal state of every junction
    int greenFrom[NUM_JUNCTIONS + 1];   // the approach (lane greenFrom -> j) that has green, 0 = none yet
    long long greenUntil[NUM_JUNCTIONS + 1];


    /* -------------------------
        Foundation
       ------------------------- */
    void log(string msg);

    string name(int j){
        return "J" + to_string(j);
    }

    // "Ambulance 903"
    string who(int slot){
        return movers[slot].label + " " + to_string(movers[slot].veh->getID());
    }

    int findSlot(int id);

    int freeMoverSlot();

    int countEVs();

    // how many cells a road of this length has (2 cells per unit of length, but not too few or too many)
    int cellsForLength(int length);

    // makes sure both directions of the road a - b exist as lanes. Once a lane exists it keeps its
    // number of cells for ever, even if the road is closed and opened again
    void ensureLane(int a, int b, int length);

    void buildMap();

    void clearMover(int i);

    // the node of the route where the EV's "upcoming junctions" begin
    SinglyLinkedList::Node* upcomingStart(int slot);


    /* ---------------------------
        Removing vehicles, routes
       --------------------------- */
    void removeMover(int slot);

    // If the next road of the route has been closed, plan a new route from where the vehicle is.
    // Returns false if there is no way left: then the vehicle has been REMOVED
    bool ensureRouteOpen(int slot);


    /* --------------------------------
        Emergency order and green wave
       -------------------------------- */
    int orderEVs(int order[]);

    // GREEN WAVE: every tick, in priority order, each EV reserves the next 1-2 junctions of its route.
    // A junction that a more important EV has reserved already stays with that EV.
    void updateReservations();


    /* -----------------------
        Priority rules
       ----------------------- */
    // the EV claims the segment j -> n: from now on normal vehicles on it move to the shoulders
    void commitEdge(int j, int n, int slot);

    // remembers why an EV waits and writes it into the log (only when the reason CHANGES, so the log does not fill up)
    void setWait(int slot, int code, int j);

    // is EV x more important than EV y? (smaller priority number, or the same number and it arrived first)
    // This is the SAME order the custom priority queue uses, so there is one total order of all EVs
    bool outranks(int x, int y);

    // Is there an EV that is MORE important than 'slot' on the road segment a -> b
    // (driving on it, or inside the box and about to enter it)?
    // Two EVs must never meet head-on in the center lane. When they face each other, the more important one
    // drives down the center and the other one freezes on the shoulder. Because the order is total, the most
    // important EV is never blocked by a less important one, so two EVs can never block each other for ever.
    bool outrankingEVOnSegment(int a, int b, int slot);

    // Head-on pass mode only: junction j is reserved by the EV that drives toward j on the road n -> j, which is the road
    // a vehicle leaving j for n would drive on the other way. That vehicle does not cross the box of j, so the reservation
    // is no reason for it to wait: both EVs may drive on and pass each other on the road
    bool reservedByOpposingEV(int j, int n);

    // can this EV (standing at the stop line before junction j) enter the box? The box itself is free (checked by the caller)
    int evEntryCode(int slot, int j);

    // can this normal vehicle enter the box of junction j?
    bool normalCanEnter(int slot, int j);

    void enterBox(int slot, int j);



    /* ----------------------------------------
        Junction box exits and entries
       ----------------------------------------*/
    // vehicles that were inside a box leave it: onto the next lane, or out of the network
    void phaseBoxExits();

    // at most ONE vehicle per junction enters the box
    void phaseBoxEntries();

    /* ----------------------------------------
        Lane moves, spawns, patience
       ----------------------------------------*/
    // vehicles drive forward inside their lanes
    void phaseLaneMoves();

    // vehicles that wait in a spawn queue are put on the road when there is room
    void phaseSpawns();

    // a normal vehicle that cannot move for too long gives up (this also breaks a traffic jam that has locked itself)
    void phasePatience();
    /* ---------------------------------
        Adaptive signals
       ---------------------------------*/
    int laneQueue(int a, int j);        // normal vehicles in the lane a -> j (the density of that approach)
    int greenTicksFor(int queue);
    int pickNormalAdaptive(int j, int normSlots[], int nNorm);


    /* ---------------------------------
        Construction and reset
       ---------------------------------*/
    public:
        RoadSim();

        ~RoadSim(){
            reset();                    // deletes every vehicle that is still alive
        }

        // owns heap memory and a graph: never copy
        RoadSim(const RoadSim&) = delete;
        RoadSim& operator=(const RoadSim&) = delete;


        // Back to an empty city and the starting map. Every vehicle that is still alive is deleted.
        void reset();


        /* ----------------------------------------
            Spawning API
           ----------------------------------------*/
        bool spawn(int id, int type, int from, int to, const int* path = nullptr, int pathLen = 0);

        bool spawnNormal(int id, int from, int to){
            return spawn(id, 0, from, to);
        }

        bool spawnEmergency(int id, int type, int from, int to);

        // true if there is a way from 'from' to 'to' on the roads that are open now.
        // Asking costs nothing and writes nothing into the log (a spawn that fails does)
        bool canReach(int from, int to);

        // OPPOSING TRAFFIC for an EV that drives from -> to: ONE normal vehicle that starts at 'to' and
        // drives the SAME roads BACKWARDS to 'from', so it meets the EV head-on.
        // (A plain shortest path to -> from could use other roads when two routes are equally long.)
        bool spawnOpposing(int id, int from, int to);

        // slot of the emergency vehicle that was created last, -1 if there is none
        int latestEVSlot();

        /*--------------------------------
             ONE TICK of time
          --------------------------------- */
        void tick();

        /* ----------------
                Roads
           ---------------- */
        bool closeRoad(int a, int b);

        bool openRoad(int a, int b, int length);
        /* ----------------------------
            Audit log and signals
           ---------------------------- */
        void setAuditLog(bool on){ auditOn = on; }
        void setAuditFile(const string &file){ auditFile = file; }
        string getAuditFile(){ return auditFile; }
        void setAdaptiveSignals(bool on);
        bool isAdaptive(){ return adaptiveSignals; }
        // adaptive mode: the approach (the road a -> j) that has green at junction j, 0 = none
        int getGreenApproach(int j){ return validIndex(j) ? greenFrom[j] : 0; }
        // average trip time (ticks) of the normal vehicles that arrived, 0 if none yet
        double getAverageTripTicks(){ return statArrivedNormal == 0 ? 0.0 : (double)statNormalTripTicks / statArrivedNormal; }


    
    /* ----------------
        State Checks
       ---------------- */
    private:   // the parts of selfCheck(): only selfCheck() calls them
        // 1. every vehicle must be in exactly one place, and that place must say the same as the vehicle
        int checkVehiclePlaces();

        // 2. every id in a lane or box must belong to a vehicle that exists
        int checkLaneAndBoxIds();

        // 3. activeEVCount must equal the number of EVs that are really there, and EVs must never face each other
        int checkEVCounts();

        // 3b. NO HEAD-ON: a vehicle must stand still while an EV drives the opposite way on the same road.
        //     (for an EV the opposing EV must be MORE important; the more important one passes in the center)
        int checkNoHeadOn();

        // 4. a reservation must belong to a living EV
        int checkReservations();

        // 5. the counters must agree with what is alive (every new has been matched by a delete)
        int checkCounters();

    public:
        // runs all the parts above and adds up their problems (0 = healthy)
        int selfCheck();



    /* ------------------------------------------------------------------
            Read-only information (for the window and the tests)
       ------------------------------------------------------------------ */
        long long getTick(){ return tickCount; }
        int getAliveCount();
        int getEVCount(){ return countEVs(); }
        bool idInUse(int id){ return findSlot(id) != -1; }

        int getCreated(){ return statCreated; }
        int getDeleted(){ return statDeleted; }
        int getArrivedNormal(){ return statArrivedNormal; }
        int getArrivedEV(){ return statArrivedEV; }
        int getConflicts(){ return statConflicts; }
        int getTieBreaks(){ return statTieBreaks; }
        void setAnnounceTies(bool on){ announceTies = on; }
        void setHeadOnPass(bool on){ headOnPass = on; }

        // true while this EV drives on a road where another EV drives the opposite way and both are allowed to pass
        // (head-on pass mode). The window then draws both vehicles on their own side, so they pass without clipping
        bool isPassingHeadOn(int slot);
        int getYields(){ return statYields; }

        // travel time of a road (shown next to the roads in the window)
        // normal vehicles that stand still in the lane of the road a -> b (they wait for a vehicle ahead or for the junction)
        int edgeWaitingCount(int a, int b);

        // true while an emergency vehicle drives on the road in either direction (the road is pre-empted)
        bool edgePreempted(int a, int b);

        // estimated travel time in seconds of a NORMAL vehicle on the road a -> b:
        //   base time + 1 s per waiting vehicle + the preemption delay:
        //   an EV on this road makes the normal lane crawl (half speed: the base time again),
        //   an EV on the opposite lane freezes this lane until the EV has passed (length / EV speed seconds)
        int edgeTravelSeconds(int a, int b);
        int getClearanceWaits(){ return statClearanceWaits; }
        int getFrozenTicks(){ return statFrozenTicks; }
        int getReroutes(){ return statReroutes; }
        int getStranded(){ return statStranded; }
        int getTimeouts(){ return statTimeouts; }
        int getMaxAlive(){ return statMaxAlive; }

        // roads and junctions
        bool hasRoad(int a, int b){ return city.hasRoad(a, b); }
        int roadLength(int a, int b){ return city.roadLength(a, b); }
        int getCells(int a, int b);
        int getActiveEVCount(int a, int b);
        int getLaneCell(int a, int b, int c);
        int getCenterCell(int a, int b, int c);
        bool isJunctionOccupied(int j){
            return validIndex(j) && box[j].isJunctionOccupied;
        }
        int getBoxOccupant(int j);
        bool isReserved(int j){
            return validIndex(j) && box[j].isReserved;
        }
        int getReservedBy(int j);
        int getSpawnQueueLength(int j);

        // vehicles, by slot number 0 .. MAX_MOVERS-1 (the window draws all of them)
        bool isSlotUsed(int slot){ return slot >= 0 && slot < MAX_MOVERS && movers[slot].veh != nullptr; }
        int getSlotID(int slot){ return isSlotUsed(slot) ? movers[slot].veh->getID() : 0; }
        int getSlotState(int slot){ return isSlotUsed(slot) ? movers[slot].state : MV_FREE; }
        int getSlotFrom(int slot){ return isSlotUsed(slot) ? movers[slot].from : 0; }
        int getSlotTo(int slot){ return isSlotUsed(slot) ? movers[slot].to : 0; }
        int getSlotCell(int slot){ return isSlotUsed(slot) ? movers[slot].cell : 0; }
        bool isSlotEV(int slot){ return isSlotUsed(slot) && movers[slot].isEV; }
        int getSlotPriority(int slot){ return isSlotUsed(slot) ? movers[slot].priority : 0; }
        int getSlotWaitCode(int slot){ return isSlotUsed(slot) ? movers[slot].waitCode : 0; }
        int getSlotWaitTicks(int slot){ return isSlotUsed(slot) ? movers[slot].waitTicks : 0; }
        int getSlotDestination(int slot){ return isSlotUsed(slot) ? movers[slot].destination : 0; }
        int getSlotOrigin(int slot){ return isSlotUsed(slot) ? movers[slot].origin : 0; }
        string getSlotLabel(int slot){ return isSlotUsed(slot) ? movers[slot].label : ""; }
        int slotOf(int id){ return findSlot(id); }

        // the i-th junction of the route that is still ahead of an EV (0 = none). The window draws it
        int getSlotRouteJunction(int slot, int i);

        // A normal vehicle moves to the shoulder while an EV is on its segment or on the opposite one.
        // An EV moves to the shoulder only when it must give way to a MORE IMPORTANT EV coming toward it
        bool isOnShoulder(int slot);

        // A vehicle is frozen while an EV (for an EV: a more important EV) drives the opposite way on the same road
        bool isFrozen(int slot);

        // lets the window write a message (for example "click a junction first") into the event log
        void addNote(string msg){
            log(msg);
        }

        // the last events, newest first (k = 0). empty text if there are fewer events
        string getRecentEvent(int k);
        int getEventCount(){ return logTotal; }

};