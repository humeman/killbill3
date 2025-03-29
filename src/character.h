#ifndef CHARACTER_H
#define CHARACTER_H

#include <cstdint>
#include "dungeon.h"

#define MONSTER_ATTRIBUTE_INTELLIGENT 0x01
#define MONSTER_ATTRIBUTE_TELEPATHIC 0x02
#define MONSTER_ATTRIBUTE_TUNNELING 0x04
#define MONSTER_ATTRIBUTE_ERRATIC 0x08

/**
 * Updates the location of a character in the dungeon.
 * Expects **character_map and *character.
 */
#define UPDATE_CHARACTER(character_map, ch, x_, y_) { \
    if (character_map[(ch)->x][(ch)->y] == ch) \
        character_map[(ch)->x][(ch)->y] = NULL; \
    character_map[x_][y_] = ch; \
    (ch)->x = x_; \
    (ch)->y = y_; \
}

class monster_t {
    public:
        uint8_t attributes;
        uint8_t pc_seen;
        uint8_t pc_last_seen_x;
        uint8_t pc_last_seen_y;
};

typedef enum {
    CHARACTER_PC,
    CHARACTER_MONSTER
} character_type_t;

class character_t {
    public:
        char display;
        uint8_t x;
        uint8_t y;
        character_type_t type;
        uint8_t speed;
        monster_t *monster;
        uint8_t dead;
};

void place_monster(dungeon_t *dungeon, binary_heap_t *turn_queue, character_t ***character_map, uint8_t attributes);
void generate_monsters(dungeon_t *dungeon, binary_heap_t *turn_queue, character_t ***character_map, int count);
void destroy_character(dungeon_t *dungeon, character_t ***character_map, character_t *ch);
bool has_los(dungeon_t *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void next_xy(dungeon_t *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *next_x, uint8_t *next_y);
void next_turn(dungeon_t *dungeon, character_t *pc, binary_heap_t *turn_queue, character_t ***character_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, game_result_t *result, int *was_pc);

#endif