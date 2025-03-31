#include <ncurses.h>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "game.h"
#include "macros.h"
#include "dungeon.h"
#include "heap.h"
#include "character.h"
#include "ascii.h"
#include "pathfinding.h"

char CHARACTERS_BY_CELL_TYPE[CELL_TYPES] = {
    [CELL_TYPE_STONE] = ' ',
    [CELL_TYPE_ROOM] = '.',
    [CELL_TYPE_HALL] = '#',
    [CELL_TYPE_UP_STAIRCASE] = '<',
    [CELL_TYPE_DOWN_STAIRCASE] = '>',
    [CELL_TYPE_EMPTY] = '!',
    [CELL_TYPE_DEBUG] = 'X',
    [CELL_TYPE_HIDDEN] = ' '
};

int COLORS_BY_CELL_TYPE[CELL_TYPES] = {
    [CELL_TYPE_STONE] = COLORS_STONE,
    [CELL_TYPE_ROOM] = COLORS_FLOOR,
    [CELL_TYPE_HALL] = COLORS_FLOOR,
    [CELL_TYPE_UP_STAIRCASE] = COLORS_OBJECT,
    [CELL_TYPE_DOWN_STAIRCASE] = COLORS_OBJECT,
    [CELL_TYPE_EMPTY] = COLORS_OBJECT,
    [CELL_TYPE_DEBUG] = COLORS_OBJECT,
    [CELL_TYPE_HIDDEN] = COLORS_STONE
};

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
        try_move(x, y); \
        next_turn_ready = 1; \
    }

char LOSE[] = ASCII_LOSE;
char WIN[] = ASCII_WIN;

#define MONSTER_MENU_WIDTH 40
#define MONSTER_MENU_HEIGHT 15

#define MONSTER_MENU_X_BEGIN WIDTH / 2 - MONSTER_MENU_WIDTH / 2
#define MONSTER_MENU_Y_BEGIN HEIGHT / 2 - MONSTER_MENU_HEIGHT / 2

