#include "game.h"
#include "macros.h"
#include "dungeon.h"

#include <string.h>
#include <ncurses.h>
#include <stdlib.h>

typedef enum colors {
    COLORS_FLOOR = 1,
    COLORS_STONE,
    COLORS_CHARACTER,
    COLORS_OBJECT,
    COLORS_TEXT
} colors;

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
    KB_DOWN_STAIRS = '<',
    KB_REST_0 = '5',
    KB_REST_1 = ' ',
    KB_REST_2 = '.',
    KB_MONSTERS = 'm',
    KB_SCROLL_UP = KEY_UP,
    KB_SCROLL_DOWN = KEY_DOWN,
    KB_ESCAPE = 27 // can't find a constant for this???
} colors;

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

// special macro that cleans up ncurses stuff
#define ERROR_AND_EXIT(message) { \
    endwin(); \
    RETURN_ERROR(message); \
}

int game_start(dungeon *dungeon) {
    int c;
    int x, y;
    char *message = malloc(1 + WIDTH);
    if (!message) RETURN_ERROR("failed to allocate memory");
    message[0] = '\0';
    initscr();
    if (start_color() != OK) ERROR_AND_EXIT("failed to init ncurses (no color support)");
    if (init_pair(COLORS_FLOOR, COLOR_BLUE, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_CHARACTER, COLOR_GREEN, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_OBJECT, COLOR_MAGENTA, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_STONE, COLOR_BLACK, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (init_pair(COLORS_TEXT, COLOR_WHITE, COLOR_BLACK) != OK) ERROR_AND_EXIT("failed to init ncurses (init color)");
    if (keypad(stdscr, TRUE) != OK) ERROR_AND_EXIT("failed to init ncurses (special kb keys)");
                    // man this library is weird
    if (curs_set(0) == ERR) ERROR_AND_EXIT("failed to init ncurses (disable cursor)");

    strncpy(message, "Hello world!", WIDTH);
    while (1) {
        // Print out the UI
        for (y = 0; y < HEIGHT - 3; y++) {
            move(y + 1, 0);
            for (x = 0; x < WIDTH; x++) {
                if (dungeon->cells[x][y].character) {
                    attrset(COLOR_PAIR(COLORS_CHARACTER | A_BOLD));
                    addch(dungeon->cells[x][y].character->display);
                    attroff(COLOR_PAIR(COLORS_CHARACTER | A_BOLD));
                } else {
                    attrset(COLOR_PAIR(COLORS_BY_CELL_TYPE[dungeon->cells[x][y].type]));
                    addch(CHARACTERS_BY_CELL_TYPE[dungeon->cells[x][y].type]);
                    attroff(COLOR_PAIR(COLORS_BY_CELL_TYPE[dungeon->cells[x][y].type]));
                }
            }
        } 

        // Print a centered message
        x = ((WIDTH - 1) / 2) - (strlen(message) / 2) - 1;

        move(0, x);
        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        printw(" %s ", message);
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);

        refresh();

        // Read the next command
        c = getch();
        switch (c) {
            case KB_ESCAPE:
                break;
        }
    }

    endwin();
    return 0;
}