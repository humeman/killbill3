#ifndef CHARACTER_H
#define CHARACTER_H

#include <cstdint>
#include "dungeon.h"

#define MONSTER_ATTRIBUTE_INTELLIGENT 0x01
#define MONSTER_ATTRIBUTE_TELEPATHIC 0x02
#define MONSTER_ATTRIBUTE_TUNNELING 0x04
#define MONSTER_ATTRIBUTE_ERRATIC 0x08

typedef enum {
    CHARACTER_TYPE_PC,
    CHARACTER_TYPE_MONSTER
} character_type;

class character_t {
    public:
        char display;
        uint8_t x;
        uint8_t y;
        uint8_t speed;
        uint8_t dead;
        virtual ~character_t() {};

        void move_to(coordinates_t to, character_t ***character_map);
        bool has_los(dungeon_t *dungeon, coordinates_t to);
        virtual character_type type() = 0;
};

// Just serves as a classification only -- no special stuff here (yet)
class pc_t : public character_t {
    public:
        ~pc_t() {};
        character_type type() override;
};

class monster_t : public character_t {
    public:
        ~monster_t() {};
        uint8_t attributes;
        uint8_t pc_seen;
        uint8_t pc_last_seen_x;
        uint8_t pc_last_seen_y;
        coordinates_t next_xy(dungeon_t *dungeon, coordinates_t to);
        character_type type() override;
};

coordinates_t random_location_no_kill(dungeon_t *dungeon, character_t ***character_map);
void place_monster(dungeon_t *dungeon, binary_heap_t *turn_queue, character_t ***character_map, uint8_t attributes);
void generate_monsters(dungeon_t *dungeon, binary_heap_t *turn_queue, character_t ***character_map, int count);
void next_turn(dungeon_t *dungeon, character_t *pc, binary_heap_t *turn_queue, character_t ***character_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, game_result_t *result, bool *was_pc);
void destroy_character(character_t ***character_map, character_t *ch);

#endif