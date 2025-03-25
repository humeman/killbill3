#include "game.h"
#include "macros.h"
#include "dungeon.h"

#include <ncurses.h>

typedef enum colors {
    COLORS_FLOOR,
    COLORS_STONE,
    COLORS_CHARACTER,
    COLORS_OBJECT
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

void run(dungeon *dungeon) {
    char c;
    int x, y;
    initscr();
    if (start_color()) ERROR_AND_EXIT("failed to init ncurses");
    if (init_pair(COLORS_FLOOR, COLOR_BLUE, COLOR_BLACK)) ERROR_AND_EXIT("failed to init ncurses");
    if (init_pair(COLORS_CHARACTER, COLOR_GREEN, COLOR_BLACK)) ERROR_AND_EXIT("failed to init ncurses");
    if (init_pair(COLORS_OBJECT, COLOR_MAGENTA, COLOR_BLACK)) ERROR_AND_EXIT("failed to init ncurses");
    if (init_pair(COLORS_STONE, COLOR_WHITE, COLOR_WHITE)) ERROR_AND_EXIT("failed to init ncurses");

    while (1) {
        // Print out the UI
        move(1, 0);
        for (y = 0; y < HEIGHT - 3; y++) {
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

        refresh();
    }
}

