#include "../include/LiveRenderer.hpp"

Pt livePoint(int a, int b, float cellPos, int cells){
    float dx = JX[b] - JX[a];
    float dy = JY[b] - JY[a];
    float len = sqrt(dx * dx + dy * dy);
    float margin = ROAD_MARGIN_PX / len;                // the same margin as a fraction of the road
    float t = (cellPos + 0.5f) / (float)cells;          // 0..1 along the lanes
    float f = margin + t * (1.f - 2.f * margin);        // 0..1 along the whole road
    Pt p;
    p.x = JX[a] + dx * f;
    p.y = JY[a] + dy * f;
    return p;
}

Pt rightNormal(int a, int b){
    float dx = JX[b] - JX[a];
    float dy = JY[b] - JY[a];
    float len = sqrt(dx * dx + dy * dy);
    Pt n;
    n.x = -dy / len;
    n.y = dx / len;
    return n;
}

Pt statePoint(RoadSim &w, int state, int from, int to, int cell){
    Pt p;
    p.x = 0.f;
    p.y = 0.f;
    if(state == MV_ON_ROAD && w.getCells(from, to) > 0){
        p = livePoint(from, to, (float)cell, w.getCells(from, to));
    }
    else if(state == MV_IN_BOX || state == MV_QUEUED){
        p.x = JX[from];                                 // in the middle of the junction
        p.y = JY[from];
    }
    return p;
}

void drawLiveRoads(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w){
    for(int a = 1; a <= NUM_JUNCTIONS; a++){
        for(int b = a + 1; b <= NUM_JUNCTIONS; b++){
            if(!w.hasRoad(a, b)){
                continue;
            }
            float ax = JX[a], ay = JY[a], bx = JX[b], by = JY[b];
            Pt n = rightNormal(a, b);

            drawLine(target, ax, ay, bx, by, LIVE_ROAD_WIDTH, sf::Color(58, 64, 80));                   // the road
            // the shoulders: a lighter strip along both outer edges
            for(int side = -1; side <= 1; side += 2){
                float o = side * (LIVE_ROAD_WIDTH / 2.f - 2.5f);
                drawLine(target, ax + n.x * o, ay + n.y * o, bx + n.x * o, by + n.y * o, 5.f, sf::Color(82, 90, 108));
            }
            drawDashedLine(target, ax, ay, bx, by, 12.f, 10.f, 2.f, sf::Color(235, 225, 120, 200));     // line between the two lanes
            // the number next to the road is its estimated TRAVEL TIME in seconds (the slower of the two directions).
            // grey = free road, orange = normal vehicles are waiting on it, red = an emergency vehicle pre-empts it
            int seconds = w.edgeTravelSeconds(a, b);
            if(w.edgeTravelSeconds(b, a) > seconds){
                seconds = w.edgeTravelSeconds(b, a);
            }
            sf::Color labelColor(150, 156, 172);
            if(w.edgeWaitingCount(a, b) + w.edgeWaitingCount(b, a) > 0){
                labelColor = sf::Color(255, 190, 90);
            }
            if(w.edgePreempted(a, b)){
                labelColor = sf::Color(255, 100, 100);
            }
            drawText(target, font, fontOk, to_string(seconds) + "s", 11, (ax + bx) / 2.f - 7.f, (ay + by) / 2.f - 6.f, labelColor);
        }
    }
    // closed roads: red dashed line with a cross
    for(int k = 0; k < START_ROADS; k++){
        int a = ROAD_A[k];
        int b = ROAD_B[k];
        if(w.hasRoad(a, b)){
            continue;
        }
        float mx = (JX[a] + JX[b]) / 2.f;
        float my = (JY[a] + JY[b]) / 2.f;
        drawDashedLine(target, JX[a], JY[a], JX[b], JY[b], 8.f, 8.f, 3.f, sf::Color(220, 70, 70, 200));
        drawLine(target, mx - 9.f, my - 9.f, mx + 9.f, my + 9.f, 4.f, sf::Color(240, 70, 70));
        drawLine(target, mx - 9.f, my + 9.f, mx + 9.f, my - 9.f, 4.f, sf::Color(240, 70, 70));
        drawText(target, font, fontOk, "CLOSED", 11, mx - 20.f, my + 12.f, sf::Color(240, 110, 110));
    }
}

