#include <ncurses.h>
#include <cstring>
#include <cstdlib>

#include "game.h"
#include "macros.h"
#include "dungeon.h"
#include "heap.h"
#include "character.h"
#include "ascii.h"
#include "pathfinding.h"

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

char LOSE[] = ASCII_LOSE;
char WIN[] = ASCII_WIN;
char EQUIP_SLOT_CHARS[] = "asdfghjkl";

#define MONSTER_MENU_WIDTH 40
#define MONSTER_MENU_HEIGHT 10

#define MONSTER_MENU_X_BEGIN WIDTH / 2 - MONSTER_MENU_WIDTH / 2
#define MONSTER_MENU_Y_BEGIN 1

void game_t::run() {
    try {
        initscr();
        if (COLS < WIDTH || LINES < HEIGHT)
            throw dungeon_exception(__PRETTY_FUNCTION__, "terminal size is too small, minimum is "
                + std::to_string(WIDTH) + "x" + std::to_string(HEIGHT) + " (yours is " + std::to_string(COLS) + "x" + std::to_string(LINES) + ")");
        if (start_color() != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (no color support)");
        if (init_pair(COLORS_FLOOR, COLOR_BLUE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_PC, COLOR_WHITE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_MONSTER, COLOR_RED, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_OBJECT, COLOR_MAGENTA, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_STONE, COLOR_BLACK, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_TEXT, COLOR_WHITE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_TEXT_RED, COLOR_RED, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_MENU_TEXT, COLOR_BLACK, COLOR_WHITE) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_MENU_TEXT_SELECTED, COLOR_WHITE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FOG_OF_WAR_TERRAIN, COLOR_WHITE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY, COLOR_RED, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY + 1, COLOR_GREEN, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY + 2, COLOR_YELLOW, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY + 3, COLOR_BLUE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY + 4, COLOR_MAGENTA, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY + 5, COLOR_CYAN, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY + 6, COLOR_WHITE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");
        if (init_pair(COLORS_FLOOR_ANY + 7, COLOR_WHITE, COLOR_BLACK) != OK) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to init ncurses (init color)");

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
    int c, i, j;
    bool next_turn_ready;
    uint8_t x, y;
    bool monster_menu_on = 0;
    bool hide_fog_of_war = false;
    bool teleport_mode = false;
    bool sticky_message = false;
    coordinates_t teleport_pointer;
    game_result_t result = GAME_RESULT_RUNNING;
    char *ascii;
    item_t *target_item;
    float hp;
    message[0] = '\0';

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
        x = ((WIDTH - 1) / 2) - (strlen(message) / 2) - 1;
        move(0, x);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw(" %s ", message);
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);

        // And health/inventory at the bottom
        move(HEIGHT - 3, 0);
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
        move(HEIGHT - 2, 0);
        printw("INVENTORY: ");
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        j = pc.inventory_size();
        attrset(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        for (i = 0; i < 10; i++) {
            printw("%d  ", i);
        }
        attroff(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw("  EQUIPMENT: ");
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        for (i = 0; i < 9; i++) {
            printw("%c  ", EQUIP_SLOT_CHARS[i]);
        }
        attroff(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        for (i = 0; i < 10; i++) {
            if (i < j) {
                target_item = pc.inventory_at(i);
                c = COLORS_FLOOR_ANY + target_item->next_color();
                attrset(COLOR_PAIR(c));
                mvprintw(HEIGHT - 2, 12 + 3 * i, "%c", target_item->regular_symbol());
                attroff(COLOR_PAIR(c));
            }
        }
        for (i = 0; i < 9; i++) {
            target_item = pc.equipment[i];
            if (target_item == NULL) continue;
            c = COLORS_FLOOR_ANY + target_item->next_color();
            attrset(COLOR_PAIR(c));
            mvprintw(HEIGHT - 2, 55 + 3 * i, "%c", target_item->regular_symbol());
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

        // Now we can read the next command
        if (!sticky_message)
            message[0] = '\0';
        c = getch();
        next_turn_ready = 0;
        switch (c) {
            case KB_MONSTERS:
                monster_menu();
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
                    snprintf(message, WIDTH, "There isn't an up staircase here.");
                    break;
                }
                fill_and_place_on(CELL_TYPE_DOWN_STAIRCASE);
                snprintf(message, WIDTH, "You went up the stairs.");
                break;
            case KB_DOWN_STAIRS:
                if (monster_menu_on || teleport_mode) break;
                if (dungeon->cells[pc.x][pc.y].type != CELL_TYPE_DOWN_STAIRCASE) {
                    snprintf(message, WIDTH, "There isn't a down staircase here.");
                    break;
                }
                fill_and_place_on(CELL_TYPE_UP_STAIRCASE);
                snprintf(message, WIDTH, "You went down the stairs.");
                break;
            case KB_TOGGLE_FOG:
                if (monster_menu_on || teleport_mode) break;
                hide_fog_of_war = !hide_fog_of_war;
                break;
            case KB_TELEPORT:
                if (monster_menu_on) break;
                if (!teleport_mode) {
                    teleport_mode = true;
                    snprintf(message, WIDTH, "-- TELEPORT MODE --");
                    sticky_message = true;
                    teleport_pointer.x = pc.x;
                    teleport_pointer.y = pc.y;
                }
                else {
                    message[0] = '\0';
                    force_move(result, teleport_pointer);
                    sticky_message = false;
                    teleport_mode = false;
                    next_turn_ready = true;
                }
                break;
            case KB_TELEPORT_RANDOM:
                if (monster_menu_on) break;
                if (!teleport_mode) {
                    snprintf(message, WIDTH, "Enter teleport mode first.");
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
                    message[0] = '\0';
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
                    snprintf(message, WIDTH, "There's no item here.");
                    break;
                }
                if (pc.inventory_size() >= MAX_CARRY_SLOTS) {
                    snprintf(message, WIDTH, "Your carry slots are full!");
                    break;
                }
                if (target_item->is_stacked()) {
                    item_map[pc.x][pc.y] = target_item->detach_stack();
                } else {
                    item_map[pc.x][pc.y] = NULL;
                }
                pc.add_to_inventory(target_item);
                snprintf(message, WIDTH, "You picked up the %s. You have %d items.", target_item->definition->name.c_str(), pc.inventory_size());
                next_turn_ready = true;
                break;
            case KB_QUIT:
                return;
            default:
                snprintf(message, WIDTH, "Unrecognized command: %c", (char) c);
        }

        if (result == GAME_RESULT_RUNNING && next_turn_ready) {
            update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);
            // Run the game until the PC's turn comes up again (or it dies)
            result = run_until_pc();
            hide_fog_of_war = false;
        }
        if (result != GAME_RESULT_RUNNING) {
            hide_fog_of_war = true;
            snprintf(message, 80, "Game over. Press any key to continue.");
        }
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
            turn_queue.insert(&pc, priority + pc.speed);
            return GAME_RESULT_RUNNING;
        }

        monster = (monster_t *) ch;
        result = GAME_RESULT_RUNNING;
        monster->take_turn(dungeon, &pc, turn_queue, character_map, item_map, pathfinding_tunnel, pathfinding_no_tunnel, priority, result);
        if (pc.dead) result = GAME_RESULT_LOSE;
        return result;
    }
}
