#include "character.h"
#include "dungeon.h"
#include "macros.h"
#include "pathfinding.h"
#include "heap.h"

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

#define MAX_ATTEMPTS 2 * DUNGEON_WIDTH * DUNGEON_HEIGHT

coordinates_t random_location_no_kill(dungeon_t *dungeon, character_t ***character_map) {
    int i;
    coordinates_t coords;
    for (i = 0; i < MAX_ATTEMPTS; i++) {
        try {
            coords = dungeon->random_location();
            if (character_map[coords.x][coords.y]) continue;
            return coords;
        } catch (dungeon_exception &e) {}
    }
    throw dungeon_exception(__PRETTY_FUNCTION__, "no available space for a new monster in dungeon");
}

void place_monster(dungeon_t *dungeon, binary_heap_t *turn_queue, character_t ***character_map, uint8_t attributes) {
    monster_t *monster = new monster_t;
    coordinates_t coords;
    try {
        coords = random_location_no_kill(dungeon, character_map);
    } catch (dungeon_exception &e) {
        delete monster;
        throw dungeon_exception(__PRETTY_FUNCTION__, e);
    }
    monster->x = coords.x;
    monster->y = coords.y;
    monster->speed = (rand() % 16) + 5;
    monster->dead = 0;
    monster->attributes = attributes;
    monster->display = monster->attributes < 10 ? '0' + monster->attributes : 'a' + (monster->attributes - 10);
    monster->pc_seen = 0;

    character_map[coords.x][coords.y] = monster;
    try {
        turn_queue->insert((void *) &monster, monster->speed);
     } catch (dungeon_exception &e) {
        delete monster;
        throw dungeon_exception(__PRETTY_FUNCTION__, e, "failed to insert monster into turn queue");
    }
}

void generate_monsters(dungeon_t *dungeon, binary_heap_t *turn_queue, character_t ***character_map, int count) {
    int i;
    uint8_t attributes;
    character_t *ch;
    character_t *pc;
    uint32_t pc_priority;
    uint32_t priority;
    for (i = 0; i < count; i++) {
        attributes = 0;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_INTELLIGENT;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_TELEPATHIC;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_TUNNELING;
        if (rand() % 2 == 1) attributes |= MONSTER_ATTRIBUTE_ERRATIC;
        try {
            place_monster(dungeon, turn_queue, character_map, attributes);
        } catch (dungeon_exception &e) {
            pc = NULL;
            while (turn_queue->size() != 0) {
                priority = turn_queue->remove((void *) &ch);
                if (ch->type() == CHARACTER_TYPE_PC) {
                    pc = ch;
                    pc_priority = priority;
                }
                else {
                    destroy_character(character_map, ch);
                }
            }
            if (pc != NULL)
                turn_queue->insert((void *) &pc, pc_priority);
            throw dungeon_exception(__PRETTY_FUNCTION__, e, "failed to generate monsters");
        }
    }
}

void destroy_character(character_t ***character_map, character_t *ch) {
    if (character_map[ch->x][ch->y] == ch)
        character_map[ch->x][ch->y] = NULL;
    delete ch;
}

void character_t::move_to(coordinates_t to, character_t ***character_map) {
    if (character_map[x][y] == this) {
        character_map[x][y] = NULL;
    }
    character_map[to.x][to.y] = this;
    x = to.x;
    y = to.y;
}

