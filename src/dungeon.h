/**
 * Structures and functions useful for the generation of the dungeon data structure.
 * 
 * Author: csenneff
 */

#ifndef DUNGEON_H
#define DUNGEON_H

#include <cstdint>

#include "heap.h"

// Since these next 3 will be included in the dungeon struct,
// I'm including them here so we don't have a (circular) dependency on character.h.
typedef struct monster {
    uint8_t attributes;
    uint8_t pc_seen;
    uint8_t pc_last_seen_x;
    uint8_t pc_last_seen_y;
} monster_t;

typedef enum {
    CHARACTER_PC,
    CHARACTER_MONSTER
} character_type_t;

typedef struct character {
    char display;
    uint8_t x;
    uint8_t y;
    character_type_t type;
    uint8_t speed;
    monster_t *monster;
    uint8_t dead;
} character_t;
// End of character stuff

// For lack of a better place, this will go here too.
typedef enum {
    GAME_RESULT_RUNNING = 0,
    GAME_RESULT_WIN = 1,
    GAME_RESULT_LOSE = 2
} game_result_t;

#define CELL_TYPES 7
typedef enum {
    CELL_TYPE_STONE,
    CELL_TYPE_ROOM,
    CELL_TYPE_HALL,
    CELL_TYPE_UP_STAIRCASE,
    CELL_TYPE_DOWN_STAIRCASE,
    CELL_TYPE_EMPTY,
    CELL_TYPE_DEBUG
} cell_type_t;

typedef struct {
    uint8_t x0;
    uint8_t y0;
    uint8_t x1;
    uint8_t y1;
} room_t;

typedef struct {
    cell_type_t type;
    uint8_t hardness;
    character_t* character;
    uint8_t attributes;
} cell_t;

typedef enum {
    CELL_ATTRIBUTE_IMMUTABLE = 0x01,
    CELL_ATTRIBUTE_SEEN = 0x02
} cell_attributes_t;

typedef struct {
    uint8_t width;
    uint8_t height;
    uint16_t room_count;
    uint16_t min_room_count;
    uint16_t max_room_count;
    room_t *rooms;
    cell_t **cells;
    uint32_t **pathfinding_no_tunnel;
    uint32_t **pathfinding_tunnel;
    character_t pc;
    binary_heap_t *turn_queue;
} dungeon_t;

typedef struct {
    uint8_t x;
    uint8_t y;
} coordinates_t;

class dungeon_t_new {
    private:
        uint8_t width;
        uint8_t height;
        uint16_t room_count;
        uint16_t min_room_count;
        uint16_t max_room_count;
        room_t *rooms;
        cell_t **cells;
        uint32_t **pathfinding_no_tunnel;
        uint32_t **pathfinding_tunnel;
        character_t pc;
        binary_heap_t *turn_queue;

    public:
        dungeon_t_new(uint8_t width, uint8_t height, int max_rooms);
        ~dungeon_t_new();

        /**
         * Writes a .pgm file for the dungeon hardness map for debugging purposes.
         * The file is saved as "dungeon.pgm".
         */
        void write_pgm();

        /**
         * Fills a dungeon (2D array) with a randomly generated one.
         * 
         * Parameters:
         * - min_rooms: Minimum rooms to generate
         * - room_count_randomness_max: Max number of extra rooms to be randomly added
         * - room_min_width: Minimum room width.
         * - room_min_height: Minimum room height.
         * - room_size_randomness_max: The maximum number that can be randomly 
         *      added to either dimension of the room size.
         * - debug: Enables debug logs/files if 1.
         */
        void fill(int min_rooms, int room_count_randomness_max, int room_min_width, int room_min_height, int room_size_randomness_max, int debug);
       
        /**
         * Picks a random, unobstructred location in a room within a dungeon.
         * 
         * Parameters:
         * - room: Room to find an open space in
         * Returns: The picked coordinates
         */
        coordinates_t random_location_in_room(room_t *room);

        /**
         * Picks a random, unobstructred location in any room within a dungeon.
         * 
         * Returns: The picked coordinates
         */
        coordinates_t random_location();

    private:
        /**
         * Fills the dungeon with randomly-generated stone. Overwrites everything
         * while doing so -- only run on a blank dungeon.
         */
        void fill_stone();

        /**
         * Fills only the outside cell of a dungeon with stone.
         */
        void fill_outside();

        /**
         * Places several random rooms within the dungeon.
         * 
         * Parameters:
         * - min_width: Minimum room width.
         * - min_height: Minimum room height.
         * - size_randomness_max: The maximum number that can be randomly 
         *      added to either dimension of the room size.
         */
        void create_rooms(int count, uint8_t min_width, uint8_t min_height, int size_randomness_max);

        /**
         * Places a single room of a particular width and height somewhere in the dungeon.
         * This will attempt every possible location, returning a non-zero status code if
         * placement fails.
         * 
         * Parameters:
         * - room: Room struct to update with coordinates
         * - room_width: Width of the room
         * - room_height: Height of the room
         */
        void create_room(room_t *room, uint8_t room_width, uint8_t room_height);

        /**
         * Connects every room in the dungeon.
         */
        void connect_rooms();

        /**
         * Connects two points with a semi-random walkway.
         * 
         * Parameters:
         * - x0: X point 0
         * - y0: Y point 0
         * - x1: X point 1
         * - y1: Y point 1
         */
        void connect_points(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

        /**
         * Places an up and down staircase somewhere on the map.
         * They will be placed randomly within the center of a room.
         * There must be at least 2 rooms.
         * 
         * Returns: 0 if successful
         */
        void place_staircases();

        /**
         * Places a material randomly within a room.
         * Avoids overwriting existing non-floor materials, and additionally
         * will not place such that it obstructs a hallway.
         * The provided room must be in the dungeon or an error may be returned.
         * 
         * Parameters:
         * - room: Room to place in
         * - material: Material to place
         * Returns: coordinates to location of placed material
         */
        coordinates_t place_in_room(room_t *room, cell_type_t material);
};

/**
 * Used in the Gaussian diffusion.
 */
class queue_node_t {
    public:
        int x;
        int y;
        queue_node_t *next;

};

#endif