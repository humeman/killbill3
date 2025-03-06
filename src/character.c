#include "character.h"
#include "dungeon.h"
#include "macros.h"
#include "pathfinding.h"
#include "heap.h"

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int place_monster(dungeon *dungeon, uint8_t attributes) {
    uint8_t x, y;
    character *character;
    monster *monster;
    character = malloc(sizeof (*character));
    if (!character) RETURN_ERROR("failed to allocate character");
    monster = malloc(sizeof (*monster));
    if (!monster) {
        free(character);
        RETURN_ERROR("failed to allocate monster");
    }
    character->monster = monster;
    if (random_location(dungeon, &x, &y)) {
        free(character);
        free(monster);
        RETURN_ERROR("no available space to place monster")
    }
    character->x = x;
    character->y = y;
    character->speed = (rand() % 16) + 5;
    character->dead = 0;
    character->type = CHARACTER_MONSTER;
    monster->attributes = attributes;
    character->display = monster->attributes < 10 ? '0' + monster->attributes : 'a' + (monster->attributes - 10);
    monster->pc_seen = 0;

    dungeon->cells[x][y].character = character;
    if (heap_insert(dungeon->turn_queue, (void *) &character, character->speed)) {
        free(monster);
        free(character);
        RETURN_ERROR("failed to insert monster into turn queue");
    }

    return 0;
}

int generate_monsters(dungeon *dungeon, int count) {
    int i;
    uint8_t attributes;
    character *character;
    uint8_t pc_removed;
    uint32_t pc_priority;
    uint32_t priority;
    for (i = 0; i < count; i++) {
        attributes = 0;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_INTELLIGENT;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_TELEPATHIC;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_TUNNELING;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_ERRATIC;
        if (place_monster(dungeon, attributes)) {
            pc_removed = 0;
            while (heap_size(dungeon->turn_queue) != 0) {
                if (heap_remove(dungeon->turn_queue, (void *) &character, &priority)) RETURN_ERROR("catastrophe: failed to remove from heap while cleaning up an error");
                if (character == &(dungeon->pc)) {
                    pc_removed = 1;
                    pc_priority = priority;
                }
                else {
                    destroy_character(dungeon, character);
                }
            }
            if (pc_removed && heap_insert(dungeon->turn_queue, (void *) &(dungeon->pc), pc_priority)) RETURN_ERROR("catastrophe: failed to reinsert PC into heap while cleaning up an error");
            RETURN_ERROR("failed to generate monsters");
        }
    }

    return 0;
}

void destroy_character(dungeon *dungeon, character *character) {
    if (dungeon->cells[character->x][character->y].character == character)
        dungeon->cells[character->x][character->y].character = NULL;
    if (character->monster) free(character->monster);
    free(character);
}

int has_los(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    // Regular line equation (rounded to grid points).
    // The special case is if we have a slope > 1 or < -1 -- in that case we can iterate over Y instead of X.
    // Don't bother if the points are equal
    if (x0 == x1 && y0 == y1) return 1;
    int x_diff = x1 - x0;
    int y_diff = y1 - y0;
    // y = mx + b
    float m, b; 
    if (x_diff != 0) m = y_diff / (float) x_diff;
    uint8_t x = x0;
    uint8_t y = y0;
    int dir;

    if ((x_diff != 0) && m >= -1 && m <= 1) {
        // Regular case, iterate over X.
        b = y0 - m * x0;
        dir = (x_diff > 0 ? 1 : -1);
        for (x = x0; x != x1 + dir; x += dir) {
            y = (uint8_t) round(m * x + b);
            if (dungeon->cells[x][y].type == CELL_TYPE_STONE) return 0;
        }
    }
    else {
        // Vertical line, iterate over Y.
        // Adjust our slope as if the Y axis is X.
        // This allows us to deal with infinite/NaN slopes.
        // x = my + b
        if (x_diff == 0) {
            // Infinite slope becomes a 0 slope here.
            m = 0;
        } else {
            m = 1 / m;
        }
        b = x0 - m * y0;
        dir = (y_diff > 0 ? 1 : -1);
        for (y = y0; y != y1 + dir; y += dir) {
            x = (uint8_t) round(m * y + b);
            if (dungeon->cells[x][y].type == CELL_TYPE_STONE) return 0;
        }
    }
    return 1;
}

