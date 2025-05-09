/**
 * Functions for running Dijkstra's on a dungeon to find optimal paths to the PC.
 * 
 * Author: csenneff
 */

#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "dungeon.h"
#include "character.h"

/**
 * Updates both pathfinding maps for the specified dungeon.
 * 
 * Params:
 *  - dungeon: The dungeon to generate the pathfinding maps for.
 */
void update_pathfinding(Dungeon *dungeon, uint32_t **pathfinding_no_tunnel, uint32_t **pathfinding_tunnel, IntPair loc);

/**
 * Generates a pathfinding map for the specified dungeon and writes it
 * to the specified grid pointer as a 2D array.
 * 
 * Params:
 *  - dungeon: The dungeon to generate the pathfinding map for.
 *  - grid: A pointer to a pointer of ints where the grid will be written, [x][y].
 *      Must already be allocated as a 2D array of ints using the dungeon's size.
 *  - allow_tunneling: If non-zero, generates a map allowing tunneling through rock.
 *  - pc: A pointer to the coordinates of the PC (destination).
 */
void generate_pathfinding_map(Dungeon *dungeon, uint32_t **grid, int allow_tunneling, IntPair loc);

#endif