void drawLiveRoutes(sf::RenderTarget &target, RoadSim &w){
    sf::Color orange(255, 140, 0, 150);
    for(int s = 0; s < MAX_MOVERS; s++){
        if(!w.isSlotEV(s)){
            continue;
        }
        int i = 0;
        while(w.getSlotRouteJunction(s, i) != 0 && w.getSlotRouteJunction(s, i + 1) != 0){
            int a = w.getSlotRouteJunction(s, i);
            int b = w.getSlotRouteJunction(s, i + 1);
            drawLine(target, JX[a], JY[a], JX[b], JY[b], 4.f, orange);
            i++;
        }
    }
}

void drawLiveSignal(sf::RenderTarget &target, sf::Font &font, bool fontOk, bool reserved, float x, float y){
    sf::RectangleShape box(sf::Vector2f(40.f, 38.f));
    box.setPosition(x, y);
    box.setFillColor(sf::Color(15, 17, 22));
    box.setOutlineColor(reserved ? sf::Color(110, 230, 130) : sf::Color(70, 74, 86));
    box.setOutlineThickness(reserved ? 2.f : 1.f);
    target.draw(box);

    sf::Color redOn(235, 60, 60), redOff(70, 25, 25);
    sf::Color greenOn(80, 230, 100), greenOff(25, 70, 30);
    for(int head = 0; head < 2; head++){
        float cx = x + 6.f + head * 18.f;
        bool green;
        if(head == 0){
            green = reserved;
        }
        else{
            green = !reserved;
        }
        sf::CircleShape top(5.f);
        top.setPosition(cx, y + 5.f);
        top.setFillColor(green ? redOff : redOn);
        target.draw(top);
        sf::CircleShape bottom(5.f);
        bottom.setPosition(cx, y + 22.f);
        bottom.setFillColor(green ? greenOn : greenOff);
        target.draw(bottom);
    }
    drawText(target, font, fontOk, "EV", 9, x + 4.f, y + 39.f, sf::Color(150, 155, 170));
    drawText(target, font, fontOk, "NOR", 9, x + 21.f, y + 39.f, sf::Color(150, 155, 170));
}

void drawLiveJunctions(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, float seconds){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        float x = JX[j];
        float y = JY[j];
        bool reserved = w.isReserved(j);

        if(reserved){                                           // green wave: pulsing green ring
            float r = LIVE_JUNCTION_RADIUS + 5.f + 3.f * sin(seconds * 6.f);
            sf::CircleShape ring(r);
            ring.setOrigin(r, r);
            ring.setPosition(x, y);
            ring.setFillColor(sf::Color(0, 0, 0, 0));
            ring.setOutlineColor(sf::Color(110, 230, 130));
            ring.setOutlineThickness(3.f);
            target.draw(ring);
        }

        sf::CircleShape circle(LIVE_JUNCTION_RADIUS);           // the junction box
        circle.setOrigin(LIVE_JUNCTION_RADIUS, LIVE_JUNCTION_RADIUS);
        circle.setPosition(x, y);
        circle.setFillColor(sf::Color(38, 58, 96));
        circle.setOutlineColor(sf::Color(210, 225, 245));
        circle.setOutlineThickness(2.f);
        if(w.isJunctionOccupied(j)){
            circle.setOutlineColor(sf::Color(255, 90, 90));     // occupied box: red outline
            circle.setOutlineThickness(4.f);
        }
        if(j == view.startJ){
            circle.setOutlineColor(sf::Color(90, 240, 120));
            circle.setOutlineThickness(4.f);
        }
        if(j == view.destJ){
            circle.setOutlineColor(sf::Color(255, 220, 60));
            circle.setOutlineThickness(4.f);
        }
        target.draw(circle);

        drawText(target, font, fontOk, "J" + to_string(j), 14, x - 10.f, y - 31.f - 1.f, sf::Color(235, 240, 250));
        if(j == view.startJ){
            drawText(target, font, fontOk, "START", 12, x - 20.f, y - 52.f, sf::Color(90, 240, 120));
        }
        if(j == view.destJ){
            drawText(target, font, fontOk, "DESTINATION", 12, x - 38.f, y - 52.f, sf::Color(255, 220, 60));
        }
        drawLiveSignal(target, font, fontOk, reserved, x + 32.f, y - 18.f);

        // small notes under the junction
        float ty = y + LIVE_JUNCTION_RADIUS + 3.f;
        if(w.getSpawnQueueLength(j) > 0){
            drawText(target, font, fontOk, "waiting: " + to_string(w.getSpawnQueueLength(j)), 10, x - 28.f, ty, sf::Color(200, 205, 220));
            ty = ty + 11.f;
        }
        if(reserved){
            drawText(target, font, fontOk, "green wave: EV " + to_string(w.getReservedBy(j)), 10, x - 40.f, ty, sf::Color(120, 235, 140));
        }
    }
}

