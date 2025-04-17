#include "character.h"
#include "dungeon.h"
#include "macros.h"
#include "heap.h"

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>

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

void destroy_character(character_t ***character_map, character_t *ch) {
    if (character_map[ch->x][ch->y] == ch)
        character_map[ch->x][ch->y] = NULL;
    delete ch;
}

void character_t::move_to(coordinates_t to, character_t ***character_map) {
    if (location_initialized && character_map[x][y] == this) {
        character_map[x][y] = NULL;
    }
    character_map[to.x][to.y] = this;
    x = to.x;
    y = to.y;
    location_initialized = true;
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


monster_t::monster_t(monster_definition_t *definition) {
    this->definition = definition;
    this->hp = definition->hp->roll();
    this->speed = (uint8_t) CLAMP(definition->speed->roll(), 1, 255);
    this->attributes = (uint16_t) definition->abilities;
    this->display = definition->symbol;
    this->pc_seen = false;
    this->dead = false;
    color_count = 0;
    int i;
    int color_val = definition->color;
    for (i = 0; i < 8; i++) {
        if (color_val & 1) color_count++;
        color_val >>= 1;
    }
}

uint8_t monster_t::next_color() {
    int i;
    int found = -1;
    int color_val = definition->color;
    color_i = (color_i + 1) % color_count;
    for (i = 0; i < 8; i++) {
        if (color_val & 1) found++;
        if (found == color_i) return i;
        color_val >>= 1;
    }
    throw dungeon_exception(__PRETTY_FUNCTION__, "did not find target color (was it modified?)");
}

void monster_t::take_turn(dungeon_t *dungeon, character_t *pc, binary_heap_t<character_t *> &turn_queue, character_t ***character_map, item_t ***item_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, uint32_t priority, game_result_t &result) {
    // Find out which direction this monster wants to go.
    // - Telepathic: Directly to the PC
    // - Intelligent: Towards the last seen location
    // - None: Only towards the PC if there's LOS
    uint8_t min, x_offset, y_offset, target_x, target_y;
    coordinates_t next;
    int i, j, x1, y1;
    uint32_t** map;
    cell_t* next_cell;
    bool can_move;

    // Slightly inefficient but I prefer the readability since this algorithm is a bit more complex.
    // 1: Determine if the monster is allowed to go to the PC.
    can_move = false;
    //    a: If it's telepathic, it can.
    if (attributes & MONSTER_ATTRIBUTE_TELEPATHIC) {
        can_move = 1;
        target_x = pc->x;
        target_y = pc->y;
    }
    //    b: If it has line of sight, it can.
    else if (has_los(dungeon, (coordinates_t) {pc->x, pc->y})) {
        can_move = 1;
        target_x = pc->x;
        target_y = pc->y;

        // For future use, we can also mark that the PC was seen.
        pc_seen = 1;
        pc_last_seen_x = pc->x;
        pc_last_seen_y = pc->y;
    }
    //    c: If it has a last seen location in mind, it can.
    else if (pc_seen) {
        can_move = 1;
        target_x = pc_last_seen_x;
        target_y = pc_last_seen_y;
    }

    // 2: If we can move, find out the cell we'll move to next.
    if (can_move) {
        // If the monster's intelligent, follow the shortest path to the destination.
        if (attributes & MONSTER_ATTRIBUTE_INTELLIGENT) {
            // We can use the pathfinding maps to go to the PC
            if (attributes & (MONSTER_ATTRIBUTE_TUNNELING | MONSTER_ATTRIBUTE_GHOST))
                map = pathfinding_tunnel;
            else
                map = pathfinding_no_tunnel;

            // Pick the best direction
            min = UINT8_MAX;
            for (x1 = x - 1; x1 <= x + 1; x1++) {
                for (y1 = y - 1; y1 <= y + 1; y1++) {
                    if (x1 == x && y1 == y) continue;
                    if (x1 < 0 || x1 >= dungeon->width || y1 < 0 || y1 >= dungeon->height) continue;
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
            next = next_xy(dungeon, (coordinates_t) {target_x, target_y});
            // Can't if it's non-tunneling and going towards stone.
            if (!(attributes & (MONSTER_ATTRIBUTE_TUNNELING | MONSTER_ATTRIBUTE_GHOST)) && dungeon->cells[next.x][next.y].type == CELL_TYPE_STONE) {
                can_move = 0;
            }
        }
    }

    // And, of course, there's the possibility that the monster is erratic and will move randomly.
    if ((attributes & MONSTER_ATTRIBUTE_ERRATIC) && rand() % 2 == 1) {
        // Pick a random available cell around the monster.
        x_offset = rand();
        y_offset = rand();
        can_move = 0;
        for (i = 0; i < 3; i++) {
            x1 = (int) x - 1 + (i + (int) x_offset) % 3;
            if (x1 < 0 || x1 >= dungeon->width) continue;
            for (j = 0; j < 3; j++) {
                y1 = (int) y - 1 + (j + (int) y_offset) % 3;
                if (y1 < 0 || y1 >= dungeon->height) continue;
                if (x1 == x && y1 == y) continue;
                if (dungeon->cells[x1][y1].attributes & CELL_ATTRIBUTE_IMMUTABLE) continue;
                if (dungeon->cells[x1][y1].type == CELL_TYPE_STONE && !(attributes & (MONSTER_ATTRIBUTE_TUNNELING | MONSTER_ATTRIBUTE_GHOST))) continue;
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
        if (pc_seen && pc_last_seen_x == next.x && pc_last_seen_y == next.y)
            pc_seen = 0;

        // Our next coordinates are in next_x and next_y.
        // If that's open space, just go there.
        next_cell = &(dungeon->cells[next.x][next.y]);
        if (next_cell->type == CELL_TYPE_STONE) {
            if (attributes & MONSTER_ATTRIBUTE_TUNNELING) {
                next_cell->hardness -= MIN(next_cell->hardness, 85);
                if (next_cell->hardness > 0) can_move = 0;
                else next_cell->type = CELL_TYPE_HALL;
            }
        }
        // This use of can_move is separate from the can_move that brought us into this loop.
        // 1 byte of memory > readability :)
        if (can_move) {
            // If there's an item there, destroy it or pick it up.
            if (item_map[next.x][next.y] != NULL) {
                if (attributes & MONSTER_ATTRIBUTE_PICKUP) {
                    add_to_inventory(item_map[next.x][next.y]);
                    item_map[next.x][next.y] = NULL;
                }
                else if (attributes & MONSTER_ATTRIBUTE_DESTROY) {
                    delete item_map[next.x][next.y];
                    item_map[next.x][next.y] = NULL;
                }
            }

            // If there's already a character there, kill it.
            if (character_map[next.x][next.y] != NULL) {
                // Special case is the PC, in which case the game ends.
                if (character_map[next.x][next.y] == pc) {
                    pc->dead = true;
                } else if (character_map[next.x][next.y]->type() == CHARACTER_TYPE_MONSTER) {
                    ((monster_t *) character_map[next.x][next.y])->die(result, character_map, item_map);
                }
            }
            move_to(next, character_map);
        }
    }

    turn_queue.insert(this, priority + speed);
    return;
}

void monster_t::die(game_result_t &result, character_t ***character_map, item_t ***item_map) {
    // If the monster has stuff in its inventory, drop it here.
    if (item != NULL) {
        if (item_map[x][y]) {
            item_map[x][y]->add_to_stack(item);
        } else {
            item_map[x][y] = item;
        }
    }
    // Clear out this location on the character map...
    if (character_map[x][y] == this)
        character_map[x][y] = NULL;
    // Deleting is left to whatever called this.
    dead = true;

    if (definition->abilities & MONSTER_ATTRIBUTE_UNIQUE) definition->unique_slain = true;
    if (definition->abilities & MONSTER_ATTRIBUTE_BOSS) result = GAME_RESULT_WIN;
}

int character_t::inventory_size() {
    // We could use a separate size variable here, but we're returning item_ts directly,
    // and they have methods on them to manage the stack. It's more reliable to re-count
    // every time, though less efficient.
    int i = 0;
    item_t *current = item;
    while (current != NULL) {
        i++;
        current = current->next_in_stack();
    }
    return i;
}
void character_t::add_to_inventory(item_t *item) {
    if (this->item == NULL) this->item = item;
    else this->item->add_to_stack(item);
    item_count++;
}
item_t *character_t::remove_from_inventory(int i) {
    if (i < 0 || i >= item_count) throw dungeon_exception(__PRETTY_FUNCTION__, "inventory index out of bounds");
    item_t *current = item;
    int j;
    for (j = 0; j < i - 1; j++) {
        current = current->next_in_stack();
        if (current == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "inventory index out of bounds");
    }
    if (i == 0) {
        current = item;
        item = item->detach_stack();
        return current;
    }
    else {
        return current->remove_next_in_stack();
    }
}
item_t *character_t::remove_inventory_stack() {
    item_t *stack = item;
    item_count = 0;
    item = NULL;
    return stack;
}

item_t *character_t::inventory_at(int i) {
    if (i < 0 || i >= item_count) throw dungeon_exception(__PRETTY_FUNCTION__, "inventory index out of bounds");
    item_t *current = item;
    int j;
    for (j = 0; j < i; j++) {
        current = current->next_in_stack();
        if (current == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "inventory index out of bounds");
    }
    return current;
}

pc_t::pc_t() {
    dice_t dice(100, 1, 20);
    base_hp = dice.roll();
    hp = base_hp;
    for (unsigned long i = 0; i < sizeof (equipment) / sizeof (equipment[0]); i++)
        equipment[i] = NULL;
}

character_type pc_t::type() {
    return CHARACTER_TYPE_PC;
}

character_type monster_t::type() {
    return CHARACTER_TYPE_MONSTER;
}
