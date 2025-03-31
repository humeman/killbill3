#include <cstdio>

#include "pathfinding.h"
#include "macros.h"
#include "heap.h"
#include "dungeon.h"
#include "character.h"
#include <stdexcept>

#define HARDNESS_OF(hardness) (hardness == 0 ? 1 : 1 + (hardness / 85))

/**
 * A comparator function (for use in a heap) which evaluates the equality of two coordinates,
 * returning 0 if equivalent and 1 otherwise.
 */
bool compare_coords(void *a, void *b) {
    coordinates_t *ca = (coordinates_t *) a;
    coordinates_t *cb = (coordinates_t *) b;
    return !(ca->x == cb->x && ca->y == cb->y);
}

void update_pathfinding(dungeon_t *dungeon, uint32_t **pathfinding_no_tunnel, uint32_t **pathfinding_tunnel, character_t *pc) {
    generate_pathfinding_map(dungeon, pathfinding_no_tunnel, 0, pc);
    generate_pathfinding_map(dungeon, pathfinding_tunnel, 1, pc);
}

/**
 * This algorithm is partially based on the pseuducode provided here:
 * https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm#Using_a_priority_queue
 */
void generate_pathfinding_map(dungeon_t *dungeon, uint32_t **grid, int allow_tunneling, character_t *pc) {
    uint8_t x, y, x1, y1;
    uint8_t src_x, src_y;
    uint32_t distance;
    coordinates_t coords;
    int done[dungeon->width][dungeon->height];
    binary_heap_t queue(sizeof (coordinates_t), compare_coords);
    src_x = pc->x;
    src_y = pc->y;

    // Set the source cell to distance 0, add to queue
    grid[src_x][src_y] = 0;
    coords = (coordinates_t) {src_x, src_y};
    queue.insert(&coords, 0);
    
    // Add every other cell with a distance of infinity
    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            if (x == src_x && y == src_y) continue;
            grid[x][y] = UINT32_MAX;
            if (
                (!allow_tunneling && dungeon->cells[x][y].type == CELL_TYPE_STONE)
                || (dungeon->cells[x][y].hardness == UINT8_MAX)) {
                    done[x][y] = 1; // to signal not to attempt any checks against this cell
            } else {
                done[x][y] = 0;
                coords = (coordinates_t) {x, y};
                queue.insert(&coords, UINT32_MAX);
            }
        }
    }

    while (queue.size() != 0) {
        // Extract the minimal cell
        queue.remove((void*) &coords);
        x = coords.x;
        y = coords.y;
        done[x][y] = 1;
        // Iterate over the neighbors of that cell
        for (x1 = x - 1; x1 <= x + 1; x1++) {
            for (y1 = y - 1; y1 <= y + 1; y1++) {
                if (x1 == x && y1 == y) continue; // don't double process the current cell
                if (x1 < 0 || x1 >= dungeon->width || y1 < 0 || y1 >= dungeon->height) continue; // don't process out of bounds
                if (done[x1][y1]) continue; // don't process completed cells
                if (grid[x][y] == UINT32_MAX) continue; // don't process unreachable cells
                // Calculate the distance to this cell
                // We would expect that this would be grid[x][y] + hardness(x1, y1),
                // but we're calculating the distances from the neighbor to the destination
                // cell (that's how the monsters will be travelling). So, instead, we do
                // grid[x][y] + hardness(x, y), which matches the sample dungeons.
                distance = grid[x][y] + HARDNESS_OF(dungeon->cells[x][y].hardness);
                if (distance < grid[x1][y1]) {
                    grid[x1][y1] = distance;
                    coords = (coordinates_t) {x1, y1};
                    queue.decrease_priority(&coords, distance);
                }
            }
        }
    }
}