void drawLiveVehicle(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, int slot, float seconds){
    int state = w.getSlotState(slot);
    if(state == MV_FREE || state == MV_QUEUED){
        return;                                                 // vehicles in a waiting line are shown as a number at the junction
    }
    int id = w.getSlotID(slot);
    int from = w.getSlotFrom(slot);
    int to = w.getSlotTo(slot);
    float alpha = view.getAlpha();
    float lat = view.lateral[slot];

    // where it is now ...
    Pt cur = statePoint(w, state, from, to, w.getSlotCell(slot));
    // ... and where it was before the last tick
    PosSnap &pv = view.prev[slot];
    bool hadPrev = (pv.id == id && pv.state != MV_FREE);
    Pt old = cur;
    if(hadPrev){
        old = statePoint(w, pv.state, pv.from, pv.to, pv.cell);
    }

    // sideways offset (only while on a road; inside the junction box a vehicle is in the middle)
    Pt curN;
    curN.x = 0.f;
    curN.y = 0.f;
    if(state == MV_ON_ROAD){
        curN = rightNormal(from, to);
    }
    Pt oldN;
    oldN.x = 0.f;
    oldN.y = 0.f;
    if(hadPrev && pv.state == MV_ON_ROAD){
        oldN = rightNormal(pv.from, pv.to);
    }
    float cx = cur.x + curN.x * lat;
    float cy = cur.y + curN.y * lat;
    float ox = old.x + oldN.x * lat;
    float oy = old.y + oldN.y * lat;

    float x = ox + (cx - ox) * alpha;                           // slide from the old point to the new one
    float y = oy + (cy - oy) * alpha;

    // which way does it face?
    float hx = 1.f;
    float hy = 0.f;
    if(state == MV_ON_ROAD){
        hx = JX[to] - JX[from];
        hy = JY[to] - JY[from];
    }
    else{
        int next = w.getSlotRouteJunction(slot, 0);             // inside the box: toward the junction it will drive to
        if(next != 0){
            hx = JX[next] - JX[from];
            hy = JY[next] - JY[from];
        }
        else if(hadPrev && pv.state == MV_ON_ROAD){
            hx = JX[pv.to] - JX[pv.from];
            hy = JY[pv.to] - JY[pv.from];
        }
    }
    float angle = atan2(hy, hx) * 180.f / 3.14159265f;

    if(w.isSlotEV(slot)){
        bool phase = ((int)(seconds * 6.f + slot)) % 2 == 0;    // the lights flash six times a second
        int kind = w.getSlotPriority(slot);                     // 1 Ambulance, 2 Fire Truck, 3 Police: each is drawn differently
        drawEVBody(target, x, y, angle, kind, phase);

        string label = w.getSlotLabel(slot) + " " + to_string(id) + " (p" + to_string(kind) + ")";
        sf::Color labelColor(255, 255, 255);
        if(w.getSlotWaitCode(slot) != WAIT_NONE){
            labelColor = sf::Color(255, 220, 80);               // waiting at a line
            label = label + " waits";
        }
        else if(w.isFrozen(slot)){
            labelColor = sf::Color(255, 160, 90);
            label = label + " gives way";
        }
        drawText(target, font, fontOk, label, 11, x - 30.f, y - 28.f, labelColor);
    }
    else{
        // a normal vehicle: grey-blue; amber on the shoulder; with a red edge when it is frozen
        sf::Color palette[6] = {
            sf::Color(150, 172, 205), sf::Color(165, 190, 150), sf::Color(200, 170, 140),
            sf::Color(160, 150, 200), sf::Color(190, 190, 150), sf::Color(150, 200, 200)
        };
        sf::Color fill = palette[id % 6];
        bool onShoulder = w.isOnShoulder(slot);
        bool frozen = w.isFrozen(slot);
        if(onShoulder){
            fill = sf::Color(255, 190, 80);
        }
        sf::RectangleShape car(sf::Vector2f(15.f, 7.5f));
        car.setOrigin(7.5f, 3.75f);
        car.setPosition(x, y);
        car.setRotation(angle);
        car.setFillColor(fill);
        if(frozen){
            car.setOutlineColor(sf::Color(235, 70, 70));
            car.setOutlineThickness(1.5f);
        }
        target.draw(car);
    }
}

