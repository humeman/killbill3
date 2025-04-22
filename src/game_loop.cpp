#include <ncurses.h>
#include <cstring>
#include <cstdlib>

#include "game.h"
#include "item.h"
#include "macros.h"
#include "dungeon.h"
#include "heap.h"
#include "character.h"
#include "ascii.h"
#include "pathfinding.h"
#include "message_queue.h"

#define PRINT_REPEATED(x, y, count, char) { \
    move(y, x); \
    for (int i = 0; i < count; i++) addch(char); }

// This lovely macro uses a very inefficient double printf
// to determine the buffer size, and in turn, the length of the formatted
// string. Pretty awful, but it gives us some nice centered text :)
// The empty snprintf is courtesy of https://stackoverflow.com/a/10388547
#define PRINTW_CENTERED_AT(x, y, msg, ...) { \
    int len = snprintf(NULL, 0, msg, ##__VA_ARGS__); \
    move(y, x - len / 2); \
    printw(msg, ##__VA_ARGS__); }

// special macro that cleans up ncurses stuff
#define ERROR_AND_EXIT(message, ...) { \
    endwin(); \
    fprintf(stderr, "err: " message "\n", ##__VA_ARGS__); \
    goto exit_err; \
}

#define KB_CONTROLS(x, y) \
    if (monster_menu_on) break; \
    if (teleport_mode) \
        move_coords(teleport_pointer, x, y); \
    else { \
        try_move(result, x, y); \
        next_turn_ready = 1; \
    }

#define SET_COLOR(name, fg, bg) if (init_pair(name, fg, bg) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");

#define INVENTORY_BOX_WIDTH 38
#define MONSTER_MENU_WIDTH 40
#define MONSTER_MENU_HEIGHT 10

#define MONSTER_MENU_X_BEGIN WIDTH / 2 - MONSTER_MENU_WIDTH / 2
#define MONSTER_MENU_Y_BEGIN 1

char LOSE[] = ASCII_LOSE;
char WIN[] = ASCII_WIN;
char EQUIP_SLOT_CHARS[] = "abcdefghijkl";
char INVENTORY_SLOT_CHARS[] = "0123456789";

item_type_t EQUIPPABLE_ITEMS[] = {
    ITEM_TYPE_WEAPON,
    ITEM_TYPE_OFFHAND,
    ITEM_TYPE_RANGED,
    ITEM_TYPE_ARMOR,
    ITEM_TYPE_HELMET,
    ITEM_TYPE_CLOAK,
    ITEM_TYPE_GLOVES,
    ITEM_TYPE_BOOTS,
    ITEM_TYPE_AMULET,
    ITEM_TYPE_LIGHT,
    ITEM_TYPE_RING
};

char prompt(std::string options, std::string prompt);

void game_t::run() {
    try {
        initscr();
        if (COLS < WIDTH || LINES < HEIGHT)
            throw dungeon_exception(__PRETTY_FUNCTION__, "terminal size is too small, minimum is "
                + std::to_string(WIDTH) + "x" + std::to_string(HEIGHT) + " (yours is " + std::to_string(COLS) + "x" + std::to_string(LINES) + ")");
        if (start_color() != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (no color support)");
        SET_COLOR(COLORS_FLOOR, COLOR_BLUE, COLOR_BLACK);
        SET_COLOR(COLORS_PC, COLOR_WHITE, COLOR_BLACK);
        SET_COLOR(COLORS_MONSTER, COLOR_RED, COLOR_BLACK);
        SET_COLOR(COLORS_OBJECT, COLOR_MAGENTA, COLOR_BLACK);
        SET_COLOR(COLORS_STONE, COLOR_BLACK, COLOR_BLACK);
        SET_COLOR(COLORS_TEXT, COLOR_WHITE, COLOR_BLACK);
        SET_COLOR(COLORS_TEXT_RED, COLOR_RED, COLOR_BLACK);
        SET_COLOR(COLORS_MENU_TEXT, COLOR_BLACK, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_TEXT_SELECTED, COLOR_WHITE, COLOR_BLACK);
        SET_COLOR(COLORS_FOG_OF_WAR_TERRAIN, COLOR_WHITE, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY, COLOR_RED, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY + 1, COLOR_GREEN, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY + 2, COLOR_YELLOW, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY + 3, COLOR_BLUE, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY + 4, COLOR_MAGENTA, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY + 5, COLOR_CYAN, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY + 6, COLOR_WHITE, COLOR_BLACK);
        SET_COLOR(COLORS_FLOOR_ANY + 7, COLOR_WHITE, COLOR_BLACK);
        SET_COLOR(COLORS_MENU_ANY, COLOR_RED, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_ANY + 1, COLOR_GREEN, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_ANY + 2, COLOR_YELLOW, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_ANY + 3, COLOR_BLUE, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_ANY + 4, COLOR_MAGENTA, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_ANY + 5, COLOR_CYAN, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_ANY + 6, COLOR_BLACK, COLOR_WHITE);
        SET_COLOR(COLORS_MENU_ANY + 7, COLOR_BLACK, COLOR_WHITE);

        if (keypad(stdscr, TRUE) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (special kb keys)");
                        // man this library is weird
        if (curs_set(0) == ERR) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (disable cursor)");
        if (noecho() == ERR) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (noecho)");

        run_internal();
    } catch (dungeon_exception &e) {
        endwin();
        throw dungeon_exception(__PRETTY_FUNCTION__, e);
    }
    endwin();
}

void game_t::run_internal() {
    if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
    int c, i, j, swap_slot, type;
    bool next_turn_ready;
    uint8_t x, y;
    bool monster_menu_on = false;
    bool hide_fog_of_war = false;
    bool teleport_mode = false;
    bool sticky_message = false;
    int no_action_time = 0;
    coordinates_t teleport_pointer;
    game_result_t result = GAME_RESULT_RUNNING;
    char *ascii;
    char sel;
    item_t *target_item;
    float hp;
    message_queue_t::get()->add("Welcome to the dungeon.");
    message_queue_t::get()->add("Your task is to slay the ruthless &2&bSpongeBob SquarePants&r,");
    message_queue_t::get()->add("and free the dungeon's creatures from his terrible grasp.");

    update_fog_of_war();
    while (1) {
        // Print the dungeon
        clear();
        for (y = 0; y < dungeon->height; y++) {
            move(y + 1, 0);
            for (x = 0; x < dungeon->width; x++) {
                // If this is within the sight radius, render it.
                // Also, if FOW is disabled, render it.
                if (hide_fog_of_war || teleport_mode || (x >= pc.x - FOG_OF_WAR_DISTANCE && x <= pc.x + FOG_OF_WAR_DISTANCE
                    && y >= pc.y - FOG_OF_WAR_DISTANCE && y <= pc.y + FOG_OF_WAR_DISTANCE)) {
                    if (teleport_mode && x == teleport_pointer.x && y == teleport_pointer.y) {
                        attrset(COLOR_PAIR(COLORS_PC) | A_BOLD);
                        addch(TELEPORT_POINTER);
                        attroff(COLOR_PAIR(COLORS_PC) | A_BOLD);
                    } else if (character_map[x][y]) {
                        if (character_map[x][y] == &pc) {
                            c = COLORS_PC;
                        } else {
                            c = COLORS_FLOOR_ANY + ((monster_t *) character_map[x][y])->next_color();
                        }
                        attrset(COLOR_PAIR(c) | A_BOLD);
                        addch(character_map[x][y]->display);
                        attroff(COLOR_PAIR(c) | A_BOLD);
                    } else if (item_map[x][y]) {
                        c = COLORS_FLOOR_ANY + item_map[x][y]->next_color();
                        attrset(COLOR_PAIR(c));
                        addch(item_map[x][y]->current_symbol());
                        attroff(COLOR_PAIR(c));
                    } else {
                        c = COLORS_BY_CELL_TYPE[dungeon->cells[x][y].type];
                        attrset(COLOR_PAIR(c));
                        addch(CHARACTERS_BY_CELL_TYPE[dungeon->cells[x][y].type]);
                        attrset(COLOR_PAIR(c));
                    }
                }
                else {
                    // Otherwise, this is outside of the sight of the PC, and should be rendered
                    // using the seen map.
                    attrset(COLOR_PAIR(COLORS_FOG_OF_WAR_TERRAIN) | A_DIM);
                    addch(seen_map[x][y]);
                    attroff(COLOR_PAIR(COLORS_FOG_OF_WAR_TERRAIN) | A_DIM);
                }
            }
        }

        // Print the status message at the top (approx. centered to 80 chars)
        message_queue_t::get()->emit(0, true);

        // And health/inventory at the bottom
        move(HEIGHT - 2, 0);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw("HEALTH: [");
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        // Find the percentage of health left
        hp = (float) pc.hp / pc.base_hp;
        attrset(COLOR_PAIR(COLORS_TEXT_RED));
        for (i = 0; i < WIDTH - 10; i++) {
            if (i < hp * (WIDTH - 10)) addch('#');
            else addch(' ');
        }
        attroff(COLOR_PAIR(COLORS_TEXT_RED));
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw("]");
        // Now the carry slots
        move(HEIGHT - 1, 0);
        printw("INVENTORY: ");
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        j = pc.inventory_size();
        attrset(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        for (i = 1; i < 11; i++) {
            printw("%d  ", i % 10);
        }
        attroff(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw("    ");
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        for (i = 0; i < 12; i++) {
            printw("%c  ", 'a' + i);
        }
        attroff(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        for (i = 0; i < 10; i++) {
            if (i < j) {
                target_item = pc.inventory_at(i);
                c = COLORS_FLOOR_ANY + target_item->next_color();
                attrset(COLOR_PAIR(c));
                mvprintw(HEIGHT - 1, 12 + 3 * i, "%c", target_item->regular_symbol());
                attroff(COLOR_PAIR(c));
            }
        }
        for (i = 0; i < (int) (sizeof (pc.equipment) / sizeof(pc.equipment[0])); i++) {
            target_item = pc.equipment[i];
            if (target_item == NULL) continue;
            c = COLORS_FLOOR_ANY + target_item->next_color();
            attrset(COLOR_PAIR(c));
            mvprintw(HEIGHT - 1, 46 + 3 * i, "%c", target_item->regular_symbol());
            attroff(COLOR_PAIR(c));
        }

        if (result != GAME_RESULT_RUNNING) {
            getch();
            // Print our beautiful images
            clear();
            if (result == GAME_RESULT_LOSE) {
                ascii = LOSE;
                x = WIDTH / 2 - ASCII_LOSE_WIDTH / 2;
                y = HEIGHT / 2 - ASCII_LOSE_HEIGHT / 2;
                PRINTW_CENTERED_AT(WIDTH / 2, 1, "You have failed your task; SpongeBob SquarePants remains unscathed.");
                PRINTW_CENTERED_AT(WIDTH / 2, 2, "As you take your last breath,");
                PRINTW_CENTERED_AT(WIDTH / 2, 3, "you hear his maniacal giggling taunts echo across the dungeon's walls.");
            }
            else {
                ascii = WIN;
                x = WIDTH / 2 - ASCII_WIN_WIDTH / 2;
                y = HEIGHT / 2 - ASCII_WIN_HEIGHT / 2;
                PRINTW_CENTERED_AT(WIDTH / 2, 1, "You have slain the evil SpongeBob SquarePants.");
                PRINTW_CENTERED_AT(WIDTH / 2, 2, "Never again shall the wretched, porous creature torment this dungeon.");
            }
            move(y, x);
            for (i = 0; ascii[i]; i++) {
                if (ascii[i] == '\n') {
                    y++;
                    move(y, x);
                } else {
                    addch(ascii[i]);
                }
            }
            PRINTW_CENTERED_AT(WIDTH / 2, HEIGHT - 1, "(press any key to exit)");
            getch();
            return;
        }

        // Ready to render
        refresh();

        timeout(REDRAW_TIMEOUT);
        c = getch();
        timeout(-1);
        if (c == ERR) {
            // This forces a redraw of everything, removing from the message queue (if it's been a while)
            // and cycling colors.
            no_action_time += REDRAW_TIMEOUT;
            if (no_action_time >= NO_ACTION_TIMEOUT) {
                message_queue_t::get()->drop();
                no_action_time = 0;
            }
            continue;
        }
        if (!sticky_message)
            message_queue_t::get()->drop();
        no_action_time = 0;
        next_turn_ready = 0;
        switch (c) {
            case KB_MONSTERS:
                monster_menu();
                break;
            case KB_INVENTORY:
            case KB_INSPECT_ITEM:
                inventory_menu();
                break;
            case KB_UP_LEFT_0:
            case KB_UP_LEFT_1:
                KB_CONTROLS(-1, -1);
                break;
            case KB_UP_0:
            case KB_UP_1:
                KB_CONTROLS(0, -1);
                break;
            case KB_UP_RIGHT_0:
            case KB_UP_RIGHT_1:
                KB_CONTROLS(1, -1);
                break;
            case KB_RIGHT_0:
            case KB_RIGHT_1:
                KB_CONTROLS(1, 0);
                break;
            case KB_DOWN_RIGHT_0:
            case KB_DOWN_RIGHT_1:
                KB_CONTROLS(1, 1);
                break;
            case KB_DOWN_0:
            case KB_DOWN_1:
                KB_CONTROLS(0, 1);
                break;
            case KB_DOWN_LEFT_0:
            case KB_DOWN_LEFT_1:
                KB_CONTROLS(-1, 1);
                break;
            case KB_LEFT_0:
            case KB_LEFT_1:
                KB_CONTROLS(-1, 0);
                break;
            case KB_REST_0:
            case KB_REST_1:
            case KB_REST_2:
                if (monster_menu_on || teleport_mode) break;
                next_turn_ready = 1;
                break;
            case KB_UP_STAIRS:
                if (monster_menu_on || teleport_mode) break;
                if (dungeon->cells[pc.x][pc.y].type != CELL_TYPE_UP_STAIRCASE) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bThere isn't an up staircase here!");
                    break;
                }
                fill_and_place_on(CELL_TYPE_DOWN_STAIRCASE);
                message_queue_t::get()->add("You went up the stairs.");
                break;
            case KB_DOWN_STAIRS:
                if (monster_menu_on || teleport_mode) break;
                if (dungeon->cells[pc.x][pc.y].type != CELL_TYPE_DOWN_STAIRCASE) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bThere isn't a down staircase here!");
                    break;
                }
                fill_and_place_on(CELL_TYPE_UP_STAIRCASE);
                message_queue_t::get()->add("You went down the stairs.");
                break;
            case KB_TOGGLE_FOG:
                if (monster_menu_on || teleport_mode) break;
                hide_fog_of_war = !hide_fog_of_war;
                break;
            case KB_TELEPORT:
                if (monster_menu_on) break;
                if (!teleport_mode) {
                    teleport_mode = true;
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("-- TELEPORT MODE --");
                    sticky_message = true;
                    teleport_pointer.x = pc.x;
                    teleport_pointer.y = pc.y;
                }
                else {
                    message_queue_t::get()->drop();
                    force_move(result, teleport_pointer);
                    sticky_message = false;
                    teleport_mode = false;
                    next_turn_ready = true;
                }
                break;
            case KB_TELEPORT_RANDOM:
                if (monster_menu_on) break;
                if (!teleport_mode) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bEnter teleport mode first.");
                } else {
                    // It's not noted in the spec, but I don't know why we'd want to
                    // random teleport outside of the dungeon. We'll pick an open space.
                    try {
                        teleport_pointer = random_location_no_kill(dungeon, character_map);
                    }
                    catch (dungeon_exception &e) {
                        // I don't see a reason to crash the game for this.
                        teleport_pointer.x = pc.x;
                        teleport_pointer.y = pc.y;
                    }
                    message_queue_t::get()->clear();
                    force_move(result, teleport_pointer);
                    sticky_message = false;
                    teleport_mode = false;
                    next_turn_ready = true;
                }
                break;
            case KB_PICKUP:
                if (monster_menu_on || teleport_mode) break;
                target_item = item_map[pc.x][pc.y];
                if (target_item == NULL) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bThere's no item here!");
                    break;
                }
                if (pc.inventory_size() >= MAX_CARRY_SLOTS) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bYour carry slots are full!");
                    break;
                }
                if (target_item->is_stacked()) {
                    item_map[pc.x][pc.y] = target_item->detach_stack();
                } else {
                    item_map[pc.x][pc.y] = NULL;
                }
                pc.add_to_inventory(target_item);
                message_queue_t::get()->add("You picked up &" + std::to_string(
                    target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                next_turn_ready = true;
                break;
            case KB_EQUIP:
                if (monster_menu_on || teleport_mode) break;
                render_inventory_box("INVENTORY", "1234567890  ", "Select an item here.", 1, 1);
                c = pc.inventory_size();
                for (i = 0; i < c; i++)
                    render_inventory_item(pc.inventory_at(i), i, false, 1, 1);
                render_inventory_box("EQUIPMENT", EQUIP_SLOT_CHARS, "", 3 + INVENTORY_BOX_WIDTH, 1);
                for (i = 0; i < (int) (sizeof (pc.equipment) / sizeof (pc.equipment[0])); i++)
                    if (pc.equipment[i]) render_inventory_item(pc.equipment[i], i, false, 3 + INVENTORY_BOX_WIDTH, 1);
                sel = prompt("1234567890", "Select an item or press ESC to cancel.");
                if (sel == 0) break; // Cancelled
                // Unfortunate thing here is that I've ordered it 1-9, then 0, to match the keyboard.
                if (sel == '0') i = 9;
                else i = sel - '1';
                if (i >= c) {
                    message_queue_t::get()->clear();
                    std::string err = "&0&bThere's no item in slot ";
                    err += sel;
                    err += ".";
                    message_queue_t::get()->add(err);
                    break;
                }
                target_item = pc.inventory_at(i);
                type = target_item->definition->type;
                // Make sure it's equippable.
                if (type < ITEM_TYPE_WEAPON || type > ITEM_TYPE_RING) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bYou can't equip &" + std::to_string(
                        target_item->current_color()) + escape_col(target_item->definition->name) + "&0&b!");
                    break;
                }
                c = 0;
                swap_slot = type;
                // Prefer the second ring slot if they're both full.
                if (pc.equipment[type] != NULL && type == ITEM_TYPE_RING) {
                    swap_slot = type + 1;
                }
                // Remove that item from the inventory...
                target_item = pc.remove_from_inventory(i);
                if (target_item->is_stacked()) throw dungeon_exception(__PRETTY_FUNCTION__, "invalid state: removed item is stacked");
                // If the swap slot is empty, just add the item there.
                if (pc.equipment[swap_slot] == NULL) {
                    message_queue_t::get()->add("You equipped &" + std::to_string(
                        target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                    pc.equipment[swap_slot] = target_item;
                } else {
                    // Then we have to move that item out to the inventory.
                    // This will reorder them, but it seems way more difficult than it's worth not to.
                    message_queue_t::get()->add("You equipped &"
                            + std::to_string(target_item->current_color())
                            + escape_col(target_item->definition->name) + "&r, swapping out &"
                            + std::to_string(pc.equipment[swap_slot]->current_color())
                            + escape_col(pc.equipment[swap_slot]->definition->name) + "&r.");
                    pc.add_to_inventory(pc.equipment[swap_slot]);
                    pc.equipment[swap_slot] = target_item;
                }
                break;
            case KB_UNEQUIP:
                if (monster_menu_on || teleport_mode) break;
                // Check that there's space
                if (pc.inventory_size() >= 10) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bYou have no carry slots open!");
                    break;
                }
                render_inventory_box("INVENTORY", "1234567890  ", "", 1, 1);
                c = pc.inventory_size();
                for (i = 0; i < c; i++)
                    render_inventory_item(pc.inventory_at(i), i, false, 1, 1);
                render_inventory_box("EQUIPMENT", EQUIP_SLOT_CHARS, "Select an item here.", 3 + INVENTORY_BOX_WIDTH, 1);
                for (i = 0; i < (int) (sizeof (pc.equipment) / sizeof (pc.equipment[0])); i++)
                    if (pc.equipment[i]) render_inventory_item(pc.equipment[i], i, false, 3 + INVENTORY_BOX_WIDTH, 1);
                sel = prompt(EQUIP_SLOT_CHARS, "Select an item or press ESC to cancel.");
                if (sel == 0) break; // Cancelled
                i = sel - 'a';
                target_item = pc.equipment[i];
                if (target_item == NULL) {
                    message_queue_t::get()->clear();
                    std::string err = "&0&bThere's no item in slot ";
                    err += sel;
                    err += ".";
                    message_queue_t::get()->add(err);
                    break;
                }
                pc.add_to_inventory(target_item);
                pc.equipment[i] = NULL;
                message_queue_t::get()->add("You unequipped &" + std::to_string(
                    target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                break;
            case KB_DROP:
                if (monster_menu_on || teleport_mode) break;
                if (pc.inventory_size() == 0) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bYou have no items!");
                    break;
                }
                x = (WIDTH - INVENTORY_BOX_WIDTH) / 2;
                render_inventory_box("INVENTORY", "1234567890", "Select an item here.", x, 1);
                c = pc.inventory_size();
                for (i = 0; i < c; i++)
                    render_inventory_item(pc.inventory_at(i), i, false, x, 1);
                sel = prompt("1234567890", "Select an item or press ESC to cancel.");
                if (sel == 0) break; // Cancelled
                // Unfortunate thing here is that I've ordered it 1-9, then 0, to match the keyboard.
                if (sel == '0') i = 9;
                else i = sel - '1';
                if (i >= pc.inventory_size()) {
                    message_queue_t::get()->clear();
                    std::string err = "&0&bThere's no item in slot ";
                    err += sel;
                    err += ".";
                    message_queue_t::get()->add(err);
                    break;
                }
                target_item = pc.remove_from_inventory(i);
                if (item_map[pc.x][pc.y]) {
                    item_map[pc.x][pc.y]->add_to_stack(target_item);
                } else {
                    item_map[pc.x][pc.y] = target_item;
                }
                message_queue_t::get()->add("You dropped &" + std::to_string(
                    target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                break;
            case KB_EXPUNGE:
                if (monster_menu_on || teleport_mode) break;
                if (pc.inventory_size() == 0) {
                    message_queue_t::get()->clear();
                    message_queue_t::get()->add("&0&bYou have no items!");
                    break;
                }
                x = (WIDTH - INVENTORY_BOX_WIDTH) / 2;
                render_inventory_box("INVENTORY", "1234567890", "Select an item here.", x, 1);
                c = pc.inventory_size();
                for (i = 0; i < c; i++)
                    render_inventory_item(pc.inventory_at(i), i, false, x, 1);
                sel = prompt("1234567890", "Select an item or press ESC to cancel.");
                if (sel == 0) break; // Cancelled
                // Unfortunate thing here is that I've ordered it 1-9, then 0, to match the keyboard.
                if (sel == '0') i = 9;
                else i = sel - '1';
                if (i >= pc.inventory_size()) {
                    message_queue_t::get()->clear();
                    std::string err = "&0&bThere's no item in slot ";
                    err += sel;
                    err += ".";
                    message_queue_t::get()->add(err);
                    break;
                }
                target_item = pc.remove_from_inventory(i);
                message_queue_t::get()->add("You expunged &" + std::to_string(
                    target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                delete target_item;
                break;
            case KB_NEXT_MESSAGE:
                break;
            case KB_QUIT:
                return;
            default:
                message_queue_t::get()->clear();
                message_queue_t::get()->add("&0&bUnrecognized command. &r&d(" + std::to_string(c) + ")");
                std::cout << c << std::endl;
        }

        if (result == GAME_RESULT_RUNNING && next_turn_ready) {
            update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);
            // Run the game until the PC's turn comes up again (or it dies)
            result = run_until_pc();
            hide_fog_of_war = false;
        }
        if (result != GAME_RESULT_RUNNING) {
            hide_fog_of_war = true;
            message_queue_t::get()->clear();
            message_queue_t::get()->add("&bGame over. Press any key to continue.");
        }
    }
}

/*
We need to be able to draw the inventory/equipment slots to the screen.
Complicating this more, I want this to function like the monster menu -- they should be able to
move up/down to select an item, reading its description. And this needs to happen without duplicate
logic in each of the 4 different rendering possibilities:
- Just display the inventory list
- Just display the equipment list
- Display both the inventory and equipment list in one (side-by-side?)
- Display both the inventory and equipment list in one, enabling selection

The first component is drawing the inventory and equipment. This should be able to operate on an array
and an item stack.
To accomplish the stack/array interoperability, there should be a starter function draws out the border
of the menu and the slot labels, and another that adds an individual item to the menu. That leaves
iterating over whatever method the items are stored in to the caller.
Now we need to deal with just drawing one/both and allowing selection.
The method to draw the border will take in a title, a string of labels, and a position (top left corner).
The method to draw the items will take in an item (duh), the index it gets drawn at, and if it's selected or not.
And a third method will draw the description at the bottom of the screen.

That way, all of the input logic is left to the caller -- if it's the item list, the up/down arrows will change
the index (or any of the label hotkeys) with selection enabled. If it's any of the other prompt-related ones,
it'll just draw the list and use the normal prompt feature.
*/
void game_t::render_inventory_box(std::string title, std::string labels, std::string input_tip, int x0, int y0) {
    // The width will be fixed to 38 chars so that we have room to draw both windows with borders.
    // We need, like the monster menu, the top line for the title, and the last line to indicate how to leave.
    int height = labels.length() + 2;
    int i, y;
    // Clear out the area.
    attrset(COLOR_PAIR(COLORS_MENU_TEXT));
    for (y = y0; y < y0 + height; y++)
        PRINT_REPEATED(x0, y, INVENTORY_BOX_WIDTH, ' ');
    // Title...
    attron(A_BOLD);
    PRINTW_CENTERED_AT(x0 + INVENTORY_BOX_WIDTH / 2, y0, "%s", title.c_str());
    attroff(A_BOLD);
    // Input tip...
    PRINTW_CENTERED_AT(x0 + INVENTORY_BOX_WIDTH / 2, y0 + height - 1, "%s", input_tip.c_str());
    // And the labels.
    for (i = 0; i < (int) labels.length(); i++)
        mvprintw(y0 + 1 + i, x0, "%c", labels[i]);
    attroff(COLOR_PAIR(COLORS_MENU_TEXT));
}

void game_t::render_inventory_item(item_t *item, int i, bool selected, int x0, int y0) {
    // First we'll draw out the item symbol (in color!)
    int c = (selected ? COLORS_FLOOR_ANY : COLORS_MENU_ANY) + item->current_color();
    attrset(COLOR_PAIR(c) | A_BOLD);
    mvaddch(y0 + 1 + i, x0 + 2, item->regular_symbol());
    attroff(COLOR_PAIR(c) | A_BOLD);
    // Now the item title
    c = selected ? COLORS_MENU_TEXT_SELECTED : COLORS_MENU_TEXT;
    attrset(COLOR_PAIR(c));
    printw(" %s", item->definition->name.c_str());
    attroff(COLOR_PAIR(c));
}

void game_t::render_inventory_details(item_t *item, int y0) {
    int i, y;
    std::string desc;
    // Draw out a box, taking up the whole width of the screen at y0.
    // This determines how many lines of description we get until it cuts off.
    attrset(COLOR_PAIR(COLORS_MENU_TEXT));
    for (y = y0; y < HEIGHT; y++)
        PRINT_REPEATED(0, y, WIDTH, ' ');

    if (!item) {
        attroff(COLOR_PAIR(COLORS_MENU_TEXT));
        return;
    }

    attron(A_BOLD);
    PRINTW_CENTERED_AT(WIDTH / 2, y0, "%s", item->definition->name.c_str());
    attroff(A_BOLD);
    // Just printing out the whole string clears out the color on the remainder of the line when there's
    // a newline character. It also doesn't let us protect against overflow, so unfortunately, this'll be
    // by hand.
    y = y0 + 1;
    move(y, 0);
    desc = item->definition->description;
    for (i = 0; y < HEIGHT - 1 && desc[i]; i++) {
        if (desc[i] == '\r') continue; // Fairly certain this breaks my parser anyway, but...
        if (desc[i] == '\n') {
            y++;
            move(y, 0);
            continue;
        }
        addch(desc[i]);
    }
    // The last line will be the dice.
    attron(A_DIM);
    PRINTW_CENTERED_AT(WIDTH / 2, HEIGHT - 1, "%sdmg: %s, def: %s, speed: %s, dodge: %s",
        item->definition->artifact ? "*ARTIFACT* " : "",
        item->definition->damage_bonus->str().c_str(),
        item->definition->defense_bonus->str().c_str(),
        item->definition->speed_bonus->str().c_str(),
        item->definition->dodge_bonus->str().c_str());
    attroff(COLOR_PAIR(COLORS_MENU_TEXT) | A_DIM);
}

bool str_contains_char(std::string str, char ch) {
    for (int i = 0; str[i]; i++) if (str[i] == ch) return true;
    return false;
}

void game_t::inventory_menu() {
    bool inventory = true;
    int menu_i = -1;
    int i, j, c;
    char ch;
    item_t *item;
    int scroll_dir;
    int inv_count = pc.inventory_size();
    int equip_count = ARRAY_SIZE(pc.equipment);

    while (true) {
        render_inventory_box("INVENTORY", "1234567890  ", "", 1, 1);
        for (i = 0; i < inv_count; i++)
            render_inventory_item(pc.inventory_at(i), i, inventory && i == menu_i, 1, 1);
        render_inventory_box("EQUIPMENT", EQUIP_SLOT_CHARS, "", 3 + INVENTORY_BOX_WIDTH, 1);
        for (i = 0; i < equip_count; i++)
            if (pc.equipment[i]) render_inventory_item(pc.equipment[i], i, !inventory && i == menu_i, 3 + INVENTORY_BOX_WIDTH, 1);
        if (menu_i < 0) item = NULL;
        else item = inventory ? pc.inventory_at(menu_i) : pc.equipment[menu_i];
        render_inventory_details(item, 16);

        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        PRINT_REPEATED(0, 0, WIDTH, ' ');
        PRINTW_CENTERED_AT(WIDTH / 2, 0, "Enter an item, use the arrow keys, or press ESC.");
        c = getch();
        scroll_dir = 0;
        switch (c) {
            case KB_SCROLL_DOWN:
                scroll_dir = 1;
                break;
            case KB_SCROLL_UP:
                scroll_dir = -1;
                break;
            case KB_SCROLL_LEFT:
            case KB_SCROLL_RIGHT:
                inventory = !inventory;
                menu_i = 0;
                break;
            case KB_ESCAPE:
                return;
            default:
                ch = (char) c;
                if (ch == c) {
                    // If it's one of our slot keys, use those
                    if (str_contains_char(INVENTORY_SLOT_CHARS, c)) {
                        inventory = true;
                        i = c == '0' ? 9 : c - '1';
                        if (i >= inv_count) {
                            break;
                        }
                    }
                    else if (str_contains_char(EQUIP_SLOT_CHARS, c)) {
                        inventory = false;
                        i = c - 'a';
                    }
                    menu_i = i;
                    break;
                }
                break;
        }
        if (scroll_dir != 0) {
            if (inventory) {
                menu_i += scroll_dir;
                if (menu_i >= inv_count) menu_i = 0;
                else if (menu_i < 0) menu_i = inv_count - 1;
            }
            else {
                // Seek to the next item
                i = menu_i;
                item = NULL;
                for (j = 0; j < equip_count && !item; j++) {
                    i += scroll_dir;
                    if (i < 0) i += equip_count;
                    i %= equip_count;
                    item = pc.equipment[i];
                }
                if (!item) menu_i = -1;
                else menu_i = i;
            }
        }
        if (inventory && menu_i >= inv_count) menu_i = -1;
        else if (!inventory && menu_i >= equip_count) menu_i = -1;
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
    }
}

void game_t::monster_menu() {
    int menu_i = 0;
    int count;
    int c, y, i, x_offset, y_offset;
    character_t *ch;
    monster_t *targeted_monster;
    std::string desc;


    while (true) {
        attron(COLOR_PAIR(COLORS_MENU_TEXT));
        // Requirements are the symbol and relative location.
        // Doesn't take much space. We'll reserve half (40) for now.
        // And add a border (of whitespace) around the screen too.
        // To avoid complicating the PRINTW_CENTERED_AT macro, I'll just do that now.
        for (y = MONSTER_MENU_Y_BEGIN; y < MONSTER_MENU_Y_BEGIN + MONSTER_MENU_HEIGHT; y++)
            PRINT_REPEATED(MONSTER_MENU_X_BEGIN, y, MONSTER_MENU_WIDTH, ' ');

        attron(A_BOLD);
        count = turn_queue.size();
        PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN, "MONSTERS (%d)", count - 1);
        attroff(A_BOLD);

        // Find the first n monsters starting at menu_i (irrespective of PC).
        c = -1;
        targeted_monster = NULL;
        for (i = 0; i < count; i++) {
            ch = turn_queue.at(i);
            if (ch == &pc) continue;
            c++;
            if (c >= menu_i + MONSTER_MENU_HEIGHT - 4) break; // Screen is full
            if (c >= menu_i) {
                // We can render this one.
                if (c == menu_i) {
                    targeted_monster = (monster_t *) ch;
                    attroff(COLOR_PAIR(COLORS_MENU_TEXT));
                    attrset(COLOR_PAIR(COLORS_MENU_TEXT_SELECTED) | A_BOLD);
                }
                x_offset = ((int) pc.x) - ((int) ch->x);
                y_offset = ((int) pc.y) - ((int) ch->y);
                PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + 2 + c - menu_i,
                    " %c: %2d %s, %2d %s ", ch->display,
                    y_offset < 0 ? -1 * y_offset : y_offset,
                    y_offset < 0 ? "south" : "north",
                    x_offset < 0 ? -1 * x_offset : x_offset,
                    x_offset < 0 ? "east" : "west"
                );
                if (c == menu_i) {
                    attroff(COLOR_PAIR(COLORS_MENU_TEXT_SELECTED) | A_BOLD);
                    attrset(COLOR_PAIR(COLORS_MENU_TEXT));
                }
            }
        }

        // Add some nice indicators to say there's more monsters above/below
        if (menu_i > 0)
            PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + 1, "^")
        if (menu_i < count - (MONSTER_MENU_HEIGHT - 4) - 1)
            PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + MONSTER_MENU_HEIGHT - 2, "v")

        PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + MONSTER_MENU_HEIGHT - 1, "Press ESC to return.");

        // And at the bottom of the screen, we'll display the attributes.
        if (targeted_monster != NULL) {
            for (y = HEIGHT - 12; y < HEIGHT; y++)
                PRINT_REPEATED(0, y, WIDTH, ' ');

            attron(A_BOLD);
            PRINTW_CENTERED_AT(WIDTH / 2, HEIGHT - 12, "%s", targeted_monster->definition->name.c_str());
            attroff(A_BOLD);
            // Just printing out the whole string clears out the color on the remainder of the line when there's
            // a newline character. It also doesn't let us protect against overflow, so unfortunately, this'll be
            // by hand.
            y = HEIGHT - 11;
            move(y, 0);
            desc = targeted_monster->definition->description;
            for (i = 0; y < HEIGHT - 1 && desc[i]; i++) {
                if (desc[i] == '\r') continue; // Fairly certain this breaks my parser anyway, but...
                if (desc[i] == '\n') {
                    y++;
                    move(y, 0);
                    continue;
                }
                addch(desc[i]);
            }
            // The last line will be the attributes.
            desc = "";
            APPEND_MONST_ATTRS(targeted_monster->definition->abilities, desc);
            attron(A_DIM);
            PRINTW_CENTERED_AT(WIDTH / 2, HEIGHT - 1, "%s", desc.c_str());
            attroff(A_DIM);
        }
        attroff(COLOR_PAIR(COLORS_MENU_TEXT));

        c = getch();
        count = turn_queue.size() - 1;
        switch (c) {
            case KB_SCROLL_DOWN:
                menu_i = (menu_i + 1) % count;
                break;
            case KB_SCROLL_UP:
                menu_i--;
                if (menu_i < 0) menu_i = count - 1;
                break;
            case KB_ESCAPE:
                return;
        }
    }
}

char prompt(std::string options, std::string prompt) {
    int c = 0;
    int i;
    bool valid;
    attron(A_BOLD);
    PRINT_REPEATED(0, 0, WIDTH, ' ');
    PRINTW_CENTERED_AT(WIDTH / 2, 0, "%s", prompt.c_str());
    attroff(A_BOLD);
    while (true) {
        c = getch();
        if (c == KB_ESCAPE) return 0;
        valid = false;
        for (i = 0; i < (int) options.length(); i++) {
            if (c == options[i]) {
                return options[i];
            }
        }
        if (!valid) {
            attron(A_BOLD);
            PRINT_REPEATED(0, 0, WIDTH, ' ');
            PRINTW_CENTERED_AT(WIDTH / 2, 0, "Invalid option. %s", prompt.c_str());
            attroff(A_BOLD);
            c = 0;
        }
    }
}

game_result_t game_t::run_until_pc() {
    character_t *ch = NULL;
    monster_t *monster;
    uint32_t priority;
    game_result_t result;

    while (true) {
        ch = NULL;
        while (turn_queue.size() > 0 && ch == NULL) {
            priority = turn_queue.top_priority();
            ch = turn_queue.remove();
            // The dead flag here avoids us having to remove from the heap at an arbitrary location.
            // We just destroy it when it comes off the queue next.
            if (ch->dead) {
                if (ch != &pc) {
                    destroy_character(character_map, ch);
                    ch = NULL;
                }
            }
        }

        if (ch == NULL)
            throw dungeon_exception(__PRETTY_FUNCTION__, "turn queue is empty");

        // If this was the PC's turn, signal that back to the caller
        if (ch == &pc) {
            turn_queue.insert(&pc, priority + (pc.speed_bonus()));
            return GAME_RESULT_RUNNING;
        }

        monster = (monster_t *) ch;
        result = GAME_RESULT_RUNNING;
        monster->take_turn(dungeon, &pc, turn_queue, character_map, item_map, pathfinding_tunnel, pathfinding_no_tunnel, priority, result);
        if (pc.dead) result = GAME_RESULT_LOSE;
        return result;
    }
}
