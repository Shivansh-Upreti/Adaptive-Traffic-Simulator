#pragma once
#include <iostream>
#include <string>
using namespace std;
#include "EmergencyRequest.hpp"
#include "Vehicle.hpp"
#include "EmergencyVehicle.hpp"
#include "ArrayPriorityQueue.hpp"
#include "ArrayQueue.hpp"
#include "ArrayStack.hpp"
#include "SinglyLinkedList.hpp"
#include "Graph.hpp"
#include "RouteHistory.hpp"
#include "TrafficSignal.hpp"
#include "TrafficFlow.hpp"

// ======================================================================
// Simulator (phase 3) -- the shared "engine".
// SFML window talk to this one class, so the
// rules (priority, bypass, signal preemption, two-lane evacuation) are
// written only once.
//
// Rules in one place:
//   1. the most urgent emergency request is dispatched first
//      (priority 1 = Ambulance, 2 = Fire Truck, 3 = Police; normal vehicles = 10)
//   2. before the vehicle drives to the next junction:
//        - nobody waiting there  -> [BYPASS]: no signal request at all
//        - vehicles waiting      -> [SIGNAL]: green for the emergency vehicle,
//                                   cross-traffic paused, and
//                                   [EVACUATION]: the waiting vehicles pull over
//                                   into the side lane
//   3. when the vehicle has left a junction, the signal goes back to normal
//      and the pulled-over vehicles return to the main lane in the same order
//   4. a road can be closed at any time; a blocked vehicle looks for a new route
// ======================================================================

const int MAX_FLEET = 40;           // most vehicles that can exist in one run
const int MAX_VEHICLE_ID = 9999;
const int LOG_SIZE = 8;             // the last 8 events are remembered (the SFML window shows them)
const int MAX_ROAD_LENGTH = 99;

// result of startDispatch()
const int DISPATCH_STARTED = 1;
const int DISPATCH_NOTHING = 0;     // no emergency request was waiting
const int DISPATCH_FAILED = -1;     // a request was taken but no trip could start

class Simulator{
    Graph city;
    TrafficFlow flow;                   // the lanes and signals of all junctions
    ArrayPriorityQueue emergencyQ;      // waiting emergency requests

    Vehicle* fleet[MAX_FLEET];          // fleet[i] points to a vehicle made with new (nullptr = empty slot)
    int fleetCount;                     // how many vehicles exist RIGHT NOW (slots that are not nullptr).
                                        // A vehicle that leaves or arrives is deleted at once, so its slot
                                        // can be used again and the simulation can run for ever

    long long clockTick;                // goes up by 1 for every emergency request: this is the arrival time
    string eventLog[LOG_SIZE];
    int logTotal;

    // the emergency trip that is running right now
    bool dispatchActive;
    bool lastTripArrived;
    int activeIdx;                      // position of the emergency vehicle inside fleet[]
    EmergencyRequest activeReq;
    SinglyLinkedList::Node* routeCur;   // the junction the vehicle stands at, a node inside its route list
    int lastJunction;                   // the junction the vehicle stood at last (valid even when routeCur is nullptr)
    RouteHistory history;
    bool tookHold[NUM_JUNCTIONS + 1];   // tookHold[j] is true while THIS trip holds the signal of junction j

    // numbers for the HUD and the tests
    int statBypass;
    int statPreempt;
    int statEvacuated;

    /* ------------------
        Foundation
       ------------------ */
    void log(string msg);

    string name(int j){
        return "J" + to_string(j);
    }

    void buildMap();

    // position of vehicle id inside fleet[], -1 if there is no such vehicle.
    // fleet[] can have empty slots (nullptr) between vehicles, so every slot is looked at
    int findIndex(int id);

    // the first empty slot of fleet[], -1 if all MAX_FLEET slots are in use
    int freeSlot();

    // deletes the vehicle in slot idx and makes the slot free again
    void removeVehicle(int idx);

    // how many times id stands in a lane (main or side) of ANY junction. must be 1 for a vehicle on the road
    int countInLanes(int id);


    /* ------------------
        Checks
       ------------------ */

    // true while the side lane of j still holds pulled-over vehicles.
    // nobody may join the lane then, or the order of the pulled-over vehicles would be lost
    bool laneBusy(int j){
        return flow.sideLength(j) > 0;
    }

    // checks that a vehicle can join the lane of junction j. writes the reason into the log if not
    bool laneAvailable(int j);

    // checks everything that is needed for a NEW vehicle id
    bool checkNewId(int id);

    bool isEmergencyId(int id);

    // how many OTHER emergency vehicles (not skipID) are parked in the side lane of j.
    // called right after the evacuation, when the pulled-over vehicles are in the side lane
    int countParkedEmergency(int j, int skipID);

    // id of the first NORMAL vehicle in the main lane of j (parked emergency vehicles are skipped), -1 if none
    int firstNormalInLane(int j);

    string routeText(SinglyLinkedList &route);


    // "Ambulance 102"
    string activeName(){
        return activeReq.label + " " + to_string(fleet[activeIdx]->getID());
    }


    /* ------------------
        Junction control
       ------------------ */
    
    // the emergency vehicle is about to use junction j (or already stands there)
    void prepareJunction(int j);