void drawLiveVehicles(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, float seconds){
    for(int s = 0; s < MAX_MOVERS; s++){                        // normal vehicles first, so the EVs are drawn on top
        if(w.isSlotUsed(s) && !w.isSlotEV(s)){
            drawLiveVehicle(target, font, fontOk, w, view, s, seconds);
        }
    }
    for(int s = 0; s < MAX_MOVERS; s++){
        if(w.isSlotUsed(s) && w.isSlotEV(s)){
            drawLiveVehicle(target, font, fontOk, w, view, s, seconds);
        }
    }
}

void drawLiveLegend(sf::RenderTarget &target, sf::Font &font, bool fontOk){
    float x = 16.f;
    float y = (float)WINDOW_H - 26.f;
    sf::RectangleShape c1(sf::Vector2f(12.f, 6.5f));
    c1.setPosition(x, y);
    c1.setFillColor(sf::Color(150, 172, 205));
    target.draw(c1);
    drawText(target, font, fontOk, "normal", 10, x + 16.f, y - 3.f, sf::Color(170, 175, 190));

    sf::RectangleShape c2(sf::Vector2f(12.f, 6.5f));
    c2.setPosition(x + 70.f, y);
    c2.setFillColor(sf::Color(255, 190, 80));
    target.draw(c2);
    drawText(target, font, fontOk, "shoulder (EV near)", 10, x + 86.f, y - 3.f, sf::Color(170, 175, 190));

    sf::RectangleShape c3(sf::Vector2f(12.f, 6.5f));
    c3.setPosition(x + 200.f, y);
    c3.setFillColor(sf::Color(255, 190, 80));
    c3.setOutlineColor(sf::Color(235, 70, 70));
    c3.setOutlineThickness(1.5f);
    target.draw(c3);
    drawText(target, font, fontOk, "frozen (EV opposite)", 10, x + 216.f, y - 3.f, sf::Color(170, 175, 190));

    drawEVBody(target, x + 372.f, y + 3.f, 0.f, PRIORITY_AMBULANCE, true);
    drawText(target, font, fontOk, "1 Ambulance", 10, x + 390.f, y - 3.f, sf::Color(170, 175, 190));
    drawEVBody(target, x + 484.f, y + 3.f, 0.f, PRIORITY_FIRE_TRUCK, true);
    drawText(target, font, fontOk, "2 Fire Truck", 10, x + 506.f, y - 3.f, sf::Color(170, 175, 190));
    drawEVBody(target, x + 600.f, y + 3.f, 0.f, PRIORITY_POLICE, true);
    drawText(target, font, fontOk, "3 Police", 10, x + 618.f, y - 3.f, sf::Color(170, 175, 190));
}

