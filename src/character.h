#ifndef CHARACTER_H
#define CHARACTER_H

#include <cinttypes>
#include <cstdint>
#include <string>
#include <ncurses.h>
#include "dungeon.h"
#include "random.h"
#include "item.h"

#define MONSTER_ATTRIBUTE_INTELLIGENT 0x001
#define MONSTER_ATTRIBUTE_TELEPATHIC 0x002
#define MONSTER_ATTRIBUTE_TUNNELING 0x004
#define MONSTER_ATTRIBUTE_ERRATIC 0x008
#define MONSTER_ATTRIBUTE_GHOST 0x010
#define MONSTER_ATTRIBUTE_PICKUP 0x020
#define MONSTER_ATTRIBUTE_DESTROY 0x040
#define MONSTER_ATTRIBUTE_UNIQUE 0x080
#define MONSTER_ATTRIBUTE_BOSS 0x100

typedef enum {
    CHARACTER_TYPE_PC,
    CHARACTER_TYPE_MONSTER
} character_type;

class monster_definition_t {
    public:
        std::string name;
        std::string description;
        int color;
        dice_t *speed;
        int abilities;
        dice_t *hp;
        dice_t *damage;
        char symbol;
        int rarity;
        bool unique_slain = false;
};

void verify_monster_definition(monster_definition_t *def);

/**
 * The base class (abstract) for a dungeon character.
 */
class character_t {
    protected:
        item_t *item = NULL;
        int item_count = 0;

    public:
        char display;
        uint8_t x;
        uint8_t y;
        uint8_t speed;
        bool dead;
        bool location_initialized = false;
        virtual ~character_t() {};

        /**
         * Moves this character to a location.
         *
         * Params:
         * - to: Coordinates to move to
         * - character_map: Map of character pointers to update
         */
        void move_to(coordinates_t to, character_t ***character_map);
        /**
         * Checks if this character has line-of-sight with a coordinate,
         *  defined by a direct straight line to the point that isn't
         *  obstructed by stone.
         *
         * Params:
         * - dungeon: Dungeon containing the cell map
         * - to: Coordinates to check LOS to
         * Returns: True if has LOS
         */
        bool has_los(dungeon_t *dungeon, coordinates_t to);
        /**
         * Gets the type of this character.
         *
         * Returns: CHARACTER_TYPE_PC or CHARACTER_TYPE_MONSTER
         */
        virtual character_type type() = 0;

        int inventory_size();
        void add_to_inventory(item_t *item);
        item_t *remove_from_inventory(int i);
        item_t *remove_inventory_stack();
};

// Just serves as a classification only -- no special stuff here (yet)
class pc_t : public character_t {
    public:
        ~pc_t() {};
        character_type type() override;
};

class monster_t : public character_t {
    private:
        int hp;
        bool pc_seen;
        uint8_t pc_last_seen_x;
        uint8_t pc_last_seen_y;
        uint16_t attributes;
        uint8_t color_i = 0;
        uint8_t color_count;

    public:
        monster_definition_t *definition;
        monster_t(monster_definition_t *definition);
        ~monster_t() {};
        /**
         * Finds the next coordinate to move to on a direct line to a
         *  coordinate pair.
         *
         * Params:
         * - dungeon: Dungeon containing the cell map
         * - to: Coordinates to path to
         * Returns: Next coordinates to move to (if possible),
         *  otherwise the current coordinates
         */
        coordinates_t next_xy(dungeon_t *dungeon, coordinates_t to);
        // A few too many parameters, but it'd be annoying to rework. Oh well.
        void take_turn(dungeon_t *dungeon, character_t *pc, binary_heap_t<character_t *> &turn_queue, character_t ***character_map, item_t ***item_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, uint32_t priority, game_result_t &result);
        void die(game_result_t &result, character_t ***character_map, item_t ***item_map);
        uint8_t next_color();
        character_type type() override;
};

/**
 * Finds a random location in a room or hall in the dungeon that is not
 *  occupied by another character.
 *
 * Params:
 * - dungeon
 * - character_map: Map of character pointers
 */
coordinates_t random_location_no_kill(dungeon_t *dungeon, character_t ***character_map);

/**
 * Places a monster randomly into the character map and turn queue.
 *
 * Params:
 * - dungeon
 * - turn_queue: Character turn binary heap
 * - character_map: Map of character pointers
 * - attributes: The attributes (0-F) to apply to the monster
 */
void place_monster(dungeon_t *dungeon, binary_heap_t<character_t *> &turn_queue, character_t ***character_map, uint8_t attributes);

/**
 * Generates a specified number of random monsters and inserts them
 *  into the character map and turn queue.
 *
 * Params:
 * - dungeon
 * - turn_queue: Character turn binary heap
 * - character_map: Map of character pointers
 * - attributes: The attributes (0-F) to apply to the monster
 * - nummon: Number of monsters to generate
 */
void generate_monsters(dungeon_t *dungeon, binary_heap_t<character_t *> &turn_queue, character_t ***character_map, int count);

/**
 * Takes the turn of the next available character in the turn queue.
 *
 * Params:
 * - dungeon
 * - pc: Pointer to the PC character
 * - turn_queue: Character turn binary heap
 * - character_map: Map of character pointers
 * - pathfinding_tunnel: Pathfinding map to PC with tunneling
 * - pathfinding_no_tunnel: Pathfinding map to PC without tunneling
 * - result: Pointer to a game result, which will be updated to reflect win/lose
 * - was_pc: Pointer that's set to true if the turn just taken was the PC's (NOOP)
 */
void next_turn(dungeon_t *dungeon, character_t *pc, binary_heap_t<character_t *> &turn_queue, character_t ***character_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, game_result_t *result, bool *was_pc);

/**
 * Cleans up the memory for a character and removes it from the character map.
 *
 * Params:
 * - character_map: Map of character pointers
 * - ch: Character to destroy
 */
void destroy_character(character_t ***character_map, character_t *ch);

#endif
