#include "decorations.h"
#include "dungeon.h"
#include "logger.h"

void apply_decoration(Dungeon *dungeon, const IntPair &coords, std::string decoration) {
    Cell *cell = &(dungeon->cells[coords.x][coords.y]);
    cell->type = CELL_TYPE_DECORATION;
    if (cell->decoration_texture) delete cell->decoration_texture;
    cell->decoration_texture = new std::string(decoration);
}

bool apply_scheme_lobby(Dungeon *dungeon, Room *room) {
    unsigned int width = room->x1 - room->x0;
    unsigned int height = room->y1 - room->y0;
    
    unsigned int i;
    // 3x1 DESK
    unsigned int x;
    if (room->x1 - room->x0 > 6) {
        IntPair coords = dungeon->random_area_in_room(room, 3, 1);
        for (x = coords.x; x < (unsigned int) (coords.x + 3); x++) {
            apply_decoration(dungeon, IntPair{(int) x, coords.y}, "decorations_couch_3_" + std::to_string(x + 1 - coords.x));
        }
    }

    // PLANTS
    unsigned int plants = width / 3 + height / 3;
    for (i = 0; i < plants; i++) {
        IntPair coords = dungeon->random_location_along_edge(room);
        apply_decoration(dungeon, coords, "decorations_plant_" + std::to_string(RAND_BETWEEN(1, 2)));
    }

    // 1x1 DESKS
    unsigned int desks = width / 5 + height / 5;
    for (i = 0; i < desks; i++) {
        if (rand() % 2) {
            IntPair coords = dungeon->random_area_in_room(room, 2, 1);
            if (rand() % 2) {
                apply_decoration(dungeon, coords, "decorations_chair_red_l");
                apply_decoration(dungeon, IntPair{coords.x + 1, coords.y}, "decorations_microdesk");
            } else {
                apply_decoration(dungeon, coords, "decorations_microdesk");
                apply_decoration(dungeon, IntPair{coords.x + 1, coords.y}, "decorations_chair_red_r");
            }
        } else {
            IntPair coords = dungeon->random_area_in_room(room, 1, 2);
            if (rand() % 2) {
                apply_decoration(dungeon, coords, "decorations_chair_red_t");
                apply_decoration(dungeon, IntPair{coords.x, coords.y + 1}, "decorations_microdesk");
            } else {
                apply_decoration(dungeon, coords, "decorations_microdesk");
                apply_decoration(dungeon, IntPair{coords.x, coords.y + 1}, "decorations_chair_red_b");
            }
        }
    }
    return true;
}

bool apply_scheme_server_room(Dungeon *dungeon, Room *room) {
    unsigned int width = room->x1 - room->x0;
    unsigned int height = room->y1 - room->y0;
    
    unsigned int i;
    // Goal is to fill up the room with as many 3x1 server racks as possible.
    // If it's a smaller room, we'll just scatter 1x1s.
    if (width < 5 || height < 3) {
        // Scatter
        unsigned int racks = width / 2 + height / 2;
        for (i = 0; i < racks; i++) {
            IntPair coords = dungeon->random_location_in_room(room);
            apply_decoration(dungeon, coords, "decorations_server_rack_1x1");
        }
    } else {
        // Start at the top left and just make rows
        unsigned int x, y;
        x = room->x0 + 1;
        y = room->y0 + 1;

        for (x = room->x0 + 1; x < (unsigned int) (room->x1 - 2); x += 4) {
            for (y = room->y0 + 1; y < (unsigned int) (room->y1); y += 2) {
                // These are guaranteed to have no obstructions yet, so we won't check.
                // Bad if generation logic ever changes though.
                apply_decoration(dungeon, IntPair{(int) x, (int) y}, "decorations_server_rack_3_1");
                apply_decoration(dungeon, IntPair{(int) x + 1, (int) y}, "decorations_server_rack_3_2");
                apply_decoration(dungeon, IntPair{(int) x + 2, (int) y}, "decorations_server_rack_3_3");
            }
        }
    }
    return true;
}

bool apply_scheme_computer_room(Dungeon *dungeon, Room *room) {
    unsigned int width = room->x1 - room->x0;
    unsigned int height = room->y1 - room->y0;
    
    unsigned int i;
    unsigned int racks = width / 3 + height / 3;
    for (i = 0; i < racks; i++) {
        IntPair coords = dungeon->random_location_in_room(room);
        apply_decoration(dungeon, coords, "decorations_server_rack_1x1");
    }
    unsigned int computers = width / 3 + height / 3;
    for (i = 0; i < computers; i++) {
        IntPair coords = dungeon->random_location_in_room(room);
        apply_decoration(dungeon, coords, "decorations_computer_" + std::string(rand() % 2 ? "v" : "h"));
    }
    unsigned int plants = width / 4 + height / 4;
    for (i = 0; i < plants; i++) {
        IntPair coords = dungeon->random_location_along_edge(room);
        apply_decoration(dungeon, coords, "decorations_plant_" + std::to_string(RAND_BETWEEN(1, 2)));
    }
    return true;
}

