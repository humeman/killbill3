#ifndef CHARACTER_H
#define CHARACTER_H

#include <cinttypes>
#include <cstdint>
#include <string>
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
    CharacterYPE_PC,
    CharacterYPE_MONSTER
} Characterype;

class MonsterDefinition {
    public:
        std::string name;
        std::string description;
        int color;
        Dice *speed;
        int abilities;
        Dice *hp;
        Dice *damage;
        char symbol;
        int rarity;
        bool unique_slain = false;
        std::string floor_texture_n;
        std::string floor_texture_e;
        std::string floor_texture_w;
        std::string floor_texture_s;
        std::string ui_texture;
};

void verify_monster_definition(MonsterDefinition *def);

typedef enum {
    DIRECTION_NORTH,
    DIRECTION_EAST,
    DIRECTION_SOUTH,
    DIRECTION_WEST
} direction_t;

/**
 * The base class (abstract) for a dungeon character.
 */
class Character {
    protected:
        Item *item = NULL;
        int item_count = 0;
        
    public:
        direction_t direction = DIRECTION_NORTH;
        char display;
        uint8_t x;
        uint8_t y;
        uint8_t speed;
        bool dead;
        bool location_initialized = false;
        int hp;
        int base_hp;
        virtual ~Character() {};

        /**
         * Deals damage.
         *
         * Params:
         * - amount: Amount of damage to deal
         * - character_map (modified if dead)
         */
        virtual int damage(int amount, game_result_t &result, Item ***item_map, Character ***character_map) = 0;

        /**
         * Moves this character to a location.
         *
         * Params:
         * - to: Coordinates to move to
         * - character_map: Map of character pointers to update
         */
        void move_to(IntPair to, Character ***character_map);
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
        bool has_los(Dungeon *dungeon, IntPair to);
        /**
         * Gets the type of this character.
         *
         * Returns: CharacterYPE_PC or CharacterYPE_MONSTER
         */
        virtual Characterype type() = 0;

        int inventory_size();
        void add_to_inventory(Item *item);
        Item *remove_from_inventory(int i);
        Item *remove_inventory_stack();
        Item *inventory_at(int i);
};

typedef enum {
    PC_SLOT_WEAPON,
    PC_SLOT_HAT,
    PC_SLOT_SHIRT,
    PC_SLOT_PANTS,
    PC_SLOT_SHOES,
    PC_SLOT_GLASSES,
    PC_SLOT_POCKET_0,
    PC_SLOT_POCKET_1 // Must be the last one for counting
} pc_slot_t;

class PC : public Character {
    public:
        Item *equipment[PC_SLOT_POCKET_1 + 1];
        Dice base_damage = Dice(0, 1, 4);

        PC();
        ~PC() {};
        Characterype type() override;
        int damage(int amount, game_result_t &result, Item ***item_map, Character ***character_map) override;
        int speed_bonus();
        int damage_bonus();
        int dodge_bonus();
        int defense_bonus();
};

class Monster : public Character {
    private:
        bool pc_seen;
        uint8_t pc_last_seen_x;
        uint8_t pc_last_seen_y;
        uint16_t attributes;
        uint8_t color_i = 0;
        uint8_t color_count;

    public:
        MonsterDefinition *definition;
        Monster(MonsterDefinition *definition);
        ~Monster() {};
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
        IntPair next_xy(Dungeon *dungeon, IntPair to);
        // A few too many parameters, but it'd be annoying to rework. Oh well.
        void take_turn(Dungeon *dungeon, PC *pc, BinaryHeap<Character *> &turn_queue, Character ***character_map, Item ***item_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, uint32_t priority, game_result_t &result);
        void die(game_result_t &result, Character ***character_map, Item ***item_map);
        uint8_t next_color();
        uint8_t current_color();
        Characterype type() override;
        int damage(int amount, game_result_t &result, Item ***item_map, Character ***character_map) override;
};

/**
 * Finds a random location in a room or hall in the dungeon that is not
 *  occupied by another character.
 *
 * Params:
 * - dungeon
 * - character_map: Map of character pointers
 */
IntPair random_location_no_kill(Dungeon *dungeon, Character ***character_map);

/**
 * Places a monster randomly into the character map and turn queue.
 *
 * Params:
 * - dungeon
 * - turn_queue: Character turn binary heap
 * - character_map: Map of character pointers
 * - attributes: The attributes (0-F) to apply to the monster
 */
void place_monster(Dungeon *dungeon, BinaryHeap<Character *> &turn_queue, Character ***character_map, uint8_t attributes);

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
void generate_monsters(Dungeon *dungeon, BinaryHeap<Character *> &turn_queue, Character ***character_map, int count);

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
void next_turn(Dungeon *dungeon, Character *pc, BinaryHeap<Character *> &turn_queue, Character ***character_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, game_result_t *result, bool *was_pc);

/**
 * Cleans up the memory for a character and removes it from the character map.
 *
 * Params:
 * - character_map: Map of character pointers
 * - ch: Character to destroy
 */
void destroy_character(Character ***character_map, Character *ch);

#endif
