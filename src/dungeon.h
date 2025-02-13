#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdint.h>

typedef enum {
    CELL_TYPE_STONE = ' ',
    CELL_TYPE_ROOM = '.',
    CELL_TYPE_HALL = '#',
    CELL_TYPE_UP_STAIRCASE = '<',
    CELL_TYPE_DOWN_STAIRCASE = '>',
    CELL_TYPE_EMPTY = '!',
    CELL_TYPE_PC = '@'
} cell_type;

typedef struct room {
    uint8_t x0;
    uint8_t y0;
    uint8_t x1;
    uint8_t y1;
} room;

typedef struct cell {
    cell_type type;
    uint8_t hardness;
    uint8_t mutable;
} cell;

typedef struct dungeon {
    uint8_t width;
    uint8_t height;
    uint16_t room_count;
    uint16_t min_room_count;
    uint16_t max_room_count;
    uint8_t pc_x;
    uint8_t pc_y;
    room *rooms;
    cell **cells;
} dungeon;

typedef struct coordinates {
    uint8_t x;
    uint8_t y;
} coordinates;

/**
 * Initializes a dungeon data structure. It is an error to use
 *  any of the below functions with an uninitialized dungeon, and
 *  doing so will very likely cause a segmentation fault.
 * 
 * Parameters:
 * - dungeon: Pointer to dungeon data structure
 * - width: Width of dungeon to allocate
 * - height: Height of dungeon to allocate
 * - max_rooms: Number of rooms to allocate
 */
int dungeon_init(dungeon *dungeon, int width, int height, int max_rooms);

/**
 * Destroys a dungeon instance and frees any memory associated with it.
 * Failing to call this method when exiting the program will leak memory.
 * 
 * Parameters:
 * - dungeon: Pointer to dungeon data structure
 */
void dungeon_destroy(dungeon *dungeon);

/**
 * Writes a .pgm file for the dungeon hardness map for debugging purposes.
 * The file is saved as "dungeon.pgm".
 * 
 * Params:
 * - dungeon: Dungeon to write
 */
void write_dungeon_pgm(dungeon *dungeon);

/**
 * Fills a dungeon (2D array) with a randomly generated one.
 * 
 * Parameters:
 * - dungeon: Dungeon to fill
 * - min_rooms: Minimum rooms to generate
 * - room_count_randomness_max: Max number of extra rooms to be randomly added
 * - room_min_width: Minimum room width.
 * - room_min_height: Minimum room height.
 * - room_size_randomness_max: The maximum number that can be randomly 
 *      added to either dimension of the room size.
 * - debug: Enables debug logs/files if 1.
 * 
 * Returns: 0 if successful
 */
int fill_dungeon(dungeon *dungeon, int min_rooms, int room_count_randomness_max, int room_min_width, int room_min_height, int room_size_randomness_max, int debug);

/**
 * Fills the dungeon with randomly-generated stone. Overwrites everything
 * while doing so -- only run on a blank dungeon.
 * 
 * Params:
 * - dungeon: Dungeon to fill.
 */
int fill_stone(dungeon *dungeon);

/**
 * Fills only the outside cell of a dungeon with stone.
 * 
 * Parameters:
 * - dungeon: Dungeon to fill
 */
void fill_outside(dungeon *dungeon);

/**
 * Places several random rooms within the dungeon.
 * 
 * Parameters:
 * - dungeon: Dungeon to fill
 * - min_width: Minimum room width.
 * - min_height: Minimum room height.
 * - size_randomness_max: The maximum number that can be randomly 
 *      added to either dimension of the room size.
 * 
 * Returns: 0 if successful
 */
int create_rooms(dungeon *dungeon, int count, int min_width, int min_height, int size_randomness_max);

/**
 * Places a single room of a particular width and height somewhere in the dungeon.
 * This will attempt every possible location, returning a non-zero status code if
 * placement fails.
 * 
 * Parameters:
 * - dungeon: Dungeon to fill
 * - room: Room struct to update with coordinates
 * - room_width: Width of the room
 * - room_height: Height of the room
 * Returns: 0 if successful
 */
int create_room(dungeon *dungeon, room *room, int room_width, int room_height);

/**
 * Connects every room in the dungeon.
 * 
 * Parameters:
 * - dungeon: Dungeon to connect
 * Returns: 0 if successful
 */
int connect_rooms(dungeon *dungeon);

/**
 * Connects two points with a semi-random walkway.
 * 
 * Parameters:
 * - dungeon: Dungeon to modify
 * - x0: X point 0
 * - y0: Y point 0
 * - x1: X point 1
 * - y1: Y point 1
 * Returns: 0 if successful
 */
int connect_points(dungeon *dungeon, int x0, int y0, int x1, int y1);

/**
 * Places an up and down staircase somewhere on the map.
 * They will be placed randomly within the center of a room.
 * There must be at least 2 rooms.
 * 
 * Parameters:
 * - dungeon: Dungeon to modify
 * Returns: 0 if successful
 */
int place_staircases(dungeon *dungeon);

/**
 * Places a material randomly within a room.
 * Avoids overwriting existing non-floor materials, and additionally
 * will not place such that it obstructs a hallway.
 * The provided room must be in the dungeon or an error may be returned.
 * 
 * Parameters:
 * - dungeon: Dungeon grid to fill
 * - room: Room to place in
 * - material: Material to place
 * - x_loc: Pointer to variable to store placed X coordinate in
 * - y_loc: Pointer to variable to store placed Y coordinate in
 * Returns: 0 if successful
 */
int place_in_room(dungeon *dungeon, room room, cell_type material, int *x_loc, int *y_loc);

#endif