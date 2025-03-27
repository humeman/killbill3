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
#define UPDATE_CHARACTER(cells, ch, x_, y_) { \
    if (cells[(ch)->x][(ch)->y].character == ch) \
        cells[(ch)->x][(ch)->y].character = NULL; \
    cells[x_][y_].character = ch; \
    (ch)->x = x_; \
    (ch)->y = y_; \
}

int place_monster(dungeon_t *dungeon, uint8_t attributes);
int generate_monsters(dungeon_t *dungeon, int count);
void destroy_character(dungeon_t *dungeon, character_t *ch);
int has_los(dungeon_t *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void next_xy(dungeon_t *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *next_x, uint8_t *next_y);
int next_turn(dungeon_t *dungeon, game_result_t *result, int *was_pc);

#endif