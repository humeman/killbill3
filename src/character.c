#include "character.h"
#include "dungeon.h"
#include "macros.h"
#include "heap.h"

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define MONSTER_ATTRIBUTE_INTELLIGENT 0x01
#define MONSTER_ATTRIBUTE_TELEPATHIC 0x02
#define MONSTER_ATTRIBUTE_TUNNELING 0x04
#define MONSTER_ATTRIBUTE_ERRATIC 0x08

int generate_monsters(dungeon *dungeon, int count) {
    int i;
    uint8_t x, y;
    uint32_t trash;
    character *character;
    monster *monster;
    for (i = 0; i < count; i++) {
        character = malloc(sizeof (*character));
        if (!character) RETURN_ERROR("failed to allocate character");
        monster = malloc(sizeof (*monster));
        if (!monster) RETURN_ERROR("failed to allocate monster");
        character->monster = monster;
        if (random_location(dungeon, &x, &y)) goto generate_monsters_clean;
        character->x = x;
        character->y = y;
        character->speed = (rand() % 16) + 5;
        monster->attributes = 0;
        if (rand() % 2 == 1) monster->attributes |= MONSTER_ATTRIBUTE_INTELLIGENT;
        if (rand() % 2 == 1) monster->attributes |= MONSTER_ATTRIBUTE_TELEPATHIC;
        if (rand() % 2 == 1) monster->attributes |= MONSTER_ATTRIBUTE_TUNNELING;
        if (rand() % 2 == 1) monster->attributes |= MONSTER_ATTRIBUTE_ERRATIC;
        character->type = monster->attributes < 10 ? '0' + monster->attributes : 'A' + monster->attributes - 10;

        dungeon->cells[x][y].character = character;
        if (heap_insert(dungeon->turn_queue, (void *) character, character->speed)) goto generate_monsters_clean;
    }

    return 0;

    generate_monsters_clean:
    while (heap_size(dungeon->turn_queue) != 0) {
        if (heap_top(dungeon->turn_queue, character, &trash)) RETURN_ERROR("catastrophe: failed to remove from heap while cleaning up an error");
        destroy_character(dungeon, character);
    }
    RETURN_ERROR("failed to generate monsters");
}

void destroy_character(dungeon *dungeon, character *character) {
    dungeon->cells[character->x][character->y].character = NULL;
    if (character->monster) free(character->monster);
    free(character);
}

void next_xy(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *next_x, uint8_t *next_y) {
    float m = (y1 - y0) / ((float) (x1 - x0));
    float b = y0 - m * x0;
    uint8_t x, y, new_y, y_dir;
    y = y0;
    x = x0 + 1;
    new_y = (int) round(m * x + b);
    // If Y has increased by more than 1, we have a steep slope.
    // We need to fill in the cells above/below this Y location until that
    // previous Y.
    if (abs(new_y - y) > 1) {
        y_dir = new_y > y ? 1 : -1;
        // if (dungeon->cells[x][y + y_dir].type == CELL_TYPE_STONE) {
        //     *next_x = x0;
        //     *next_y = y0;
        //     return;
        // }
        *next_x = x;
        *next_y = y + y_dir;
        return;
    }
    y = new_y;

    // if (dungeon->cells[x][y].type == CELL_TYPE_STONE) {
    //     *next_x = x0;
    //     *next_y = y0;
    //     return;
    // }
    *next_x = x;
    *next_y = y;
}

int has_los(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    float m = (y1 - y0) / ((float) (x1 - x0));
    float b = y0 - m * x0;
    uint8_t x, y, new_y, jy, y_dir;
    y = y0;
    for (x = x0; x <= x1; x++) {
        new_y = (int) round(m * x + b);
        // If Y has increased by more than 1, we have a steep slope.
        // We need to fill in the cells above/below this Y location until that
        // previous Y.
        if (abs(new_y - y) > 1) {
            y_dir = new_y > y ? 1 : -1;
            printf("jy = %d, target = %d, inc = %d\n", y + y_dir, new_y - y_dir, y_dir);
            for (jy = y + y_dir; jy != new_y; jy += y_dir) {
                if (dungeon->cells[x][jy].type == CELL_TYPE_STONE) return 0;
            }
        }
        y = new_y;

        if (dungeon->cells[x][y].type == CELL_TYPE_STONE) return 0;
    }
    return 1;
}

int next_turn(dungeon *dungeon) {
    character *character;
    uint32_t priority;
    uint8_t x, y, x1, y1, x_min, y_min, min;
    int dead;
    uint32_t** map;
    if (heap_remove(dungeon->turn_queue, (void*) &character, &priority)) RETURN_ERROR("failed to remove from top of turn queue");
    
    x = character->x;
    y = character->y;
    dead = 0;

    // First, let's find out which direction this monster wants to go.
    // - Telepathic: Directly to the PC
    // - Intelligent: Towards the last seen location
    // - None: Only towards the PC if there's LOS

    // -- Telepathic movement --
    if (character->monster->attributes & MONSTER_ATTRIBUTE_TELEPATHIC) {
        // We can use the pathfinding maps to go to the PC
        if (character->monster->attributes & MONSTER_ATTRIBUTE_TUNNELING)
            map = dungeon->pathfinding_tunnel;
        else
            map = dungeon->pathfinding_no_tunnel;

        // Pick the best direction
        for (x1 = x - 1; x1 <= x + 1; x1++) {
            for (y1 = y - 1; y1 <= y + 1; y1++) {
                if (x1 == x && y1 == y) continue;
                if (map[x1][y1] == UINT32_MAX) continue;
                if (map[x1][y1] < min) {
                    min = map[x1][y1];
                    x_min = x;
                    y_min = y;
                }
            }
        }
    }

    // -- Erratic movement --
    // aaaaaaaaaaaaaaaaaaaaah
    dead = x_min + y_min;


    if (dead)
        destroy_character(dungeon, character);
    else if (heap_insert(dungeon->turn_queue, character, priority + character->speed))
        RETURN_ERROR("failed to insert character into turn queue");
    return 0;
    
}