bool character_t::has_los(dungeon_t *dungeon, coordinates_t to) {
    uint8_t x0 = x;
    uint8_t y0 = y;
    uint8_t x1 = to.x;
    uint8_t y1 = to.y;
    // Regular line equation (rounded to grid points).
    // The special case is if we have a slope > 1 or < -1 -- in that case we can iterate over Y instead of X.
    // Don't bother if the points are equal
    if (x0 == x1 && y0 == y1) return 1;
    int x_diff = x1 - x0;
    int y_diff = y1 - y0;
    // y = mx + b
    float m, b; 
    if (x_diff != 0) m = y_diff / (float) x_diff;
    uint8_t x_new = x0;
    uint8_t y_new = y0;
    int dir;

    if ((x_diff != 0) && m >= -1 && m <= 1) {
        // Regular case, iterate over X.
        b = y0 - m * x0;
        dir = (x_diff > 0 ? 1 : -1);
        for (x_new = x0; x_new != x1 + dir; x_new += dir) {
            y_new = (uint8_t) round(m * x_new + b);
            if (dungeon->cells[x_new][y_new].type == CELL_TYPE_STONE) return false;
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
        for (y_new = y0; y_new != y1 + dir; y_new += dir) {
            x_new = (uint8_t) round(m * y_new + b);
            if (dungeon->cells[x_new][y_new].type == CELL_TYPE_STONE) return false;
        }
    }
    return true;
}

coordinates_t monster_t::next_xy(dungeon_t *dungeon, coordinates_t to) {
    // Butchered version of has_los.
    uint8_t x0 = x;
    uint8_t y0 = y;
    uint8_t x1 = to.x;
    uint8_t y1 = to.y;
    float m, b; 
    int x_diff, y_diff, dir;
    coordinates_t next;
    next.x = x0;
    next.y = y0;
    if (x0 == x1 && y0 == y1) return next;
    x_diff = x1 - x0;
    y_diff = y1 - y0;
    if (x_diff != 0) m = y_diff / (float) x_diff;

    if ((x_diff != 0) && m >= -1 && m <= 1) {
        // Regular case, iterate over X.
        b = y0 - m * x0;
        dir = (x_diff > 0 ? 1 : -1);
        next.x = x0 + dir;
        next.y = (uint8_t) round(m * next.x + b);
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
        next.y = y0 + dir;
        next.x = (uint8_t) round(m * next.y + b);
    }
    return next;
}

void next_turn(dungeon_t *dungeon, character_t *pc, binary_heap_t *turn_queue, character_t ***character_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, game_result_t *result, bool *was_pc) {
    character_t *ch = NULL;
    monster_t *monster;
    uint32_t priority;
    uint8_t x, y, min, x_offset, y_offset, can_move, target_x, target_y;
    coordinates_t next;
    int i, j, x1, y1;
    uint32_t** map;
    cell_t* next_cell;
    while (turn_queue->size() > 0 && ch == NULL) {
        priority = turn_queue->remove((void*) &ch);
        // The dead flag here avoids us having to remove from the heap at an arbitrary location.
        // We just destroy it when it comes off the queue next.
        if (ch->dead) {
            if (ch != pc) {
                destroy_character(character_map, ch);
                ch = NULL;
            }
        }
    }
    if (ch == NULL || (ch == pc && turn_queue->size() == 0)) {
        // Everyone is dead. Game over.
        *result = GAME_RESULT_WIN;
        return;
    }
    
    x = ch->x;
    y = ch->y;

    // If this was the PC's turn, signal that back to the caller
    // Previously for random movement, but now we'll render the changes
    // and wait for user input
    if (ch == pc) {
        turn_queue->insert((void*) &pc, priority + pc->speed);
        *was_pc = 1;
        return; // No open space (impossible with a normal map)
    }

    monster = (monster_t *) ch;

    // Find out which direction this monster wants to go.
    // - Telepathic: Directly to the PC
    // - Intelligent: Towards the last seen location
    // - None: Only towards the PC if there's LOS

    // Slightly inefficient but I prefer the readability since this algorithm is a bit more complex.
    // 1: Determine if the monster is allowed to go to the PC.
    can_move = 0;
    //    a: If it's telepathic, it can.
    if (monster->attributes & MONSTER_ATTRIBUTE_TELEPATHIC) {
        can_move = 1;
        target_x = pc->x;
        target_y = pc->y;
    }
    //    b: If it has line of sight, it can.
    else if (monster->has_los(dungeon, (coordinates_t) {pc->x, pc->y})) {
        can_move = 1;
        target_x = pc->x;
        target_y = pc->y;

        // For future use, we can also mark that the PC was seen.
        monster->pc_seen = 1;
        monster->pc_last_seen_x = pc->x;
        monster->pc_last_seen_y = pc->y;
    }
    //    c: If it has a last seen location in mind, it can.
    else if (monster->pc_seen)  {
        can_move = 1;
        target_x = monster->pc_last_seen_x;
        target_y = monster->pc_last_seen_y;
    }

    // 2: If we can move, find out the cell we'll move to next.
    if (can_move) {
        // If the monster's intelligent, follow the shortest path to the destination.
        if (monster->attributes & MONSTER_ATTRIBUTE_INTELLIGENT) {
            // We can use the pathfinding maps to go to the PC
            if (monster->attributes & MONSTER_ATTRIBUTE_TUNNELING)
                map = pathfinding_tunnel;
            else
                map = pathfinding_no_tunnel;

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
                        next.x = x1;
                        next.y = y1;
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
            next = monster->next_xy(dungeon, (coordinates_t) {target_x, target_y});
            // Can't if it's non-tunneling and going towards stone.
            if (!(monster->attributes & MONSTER_ATTRIBUTE_TUNNELING) && dungeon->cells[next.x][next.y].type == CELL_TYPE_STONE) {
                can_move = 0;
            }
        }
    }

    // And, of course, there's the possibility that the monster is erratic and will move randomly.
    if ((monster->attributes & MONSTER_ATTRIBUTE_ERRATIC) && rand() % 2 == 1) {
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
                if (dungeon->cells[x1][y1].type == CELL_TYPE_STONE && !(monster->attributes & MONSTER_ATTRIBUTE_TUNNELING)) continue;
                // This cell is open
                next.x = x1;
                next.y = y1;
                can_move = 1;
                break;
            }
            if (can_move) break;
        }
    }

    // 3: Time to move!
    if (can_move && (next.x != x || next.y != y)) {
        // In the special case of non-telepathic monsters, we want to clear out the PC seen flag once we reach
        // its last known location.
        if (monster->pc_seen && monster->pc_last_seen_x == next.x && monster->pc_last_seen_y == next.y)
            monster->pc_seen = 0;

        // Our next coordinates are in next_x and next_y.
        // If that's open space, just go there.
        next_cell = &(dungeon->cells[next.x][next.y]);
        if (next_cell->type == CELL_TYPE_STONE) {
            next_cell->hardness -= MIN(next_cell->hardness, 85);
            if (next_cell->hardness > 0) can_move = 0;
            else next_cell->type = CELL_TYPE_HALL;
        }
        // This use of can_move is separate from the can_move that brought us into this loop.
        // 1 byte of memory > readability :)
        if (can_move) {
            // If there's already a character there, kill it.
            if (character_map[next.x][next.y] != NULL) {
                character_map[next.x][next.y]->dead = 1;
                // Special case is the PC, in which case the game ends.
                if (character_map[next.x][next.y]->type() == CHARACTER_TYPE_PC) {
                    monster->move_to(next, character_map);
                    *result = GAME_RESULT_LOSE;
                    turn_queue->insert(&ch, priority + ch->speed); // So this character gets free'd
                    return;
                }
            }
            monster->move_to(next, character_map);
        }
    }

    turn_queue->insert(&ch, priority + ch->speed);
    return;
}

character_type pc_t::type() {
    return CHARACTER_TYPE_PC;
}

character_type monster_t::type() {
    return CHARACTER_TYPE_MONSTER;
}