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

typedef enum colors {
    COLORS_FLOOR = 1,
    COLORS_STONE,
    COLORS_PC,
    COLORS_MONSTER,
    COLORS_OBJECT,
    COLORS_TEXT,
    COLORS_MENU_TEXT
} colors_t;

typedef enum keybinds {
    KB_UP_LEFT_0 = '7',
    KB_UP_LEFT_1 = 'y',
    KB_UP_0 = '8',
    KB_UP_1 = 'k',
    KB_UP_RIGHT_0 = '9',
    KB_UP_RIGHT_1 = 'u',
    KB_RIGHT_0 = '6',
    KB_RIGHT_1 = 'l',
    KB_DOWN_RIGHT_0 = '3',
    KB_DOWN_RIGHT_1 = 'n',
    KB_DOWN_0 = '2',
    KB_DOWN_1 = 'j',
    KB_DOWN_LEFT_0 = '1',
    KB_DOWN_LEFT_1 = 'b',
    KB_LEFT_0 = '4',
    KB_LEFT_1 = 'h',
    KB_DOWN_STAIRS = '>',
    KB_UP_STAIRS = '<',
    KB_REST_0 = '5',
    KB_REST_1 = ' ',
    KB_REST_2 = '.',
    KB_MONSTERS = 'm',
    KB_SCROLL_UP = KEY_UP,
    KB_SCROLL_DOWN = KEY_DOWN,
    KB_ESCAPE = 27, // can't find a constant for this???
    KB_QUIT = 'Q'
} keybinds_t;

char CHARACTERS_BY_CELL_TYPE[CELL_TYPES] = {
    [CELL_TYPE_STONE] = ' ',
    [CELL_TYPE_ROOM] = '.',
    [CELL_TYPE_HALL] = '#',
    [CELL_TYPE_UP_STAIRCASE] = '<',
    [CELL_TYPE_DOWN_STAIRCASE] = '>',
    [CELL_TYPE_EMPTY] = '!',
    [CELL_TYPE_DEBUG] = 'X'
};

int COLORS_BY_CELL_TYPE[CELL_TYPES] = {
    [CELL_TYPE_STONE] = COLORS_STONE,
    [CELL_TYPE_ROOM] = COLORS_FLOOR,
    [CELL_TYPE_HALL] = COLORS_FLOOR,
    [CELL_TYPE_UP_STAIRCASE] = COLORS_OBJECT,
    [CELL_TYPE_DOWN_STAIRCASE] = COLORS_OBJECT,
    [CELL_TYPE_EMPTY] = COLORS_OBJECT,
    [CELL_TYPE_DEBUG] = COLORS_OBJECT
};

#define WIDTH 80
#define HEIGHT 24

#define MONSTER_MENU_WIDTH 40
#define MONSTER_MENU_HEIGHT 15

#define MONSTER_MENU_X_BEGIN WIDTH / 2 - MONSTER_MENU_WIDTH / 2
#define MONSTER_MENU_Y_BEGIN HEIGHT / 2 - MONSTER_MENU_HEIGHT / 2

#define FOG_OF_WAR_DISTANCE 3

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

char LOSE[] = ASCII_LOSE;
char WIN[] = ASCII_WIN;

void update_fog_of_war(dungeon_t *dungeon) {
    int x, y;
    for (x = dungeon->pc.x - FOG_OF_WAR_DISTANCE; x < dungeon->pc.x + FOG_OF_WAR_DISTANCE; x++) {
        if (x < 0 || x >= dungeon->width) continue;
        for (y = dungeon->pc.y - FOG_OF_WAR_DISTANCE; y < dungeon->pc.y + FOG_OF_WAR_DISTANCE; y++) {
            if (y < 0 || y >= dungeon->height) continue;
            dungeon->cells[x][y].attributes |= CELL_ATTRIBUTE_SEEN;
        }
    }
}

void try_move(dungeon_t *dungeon, char *message, int x_offset, int y_offset) {
    int new_x = dungeon->pc.x + x_offset;
    int new_y = dungeon->pc.y + y_offset;
    if (dungeon->cells[new_x][new_y].type == CELL_TYPE_STONE) {
        snprintf(message, 80, "There's stone in the way!");
    }
    else { 
        if (dungeon->cells[new_x][new_y].character != NULL) {
            // Kill the monster there
            dungeon->cells[new_x][new_y].character->dead = 1;
            snprintf(message, 80, "You ate the %c in the way.", dungeon->cells[new_x][new_y].character->display);
        }
        UPDATE_CHARACTER(dungeon->cells, &(dungeon->pc), new_x, new_y);
        update_fog_of_war(dungeon);
    }
}

