#include "character.h"
#include "dungeon.h"
#include "macros.h"
#include "heap.h"
#include "message_queue.h"

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>

#define MAX_ATTEMPTS 2048

IntPair random_location_no_kill(Dungeon *dungeon, Character ***character_map) {
    int i;
    IntPair coords;
    for (i = 0; i < MAX_ATTEMPTS; i++) {
        try {
            coords = dungeon->random_location();
            if (character_map[coords.x][coords.y]) continue;
            return coords;
        } catch (dungeon_exception &e) {}
    }
    throw dungeon_exception(__PRETTY_FUNCTION__, "no available space for a new monster in dungeon");
}

void destroy_character(Character ***character_map, Character *ch) {
    if (character_map[ch->x][ch->y] == ch)
        character_map[ch->x][ch->y] = NULL;
    delete ch;
}

int Monster::damage(int amount, game_result_t &result, Item ***item_map, Character ***character_map) {
    hp -= amount;
    if (hp <= 0) {
        die(result, character_map, item_map);
    }
    return amount;
}

void Character::move_to(IntPair to, Character ***character_map) {
    if (location_initialized && character_map[x][y] == this) {
        character_map[x][y] = NULL;
    }
    character_map[to.x][to.y] = this;
    // Find which direction we went.
    // If we move more than 1 cell at a time, this won't work well, and that's fine.
    if (to.x > x) direction = DIRECTION_EAST;
    else if (to.x < x) direction = DIRECTION_WEST;
    else if (to.y < y) direction = DIRECTION_NORTH;
    else if (to.y > y) direction = DIRECTION_SOUTH;
    
    x = to.x;
    y = to.y;
    
    location_initialized = true;
}

