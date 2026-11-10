#include "../include/Renderer.hpp"

void drawText(sf::RenderTarget &target, sf::Font &font, bool fontOk, string str, unsigned size, float x, float y, sf::Color color){
    if(!fontOk){
        return;                     // no font found: the shapes alone still show the state
    }
    sf::Text text;
    text.setFont(font);
    text.setString(str);
    text.setCharacterSize(size);
    text.setFillColor(color);
    text.setPosition(x, y);
    target.draw(text);
}

void drawLine(sf::RenderTarget &target, float ax, float ay, float bx, float by, float thickness, sf::Color color){
    float dx = bx - ax;
    float dy = by - ay;
    float length = sqrt(dx * dx + dy * dy);
    float angle = atan2(dy, dx) * 180.f / 3.14159265f;

    sf::RectangleShape line(sf::Vector2f(length, thickness));
    line.setOrigin(0.f, thickness / 2.f);       // turn around the middle of the left end
    line.setPosition(ax, ay);
    line.setRotation(angle);
    line.setFillColor(color);
    target.draw(line);
}

void drawDashedLine(sf::RenderTarget &target, float ax, float ay, float bx, float by, float dash, float gap, float thickness, sf::Color color){
    float dx = bx - ax;
    float dy = by - ay;
    float length = sqrt(dx * dx + dy * dy);
    if(length < 1.f){
        return;
    }
    float ux = dx / length;                     // unit vector along the line
    float uy = dy / length;
    int count = (int)(length / (dash + gap));
    for(int i = 0; i < count; i++){
        float t = i * (dash + gap);
        drawLine(target, ax + ux * t, ay + uy * t, ax + ux * (t + dash), ay + uy * (t + dash), thickness, color);
    }
}

int wrapText(string text, int width, string lines[], int maxLines){
    int count = 0;
    while(text.length() > 0 && count < maxLines){
        if((int)text.length() <= width){
            lines[count] = text;
            count++;
            break;
        }
        int cut = width;
        while(cut > 0 && text[cut] != ' '){     // look backwards for a space
            cut--;
        }
        if(cut == 0){
            cut = width;                        // no space at all: cut in the middle of a word
        }
        lines[count] = text.substr(0, cut);
        count++;
        text = text.substr(cut);
        while(text.length() > 0 && text[0] == ' '){
            text = text.substr(1);
        }
    }
    return count;
}

sf::Color tagColor(string msg){
    if(msg.find("[SIGNAL]") == 0)     return sf::Color(110, 230, 130);
    if(msg.find("[EVACUATION]") == 0) return sf::Color(255, 175, 70);
    if(msg.find("[BYPASS]") == 0)     return sf::Color(110, 210, 240);
    if(msg.find("[ARRIVED]") == 0)    return sf::Color(150, 235, 150);
    if(msg.find("[REROUTE]") == 0)    return sf::Color(255, 210, 90);
    if(msg.find("[ROAD]") == 0)       return sf::Color(205, 165, 255);
    if(msg.find("[ERROR]") == 0 || msg.find("[BLOCKED]") == 0 || msg.find("[STOPPED]") == 0 || msg.find("[NO ROUTE]") == 0){
        return sf::Color(245, 115, 115);
    }
    if(msg.find("[DISPATCH]") == 0 || msg.find("[ROUTE]") == 0 || msg.find("[SPAWN]") == 0){
        return sf::Color(150, 190, 255);
    }
    return sf::Color(195, 200, 210);
}

