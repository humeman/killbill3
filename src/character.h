#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>
#include "dungeon.h"

/**
 * Updates the location of a character in the dungeon.
 * Expects **cells and *character.
 */
#define UPDATE_CHARACTER(cells, character, x, y) { \
    if (cells[(character)->x][(character)->y].character == character) \
        cells[(character)->x][(character)->y].character = NULL; \
    cells[x][y].character = character; \
    (character)->x = x; \
    (character)->y = y; \
}

int generate_monsters(dungeon *dungeon, int count);
void destroy_character(dungeon *dungeon, character *character);
int has_los(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void next_xy(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *next_x, uint8_t *next_y);

#endif