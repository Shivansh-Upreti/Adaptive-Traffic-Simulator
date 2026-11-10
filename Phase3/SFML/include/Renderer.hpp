#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <string>
using namespace std;
#include "../../core/include/Simulator.hpp"
#include "ViewModel.hpp"

// ======================================================================
// Renderer (phase 3, SFML version): draws the state of the Simulator with
// plain shapes (rectangles and circles) and text. It only READS the
// simulator, it never changes it.
//
// Every function draws onto "target". A RenderWindow is a RenderTarget, so
// main.cpp passes its window; the same functions can also draw into a
// picture in memory.
//
// Colours follow the earlier SFML prototype: dark background, grey roads
// with a dashed centre line, blue junctions, a lamp box at every junction,
// an orange line for the route and a panel on the right.
// ======================================================================

const unsigned WINDOW_W = 1280;
const unsigned WINDOW_H = 720;
const float SIDEBAR_X = 960.f;

// where every junction is drawn (index = junction number, index 0 is not used)
const float JX[NUM_JUNCTIONS + 1] = {0.f, 100.f, 290.f, 290.f, 500.f, 690.f, 820.f};
const float JY[NUM_JUNCTIONS + 1] = {0.f, 340.f, 130.f, 560.f, 340.f, 560.f, 240.f};

// top-left corner of the lane panel (main lane + side lane) of every junction
const float PX[NUM_JUNCTIONS + 1] = {0.f, 30.f, 215.f, 215.f, 420.f, 620.f, 760.f};
const float PY[NUM_JUNCTIONS + 1] = {0.f, 385.f, 55.f, 612.f, 372.f, 612.f, 170.f};

// the nine roads of the starting map. A road that is missing from the graph is
// drawn as a red dashed line, so a closed road stays visible
const int START_ROADS = 9;
const int ROAD_A[START_ROADS] = {1, 1, 2, 3, 3, 4, 2, 4, 5};
const int ROAD_B[START_ROADS] = {2, 3, 4, 4, 5, 5, 6, 6, 6};

const int BOXES_SHOWN = 8;          // at most 8 vehicle boxes per lane row, the rest is shown as "+N"

// ----------------------------------------------------------------------
// small drawing helpers
// ----------------------------------------------------------------------

void drawText(sf::RenderTarget &target, sf::Font &font, bool fontOk, string str, unsigned size, float x, float y, sf::Color color);

// a straight line of the given thickness: a thin rectangle that is turned to the right angle
void drawLine(sf::RenderTarget &target, float ax, float ay, float bx, float by, float thickness, sf::Color color);

void drawDashedLine(sf::RenderTarget &target, float ax, float ay, float bx, float by, float dash, float gap, float thickness, sf::Color color);

// cuts a long text into lines of at most 'width' characters, breaking at spaces.
// the lines go into lines[] (at most maxLines). returns how many lines there are
int wrapText(string text, int width, string lines[], int maxLines);

// the colour of an event line depends on its [TAG]
sf::Color tagColor(string msg);

// ----------------------------------------------------------------------
// the map
// ----------------------------------------------------------------------

void drawRoads(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim);

// orange line along the route that is still ahead of the emergency vehicle
void drawRoute(sf::RenderTarget &target, Simulator &sim, ViewModel &vm);

// two lamp heads: "EV" for the emergency vehicle and "NOR" for normal traffic.
// normal state:   EV red,   NOR green
// emergency hold: EV GREEN, NOR red   <- the signal turns green for the emergency vehicle
//                                        and the normal (cross) traffic is paused
void drawSignal(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, int j, float x, float y);

// ----------------------------------------------------------------------
// The three kinds of emergency vehicle, told apart by their PRIORITY (= the key that created them):
//   1  Ambulance   white, red cross and red side stripes, red / white lights
//   2  Fire Truck  solid red and slightly LARGER, yellow / white lights
//   3  Police      dark blue with a black stripe, flashing red / blue lights
// (x, y) = the middle of the vehicle, angle = driving direction in degrees, flash = which light is on
// ----------------------------------------------------------------------
void drawEVBody(sf::RenderTarget &target, float x, float y, float angle, int priority, bool flash);

void drawJunctions(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, float seconds);

// one row of small boxes: the vehicles of a lane, front of the lane on the left.
// the vehicle that is driving right now is not drawn here (it is drawn on the road)
void drawLaneRow(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, int j, bool sideRow, float x, float y);

void drawLanePanels(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm);

// the emergency vehicle: a white box with two flashing lights
void drawEmergencyVehicle(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, float seconds);

// ----------------------------------------------------------------------
// the panel on the right: numbers, controls and the event log
// ----------------------------------------------------------------------
void drawSidebar(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm);

// ----------------------------------------------------------------------
// one whole picture
// ----------------------------------------------------------------------
void drawFrame(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, float seconds);

// the junction at a point of the PICTURE (0 if there is none).
// x and y must be picture coordinates, not window pixels: main.cpp converts them with
// window.mapPixelToCoords(), because a resized window stretches the picture
int junctionAt(float x, float y);

// tries a few places for a font. returns false if none can be loaded
bool loadFont(sf::Font &font);
