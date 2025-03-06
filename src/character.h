#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>
#include "dungeon.h"

#define MONSTER_ATTRIBUTE_INTELLIGENT 0x01
#define MONSTER_ATTRIBUTE_TELEPATHIC 0x02
#define MONSTER_ATTRIBUTE_TUNNELING 0x04
#define MONSTER_ATTRIBUTE_ERRATIC 0x08

/**
 * Updates the location of a character in the dungeon.
 * Expects **cells and *character.
 */
#define UPDATE_CHARACTER(cells, character, x_, y_) { \
    if (cells[(character)->x][(character)->y].character == character) \
        cells[(character)->x][(character)->y].character = NULL; \
    cells[x_][y_].character = character; \
    (character)->x = x_; \
    (character)->y = y_; \
}

int place_monster(dungeon *dungeon, uint8_t attributes);
int generate_monsters(dungeon *dungeon, int count);
void destroy_character(dungeon *dungeon, character *character);
int has_los(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void next_xy(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *next_x, uint8_t *next_y);
int next_turn(dungeon *dungeon, game_result *result);

#endif