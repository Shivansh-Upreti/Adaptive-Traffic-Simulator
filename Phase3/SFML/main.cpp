// ADAPTIVE TRAFFIC SIGNAL AND EMERGENCY VEHICLE SIMULATOR -- PHASE 3 (SFML)
//
// The window has two modes (key Tab switches between them):
//   LIVE  mode (default): RoadSim, vehicles that drive along the roads. Junction boxes, green
//         wave, center-overtake, EV priority conflicts.   Drawing: LiveRenderer.hpp, keys: LiveView.hpp
//   LAB   mode: Simulator, the junction-to-junction dispatch lab with signals and side lanes.
//         Drawing: Renderer.hpp, keys: ViewModel.hpp
// The window only shows and controls the shared engines in ../core.
//
// build (MinGW, SFML 2.6.2 in C:/SFML-2.6.2). CMake does all of this; by hand (Git Bash, from this folder) the program is
// main.cpp + the 4 files of src/ + the 13 files of ../core/src:
//   g++ -std=c++17 -Wall -Wextra -IC:/SFML-2.6.2/include main.cpp src/*.cpp ../core/src/*.cpp -o traffic_sfml -LC:/SFML-2.6.2/lib -lsfml-graphics -lsfml-window -lsfml-system
// the program needs the three sfml-*-2.dll files from C:/SFML-2.6.2/bin next to it
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
using namespace std;

#include "include/LiveRenderer.hpp"

