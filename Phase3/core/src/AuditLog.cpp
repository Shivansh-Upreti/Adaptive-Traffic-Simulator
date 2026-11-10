#include "../include/AuditLog.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>

bool appendEVAuditRow(const string &fileName, long long tick, int vehicleID, int priority,
                      int startJ, int destJ, const string &path, long long timeTaken){
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
            file << AUDIT_HEADER << endl;
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

bool clearEVAuditLog(const string &fileName){
    ofstream wipe(fileName.c_str(), ios::trunc);
    if(!wipe.is_open()){
        return false;
    }
    wipe.close();
    return true;
}
