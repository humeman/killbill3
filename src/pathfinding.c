#include "pathfinding.h"
#include "macros.h"
#include "heap.h"
#include "dungeon.h"

#include <stdio.h>

/**
 * This algorithm is partially based on the pseuducode provided here:
 * https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm#Using_a_priority_queue
 */
int generate_pathfinding_map(dungeon *dungeon, uint32_t **grid, int allow_tunneling) {
    int x, y, i;
    int src_x, src_y;
    binary_heap *queue;
    coordinates coords;
    if (heap_init(&queue, sizeof (coordinates))) RETURN_ERROR("failed to initialize heap");
    src_x = dungeon->pc_x;
    src_y = dungeon->pc_y;

    // Set the source cell to distance 0, add to queue
    coords = (coordinates) {src_x, src_y};
    grid[src_x][src_y] = 0;
    heap_insert(queue, &coords, 0);
    
    // Add every other cell with a distance of infinity
    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            if (x == src_x && y == src_y) continue;
            grid[x][y] = UINT32_MAX;
            coords = (coordinates) {x, y};
            heap_insert(queue, &coords, UINT32_MAX);
        }
    }

}