// ----------------------------------------------------------------------
// The multi-EV demo scenes (key D in live mode opens a SUB-MENU, then the key 1 - 9 or 0 (= scene 10) picks the scenario).
//   1  J4 CONVERGENCE   Ambulance (priority 1), Fire Truck (2) and Police car (3) start from three different junctions
//                       (J3, J5, J2) at the same moment and all reach J4 together. The priority queue sorts them:
//                       1st Ambulance, 2nd Fire Truck, 3rd Police car.
//   2  AMBULANCE TIE    two Ambulances (both priority 1) meet at J4, the second is created one tick later
//   3  FIRE TRUCK TIE   two Fire Trucks (both priority 2) meet at J4, same delay
//   4  POLICE TIE       two Police cars (both priority 3) meet at J4, same delay
//   5  ULTIMATE J4 BOTTLENECK  FOUR EVs: Ambulance A and B (priority 1) start at the SAME junction (B one tick later) and drive
//                       the same road behind each other; a Fire Truck (2) and a Police car (3) come from other junctions.
//                       Passing order at J4: Ambulance A, Ambulance B, Fire Truck, Police car
//   6  HEAD-ON ROAD     Ambulance A (J2 -> J4) and Ambulance B (J4 -> J2) enter the SAME road in the same tick. Both keep driving:
//                       each one moves to its own side of the road (head-on pass mode), so they pass without clipping
//   7  JUNCTION ARRIVAL Ambulance A (J3 -> J6) and B (J6 -> J3) enter J4 from OPPOSITE sides in the same tick: the arrival
//                       time decides who gets the junction first
//   8  ROAD CLOSED      Ambulance J1 -> J6: the road J1 - J2 closes right after it is created, Dijkstra re-plans it
//                       onto the detour J1 -> J3 -> J4 -> J6
//   9  GRIDLOCK         20 normal vehicles jam J5 -> J6, the Ambulance drives down the center (center-overtake)
//   10 NO WAY THROUGH   (key 0) all roads into J6 close mid-trip: the Ambulance is stopped safely at J2
//   In 2 - 4 and 7 the equal priority is decided by the EARLIER ARRIVAL TIME: ArrayPriorityQueue (Phase 2) is unchanged.
// Every scene has 40 ticks of background traffic first (each vehicle with its OWN random destination), never more than
// 3 emergency vehicles (scene 5: 4), and ENDS BY ITSELF when they have arrived; then D can be pressed again.
// The console prints [QUEUE ORDER] (1st, 2nd, 3rd), [TIE-BREAKER PROVED] and [PASS J4] no. 1, 2, 3 ... while the scene runs.
// The spawn calls are in buildMultiEVDemo() (core/src/DemoScene.cpp), so that the tests run the very same scenes.
// ----------------------------------------------------------------------
void runMultiEVDemoScene(RoadSim &world, LiveView &view, int scenario){
    switch(scenario){
        case 1:
            cout << "\n===== DEMO 1: J4 CONVERGENCE =====" << endl;
            cout << "Ambulance (1), Fire Truck (2) and Police car (3) start from different junctions and reach J4 together." << endl;
            cout << "Expected queue order: 1st Ambulance, 2nd Fire Truck, 3rd Police car." << endl;
            break;
        case 2:
            cout << "\n===== DEMO 2: AMBULANCE TIE-BREAKER =====" << endl;
            cout << "Two Priority 1 Ambulances meet at J4. The one created first (ID 901) must win." << endl;
            break;
        case 3:
            cout << "\n===== DEMO 3: FIRE TRUCK TIE-BREAKER =====" << endl;
            cout << "Two Priority 2 Fire Trucks meet at J4. The one created first (ID 901) must win." << endl;
            break;
        case 4:
            cout << "\n===== DEMO 4: POLICE TIE-BREAKER =====" << endl;
            cout << "Two Priority 3 Police cars meet at J4. The one created first (ID 901) must win." << endl;
            break;
        case 5:
            cout << "\n===== DEMO 5: ULTIMATE J4 BOTTLENECK =====" << endl;
            cout << "Ambulance A (901) and Ambulance B (902) start at J2, B one tick later; Fire Truck (903) from J3, Police car (904) from J5." << endl;
            cout << "Expected passing order at J4: 901 Ambulance A, 902 Ambulance B, 903 Fire Truck, 904 Police car." << endl;
            break;
        case 6:
            cout << "\n===== DEMO 6: HEAD-ON ROAD CONFLICT (J2 <-> J4) =====" << endl;
            cout << "Ambulance A (901) drives J2 -> J4 and Ambulance B (902) drives J4 -> J2 on the SAME road, entering it in the same tick." << endl;
            cout << "Expected: the road sees 2 EVs (activeEVCount), the normal vehicles move to the shoulders, A and B each move to their own side and pass without clipping." << endl;
            break;
        case 7:
            cout << "\n===== DEMO 7: SIMULTANEOUS JUNCTION ARRIVAL & TIE-BREAKER =====" << endl;
            cout << "Ambulance A (901) J3 -> J6 and Ambulance B (902) J6 -> J3 enter J4 from opposite sides in the same tick." << endl;
            cout << "Expected: equal priority, so the earlier arrival time decides: 901 gets J4 first." << endl;
            break;
        case 8:
            cout << "\n===== DEMO 8: ROAD CLOSED - DIJKSTRA DETOUR =====" << endl;
            cout << "Ambulance (901) is planned J1 -> J2 -> J6. The road J1 - J2 closes right after it is created." << endl;
            cout << "Expected: Dijkstra re-plans it at once onto the detour J1 -> J3 -> J4 -> J6." << endl;
            break;
        case 9:
            cout << "\n===== DEMO 9: GRIDLOCK - CENTER-OVERTAKE =====" << endl;
            cout << "20 normal vehicles jam the road J5 -> J6, then an Ambulance (901) drives J5 -> J6." << endl;
            cout << "Expected: the jammed vehicles wait on the shoulders and the Ambulance drives down the center without stopping." << endl;
            break;
        case 10:
            cout << "\n===== DEMO 10: NO WAY THROUGH =====" << endl;
            cout << "Ambulance (901) starts J1 -> J2 -> J6, then all three roads into J6 close." << endl;
            cout << "Expected: at J2 it finds no way left and is stopped safely ([STOPPED] in the log), no crash." << endl;
            break;
        default:
            return;                         // not a scenario: nothing to do
    }
    view.actionDemo(world, scenario);       // buildMultiEVDemo(world, feeder, scenario) + background traffic ON + demo marked as running
}

