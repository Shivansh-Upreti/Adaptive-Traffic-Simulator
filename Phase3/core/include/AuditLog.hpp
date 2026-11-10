#pragma once
#include <string>
using namespace std;

// CSV audit log of the emergency vehicle trips (file handling with <fstream>)
// Every finished EV trip is ONE new row at the end of the file ev_audit_log.csv:
//   tick,vehicle_id,priority,start,destination,path,Time Taken (Ticks)

const string AUDIT_FILE_NAME = "ev_audit_log.csv";
const string AUDIT_HEADER = "tick,vehicle_id,priority,start,destination,path,Time Taken (Ticks)";

// Appends one row. Returns true if the row was written, false if anything went wrong
// (the simulation simply goes on: a broken log file must never stop the traffic).
bool appendEVAuditRow(const string &fileName, long long tick, int vehicleID, int priority,
                      int startJ, int destJ, const string &path, long long timeTaken);

// Empties the file (the R key of the window). Returns false if it cannot be opened.
bool clearEVAuditLog(const string &fileName);