    // the emergency vehicle has left junction j: give the signal back and bring the pulled-over vehicles back.
    // does nothing if this trip holds nothing at j, so it is safe to call more than once
    void cleanupJunction(int j);

    void releaseAllHolds();


    /* ----------------------
        Trip end and reroute
       ---------------------- */

    // the trip ends here without arriving. the vehicle stays parked in the lane where it stands
    void stopTrip(string reason);

    // the vehicle stands at its destination
    void finishTrip();

    // plans a new shortest route from junction 'from' to the destination
    bool reroute(int from);


     /* ------------------------
        Construction and reset
       ------------------------ */

    public:
        Simulator();

        ~Simulator(){
            reset();                    // deletes every vehicle, gives every signal hold back
        }

        // the simulator owns heap memory and a graph, so it must never be copied
        Simulator(const Simulator&) = delete;
        Simulator& operator=(const Simulator&) = delete;

        // Back to a fresh start: every vehicle is deleted, all lanes are empty, the map
        // is the starting map again. Replaces "delete the simulator and make a new one",
        // so no pointer to a simulator is needed.
        void reset();


         /* ----------
           Spawning
          ----------- */
        bool idInUse(int id){
            return findIndex(id) != -1;
        }

        bool spawnNormal(int id, int j);

        // type: 1 = Ambulance, 2 = Fire Truck, 3 = Police
        bool spawnEmergency(int id, int type, int from, int to);


        /* -------------------
           The emergency trip
          --------------------- */

        // takes the most urgent request and starts its trip.
        // the vehicle then waits at its start junction until stepDispatch() moves it
        int startDispatch();

        // moves the active emergency vehicle ONE road forward.
        // returns true while the trip goes on, false when it is over (arrived or stopped)
        bool stepDispatch();

        /* ------------------------
          Normal traffic and roads
          ------------------------- */

        // the first normal vehicle at 'from' drives to 'to' (needs a road between them)
        bool moveNormal(int from, int to);

        // the first normal vehicle at j leaves the network
        bool departNormal(int j);

        /* ------------------
            Roads
           ------------------ */
        bool closeRoad(int a, int b);

        bool openRoad(int a, int b, int length);


    /* ------------------------------------------------------------------------------------
        state check: "can anything have gone wrong?" Returns the number of problems found.
       ------------------------------------------------------------------------------------ */
    private:   // the parts of selfCheck(): only selfCheck() calls them
        // every vehicle in a lane must exist and stand in exactly ONE place
        int checkLaneContents();

        // every vehicle that exists must really be in a lane (none may have vanished).
        // A vehicle that has left was deleted, so nothing "has left but is still there" any more
        int checkNoVanishedVehicles();

        // with no emergency vehicle on the road nothing may be held or pulled over
        int checkIdleState();

        // every waiting request must belong to a vehicle that exists
        int checkRequests();


     /* ---------------
            Self-check	
        ----------------- */
    public:
        // runs all the parts above and adds up their problems (0 = healthy)
        int selfCheck();





        /* ------------------------------------------------------------------------
           read-only information (used by the menu, the tests and the SFML window)
         -------------------------------------------------------------------------- */

        bool isDispatchActive(){ return dispatchActive; }
        bool wasLastTripArrived(){ return lastTripArrived; }
        int getPendingCount(){ return emergencyQ.size(); }
        int getFleetCount(){ return fleetCount; }
        int getBypassCount(){ return statBypass; }
        int getPreemptCount(){ return statPreempt; }
        int getEvacuatedCount(){ return statEvacuated; }

        
        /* ----------------
            Getters
           ---------------- */

        int getActiveVehicleID();
        int getCurrentJunction();
        int getNextJunction();

        // the i-th junction of the route that is still ahead (0 = the junction the vehicle
        // stands at now). returns 0 when there is no such junction. the SFML window uses
        // this to highlight the route
        int getRouteJunction(int i);

        // lets the window write a message (for example "click a junction first") into the event log
        void addNote(string msg){
            log(msg);
        }

        bool hasRoad(int a, int b){ return city.hasRoad(a, b); }
        int roadLength(int a, int b){ return city.roadLength(a, b); }
        bool isEmergencyVehicle(int id){ return isEmergencyId(id); }
        // 1 / 2 / 3 for an Ambulance / Fire Truck / Police, 10 for a normal vehicle, 0 if there is no such vehicle.
        // The window uses it to draw each kind of emergency vehicle in its own way
        int getVehiclePriority(int id);
        // priority of the emergency vehicle that is on its trip right now, 0 if there is none
        int getActivePriority();
        bool isPreempted(int j){ return flow.isPreempted(j); }
        int getLaneLength(int j){ return flow.laneLength(j); }
        int getLaneVehicle(int j, int i){ return flow.laneAt(j, i); }
        int getSideLength(int j){ return flow.sideLength(j); }
        int getSideVehicle(int j, int i){ return flow.sideAt(j, i); }
        string getCongestion(int j){ return flow.congestionLevel(j); }

        // the lanes and signals themselves. For the tests, which damage the state on purpose
        // to check that selfCheck() notices. Normal code never needs this
        TrafficFlow& getTraffic(){ return flow; }

        // the last events, newest first (k = 0). empty text if there are fewer events
        string getRecentEvent(int k);
        int getEventCount(){ return logTotal; }
};