int main(){
    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Phase 3 - LIVE TRAFFIC");
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);      // holding a key down must not repeat the action

    sf::Font font;
    bool fontOk = loadFont(font);

    // normal local variables: they are destroyed (and free all their vehicles) when main ends.
    // No pointers are needed, because the R key calls reset() instead of deleting anything
    RoadSim world;                          // live traffic engine
    LiveView liveView;
    liveView.consoleLog = true;             // the user actions are also written to the console for the evaluator
    Simulator lab;                          // lab engine
    ViewModel labView;
    bool liveMode = true;

    world.setAuditLog(true);                // every finished EV trip becomes a row of ev_audit_log.csv (R empties it)
    world.addNote("[INFO] Press U for User Defined Input Mode (spawn, batch, road closure, opposing traffic) or D for the demo scenes.");
    world.addNote("[INFO] Press G to switch adaptive traffic signals on or off.");

    sf::Clock frameClock;       // time since the last frame
    sf::Clock totalClock;       // time since the start (used for the blinking lights)

    while(window.isOpen()){
        // ---- 1. events: keys, mouse, closing the window ----
        sf::Event event;
        while(window.pollEvent(event)){
            if(event.type == sf::Event::Closed){
                window.close();
            }
            else if(event.type == sf::Event::MouseButtonPressed){
                // the mouse position is in window pixels. If the window was resized, the picture is
                // stretched, so the pixel must be converted to a position inside the picture first
                sf::Vector2f spot = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                int j = junctionAt(spot.x, spot.y);
                // STRICT: the LEFT button chooses the START junction, the RIGHT button the DESTINATION.
                // Every other button (middle, side buttons) does nothing. A click on empty ground (j = 0) changes nothing.
                // Vehicles are created at START and drive to DESTINATION.
                if(event.mouseButton.button == sf::Mouse::Left){
                    if(liveMode) liveView.selectStart(j);
                    else labView.selectStart(j);
                }
                else if(event.mouseButton.button == sf::Mouse::Right){
                    if(liveMode) liveView.selectDest(j);
                    else labView.selectDest(j);
                    if(liveMode && j != 0){
                        liveView.userRightClick(world);        // only does something inside User Input actions [2] and [3]
                    }
                }
            }
            else if(event.type == sf::Event::KeyPressed){
                // the digit of the key (0 1 2 3 4 9), -1 for every other key
                int digit = -1;
                switch(event.key.code){
                    case sf::Keyboard::Num1:
                    case sf::Keyboard::Numpad1:
                        digit = 1;
                        break;
                    case sf::Keyboard::Num2:
                    case sf::Keyboard::Numpad2:
                        digit = 2;
                        break;
                    case sf::Keyboard::Num3:
                    case sf::Keyboard::Numpad3:
                        digit = 3;
                        break;
                    case sf::Keyboard::Num4:
                    case sf::Keyboard::Numpad4:
                        digit = 4;
                        break;
                    case sf::Keyboard::Num9:
                    case sf::Keyboard::Numpad9:
                        digit = 9;
                        break;
                    case sf::Keyboard::Num0:
                    case sf::Keyboard::Numpad0:
                        digit = 0;
                        break;
                    default:
                        break;
                }

                if(liveMode && liveView.uiState != UI_MAIN && (digit != -1 || event.key.code == sf::Keyboard::Escape)){
                    // ---- USER DEFINED INPUT mode (key U): the digits choose / run an action, ESC goes one step back ----
                    if(event.key.code == sf::Keyboard::Escape){
                        liveView.actionUserBack();
                    }
                    else{
                        liveView.userInputKey(world, digit);
                    }
                }
                else if(liveMode && liveView.demoMenuOpen){
                    // the demo SUB-MENU is open: 1 - 9 and 0 (= 10) choose a scenario, any other key closes the menu
                    int choice = 0;
                    switch(event.key.code){
                        case sf::Keyboard::Num1:
                        case sf::Keyboard::Numpad1:
                            choice = 1;
                            break;
                        case sf::Keyboard::Num2:
                        case sf::Keyboard::Numpad2:
                            choice = 2;
                            break;
                        case sf::Keyboard::Num3:
                        case sf::Keyboard::Numpad3:
                            choice = 3;
                            break;
                        case sf::Keyboard::Num4:
                        case sf::Keyboard::Numpad4:
                            choice = 4;
                            break;
                        case sf::Keyboard::Num5:
                        case sf::Keyboard::Numpad5:
                            choice = 5;
                            break;
                        case sf::Keyboard::Num6:
                        case sf::Keyboard::Numpad6:
                            choice = 6;
                            break;
                        case sf::Keyboard::Num7:
                        case sf::Keyboard::Numpad7:
                            choice = 7;
                            break;
                        case sf::Keyboard::Num8:
                        case sf::Keyboard::Numpad8:
                            choice = 8;
                            break;
                        case sf::Keyboard::Num9:
                        case sf::Keyboard::Numpad9:
                            choice = 9;
                            break;
                        case sf::Keyboard::Num0:
                        case sf::Keyboard::Numpad0:
                            choice = 10;                // key 0 = scene 10
                            break;
                        default:
                            break;
                    }
                    if(choice > 0){
                        runMultiEVDemoScene(world, liveView, choice);
                    }
                    else{
                        liveView.demoMenuOpen = false;
                    }
                }
                // keys that work in both modes
                else if(event.key.code == sf::Keyboard::Escape){
                    window.close();
                }
                else if(event.key.code == sf::Keyboard::Tab){
                    liveMode = !liveMode;
                    if(liveMode) window.setTitle("Phase 3 - LIVE TRAFFIC");
                    else window.setTitle("Phase 3 - LAB (junction dispatch)");
                }
                else if(event.key.code == sf::Keyboard::T){
                    if(liveMode) liveView.paused = !liveView.paused;
                    else labView.paused = !labView.paused;
                }
                else if(event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal){
                    if(liveMode && liveView.speed < 3.0f) liveView.speed = liveView.speed + 0.25f;
                    if(!liveMode && labView.speed < 3.0f) labView.speed = labView.speed + 0.25f;
                }
                else if(event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Hyphen){
                    if(liveMode && liveView.speed > 0.5f) liveView.speed = liveView.speed - 0.25f;
                    if(!liveMode && labView.speed > 0.5f) labView.speed = labView.speed - 0.25f;
                }
                else if(liveMode){
                    // ---- LIVE mode keys ----
                    switch(event.key.code){
                        // Spawning, batches, road closures and opposing traffic are NOT here any more:
                        // they only exist inside the User Defined Input mode (key U), so nothing happens by accident.
                        case sf::Keyboard::U:
                            liveView.actionOpenUserMenu();             // U: User Defined Input mode (1 single, 2 batch, 3 road, 4 opposing)
                            break;
                        case sf::Keyboard::D:
                            if(liveView.uiState == UI_MAIN){
                                liveView.actionToggleDemoMenu(world);  // opens the demo sub-menu (keys 1 - 9, 0)
                            }
                            break;
                        case sf::Keyboard::K:
                            liveView.actionSelfCheck(world);
                            break;
                        case sf::Keyboard::Space:
                            liveView.paused = !liveView.paused;
                            break;
                        case sf::Keyboard::R:
                            world.reset();          // deletes every vehicle that is still alive
                            liveView.reset();       // and the window forgets its selection and timeline too
                            if(!clearEVAuditLog(world.getAuditFile())){
                                world.addNote("[AUDIT] The audit file could not be emptied.");
                            }
                            break;
                        case sf::Keyboard::G:
                            world.setAdaptiveSignals(!world.isAdaptive());
                            break;
                        default:
                            break;
                    }
                }
                else{
                    // ---- LAB mode keys ----
                    switch(event.key.code){
                        case sf::Keyboard::N:
                            labView.actionSpawnNormal(lab);
                            break;
                        case sf::Keyboard::Num1:
                        case sf::Keyboard::Numpad1:
                            labView.actionSpawnEmergency(lab, PRIORITY_AMBULANCE);
                            break;
                        case sf::Keyboard::Num2:
                        case sf::Keyboard::Numpad2:
                            labView.actionSpawnEmergency(lab, PRIORITY_FIRE_TRUCK);      // key 2: Fire Truck, priority 2
                            break;
                        case sf::Keyboard::Num3:
                        case sf::Keyboard::Numpad3:
                            labView.actionSpawnEmergency(lab, PRIORITY_POLICE);          // key 3: Police, priority 3
                            break;
                        case sf::Keyboard::Space:
                            labView.actionDispatch(lab);
                            break;
                        case sf::Keyboard::M:
                            labView.actionMoveNormal(lab);
                            break;
                        case sf::Keyboard::L:
                            labView.actionLeave(lab);
                            break;
                        case sf::Keyboard::C:
                            labView.actionToggleRoad(lab);
                            break;
                        case sf::Keyboard::B:
                            labView.actionDemo(lab);
                            break;
                        case sf::Keyboard::K:
                            labView.actionSelfCheck(lab);
                            break;
                        case sf::Keyboard::R:
                            lab.reset();            // deletes every vehicle (a running trip gives its signal back first)
                            labView.reset();
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        // ---- 2. update ----
        float dt = frameClock.restart().asSeconds();
        if(dt > 0.1f){
            dt = 0.1f;                  // a long pause (window dragged) must not make vehicles jump
        }
        if(liveMode){
            if(!liveView.paused){
                liveView.update(world, dt * liveView.speed);
            }
        }
        else{
            if(!labView.paused){
                labView.update(lab, dt * labView.speed);
            }
        }

        // ---- 3. draw ----
        float seconds = totalClock.getElapsedTime().asSeconds();
        if(liveMode){
            drawLiveFrame(window, font, fontOk, world, liveView, seconds);
        }
        else{
            drawFrame(window, font, fontOk, lab, labView, seconds);
        }
        window.display();
    }

    return 0;                           // world and lab are destroyed here: their destructors delete every vehicle
}