void drawLiveSidebar(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view){
    sf::RectangleShape panel(sf::Vector2f((float)WINDOW_W - SIDEBAR_X, (float)WINDOW_H));
    panel.setPosition(SIDEBAR_X, 0.f);
    panel.setFillColor(sf::Color(16, 18, 26));
    panel.setOutlineColor(sf::Color(60, 130, 220));
    panel.setOutlineThickness(2.f);
    target.draw(panel);

    if(!fontOk){
        return;
    }
    float x = SIDEBAR_X + 14.f;
    float y = 10.f;
    sf::Color head(120, 200, 255);
    sf::Color normal(225, 228, 235);

    drawText(target, font, fontOk, "LIVE TRAFFIC", 17, x, y, head);
    drawText(target, font, fontOk, "[Tab] lab view", 10, x + 190.f, y + 6.f, sf::Color(130, 140, 160));
    y = 36.f;
    drawText(target, font, fontOk, "Vehicles on the road: " + to_string(w.getAliveCount()) + "   EVs: " + to_string(w.getEVCount()), 12, x, y, normal);
    y = y + 16.f;
    drawText(target, font, fontOk, "created " + to_string(w.getCreated()) + "   deleted " + to_string(w.getDeleted()) + "   (slots: " + to_string(MAX_MOVERS) + ")", 11, x, y, sf::Color(170, 178, 195));
    y = y + 16.f;
    drawText(target, font, fontOk, "arrived: normal " + to_string(w.getArrivedNormal()) + "   EV " + to_string(w.getArrivedEV()), 12, x, y, normal);
    y = y + 16.f;
    drawText(target, font, fontOk, "conflicts " + to_string(w.getConflicts()) + "   yields " + to_string(w.getYields())
             + "   box waits " + to_string(w.getClearanceWaits()), 12, x, y, normal);
    y = y + 16.f;
    drawText(target, font, fontOk, "frozen vehicle-ticks " + to_string(w.getFrozenTicks()) + "   reroutes " + to_string(w.getReroutes()), 12, x, y, normal);
    y = y + 16.f;

    string startText = "-";
    string destText = "-";
    if(view.startJ != 0) startText = "J" + to_string(view.startJ);
    if(view.destJ != 0) destText = "J" + to_string(view.destJ);
    drawText(target, font, fontOk, "Start " + startText + "   Destination " + destText + "   Heavy traffic " + (view.heavyTraffic ? "ON" : "off"), 12, x, y, normal);
    y = y + 16.f;
    string speedText = "Speed " + to_string((int)(view.speed * 100.f)) + "%   tick " + to_string((int)w.getTick());
    if(view.paused){
        speedText = speedText + "   [PAUSED]";
    }
    drawText(target, font, fontOk, speedText, 12, x, y, view.paused ? sf::Color(255, 210, 90) : normal);
    y = y + 16.f;
    if(view.checkText != ""){
        drawText(target, font, fontOk, view.checkText, 12, x, y, sf::Color(150, 235, 150));
    }

    y = 188.f;
    drawText(target, font, fontOk, "Controls", 13, x, y, head);
    y = y + 18.f;
    const char* controls[6] = {
        "Left click START  /  Right click DESTINATION",
        "Press U for User Defined Input Mode",
        "D      demo scenes: sub-menu 1 - 7",
        "K self-check   T pause   + / -  speed",
        "R reset   Tab lab view   ESC quit",
        "Road number = travel time in seconds"
    };
    for(int i = 0; i < 6; i++){
        drawText(target, font, fontOk, controls[i], 11, x, y, sf::Color(185, 190, 202));
        y = y + 14.f;
    }

    y = 330.f;
    drawText(target, font, fontOk, "Event log (newest first)", 13, x, y, head);
    y = y + 20.f;
    for(int k = 0; k < ROAD_LOG_SIZE; k++){
        string msg = w.getRecentEvent(k);
        if(msg == ""){
            break;
        }
        string lines[3];
        int count = wrapText(msg, 50, lines, 3);
        sf::Color c = tagColor(msg);
        if(k > 0){
            int fade = 240 - k * 14;
            if(fade < 90) fade = 90;
            c.a = (sf::Uint8)fade;
        }
        for(int i = 0; i < count; i++){
            if(y > (float)WINDOW_H - 14.f){
                return;
            }
            string prefix = "";
            if(i > 0){
                prefix = "   ";
            }
            drawText(target, font, fontOk, prefix + lines[i], 11, x, y, c);
            y = y + 13.f;
        }
        y = y + 2.f;
    }
}