void next_xy(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *next_x, uint8_t *next_y) {
    // Butchered version of has_los.
    *next_x = x0;
    *next_y = y0;
    if (x0 == x1 && y0 == y1) return;
    int x_diff = x1 - x0;
    int y_diff = y1 - y0;
    float m, b; 
    if (x_diff != 0) m = y_diff / (float) x_diff;
    int dir;

    if ((x_diff != 0) && m >= -1 && m <= 1) {
        // Regular case, iterate over X.
        b = y0 - m * x0;
        dir = (x_diff > 0 ? 1 : -1);
        *next_x = x0 + dir;
        *next_y = (uint8_t) round(m * *next_x + b);
    }
    else {
        // Vertical line, iterate over Y.
        // Adjust our slope as if the Y axis is X.
        // This allows us to deal with infinite/NaN slopes.
        // x = my + b
        if (x_diff == 0) {
            // Infinite slope becomes a 0 slope here.
            m = 0;
        } else {
            m = 1 / m;
        }
        b = x0 - m * y0;
        dir = (y_diff > 0 ? 1 : -1);
        *next_y = y0 + dir;
        *next_x = (uint8_t) round(m * *next_y + b);
    }
}

int next_turn(dungeon *dungeon, game_result *result) {
    character *character = NULL;
    uint32_t priority;
    uint8_t x, y, next_x, next_y, min, x_offset, y_offset;
    int i, j, x1, y1;
    uint32_t** map;
    while (heap_size(dungeon->turn_queue) > 0 && character == NULL) {
        if (heap_remove(dungeon->turn_queue, (void*) &character, &priority)) RETURN_ERROR("failed to remove from top of turn queue");
        // The dead flag here avoids us having to remove from the heap at an arbitrary location.
        // We just destroy it when it comes off the queue next.
        if (character->dead) {
            if (character != &(dungeon->pc))
                destroy_character(dungeon, character);
            character = NULL;
        }
    }
    if (character == NULL || (character == &(dungeon->pc) && heap_size(dungeon->turn_queue) == 0)) {
        // Everyone is dead. Game over.
        *result = GAME_RESULT_WIN;
        return 0;
    }
    
    x = character->x;
    y = character->y;

    // PC movement (it'll randomly move around open space)
    if (character == &(dungeon->pc)) {
        // Move around randomly
        x_offset = rand();
        y_offset = rand();
        for (i = 0; i < 3; i++) {
            x1 = dungeon->pc.x - 1 + (i + x_offset) % 3;
            for (j = 0; j < 3; j++) {
                y1 = dungeon->pc.y - 1 + (j + y_offset) % 3;
                if (x1 == x && y1 == y) continue;
                if (x1 < 0 || x1 >= dungeon->width || y1 < 0 || y1 >= dungeon->height) continue;
                if (dungeon->cells[x1][y1].type == CELL_TYPE_STONE) continue;
                // Try to move there
                if (dungeon->cells[x1][y1].character) {
                    dungeon->cells[x1][y1].character->dead = 1;
                }
                UPDATE_CHARACTER(dungeon->cells, character, x1, y1);
                if (update_pathfinding(dungeon)) RETURN_ERROR("failed to update pathfinding maps");
                heap_insert(dungeon->turn_queue, (void*) &character, priority + character->speed);
                return 0;
            }
        }
        heap_insert(dungeon->turn_queue, (void*) &character, priority + character->speed);
        return 0; // No open space (impossible with a normal map)
    }

    // Find out which direction this monster wants to go.
    // - Telepathic: Directly to the PC
    // - Intelligent: Towards the last seen location
    // - None: Only towards the PC if there's LOS

    // Slightly inefficient but I prefer the readability since this algorithm is a bit more complex.
    // 1: Determine if the monster is allowed to go to the PC.
    uint8_t can_move = 0;
    uint8_t target_x, target_y;
    //    a: If it's telepathic, it can.
    if (character->monster->attributes & MONSTER_ATTRIBUTE_TELEPATHIC) {
        can_move = 1;
        target_x = dungeon->pc.x;
        target_y = dungeon->pc.y;
    }
    //    b: If it has line of sight, it can.
    else if (has_los(dungeon, x, y, dungeon->pc.x, dungeon->pc.y)) {
        can_move = 1;
        target_x = dungeon->pc.x;
        target_y = dungeon->pc.y;

        // For future use, we can also mark that the PC was seen.
        character->monster->pc_seen = 1;
        character->monster->pc_last_seen_x = dungeon->pc.x;
        character->monster->pc_last_seen_y = dungeon->pc.y;
    }
    //    c: If it has a last seen location in mind, it can.
    else if (character->monster->pc_seen)  {
        can_move = 1;
        target_x = character->monster->pc_last_seen_x;
        target_y = character->monster->pc_last_seen_y;
    }

    // 2: If we can move, find out the cell we'll move to next.
    if (can_move) {
        // If the monster's intelligent, follow the shortest path to the destination.
        if (character->monster->attributes & MONSTER_ATTRIBUTE_INTELLIGENT) {
            // We can use the pathfinding maps to go to the PC
            if (character->monster->attributes & MONSTER_ATTRIBUTE_TUNNELING)
                map = dungeon->pathfinding_tunnel;
            else
                map = dungeon->pathfinding_no_tunnel;

            // Pick the best direction
            min = UINT8_MAX;
            for (x1 = x - 1; x1 <= x + 1; x1++) {
                for (y1 = y - 1; y1 <= y + 1; y1++) {
                    if (x1 == x && y1 == y) continue;
                    if (x < 0 || x >= dungeon->width || y < 0 || y >= dungeon->height) continue;
                    if (map[x1][y1] == UINT32_MAX) continue;
                    // Find the minimum while preferring non-stone cells.
                    if (map[x1][y1] < min || (map[x1][y1] == min && dungeon->cells[x1][y1].type != CELL_TYPE_STONE)) {
                        min = map[x1][y1];
                        next_x = x1;
                        next_y = y1;
                    }
                }
            }

            // There is a possibility we can't move in any direction if we generate a bad dungeon.
            if (min == UINT8_MAX) {
                can_move = 0;
            }
        }

        // Otherwise, we'll go in a straight line.
        else {
            next_xy(dungeon, x, y, target_x, target_y, &next_x, &next_y);
            // Can't if it's non-tunneling and going towards stone.
            if (!(character->monster->attributes & MONSTER_ATTRIBUTE_TUNNELING) && dungeon->cells[next_x][next_y].type == CELL_TYPE_STONE) {
                can_move = 0;
            }
        }
    }

    // And, of course, there's the possibility that the monster is erratic and will move randomly.
    if ((character->monster->attributes & MONSTER_ATTRIBUTE_ERRATIC) && rand() % 2 == 1) {
        // Pick a random available cell around the monster.
        x_offset = rand();
        y_offset = rand();
        can_move = 0;
        for (i = 0; i < 3; i++) {
            x1 = x - 1 + (i + x_offset) % 3;
            if (x1 < 0 || x1 >= dungeon->width) continue;
            for (j = 0; j < 3; j++) {
                y1 = y - 1 + (j + y_offset) % 3;
                if (y1 < 0 || y1 >= dungeon->height) continue;
                if (x1 == x && y1 == y) continue;
                if (dungeon->cells[x1][y1].type == CELL_TYPE_STONE && !(character->monster->attributes & MONSTER_ATTRIBUTE_TUNNELING)) continue;
                // This cell is open
                next_x = x1;
                next_y = y1;
                can_move = 1;
                break;
            }
            if (can_move) break;
        }
    }

    // 3: Time to move!
    cell* next;
    if (can_move && (next_x != x || next_y != y)) {
        // In the special case of non-telepathic monsters, we want to clear out the PC seen flag once we reach
        // its last known location.
        if (character->monster->pc_seen && character->monster->pc_last_seen_x == next_x && character->monster->pc_last_seen_y == next_y)
            character->monster->pc_seen = 0;

        // Our next coordinates are in next_x and next_y.
        // If that's open space, just go there.
        next = &(dungeon->cells[next_x][next_y]);
        if (next->type != CELL_TYPE_STONE) {
            // If there's already a character there, kill it.
            if (next->character != NULL) {
                next->character->dead = 1;
                // Special case is the PC, in which case the game ends.
                if (next->character->type == CHARACTER_PC) {
                    UPDATE_CHARACTER(dungeon->cells, character, next_x, next_y);
                    *result = GAME_RESULT_LOSE;
                    heap_insert(dungeon->turn_queue, &character, priority + character->speed); // So this character gets free'd
                    return 0;
                }
            }
            UPDATE_CHARACTER(dungeon->cells, character, next_x, next_y);
        }
        else {
            // Each iteration is minus 85 hardness.
            next->hardness -= MIN(next->hardness, 85);
            if (next->hardness == 0) {
                next->type = CELL_TYPE_HALL;
                UPDATE_CHARACTER(dungeon->cells, character, next_x, next_y);
            }
        }
    }

    if (heap_insert(dungeon->turn_queue, &character, priority + character->speed))
        RETURN_ERROR("failed to re-insert character into turn queue");
    return 0;
}