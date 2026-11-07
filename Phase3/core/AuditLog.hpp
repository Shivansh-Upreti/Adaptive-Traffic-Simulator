#pragma once
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
using namespace std;

// CSV audit log of the emergency vehicle trips (file handling with <fstream>)
// Every finished EV trip is ONE new row at the end of the file ev_audit_log.csv:
//   tick,vehicle_id,priority,start,destination,path,Time Taken (Ticks)

const string AUDIT_FILE_NAME = "ev_audit_log.csv";

// Appends one row. Returns true if the row was written, false if anything went wrong
// (the simulation simply goes on: a broken log file must never stop the traffic).
bool appendEVAuditRow(const string &fileName, long long tick, int vehicleID, int priority, int startJ, int destJ, const string &path, long long timeTaken){
    try{
        // 1. is there a file with content already? (then the header row is there)
        ifstream test(fileName.c_str());
        bool isNewFile = !test.is_open() || test.peek() == EOF;     // missing OR empty (after a reset): the header is needed
        test.close();

        // 2. open it for APPENDING: ios::app puts every write at the END of the file
        ofstream file(fileName.c_str(), ios::app);
        if(!file.is_open()){
            cout << "[AUDIT] Cannot open " << fileName << ": the trip was not logged." << endl;
            return false;
        }
        file.exceptions(ofstream::failbit | ofstream::badbit);     // from now on a failed write throws an exception

        // 3. write the row
        if(isNewFile){
            file << "tick,vehicle_id,priority,start,destination,path,Time Taken (Ticks)" << endl;
        }
        file << tick << "," << vehicleID << "," << priority << ",J" << startJ << ",J" << destJ << "," << path << "," << timeTaken << endl;
        file.close();
        return true;
    }
    catch(const exception &e){
        cout << "[AUDIT] Writing to " << fileName << " failed: " << e.what() << endl;
        return false;
    }
}
