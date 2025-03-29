/**
 * Structures and functions useful for the generation of the dungeon data structure.
 * 
 * Author: csenneff
 */

#ifndef DUNGEON_H
#define DUNGEON_H

#include <cstdint>
#include <cstdio>

#include "heap.h"


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

class room_t {
    public:
        uint8_t x0;
        uint8_t y0;
        uint8_t x1;
        uint8_t y1;

};

class cell_t {
    public:
        cell_type_t type;
        uint8_t hardness;
        uint8_t attributes;

};

typedef enum {
    CELL_ATTRIBUTE_IMMUTABLE = 0x01,
    CELL_ATTRIBUTE_SEEN = 0x02
} cell_attributes_t;

class queue_node_t {
    public:
        int x;
        int y;
        queue_node_t *next;

};

class coordinates_t {
    public:
        uint8_t x;
        uint8_t y;

};

class dungeon_t {
    public:
        uint8_t width;
        uint8_t height;
        uint16_t room_count;
        uint16_t min_room_count;
        uint16_t max_room_count;
        room_t *rooms;
        cell_t **cells;

        dungeon_t(uint8_t width, uint8_t height, int max_rooms);
        ~dungeon_t();

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

        void fill_from_file(FILE *f, int debug, coordinates_t *pc_coords);

        void save_to_file(FILE *f, int debug, coordinates_t *pc_coords);

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

#endif