bool Character::has_los(Dungeon *dungeon, IntPair to) {
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

IntPair Monster::next_xy(Dungeon *dungeon, IntPair to) {
    // Butchered version of has_los.
    uint8_t x0 = x;
    uint8_t y0 = y;
    uint8_t x1 = to.x;
    uint8_t y1 = to.y;
    float m, b;
    int x_diff, y_diff, dir;
    IntPair next;
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


Monster::Monster(MonsterDefinition *definition) {
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

uint8_t Monster::next_color() {
    color_i = (color_i + 1) % color_count;
    return current_color();
}

uint8_t Monster::current_color() {
    int i;
    int found = -1;
    int color_val = definition->color;
    for (i = 0; i < 8; i++) {
        if (color_val & 1) found++;
        if (found == color_i) return i;
        color_val >>= 1;
    }
    throw dungeon_exception(__PRETTY_FUNCTION__, "did not find target color (was it modified?)");
}

int VALID_MOVES[][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

void Monster::take_turn(Dungeon *dungeon, PC *pc, BinaryHeap<Character *> &turn_queue, Character ***character_map, Item ***item_map, uint32_t **pathfinding_tunnel, uint32_t **pathfinding_no_tunnel, uint32_t priority, game_result_t &result) {
    // Find out which direction this monster wants to go.
    // - Telepathic: Directly to the PC
    // - Intelligent: Towards the last seen location
    // - None: Only towards the PC if there's LOS
    uint8_t min, x_offset, target_x, target_y;
    IntPair next;
    int i, j, x1, y1, dam, r;
    uint32_t** map;
    Cell* next_cell;
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
    else if (has_los(dungeon, (IntPair) {pc->x, pc->y})) {
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
            for (int *move : VALID_MOVES) {
                x1 = x + move[0];
                y1 = y + move[1];
                if (x1 < 0 || x1 >= dungeon->width) continue;
                if (y1 < 0 || y1 >= dungeon->height) continue;
                if (x1 == x && y1 == y) continue;
                if (map[x1][y1] == UINT32_MAX) continue;
                // Find the minimum while preferring non-stone cells.
                if (map[x1][y1] < min || (map[x1][y1] == min && dungeon->cells[x1][y1].type != CELL_TYPE_STONE)) {
                    min = map[x1][y1];
                    next.x = x1;
                    next.y = y1;
                }
            }

            // There is a possibility we can't move in any direction if we generate a bad dungeon.
            if (min == UINT8_MAX) {
                can_move = 0;
            }
        }

        // Otherwise, we'll go in a straight line.
        else {
            next = next_xy(dungeon, (IntPair) {target_x, target_y});
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
        can_move = 0;
        j = ARRAY_SIZE(VALID_MOVES);
        for (i = 0; i < j; i++) {
            x1 = x + VALID_MOVES[(x_offset + i) % j][0];
            y1 = y + VALID_MOVES[(x_offset + i) % j][1];
            if (x1 < 0 || x1 >= dungeon->width) continue;
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

            // If there's already a character there, deal damage or displace.
            if (character_map[next.x][next.y] != NULL) {
                // If it's the PC, deal damage.
                if (character_map[next.x][next.y] == pc) {
                    // See if the PC dodges.
                    r = rand() % 100;
                    if (pc->dodge_bonus() >= r) {
                        MessageQueue::get()->add(
                            "You dodge an attack from &" +
                            std::to_string(current_color()) +
                            escape_col(definition->name) +
                            "&r.");
                    } else {
                        dam = pc->damage(definition->damage->roll(), result, item_map, character_map);
                        MessageQueue::get()->add(
                            "&" +
                            std::to_string(current_color()) +
                            escape_col(definition->name) +
                            "&r hits you for " + std::to_string(dam) + ".");
                    }
                }
                // For monsters, we move them out of the way (or, if unavailable, swap).
                else {
                    for (x1 = (int) next.x - 1; x1 <= (int) next.x + 1; x1++) {
                        if (x1 < 0 || x1 >= dungeon->width) continue;
                        for (y1 = (int) next.y - 1; y1 <= (int) next.y + 1; y1++) {
                            if (y1 < 0 || y1 >= dungeon->height) continue;
                            if (x1 == next.x && y1 == next.y) continue;
                            // Only available if ROOM/HALL and no character.
                            if (
                                (dungeon->cells[x1][y1].type == CELL_TYPE_ROOM ||
                                dungeon->cells[x1][y1].type == CELL_TYPE_HALL) &&
                                !character_map[x1][y1]
                            ) {
                                goto found;
                            }
                        }
                    }
                    // No location was found, so we swap.
                    character_map[next.x][next.y]->move_to((IntPair) {x, y}, character_map);
                    goto end;
                    found:
                    // Move the monster to its displaced cell
                    character_map[next.x][next.y]->move_to((IntPair) {(uint8_t) x1, (uint8_t) y1}, character_map);
                    end:
                    move_to(next, character_map);
                }
            } else {
                move_to(next, character_map);
            }
        }
    }

    turn_queue.insert(this, priority + speed);
    return;
}

void Monster::die(game_result_t &result, Character ***character_map, Item ***item_map) {
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

int Character::inventory_size() {
    // We could use a separate size variable here, but we're returning Items directly,
    // and they have methods on them to manage the stack. It's more reliable to re-count
    // every time, though less efficient.
    int i = 0;
    Item *current = item;
    while (current != NULL) {
        i++;
        current = current->next_in_stack();
    }
    return i;
}
void Character::add_to_inventory(Item *item) {
    if (this->item == NULL) this->item = item;
    else this->item->add_to_stack(item);
    item_count++;
}
Item *Character::remove_from_inventory(int i) {
    if (i < 0 || i >= item_count) throw dungeon_exception(__PRETTY_FUNCTION__, "inventory index out of bounds");
    Item *current = item;
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
Item *Character::remove_inventory_stack() {
    Item *stack = item;
    item_count = 0;
    item = NULL;
    return stack;
}

Item *Character::inventory_at(int i) {
    if (i < 0 || i >= item_count) throw dungeon_exception(__PRETTY_FUNCTION__, "inventory index out of bounds");
    Item *current = item;
    int j;
    for (j = 0; j < i; j++) {
        current = current->next_in_stack();
        if (current == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "inventory index out of bounds");
    }
    return current;
}

PC::PC() {
    Dice dice(100, 1, 20);
    base_hp = dice.roll();
    hp = base_hp;
    for (unsigned long i = 0; i < sizeof (equipment) / sizeof (equipment[0]); i++)
        equipment[i] = NULL;
}

CHARACTER_TYPE PC::type() {
    return CHARACTER_TYPE_PC;
}

CHARACTER_TYPE Monster::type() {
    return CHARACTER_TYPE_MONSTER;
}


int PC::speed_bonus() {
    int i;
    int sp = speed;
    for (i = 0; i < ARRAY_SIZE(equipment); i++) {
        if (equipment[i]) {
            sp += equipment[i]->speed_bonus / 10;
        }
    };
    // A speed of 0 means nothing else can move.
    if (sp < 1) sp = 1;
    return sp;
}

int PC::damage_bonus() {
    int i;
    int dm = base_damage.roll();
    for (i = 0; i < ARRAY_SIZE(equipment); i++) {
        if (equipment[i]) {
            dm += equipment[i]->get_damage();
        }
    };
    return dm;
}

int PC::dodge_bonus() {
    int i;
    int dodge = 0;
    for (i = 0; i < ARRAY_SIZE(equipment); i++) {
        if (equipment[i]) {
            dodge += equipment[i]->dodge_bonus;
        }
    };
    if (dodge > 99) dodge = 99; // At least there's a chance...
    return dodge;
}

int PC::defense_bonus() {
    int i;
    int def = 0;
    for (i = 0; i < ARRAY_SIZE(equipment); i++) {
        if (equipment[i]) {
            def += equipment[i]->defense_bonus;
        }
    };
    return def;
}

int PC::damage(int amount, game_result_t &result, Item ***item_map, Character ***character_map) {
    amount -= defense_bonus();
    if (amount <= 0) return 0;
    hp -= amount;
    if (hp <= 0) {
        dead = true;
        if (character_map[x][y] == this)
            character_map[x][y] = NULL;
    }
    return amount;
}
