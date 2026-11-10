#pragma once
#include "Renderer.hpp"             // junction positions, drawText, drawLine, wrapText, tagColor ...
#include "LiveView.hpp"

// ======================================================================
// LiveRenderer (phase 3, SFML version): draws a RoadSim with plain shapes.
// It only READS the engine, it never changes it.
//
// How a vehicle gets its position on the screen (basic arithmetic only):
//   1. along the road:  point = A + (B - A) * t        (A, B = the two junctions, t = 0..1)
//   2. sideways:        point = point + normal * lateral
//        normal  = the unit vector pointing to the RIGHT of the driving direction
//        lateral = 9 px  normal lane        (right-hand traffic)
//                  15 px shoulder, the outer edge of the road (an EV is near)
//                  0 px  center of the road  (the EV itself)
//   3. between two ticks the point slides from where the vehicle was to where it is now (alpha).
// ======================================================================

const float LIVE_ROAD_WIDTH = 38.f;
const float LIVE_JUNCTION_RADIUS = 27.f;
const float ROAD_MARGIN_PX = 36.f;          // the lanes start and end this far from the junction centers

struct Pt{
    float x;
    float y;
};

// a point on the center line of the road a -> b. cellPos is in cells (0 = first cell), it may have a fraction
Pt livePoint(int a, int b, float cellPos, int cells);

// the unit vector that points to the RIGHT when driving from a to b (screen: x to the right, y downwards)
Pt rightNormal(int a, int b);

// where a vehicle is (center line, no sideways offset) for a given state
Pt statePoint(RoadSim &w, int state, int from, int to, int cell);

// ----------------------------------------------------------------------
// roads
// ----------------------------------------------------------------------
void drawLiveRoads(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w);

// orange line along the route that is still ahead of every emergency vehicle
void drawLiveRoutes(sf::RenderTarget &target, RoadSim &w);

// ----------------------------------------------------------------------
// junctions, signals, boxes
// ----------------------------------------------------------------------

// two lamp heads: EV and NOR. reserved (green wave): EV green, NOR red. normal: EV red, NOR green
void drawLiveSignal(sf::RenderTarget &target, sf::Font &font, bool fontOk, bool reserved, float x, float y);

void drawLiveJunctions(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, float seconds);

// ----------------------------------------------------------------------
// vehicles
// ----------------------------------------------------------------------
void drawLiveVehicle(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, int slot, float seconds);

void drawLiveVehicles(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, float seconds);

// a small legend in the bottom-left corner of the map
void drawLiveLegend(sf::RenderTarget &target, sf::Font &font, bool fontOk);

// ----------------------------------------------------------------------
// the panel on the right
// ----------------------------------------------------------------------
void drawLiveSidebar(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view);

// ----------------------------------------------------------------------
// one whole picture
// ----------------------------------------------------------------------
// the demo SUB-MENU (key D): a small box over the map. The keys 1 - 4 start a scenario, any other key closes it
void drawDemoMenu(sf::RenderTarget &target, sf::Font &font, bool fontOk);

// the USER DEFINED INPUT menu (key U): the box lists the four actions. The action that is switched on is yellow
void drawUserMenu(sf::RenderTarget &target, sf::Font &font, bool fontOk, LiveView &view);

void drawLiveFrame(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, float seconds);
