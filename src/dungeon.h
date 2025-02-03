#ifndef DUNGEON_H
#define DUNGEON_H

#define STONE ' '
#define ROOM '.'
#define HALL '#'
#define UP_STAIRCASE '>'
#define DOWN_STAIRCASE '<'

/**
 * Fills a dungeon (2D array) with a randomly generated one.
 * 
 * Parameters:
 * - width: Width of the dungeon (dungeon[width][height])
 * - height: Height of the dungeon (dungeon[width][height])
 * - dungeon: Dungeon grid to fill
 * 
 * Returns: 0 if successful
 */
int fill_dungeon(int width, int height, char dungeon[width][height]);

/**
 * Fills only the outside cell of a dungeon with stone.
 * 
 * Parameters:
 * - width: Width of the dungeon (dungeon[width][height])
 * - height: Height of the dungeon (dungeon[width][height])
 * - dungeon: Dungeon grid to fill
 */
void fill_outside(int width, int height, char dungeon[width][height]);

/**
 * Places several random rooms within the dungeon.
 * 
 * Parameters:
 * - width: Width of the dungeon (dungeon[width][height])
 * - height: Height of the dungeon (dungeon[width][height])
 * - dungeon: Dungeon grid to fill
 * - count: Number of rooms to place.
 * - min_width: Minimum room width.
 * - min_height: Minimum room height.
 * - size_randomness_max: The maximum number that can be randomly 
 *      added to either dimension of the room size.
 * 
 * Returns: 0 if successful
 */
int create_rooms(int width, int height, char dungeon[width][height], int count, int min_width, int min_height, int size_randomness_max);

/**
 * Places a single room of a particular width and height somewhere in the dungeon.
 * This will attempt every possible location, returning a non-zero status code if
 * placement fails.
 * 
 * Parameters:
 * - width: Width of the dungeon (dungeon[width][height])
 * - height: Height of the dungeon (dungeon[width][height])
 * - dungeon: Dungeon grid to fill
 * - room_width: Width of the room
 * - room_height: Height of the room
 */
int create_room(int width, int height, char dungeon[width][height], int room_width, int room_height);

#endif