void drawDemoMenu(sf::RenderTarget &target, sf::Font &font, bool fontOk){
    float x = 40.f;
    float y = 592.f;
    sf::RectangleShape box(sf::Vector2f(880.f, 96.f));
    box.setPosition(x, y);
    box.setFillColor(sf::Color(14, 18, 28, 235));
    box.setOutlineColor(sf::Color(255, 220, 60));
    box.setOutlineThickness(2.f);
    target.draw(box);
    sf::Color normal(230, 235, 245);
    drawText(target, font, fontOk, "DEMO SCENES - press a key:", 12, x + 12.f, y + 3.f, sf::Color(255, 220, 60));
    drawText(target, font, fontOk, "[1] Multi-Tier Priority Sorting (Ambulance, Fire, Police)", 11, x + 12.f, y + 19.f, normal);
    drawText(target, font, fontOk, "[2] Stable Queue Resolution: Priority 1 (Arrival Time)   [3] Stable Queue Resolution: Priority 2 (Arrival Time)", 11, x + 12.f, y + 34.f, normal);
    drawText(target, font, fontOk, "[4] Stable Queue Resolution: Priority 3 (Arrival Time)   [5] Advanced Multi-Lane Stress Test (4 EVs)", 11, x + 12.f, y + 49.f, normal);
    drawText(target, font, fontOk, "[6] Head-On Road Conflict (Ambulances J2 <-> J4, Same Road)     [7] Simultaneous Junction Arrival & Tie-Breaker", 11, x + 12.f, y + 64.f, normal);
    drawText(target, font, fontOk, "[ESC] Return to Main Simulation", 11, x + 12.f, y + 79.f, normal);
}

void drawUserMenu(sf::RenderTarget &target, sf::Font &font, bool fontOk, LiveView &view){
    float x = 40.f;
    float y = 592.f;
    sf::RectangleShape box(sf::Vector2f(880.f, 96.f));
    box.setPosition(x, y);
    box.setFillColor(sf::Color(14, 18, 28, 235));
    box.setOutlineColor(sf::Color(90, 200, 255));
    box.setOutlineThickness(2.f);
    target.draw(box);
    sf::Color normal(230, 235, 245);
    sf::Color active(255, 220, 60);
    drawText(target, font, fontOk, "USER INPUT MODE - Select an action:", 12, x + 12.f, y + 3.f, sf::Color(90, 200, 255));
    drawText(target, font, fontOk, "[1] Spawn Custom Vehicle (L-click Start, R-click Dest, then press 1/2/3 for EV, 9 for normal, 0 for Opposing Traffic)", 11, x + 12.f, y + 19.f, view.uiState == UI_SINGLE ? active : normal);
    drawText(target, font, fontOk, "[2] Batch Spawn Heavy Traffic (L-click Start, R-click Dest to spawn 5 normal vehicles)", 11, x + 12.f, y + 34.f, view.uiState == UI_BATCH ? active : normal);
    drawText(target, font, fontOk, "[3] Toggle Road Closure (L-click Node A, R-click Node B to open/close road)", 11, x + 12.f, y + 49.f, view.uiState == UI_ROAD ? active : normal);
    drawText(target, font, fontOk, "[4] Toggle Heavy Background Traffic On/Off", 11, x + 12.f, y + 64.f, view.heavyTraffic ? active : normal);
    if(view.uiState == UI_USER_MENU){
        drawText(target, font, fontOk, "[ESC] Return to Main Simulation", 11, x + 12.f, y + 79.f, normal);
    }
    else{
        drawText(target, font, fontOk, "[ESC] Back to this menu   (the yellow action is active)", 11, x + 12.f, y + 79.f, normal);
    }
}

void drawLiveFrame(sf::RenderTarget &target, sf::Font &font, bool fontOk, RoadSim &w, LiveView &view, float seconds){
    target.clear(sf::Color(22, 26, 36));

    drawLiveRoads(target, font, fontOk, w);
    drawLiveRoutes(target, w);
    drawLiveJunctions(target, font, fontOk, w, view, seconds);
    drawLiveVehicles(target, font, fontOk, w, view, seconds);
    drawLiveLegend(target, font, fontOk);
    drawLiveSidebar(target, font, fontOk, w, view);
    if(view.demoMenuOpen){
        drawDemoMenu(target, font, fontOk);
    }
    if(view.uiState != UI_MAIN){
        drawUserMenu(target, font, fontOk, view);
    }
}