int fill_and_place_on(dungeon_t *dungeon, cell_type_t target_cell, int nummon) {
    // Kill all the monsters (RIP)
    character_t *character;
    uint32_t trash;
    int x, y, placed;
    while (heap_size(dungeon->turn_queue) > 0) {
        if (heap_remove(dungeon->turn_queue, (void *) &character, &trash))
            RETURN_ERROR("failed to remove from turn queue while cleaning up");
        if (character == &(dungeon->pc)) continue;
        destroy_character(dungeon, character);
    }
    if (fill_dungeon(dungeon, ROOM_MIN_COUNT, ROOM_COUNT_MAX_RANDOMNESS, ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT, ROOM_MAX_RANDOMNESS, 0))
        RETURN_ERROR("couldn't regenerate dungeon");
    placed = 0;
    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            if (dungeon->cells[x][y].type == target_cell) {
                UPDATE_CHARACTER(dungeon->cells, &(dungeon->pc), x, y);
                placed = 1;
                break;
            }
        }
    }
    if (!placed) RETURN_ERROR("no target cell present to place PC on");
    // Readd the PC and new monsters
    character = &(dungeon->pc);
    if (heap_insert(dungeon->turn_queue, (void *) &character, 0))
        RETURN_ERROR("failed to insert PC into turn queue");
    if (generate_monsters(dungeon, nummon < 0 ? (rand() % (RANDOM_MONSTERS_MAX - RANDOM_MONSTERS_MIN + 1)) + RANDOM_MONSTERS_MIN : nummon))
        RETURN_ERROR("couldn't regenerate monsters");
    return 0;
}

