#include "pathfinding.h"
#include "macros.h"
#include "heap.h"
#include "dungeon.h"

#include <stdio.h>

#define hardness_of(hardness) { \
    typeof (hardness) h = (hardness); \
    h == 0 ? 1 : (h == 255 ? UINT32_MAX : 1 + (h / 85)); \
}

/**
 * A comparator function (for use in a heap) which evaluates the equality of two coordinates,
 * returning 0 if equivalent and 1 otherwise.
 */
int compare_coords(void *a, void *b) {
    coordinates *ca = (coordinates *) a;
    coordinates *cb = (coordinates *) b;
    return !(ca->x == cb->x && ca->y == cb->y);
}

int update_pathfinding(dungeon *dungeon) {
    if (generate_pathfinding_map(dungeon, dungeon->pathfinding_no_tunnel, 0)) RETURN_ERROR("failed to generate no-tunneling pathfinding map");
    if (generate_pathfinding_map(dungeon, dungeon->pathfinding_tunnel, 1)) RETURN_ERROR("failed to generate no-tunneling pathfinding map");
}

/**
 * This algorithm is partially based on the pseuducode provided here:
 * https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm#Using_a_priority_queue
 */
int generate_pathfinding_map(dungeon *dungeon, uint32_t **grid, int allow_tunneling) {
    int x, y, x1, y1, i;
    int src_x, src_y;
    uint32_t distance;
    binary_heap *queue;
    coordinates coords;
    if (heap_init(&queue, sizeof (coordinates))) RETURN_ERROR("failed to initialize heap");
    src_x = dungeon->pc_x;
    src_y = dungeon->pc_y;

    // Set the source cell to distance 0, add to queue
    coords = (coordinates) {src_x, src_y};
    grid[src_x][src_y] = 0;
    if (heap_insert(queue, &coords, 0)) RETURN_ERROR("failed to insert into heap");
    
    // Add every other cell with a distance of infinity
    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            if (x == src_x && y == src_y) continue;
            grid[x][y] = UINT32_MAX;
            coords = (coordinates) {x, y};
            if (heap_insert(queue, &coords, UINT32_MAX)) RETURN_ERROR("failed to insert into heap");
        }
    }

    while (heap_size(queue) != 0) {
        // Extract the minimal cell
        if (heap_remove(queue, (void**) &coords)) RETURN_ERROR("failed to extract top from heap");
        x = coords.x;
        y = coords.y;
        // Iterate over the neighbors of that cell
        for (x1 = x - 1; x1 <= x + 1; x1++) {
            for (y1 = y - 1; y1 <= y + 1; y1++) {
                if (x1 == x && y1 == y) continue; // don't double process the current cell
                if (x1 < 0 || x1 >= dungeon->width || y1 < 0 || y1 >= dungeon->height) continue; // don't process out of bounds
                // Depending on whether or not we're allowing tunneling here, exclude rock cells
                if (!allow_tunneling && dungeon->cells[x1][y1].type == CELL_TYPE_STONE) continue;
                // Calculate the distance to this cell
                distance = grid[x][y] + harndess_of(dungeon->cells[x1][y1].hardness);
                if (distance < grid[x1][y1]) {
                    grid[x1][y1] = distance;
                    coords = (coordinates) {x1, y1};
                    if (heap_decrease_priority(queue, compare_coords, (void *) &coords, distance)) RETURN_ERROR("failed to decrease heap priority");
                }
            }
        }
    }
    heap_destroy(queue);
    return 0;
}