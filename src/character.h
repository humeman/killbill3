#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>

typedef struct monster {
    uint8_t attributes;
} monster;

typedef enum {
    CHARACTER_PC,
    CHARACTER_MONSTER
} character_type;

typedef struct character {
    char display;
    uint8_t x;
    uint8_t y;
    character_type type;
    uint8_t speed;
    monster *monster;
} character;

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

#endif