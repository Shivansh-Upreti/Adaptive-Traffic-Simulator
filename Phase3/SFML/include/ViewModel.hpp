#pragma once
#include <string>
using namespace std;
#include "../../core/include/Simulator.hpp"

// ======================================================================
// ViewModel (phase 3, SFML version)
// Everything the window needs to REMEMBER between two frames, and what every
// key does. There is no SFML in this file, so it can be tested without a window.
//
// The engine (Simulator) moves an emergency vehicle one road per stepDispatch()
// call, instantly. The window wants to SHOW that movement, so the ViewModel
// runs a small timeline:
//
//   VM_DWELL   the vehicle stands at a junction for a moment. The signal and
//              evacuation of that junction are on show.
//      |  after DWELL_TIME seconds: call sim.stepDispatch()
//   VM_TRAVEL  the vehicle drives along the road from travelFrom to travelTo.
//      |  after TRAVEL_TIME seconds
//   back to VM_DWELL (trip goes on) or VM_IDLE (arrived or stopped)
// ======================================================================

const int VM_IDLE = 0;          // no emergency trip is shown
const int VM_DWELL = 1;         // the vehicle waits at a junction
const int VM_TRAVEL = 2;        // the vehicle drives along a road

const float DWELL_TIME = 0.9f;      // seconds at a junction
const float TRAVEL_TIME = 1.3f;     // seconds on a road
const int DEFAULT_ROAD_LENGTH = 6;  // length of a road opened with the C key

class ViewModel{
    public:
        int startJ;             // selected start junction (left click), 0 = none
        int destJ;              // selected destination junction (right click), 0 = none

        int state;              // VM_IDLE, VM_DWELL or VM_TRAVEL
        float timer;            // seconds spent in VM_DWELL
        int travelFrom;         // the road the vehicle is driving on right now
        int travelTo;
        int travelVeh;          // id of the driving vehicle (the window does not draw it in its lane box)
        int travelPriority;     // its kind: 1 Ambulance, 2 Fire Truck, 3 Police (the window draws each one differently)
        float travelT;          // 0.0 = at travelFrom, 1.0 = at travelTo

        bool paused;
        float speed;            // 1.0 = normal
        int nextNormalID;       // the keys create vehicles with the next free id
        int nextEmergencyID;
        string checkText;       // result of the last self-check

        ViewModel(){
            reset();
        }

        void reset();

        // mouse
        void selectStart(int j);
        void selectDest(int j);

        // keys
        void actionSpawnNormal(Simulator &sim);

        void actionSpawnEmergency(Simulator &sim, int type);

        void actionDispatch(Simulator &sim);

        void actionMoveNormal(Simulator &sim);

        void actionLeave(Simulator &sim);

        // closes the road between the two selected junctions, or opens it if it is closed
        void actionToggleRoad(Simulator &sim);

        void actionSelfCheck(Simulator &sim);

        // a ready-made scene: normal traffic and three emergency requests, so the
        // priority order, the bypass and the evacuation can be seen at once
        void actionDemo(Simulator &sim);

        // time
        // dt = seconds since the last call (already multiplied by the speed)
        void update(Simulator &sim, float dt);

    private:
        // asks the engine for one road and decides what the window has to show
        void stepOnce(Simulator &sim);
};