bool apply_scheme_conference_room(Dungeon *dungeon, Room *room) {
    unsigned int width = room->x1 - room->x0;
    unsigned int height = room->y1 - room->y0;
    
    if (width < 4 || height < 5) return false;

    unsigned int x_center = room->x0 + (width - 2) / 2;
    unsigned int y_center = room->y0 + (height - 3) / 2; 

    unsigned int x, y;
    unsigned int i = 1;
    for (y = y_center; y < y_center + 3; y++) {
        for (x = x_center; x < x_center + 2; x++) {
            apply_decoration(dungeon, IntPair{(int) x, (int) y}, "decorations_conference_table_" + std::to_string(i++));
        }
    }

    unsigned int plants = width / 3 + height / 3;
    for (i = 0; i < plants; i++) {
        IntPair coords = dungeon->random_location_along_edge(room);
        apply_decoration(dungeon, coords, "decorations_plant_" + std::to_string(RAND_BETWEEN(1, 2)));
    }

    return true;
}

bool apply_scheme_money_room(Dungeon *dungeon, Room *room) {
    unsigned int width = room->x1 - room->x0;
    unsigned int height = room->y1 - room->y0;
    
    if (width < 3 || height < 3) return false;

    // Each money bag is 2x2, so we'll place (approx.) 1 money bag per 3x3 area.
    unsigned int bags = width * height / 9;
    unsigned int i, type;
    for (i = 0; i < bags; i++) {
        try {
            IntPair coords = dungeon->random_area_in_room(room, 2, 2);
            type = rand() % 2;
            apply_decoration(dungeon, coords, "decorations_money_bag_" + std::string(type ? "open_" : "") + "1");
            apply_decoration(dungeon, IntPair{coords.x + 1, coords.y}, "decorations_money_bag_" + std::string(type ? "open_" : "") + "2");
            apply_decoration(dungeon, IntPair{coords.x, coords.y + 1}, "decorations_money_bag_" + std::string(type ? "open_" : "") + "3");
            apply_decoration(dungeon, IntPair{coords.x + 1, coords.y + 1}, "decorations_money_bag_" + std::string(type ? "open_" : "") + "4");
        } catch (dungeon_exception &e) {
            break; // No problem if we can't place them all
        }
    }

    // And some extra money piles for good measure
    for (i = 0; i < bags * 2; i++) {
        try {
            IntPair coords = dungeon->random_location_along_edge(room);
            apply_decoration(dungeon, coords, "decorations_money");
        } catch (dungeon_exception &e) {
            break;
        }
    }

    return true;
}


bool apply_scheme(decoration_scheme_t scheme, Dungeon *dungeon, Room *room) {
    try {
        switch (scheme) {
            case DECORATION_SCHEME_LOBBY:
                return apply_scheme_lobby(dungeon, room);
            case DECORATION_SCHEME_SERVER_ROOM:
                return apply_scheme_server_room(dungeon, room);
            case DECORATION_SCHEME_CONFERENCE_ROOM:
                return apply_scheme_conference_room(dungeon, room);
            case DECORATION_SCHEME_MONEY_ROOM:
                return apply_scheme_money_room(dungeon, room);
            case DECORATION_SCHEME_COMPUTER_ROOM:
                return apply_scheme_computer_room(dungeon, room);
            default:
                throw dungeon_exception(__PRETTY_FUNCTION__, "no mapping for specified scheme");
        }
    } catch (const dungeon_exception &e) {
        Logger::debug(__FILE__, "failure while applying decoration scheme: " + std::string(e.what()));
        return false;
    }
}

decoration_scheme_t parse_scheme(std::string &name) {
    if (name == "lobby") return DECORATION_SCHEME_LOBBY;
    if (name == "server_room") return DECORATION_SCHEME_SERVER_ROOM;
    if (name == "conference_room") return DECORATION_SCHEME_CONFERENCE_ROOM;
    if (name == "money_room") return DECORATION_SCHEME_MONEY_ROOM;
    if (name == "computer_room") return DECORATION_SCHEME_COMPUTER_ROOM;
    throw dungeon_exception(__PRETTY_FUNCTION__, "invalid decoration scheme: " + name);
}