void drawRoads(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim){
    for(int a = 1; a <= NUM_JUNCTIONS; a++){
        for(int b = a + 1; b <= NUM_JUNCTIONS; b++){
            if(!sim.hasRoad(a, b)){
                continue;
            }
            float ax = JX[a], ay = JY[a], bx = JX[b], by = JY[b];
            drawLine(target, ax, ay, bx, by, 24.f, sf::Color(70, 76, 92));                     // the road (two lanes)
            float len = sqrt((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
            float nx = -(by - ay) / len;                                                       // sideways direction
            float ny = (bx - ax) / len;
            drawLine(target, ax + nx * 11.f, ay + ny * 11.f, bx + nx * 11.f, by + ny * 11.f, 1.5f, sf::Color(200, 200, 200, 110));   // lane edge
            drawLine(target, ax - nx * 11.f, ay - ny * 11.f, bx - nx * 11.f, by - ny * 11.f, 1.5f, sf::Color(200, 200, 200, 110));   // lane edge
            drawDashedLine(target, ax, ay, bx, by, 12.f, 10.f, 2.f, sf::Color(235, 225, 120, 190));   // line between the two lanes
            drawText(target, font, fontOk, to_string(sim.roadLength(a, b)), 12, (ax + bx) / 2.f - 5.f, (ay + by) / 2.f - 22.f, sf::Color(170, 175, 190));
        }
    }

    // closed roads: red dashed line with a cross
    for(int k = 0; k < START_ROADS; k++){
        int a = ROAD_A[k];
        int b = ROAD_B[k];
        if(sim.hasRoad(a, b)){
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

void drawRoute(sf::RenderTarget &target, Simulator &sim, ViewModel &vm){
    sf::Color orange(255, 140, 0, 210);
    if(vm.state == VM_TRAVEL){
        drawLine(target, JX[vm.travelFrom], JY[vm.travelFrom], JX[vm.travelTo], JY[vm.travelTo], 6.f, orange);
    }
    int i = 0;
    while(sim.getRouteJunction(i) != 0 && sim.getRouteJunction(i + 1) != 0){
        int a = sim.getRouteJunction(i);
        int b = sim.getRouteJunction(i + 1);
        drawLine(target, JX[a], JY[a], JX[b], JY[b], 6.f, orange);
        i++;
    }
}

void drawSignal(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, int j, float x, float y){
    bool hold = sim.isPreempted(j);

    sf::RectangleShape box(sf::Vector2f(40.f, 38.f));
    box.setPosition(x, y);
    box.setFillColor(sf::Color(15, 17, 22));
    box.setOutlineColor(hold ? sf::Color(110, 230, 130) : sf::Color(70, 74, 86));
    box.setOutlineThickness(hold ? 2.f : 1.f);
    target.draw(box);

    sf::Color redOn(235, 60, 60), redOff(70, 25, 25);
    sf::Color greenOn(80, 230, 100), greenOff(25, 70, 30);

    for(int head = 0; head < 2; head++){
        float cx = x + 6.f + head * 18.f;
        bool green;
        if(head == 0){
            green = hold;               // the EV head
        }
        else{
            green = !hold;              // the NORMAL head
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

void drawEVBody(sf::RenderTarget &target, float x, float y, float angle, int priority, bool flash){
    float rad = angle * 3.14159265f / 180.f;
    float fx = cos(rad);                    // unit vector: the way the vehicle faces
    float fy = sin(rad);
    float rx = -fy;                         // unit vector: to the RIGHT of it (screen: y goes down)
    float ry = fx;

    float length = 27.f;
    float width = 10.f;
    if(priority == PRIORITY_FIRE_TRUCK){
        length = 35.f;                      // the Fire Truck is longer and wider
        width = 13.f;
    }

    sf::Color fill(248, 248, 248);          // Ambulance: white
    sf::Color edge(55, 55, 60);
    if(priority == PRIORITY_POLICE){
        fill = sf::Color(28, 48, 125);      // Police: dark blue
        edge = sf::Color(8, 8, 12);
    }
    if(priority == PRIORITY_FIRE_TRUCK){
        fill = sf::Color(214, 28, 28);      // Fire Truck: solid red
        edge = sf::Color(105, 8, 8);
    }

    sf::RectangleShape body(sf::Vector2f(length, width));
    body.setOrigin(length / 2.f, width / 2.f);
    body.setPosition(x, y);
    body.setRotation(angle);
    body.setFillColor(fill);
    body.setOutlineColor(edge);
    body.setOutlineThickness(1.3f);
    target.draw(body);

    if(priority == PRIORITY_AMBULANCE){
        // two red stripes along the sides and a red cross in the middle
        for(int side = -1; side <= 1; side += 2){
            sf::RectangleShape stripe(sf::Vector2f(length - 4.f, 1.6f));
            stripe.setOrigin((length - 4.f) / 2.f, 0.8f);
            stripe.setPosition(x + rx * side * 3.4f, y + ry * side * 3.4f);
            stripe.setRotation(angle);
            stripe.setFillColor(sf::Color(220, 40, 40));
            target.draw(stripe);
        }
        sf::RectangleShape barA(sf::Vector2f(8.f, 2.8f));       // the cross: two bars
        barA.setOrigin(4.f, 1.4f);
        barA.setPosition(x, y);
        barA.setRotation(angle);
        barA.setFillColor(sf::Color(225, 30, 30));
        target.draw(barA);
        sf::RectangleShape barB(sf::Vector2f(2.8f, 8.f));
        barB.setOrigin(1.4f, 4.f);
        barB.setPosition(x, y);
        barB.setRotation(angle);
        barB.setFillColor(sf::Color(225, 30, 30));
        target.draw(barB);
    }
    else if(priority == PRIORITY_POLICE){
        sf::RectangleShape stripe(sf::Vector2f(length - 3.f, 3.4f));    // a black stripe along the middle
        stripe.setOrigin((length - 3.f) / 2.f, 1.7f);
        stripe.setPosition(x, y);
        stripe.setRotation(angle);
        stripe.setFillColor(sf::Color(12, 12, 16));
        target.draw(stripe);
    }

    // two flashing lights; where they sit and which colours they have depends on the kind
    float along = 0.f;                      // distance from the middle toward the front
    float aside = 3.2f;                     // distance from the middle to the left / right
    sf::Color a1, a2;                       // the colour of the right / left light in the first and in the second phase
    sf::Color b1, b2;
    if(priority == PRIORITY_AMBULANCE){
        along = -2.f;
        a1 = sf::Color(255, 50, 50);  b1 = sf::Color(255, 255, 255);
        a2 = sf::Color(255, 255, 255); b2 = sf::Color(255, 50, 50);
    }
    else if(priority == PRIORITY_POLICE){
        along = 1.f;
        a1 = sf::Color(255, 40, 40);  b1 = sf::Color(60, 100, 255);
        a2 = sf::Color(60, 100, 255); b2 = sf::Color(255, 40, 40);
    }
    else{
        along = length * 0.28f;
        aside = 4.2f;
        a1 = sf::Color(255, 225, 40); b1 = sf::Color(255, 255, 255);
        a2 = sf::Color(255, 255, 255); b2 = sf::Color(255, 225, 40);
    }
    sf::CircleShape lightA(2.8f);
    lightA.setOrigin(2.8f, 2.8f);
    lightA.setPosition(x + fx * along + rx * aside, y + fy * along + ry * aside);
    lightA.setFillColor(flash ? a1 : a2);
    target.draw(lightA);
    sf::CircleShape lightB(2.8f);
    lightB.setOrigin(2.8f, 2.8f);
    lightB.setPosition(x + fx * along - rx * aside, y + fy * along - ry * aside);
    lightB.setFillColor(flash ? b1 : b2);
    target.draw(lightB);
}

void drawJunctions(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, float seconds){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        float x = JX[j];
        float y = JY[j];

        // green pulsing ring while the signal is held for an emergency vehicle
        if(sim.isPreempted(j)){
            float r = 31.f + 3.f * sin(seconds * 6.f);
            sf::CircleShape ring(r);
            ring.setOrigin(r, r);
            ring.setPosition(x, y);
            ring.setFillColor(sf::Color(0, 0, 0, 0));
            ring.setOutlineColor(sf::Color(110, 230, 130));
            ring.setOutlineThickness(3.f);
            target.draw(ring);
        }

        sf::CircleShape circle(22.f);
        circle.setOrigin(22.f, 22.f);
        circle.setPosition(x, y);
        circle.setFillColor(sf::Color(50, 130, 230));
        circle.setOutlineColor(sf::Color(210, 225, 245));
        circle.setOutlineThickness(2.f);
        if(j == vm.startJ){
            circle.setOutlineColor(sf::Color(90, 240, 120));    // selected start: green outline
            circle.setOutlineThickness(4.f);
        }
        if(j == vm.destJ){
            circle.setOutlineColor(sf::Color(255, 220, 60));    // selected destination: yellow outline
            circle.setOutlineThickness(4.f);
        }
        target.draw(circle);

        drawText(target, font, fontOk, "J" + to_string(j), 16, x - 11.f, y - 11.f, sf::Color::White);
        if(j == vm.startJ){
            drawText(target, font, fontOk, "START", 12, x - 20.f, y - 42.f, sf::Color(90, 240, 120));
        }
        if(j == vm.destJ){
            drawText(target, font, fontOk, "DESTINATION", 12, x - 38.f, y - 42.f, sf::Color(255, 220, 60));
        }
        drawSignal(target, font, fontOk, sim, j, x + 28.f, y - 17.f);
    }
}

void drawLaneRow(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, int j, bool sideRow, float x, float y){
    int length;
    if(sideRow){
        length = sim.getSideLength(j);
    }
    else{
        length = sim.getLaneLength(j);
    }

    int drawn = 0;
    int hidden = 0;
    for(int i = 0; i < length; i++){
        int id;
        if(sideRow){
            id = sim.getSideVehicle(j, i);
        }
        else{
            id = sim.getLaneVehicle(j, i);
        }
        if(vm.state == VM_TRAVEL && id == vm.travelVeh){
            continue;
        }
        if(drawn >= BOXES_SHOWN){
            hidden++;
            continue;
        }

        sf::RectangleShape box(sf::Vector2f(14.f, 14.f));
        box.setPosition(x + drawn * 17.f, y);
        if(sim.isEmergencyVehicle(id)){
            int kind = sim.getVehiclePriority(id);              // the same colours as on the road
            sf::Color fill(248, 248, 248);                      // Ambulance: white
            if(kind == PRIORITY_POLICE) fill = sf::Color(28, 48, 125);
            if(kind == PRIORITY_FIRE_TRUCK) fill = sf::Color(214, 28, 28);
            box.setFillColor(fill);
            box.setOutlineColor(sf::Color(255, 225, 80));       // yellow edge: an emergency vehicle
            box.setOutlineThickness(2.f);
        }
        else if(sideRow){
            box.setFillColor(sf::Color(255, 170, 60));          // pulled over: orange
        }
        else{
            box.setFillColor(sf::Color(150, 172, 205));         // normal vehicle: grey-blue
        }
        target.draw(box);
        drawn++;
    }
    if(hidden > 0){
        drawText(target, font, fontOk, "+" + to_string(hidden), 11, x + BOXES_SHOWN * 17.f + 2.f, y, sf::Color(220, 220, 230));
    }
}

void drawLanePanels(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        float x = PX[j];
        float y = PY[j];

        // the outline of the panel shows how busy the junction is
        string level = sim.getCongestion(j);          // asks the simulator, like everything else here
        sf::Color edge(60, 120, 80);
        if(level == "MODERATE"){
            edge = sf::Color(210, 175, 60);
        }
        if(level == "HEAVY"){
            edge = sf::Color(215, 70, 70);
        }

        sf::RectangleShape panel(sf::Vector2f(178.f, 46.f));
        panel.setPosition(x - 2.f, y - 1.f);
        panel.setFillColor(sf::Color(20, 22, 30, 215));
        panel.setOutlineColor(edge);
        panel.setOutlineThickness(1.5f);
        target.draw(panel);

        drawText(target, font, fontOk, "main", 10, x + 3.f, y + 2.f, sf::Color(160, 165, 180));
        drawText(target, font, fontOk, "side", 10, x + 3.f, y + 24.f, sf::Color(160, 165, 180));
        drawLaneRow(target, font, fontOk, sim, vm, j, false, x + 40.f, y + 4.f);
        drawLaneRow(target, font, fontOk, sim, vm, j, true, x + 40.f, y + 26.f);

        if(sim.isPreempted(j)){
            // the text goes on the side of the panel that is AWAY from the junction circle
            float textY = y + 47.f;
            if(PY[j] < JY[j]){
                textY = y - 16.f;       // this panel is above its junction, so the text goes above the panel
            }
            drawText(target, font, fontOk, "EV GREEN - cross-traffic paused", 11, x, textY, sf::Color(245, 115, 115));
        }
    }
}

void drawEmergencyVehicle(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, float seconds){
    float x = 0.f;
    float y = 0.f;
    int id = -1;
    bool show = false;

    if(vm.state == VM_TRAVEL){
        x = JX[vm.travelFrom] + (JX[vm.travelTo] - JX[vm.travelFrom]) * vm.travelT;     // a point between the two junctions
        y = JY[vm.travelFrom] + (JY[vm.travelTo] - JY[vm.travelFrom]) * vm.travelT;
        id = vm.travelVeh;
        show = true;
    }
    else if(sim.isDispatchActive() && sim.getCurrentJunction() != 0){
        x = JX[sim.getCurrentJunction()];
        y = JY[sim.getCurrentJunction()];
        id = sim.getActiveVehicleID();
        show = true;
    }
    if(!show){
        return;
    }

    bool phase = ((int)(seconds * 6.f)) % 2 == 0;           // flips six times a second

    int kind = vm.travelPriority;                           // which kind of emergency vehicle is driving
    if(vm.state != VM_TRAVEL){
        kind = sim.getActivePriority();
    }
    float angle = 0.f;
    if(vm.state == VM_TRAVEL){
        angle = atan2(JY[vm.travelTo] - JY[vm.travelFrom], JX[vm.travelTo] - JX[vm.travelFrom]) * 180.f / 3.14159265f;
    }
    drawEVBody(target, x, y, angle, kind, phase);
    drawText(target, font, fontOk, emergencyTypeName(kind) + " " + to_string(id), 11, x - 26.f, y + 25.f, sf::Color(255, 255, 255));    // below the junction circle, so "J2" stays readable
}

void drawSidebar(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm){
    sf::RectangleShape panel(sf::Vector2f((float)WINDOW_W - SIDEBAR_X, (float)WINDOW_H));
    panel.setPosition(SIDEBAR_X, 0.f);
    panel.setFillColor(sf::Color(16, 18, 26));
    panel.setOutlineColor(sf::Color(60, 130, 220));
    panel.setOutlineThickness(2.f);
    target.draw(panel);

    if(!fontOk){
        return;
    }

    float x = SIDEBAR_X + 16.f;
    float y = 12.f;
    sf::Color head(120, 200, 255);
    sf::Color normal(225, 228, 235);

    drawText(target, font, fontOk, "PHASE 3 : SMART CITY", 18, x, y, head);
    y = 42.f;
    drawText(target, font, fontOk, "Vehicles: " + to_string(sim.getFleetCount()) + " / " + to_string(MAX_FLEET)
             + "     Requests waiting: " + to_string(sim.getPendingCount()), 13, x, y, normal);
    y = y + 19.f;

    if(sim.isDispatchActive()){
        drawText(target, font, fontOk, "EV " + to_string(sim.getActiveVehicleID()) + " at J" + to_string(sim.getCurrentJunction())
                 + ", next J" + to_string(sim.getNextJunction()), 13, x, y, sf::Color(255, 150, 90));
    }
    else{
        drawText(target, font, fontOk, "No emergency vehicle on the road", 13, x, y, sf::Color(140, 220, 140));
    }
    y = y + 19.f;
    drawText(target, font, fontOk, "Signal requests " + to_string(sim.getPreemptCount()) + "   Bypasses " + to_string(sim.getBypassCount())
             + "   Pulled over " + to_string(sim.getEvacuatedCount()), 12, x, y, normal);
    y = y + 19.f;

    string startText = "-";
    string destText = "-";
    if(vm.startJ != 0) startText = "J" + to_string(vm.startJ);
    if(vm.destJ != 0) destText = "J" + to_string(vm.destJ);
    drawText(target, font, fontOk, "Start " + startText + "   Destination " + destText, 13, x, y, normal);
    y = y + 19.f;

    string speedText = "Speed " + to_string((int)(vm.speed * 100.f)) + "%";
    if(vm.paused){
        speedText = speedText + "   [PAUSED]";
    }
    drawText(target, font, fontOk, speedText, 12, x, y, vm.paused ? sf::Color(255, 210, 90) : normal);
    y = y + 17.f;
    if(vm.checkText != ""){
        drawText(target, font, fontOk, vm.checkText, 12, x, y, sf::Color(150, 235, 150));
    }

    y = 178.f;
    drawText(target, font, fontOk, "Controls", 14, x, y, head);
    y = y + 20.f;
    const char* controls[11] = {
        "Left click   select START junction",
        "Right click  select DESTINATION junction",
        "N       normal vehicle at START",
        "1 2 3   Ambulance / Fire Truck / Police START -> DEST",
        "SPACE   dispatch the next emergency vehicle",
        "M       move a normal vehicle START -> DEST",
        "L       a normal vehicle leaves at START",
        "C       close / open the road START - DEST",
        "B       build a demo scene       K  self-check",
        "T pause    + / -  speed",
        "R reset    ESC quit"
    };
    for(int i = 0; i < 11; i++){
        drawText(target, font, fontOk, controls[i], 11, x, y, sf::Color(185, 190, 202));
        y = y + 15.f;
    }

    y = 378.f;
    drawText(target, font, fontOk, "Event log (newest first)", 14, x, y, head);
    y = y + 22.f;
    for(int k = 0; k < LOG_SIZE; k++){
        string msg = sim.getRecentEvent(k);
        if(msg == ""){
            break;
        }
        string lines[3];
        int count = wrapText(msg, 46, lines, 3);
        sf::Color c = tagColor(msg);
        if(k > 0){
            c.a = (sf::Uint8)(235 - k * 18);                    // older events fade
        }
        for(int i = 0; i < count; i++){
            if(y > (float)WINDOW_H - 18.f){
                return;
            }
            string prefix = "";
            if(i > 0){
                prefix = "   ";
            }
            drawText(target, font, fontOk, prefix + lines[i], 12, x, y, c);
            y = y + 15.f;
        }
        y = y + 3.f;
    }
}

void drawFrame(sf::RenderTarget &target, sf::Font &font, bool fontOk, Simulator &sim, ViewModel &vm, float seconds){
    target.clear(sf::Color(22, 26, 36));

    drawRoads(target, font, fontOk, sim);
    drawRoute(target, sim, vm);
    drawJunctions(target, font, fontOk, sim, vm, seconds);
    drawLanePanels(target, font, fontOk, sim, vm);
    drawEmergencyVehicle(target, font, fontOk, sim, vm, seconds);
    drawSidebar(target, font, fontOk, sim, vm);
}

int junctionAt(float x, float y){
    for(int j = 1; j <= NUM_JUNCTIONS; j++){
        float dx = x - JX[j];
        float dy = y - JY[j];
        if(dx * dx + dy * dy <= 30.f * 30.f){
            return j;
        }
    }
    return 0;
}

bool loadFont(sf::Font &font){
    // SFML prints a warning for every path that fails, so the most likely one goes first
    const char* paths[5] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "arial.ttf"                                         // last: a copy next to the program
    };
    for(int i = 0; i < 5; i++){
        if(font.loadFromFile(paths[i])){
            return true;
        }
    }
    return false;
}