void game_t::run() {
    try {
        initscr();
        if (COLS < WIDTH || LINES < HEIGHT)
            throw std::runtime_error("terminal size is too small, minimum is " + std::to_string(WIDTH) 
                + "x" + std::to_string(HEIGHT) + " (yours is " + std::to_string(COLS) + "x" + std::to_string(LINES) + ")");
        if (start_color() != OK) throw std::runtime_error("failed to init ncurses (no color support)");
        if (init_pair(COLORS_FLOOR, COLOR_BLUE, COLOR_BLACK) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (init_pair(COLORS_PC, COLOR_GREEN, COLOR_BLACK) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (init_pair(COLORS_MONSTER, COLOR_RED, COLOR_BLACK) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (init_pair(COLORS_OBJECT, COLOR_MAGENTA, COLOR_BLACK) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (init_pair(COLORS_STONE, COLOR_BLACK, COLOR_BLACK) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (init_pair(COLORS_TEXT, COLOR_WHITE, COLOR_BLACK) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (init_pair(COLORS_MENU_TEXT, COLOR_BLACK, COLOR_WHITE) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (init_pair(COLORS_FOG_OF_WAR_TERRAIN, COLOR_WHITE, COLOR_BLACK) != OK) throw std::runtime_error("failed to init ncurses (init color)");
        if (keypad(stdscr, TRUE) != OK) throw std::runtime_error("failed to init ncurses (special kb keys)");
                        // man this library is weird
        if (curs_set(0) == ERR) throw std::runtime_error("failed to init ncurses (disable cursor)");
        if (noecho() == ERR) throw std::runtime_error("failed to init ncurses (noecho)");    

        run_internal();
    } catch (std::runtime_error &e) {
        endwin();
        throw e;
    }
    endwin();
}

void game_t::run_internal() {
    if (!is_initialized) throw std::runtime_error("game is not yet initialized");
    int c, i, x_offset, y_offset, count, monster_count, overflow_count;
    bool next_turn_ready, was_pc;
    uint8_t x, y;
    int monster_menu_i = 0;
    bool monster_menu_on = 0;
    bool hide_fog_of_war = false;
    bool teleport_mode = false;
    bool sticky_message = false;
    coordinates_t teleport_pointer;
    game_result_t result = GAME_RESULT_RUNNING;
    character_t *ch;
    cell_type_t cell_to_render;
    bool in_sight;
    char *ascii;
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
                    cell_to_render = dungeon->cells[x][y].type;
                    in_sight = true;
                }
                else {
                    // Otherwise, this is outside of the sight of the PC, and should be rendered
                    // using the seen map.
                    cell_to_render = seen_map[x][y];
                    in_sight = false;
                }

                if (teleport_mode && x == teleport_pointer.x && y == teleport_pointer.y) {
                    attrset(COLOR_PAIR(COLORS_PC) | A_BOLD);
                    addch(TELEPORT_POINTER);
                    attroff(COLOR_PAIR(COLORS_PC) | A_BOLD);
                } else if (in_sight && character_map[x][y]) {
                    c = character_map[x][y] == &pc ? COLORS_PC : COLORS_MONSTER;
                    attrset(COLOR_PAIR(c) | A_BOLD);
                    addch(character_map[x][y]->display);
                    attroff(COLOR_PAIR(c) | A_BOLD);
                } else {
                    attrset(COLOR_PAIR(in_sight ? COLORS_BY_CELL_TYPE[cell_to_render] : COLORS_FOG_OF_WAR_TERRAIN));
                    if (!in_sight) attrset(A_DIM);
                    addch(CHARACTERS_BY_CELL_TYPE[cell_to_render]);
                    if (!in_sight) attroff(A_DIM);
                    attroff(COLOR_PAIR(in_sight ? COLORS_BY_CELL_TYPE[cell_to_render] : COLORS_FOG_OF_WAR_TERRAIN));
                }
            }
        }

        // Print the status message at the top (approx. centered to 80 chars)
        x = ((WIDTH - 1) / 2) - (strlen(message) / 2) - 1;
        move(0, x);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw(" %s ", message);
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);

        if (result != GAME_RESULT_RUNNING) {
            getch();
            // Print our beautiful images
            clear();
            if (result == GAME_RESULT_LOSE) {
                ascii = LOSE;
                x = WIDTH / 2 - ASCII_LOSE_WIDTH / 2;
                y = HEIGHT / 2 - ASCII_LOSE_HEIGHT / 2;
            }
            else {
                ascii = WIN;
                x = WIDTH / 2 - ASCII_WIN_WIDTH / 2;
                y = HEIGHT / 2 - ASCII_WIN_HEIGHT / 2;
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

        // Print out the monster menu if available
        if (monster_menu_on) {
            attron(COLOR_PAIR(COLORS_MENU_TEXT));
            // Requirements are the symbol and relative location.
            // Doesn't take much space. We'll reserve half (40) for now.
            // And add a border (of whitespace) around the screen too.
            // To avoid complicating the PRINTW_CENTERED_AT macro, I'll just do that now.
            for (y = MONSTER_MENU_Y_BEGIN; y < MONSTER_MENU_Y_BEGIN + MONSTER_MENU_HEIGHT; y++)
                PRINT_REPEATED(MONSTER_MENU_X_BEGIN, y, MONSTER_MENU_WIDTH, ' ');

            attron(A_BOLD);
            count = turn_queue->size();
            PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + 1, "MONSTERS (%d)", count - 1);
            attroff(A_BOLD);
            for (i = 0, monster_count = 0; monster_count < MONSTER_MENU_HEIGHT - 6 + monster_menu_i && i < count; i++) {
                turn_queue->at(i, &ch);
                if (ch->type() == CHARACTER_TYPE_MONSTER) {
                    if (monster_count >= monster_menu_i) {
                        x_offset = ((int) pc.x) - ((int) ch->x);
                        y_offset = ((int) pc.y) - ((int) ch->y);

                        PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + 3 + monster_count - monster_menu_i, 
                            "%c: %2d %s, %2d %s", ch->display,
                            y_offset < 0 ? -1 * y_offset : y_offset,
                            y_offset < 0 ? "south" : "north", 
                            x_offset < 0 ? -1 * x_offset : x_offset,
                            x_offset < 0 ? "east" : "west"
                        );
                    }
                    monster_count++;
                }
            }
            for (; monster_count < MONSTER_MENU_HEIGHT - 6; monster_count++)
                PRINT_REPEATED(MONSTER_MENU_X_BEGIN, MONSTER_MENU_Y_BEGIN + 3 + monster_count, MONSTER_MENU_WIDTH, ' ');

            // Add some nice indicators to say there's more monsters above/below
            if (monster_menu_i > 0)
                PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + 2, "^")
            if (monster_menu_i < count - (MONSTER_MENU_HEIGHT - 6) - 1)
                PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + MONSTER_MENU_HEIGHT - 3, "v")

            PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + MONSTER_MENU_HEIGHT - 2, "Press ESC to return.");
            attroff(COLOR_PAIR(COLORS_MENU_TEXT));
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
                if (monster_menu_on) break;
                monster_menu_on = 1;
                monster_menu_i = 0;
                break;
            case KB_SCROLL_DOWN:
                if (!monster_menu_on) {
                    snprintf(message, WIDTH, "Can't scroll without monster menu open!");
                    break;
                }
                count = turn_queue->size() - 1;
                overflow_count = count - (MONSTER_MENU_HEIGHT - 6);
                if (overflow_count < 0) overflow_count = 0;
                monster_menu_i++;
                if (monster_menu_i > overflow_count) monster_menu_i = overflow_count;
                break;
            case KB_SCROLL_UP:
                if (!monster_menu_on) {
                    snprintf(message, WIDTH, "Can't scroll without monster menu open!");
                    break;
                }
                monster_menu_i--;
                if (monster_menu_i < 0) monster_menu_i = 0;
                break;
            case KB_ESCAPE:
                if (!monster_menu_on) break;
                monster_menu_on = 0;
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
                }
                fill_and_place_on(CELL_TYPE_DOWN_STAIRCASE);
                snprintf(message, WIDTH, "You went up the stairs.");
                break;
            case KB_DOWN_STAIRS:
                if (monster_menu_on || teleport_mode) break;
                if (dungeon->cells[pc.x][pc.y].type != CELL_TYPE_DOWN_STAIRCASE) {
                    snprintf(message, WIDTH, "There isn't a down staircase here.");
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
                    force_move(teleport_pointer);
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
                    catch (std::runtime_error &e) {
                        // I don't see a reason to crash the game for this.
                        teleport_pointer.x = pc.x;
                        teleport_pointer.y = pc.y;
                    }
                    message[0] = '\0';
                    force_move(teleport_pointer);
                    sticky_message = false;
                    teleport_mode = false;
                    next_turn_ready = true;
                }
                break;
            case KB_QUIT:
                return;
            default:
                snprintf(message, WIDTH, "Unrecognized command: %c", (char) c);
        }

        if (next_turn_ready) {
            update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);
            result = GAME_RESULT_RUNNING;
            was_pc = 0;
            while (!was_pc && result == GAME_RESULT_RUNNING) {
                next_turn(dungeon, &pc, turn_queue, character_map, pathfinding_tunnel, pathfinding_no_tunnel, &result, &was_pc);
            }

            hide_fog_of_war = false;
            if (result != GAME_RESULT_RUNNING) {
                hide_fog_of_war = true;
                snprintf(message, 80, "Game over. Press any key to continue.");
            }
        }
    }
}