int game_start(dungeon_t *dungeon, int nummon) {
    int c, i, trash, count, monster_count, overflow_count, next_turn_ready, was_pc;
    uint8_t x, y;
    int monster_menu_on = 0;
    int monster_menu_i = 0;
    game_result_t result;
    character_t **ch;
    char *ascii;
    if (!(ch = (character_t **) malloc(sizeof (*ch)))) RETURN_ERROR("failed to allocate memory");
    char *message;
    if (!(message = (char *) malloc(1 + WIDTH))) {
        free(ch);
        RETURN_ERROR("failed to allocate memory");
    }
    message[0] = '\0';
    initscr();
    if (COLS < WIDTH || LINES < HEIGHT)
        ERROR_AND_EXIT("terminal size is too small, minimum is %dx%d (yours is %dx%d)", WIDTH, HEIGHT, COLS, LINES);
    if (start_color() != OK) ERROR_AND_EXIT("failed to init ncurses (no color support)");
    if (init_pair(COLORS_FLOOR, COLOR_BLUE, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_PC, COLOR_GREEN, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_MONSTER, COLOR_RED, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_OBJECT, COLOR_MAGENTA, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_STONE, COLOR_BLACK, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_TEXT, COLOR_WHITE, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_MENU_TEXT, COLOR_BLACK, COLOR_WHITE) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (keypad(stdscr, TRUE) != OK) ERROR_AND_EXIT("failed to init ncurses (special kb keys)");
                    // man this library is weird
    if (curs_set(0) == ERR) ERROR_AND_EXIT("failed to init ncurses (disable cursor)");
    if (noecho() == ERR) ERROR_AND_EXIT("failed to init ncurses (noecho)");

    update_fog_of_war(dungeon);
    while (1) {
        // Print the dungeon
        clear();
        for (y = 0; y < dungeon->height; y++) {
            move(y + 1, 0);
            for (x = 0; x < dungeon->width; x++) {
                if (!(dungeon->cells[x][y].attributes & CELL_ATTRIBUTE_SEEN)) {
                    addch(' ');
                    continue;
                }
                if (dungeon->cells[x][y].character) {
                    c = dungeon->cells[x][y].character->type == CHARACTER_PC ? COLORS_PC : COLORS_MONSTER;
                    attrset(COLOR_PAIR(c) | A_BOLD);
                    addch(dungeon->cells[x][y].character->display);
                    attroff(COLOR_PAIR(c) | A_BOLD);
                } else {
                    attrset(COLOR_PAIR(COLORS_BY_CELL_TYPE[dungeon->cells[x][y].type]));
                    addch(CHARACTERS_BY_CELL_TYPE[dungeon->cells[x][y].type]);
                    attroff(COLOR_PAIR(COLORS_BY_CELL_TYPE[dungeon->cells[x][y].type]));
                }
            }
        } 

        // Print the status message at the top (approx. centered to 80 chars)
        x = ((WIDTH - 1) / 2) - (strlen(message) / 2) - 1;
        move(0, x);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw(" %s ", message);
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);

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
            count = heap_size(dungeon->turn_queue);
            PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + 1, "MONSTERS (%d)", count - 1);
            attroff(A_BOLD);
            for (i = 0, monster_count = 0; monster_count < MONSTER_MENU_HEIGHT - 6 + monster_menu_i && i < count; i++) {
                if (heap_at(dungeon->turn_queue, i, ch, (uint32_t *) &trash)) ERROR_AND_EXIT("failed to get monster from turn queue");
                if ((*ch)->type == CHARACTER_MONSTER) {
                    if (monster_count >= monster_menu_i) {
                        x = dungeon->pc.x - (*ch)->x;
                        y = dungeon->pc.y - (*ch)->y;

                        PRINTW_CENTERED_AT(WIDTH / 2, MONSTER_MENU_Y_BEGIN + 3 + monster_count - monster_menu_i, 
                            "%c: %2d %s, %2d %s", (*ch)->display,
                            y < 0 ? -1 * y : y,
                            y < 0 ? "south" : "north", 
                            x < 0 ? -1 * x : x,
                            x < 0 ? "east" : "west"
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
                count = heap_size(dungeon->turn_queue) - 1;
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
                if (monster_menu_on) break;
                try_move(dungeon, message, -1, -1);
                next_turn_ready = 1;
                break;
            case KB_UP_0:
            case KB_UP_1:
                if (monster_menu_on) break;
                try_move(dungeon, message, 0, -1);
                next_turn_ready = 1;
                break;
            case KB_UP_RIGHT_0:
            case KB_UP_RIGHT_1:
                if (monster_menu_on) break;
                try_move(dungeon, message, 1, -1);
                next_turn_ready = 1;
                break;
            case KB_RIGHT_0:
            case KB_RIGHT_1:
                if (monster_menu_on) break;
                try_move(dungeon, message, 1, 0);
                next_turn_ready = 1;
                break;
            case KB_DOWN_RIGHT_0:
            case KB_DOWN_RIGHT_1:
                if (monster_menu_on) break;
                try_move(dungeon, message, 1, 1);
                next_turn_ready = 1;
                break;
            case KB_DOWN_0:
            case KB_DOWN_1:
                if (monster_menu_on) break;
                try_move(dungeon, message, 0, 1);
                next_turn_ready = 1;
                break;
            case KB_DOWN_LEFT_0:
            case KB_DOWN_LEFT_1:
                if (monster_menu_on) break;
                try_move(dungeon, message, -1, 1);
                next_turn_ready = 1;
                break;
            case KB_LEFT_0:
            case KB_LEFT_1:
                if (monster_menu_on) break;
                try_move(dungeon, message, -1, 0);
                next_turn_ready = 1;
                break;
            case KB_REST_0:
            case KB_REST_1:
            case KB_REST_2:
                if (monster_menu_on) break;
                next_turn_ready = 1;
                break;
            case KB_UP_STAIRS:
                if (monster_menu_on) break;
                if (dungeon->cells[dungeon->pc.x][dungeon->pc.y].type != CELL_TYPE_UP_STAIRCASE) {
                    snprintf(message, WIDTH, "There isn't an up staircase here.");
                }
                if (fill_and_place_on(dungeon, CELL_TYPE_DOWN_STAIRCASE, nummon))
                    ERROR_AND_EXIT("failed to generate new dungeon");
                snprintf(message, WIDTH, "You went up the stairs.");
                break;
            case KB_DOWN_STAIRS:
                if (monster_menu_on) break;
                if (dungeon->cells[dungeon->pc.x][dungeon->pc.y].type != CELL_TYPE_DOWN_STAIRCASE) {
                    snprintf(message, WIDTH, "There isn't a down staircase here.");
                }
                if (fill_and_place_on(dungeon, CELL_TYPE_UP_STAIRCASE, nummon))
                    ERROR_AND_EXIT("failed to generate new dungeon");
                snprintf(message, WIDTH, "You went down the stairs.");
                break;
            case KB_QUIT:
                goto exit;
            default:
                snprintf(message, WIDTH, "Unrecognized command: %c", (char) c);
        }

        if (next_turn_ready) {
            // Take the next turn.
            result = GAME_RESULT_RUNNING;
            was_pc = 0;
            while (!was_pc && result == GAME_RESULT_RUNNING) {
                if (next_turn(dungeon, &result, &was_pc))
                    ERROR_AND_EXIT("failed to take next turn");
            }

            if (result != GAME_RESULT_RUNNING) {
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
                goto exit;
            }

            if (update_pathfinding(dungeon))
                ERROR_AND_EXIT("failed to update monster pathfinding");
        }
    }
    

    exit:
    free(ch);
    free(message);
    endwin();
    return 0;
    exit_err:
    free(ch);
    free(message);
    return 1;
}