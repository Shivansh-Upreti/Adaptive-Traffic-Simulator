#pragma once

// the traffic signal of ONE junction (new in phase 3).
//
// emergencyHolds counts how many emergency vehicles currently need this junction.
//   0  -> NORMAL: normal traffic may go
//   1+ -> EMERGENCY: the light is green for the emergency vehicle and the
//         normal (cross) traffic is paused
//
// it is a counter and not a true/false flag so that one emergency vehicle
// letting go can never switch off the hold of another one.
// this is a step-by-step model, there is no real-time clock.
class TrafficSignal{
    int emergencyHolds;

    public:
        TrafficSignal(){
            emergencyHolds = 0;
        }

        // an emergency vehicle needs this junction: green for it, cross-traffic paused
        void preempt(){
            emergencyHolds++;
        }

        // the emergency vehicle has passed. returns false if there was no hold to give back
        // (so the counter can never go below 0)
        bool release();

        bool isPreempted() const {
            return emergencyHolds > 0;
        }

        int getHolds() const {
            return emergencyHolds;
        }

        // normal vehicles may only leave this junction while there is no emergency hold
        bool allowsNormalTraffic() const {
            return emergencyHolds == 0;
        }


        void reset(){
            emergencyHolds = 0;
        }
};
