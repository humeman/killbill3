#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "dungeon.h"
#include "macros.h"

#define STONE_SEED_COUNT 10
#define GAUSSIAN_CONVOLUTION_COUNT 2

int dungeon_init(dungeon *dungeon, uint8_t width, uint8_t height, int max_rooms) {
    int i, j;
    dungeon->width = (uint8_t) width;
    dungeon->height = (uint8_t) height;
    dungeon->rooms = malloc(max_rooms * sizeof (room));
    dungeon->max_room_count = (uint16_t) max_rooms;
    dungeon->room_count = 0;
    dungeon->min_room_count = 0;
    if (dungeon->rooms == NULL) goto init_free;
    dungeon->cells = malloc(width * sizeof(cell*));
    if (dungeon->cells == NULL) {
        goto init_free_rooms;
    }
    for (i = 0; i < width; i++) {
        dungeon->cells[i] = malloc(height * sizeof(cell));
        if (dungeon->cells[i] == NULL) {
            for (j = 0; j < i; j++) free(dungeon->cells[j]);
            goto init_free_cells;
        }
    }

    for (i = 0; i < dungeon->width; i++) {
        for (j = 0; j < dungeon->height; j++) {
            dungeon->cells[i][j].type = CELL_TYPE_EMPTY;
            dungeon->cells[i][j].hardness = 0;
            dungeon->cells[i][j].mutable = 1;
            dungeon->cells[i][j].character = NULL;
        }
    }

    dungeon->pathfinding_no_tunnel = malloc(dungeon->width * sizeof (uint32_t*));
    if (dungeon->pathfinding_no_tunnel == NULL) {
        goto init_free_all_cells;
    }
    for (i = 0; i < dungeon->width; i++) {
        dungeon->pathfinding_no_tunnel[i] = malloc(dungeon->height * sizeof (uint32_t));
        if (dungeon->pathfinding_no_tunnel[i] == NULL) {
            for (j = 0; j < i; j++) free(dungeon->pathfinding_no_tunnel[j]);
            goto init_free_pathfinding_no_tunnel;
        }
    }

    dungeon->pathfinding_tunnel = malloc(dungeon->width * sizeof (uint32_t*));
    if (dungeon->pathfinding_tunnel == NULL) {
        goto init_free_all_pathfinding_no_tunnel;
    }
    for (i = 0; i < dungeon->width; i++) {
        dungeon->pathfinding_tunnel[i] = malloc(dungeon->height * sizeof (uint32_t));
        if (dungeon->pathfinding_tunnel[i] == NULL) {
            for (j = 0; j < i; j++) free(dungeon->pathfinding_tunnel[j]);
            goto init_free_pathfinding_tunnel;
        }
    }
    if (heap_init(&(dungeon->turn_queue), sizeof (character*))) {
        goto init_free_all_pathfinding_tunnel;
    }

    return 0;

    init_free_all_pathfinding_tunnel:
    for (j = 0; j < dungeon->width; j++) free(dungeon->pathfinding_tunnel[j]);
    init_free_pathfinding_tunnel:
    free(dungeon->pathfinding_tunnel);
    init_free_all_pathfinding_no_tunnel:
    for (j = 0; j < dungeon->width; j++) free(dungeon->pathfinding_no_tunnel[j]);
    init_free_pathfinding_no_tunnel:
    free(dungeon->pathfinding_no_tunnel);
    init_free_all_cells:
    for (j = 0; j < dungeon->width; j++) free(dungeon->cells[j]);
    init_free_cells:
    free(dungeon->cells);
    init_free_rooms:
    free(dungeon->rooms);
    init_free:
    RETURN_ERROR("failed to allocate dungon");
}

void dungeon_destroy(dungeon *dungeon) {
    int i;
    for (i = 0; i < dungeon->width; i++) {
        free(dungeon->cells[i]);
        free(dungeon->pathfinding_no_tunnel[i]);
        free(dungeon->pathfinding_tunnel[i]);
    }
    free(dungeon->cells);
    free(dungeon->rooms);
    free(dungeon->pathfinding_no_tunnel);
    free(dungeon->pathfinding_tunnel); 
    heap_destroy(dungeon->turn_queue);
}

void write_dungeon_pgm(dungeon *dungeon) {
    FILE* out;
    out = fopen("dungeon.pgm", "w");
    fprintf(out, "P5\n%u %u\n255\n", dungeon->width, dungeon->height);
    int x, y;
    for (y = 0; y < dungeon->height; y++) {
        for (x = 0; x < dungeon->width; x++) {
            fwrite(&(dungeon->cells[x][y].hardness), sizeof (uint8_t), 1, out);
        }
    }
    fclose(out);
}

int fill_dungeon(dungeon *dungeon, int min_rooms, int room_count_randomness_max, int room_min_width, int room_min_height, int room_size_randomness_max, int debug) {
    if (fill_stone(dungeon)) {
        fprintf(stderr, "fill_stone failed\n");
        return 1;
    }
    dungeon->min_room_count = min_rooms;
    if (create_rooms(dungeon, min_rooms + (rand() % room_count_randomness_max), room_min_width, room_min_height, room_size_randomness_max)) {
        fprintf(stderr, "create_rooms failed\n");
        return 1;
    }
    fill_outside(dungeon);
    if (connect_rooms(dungeon)) {
        fprintf(stderr, "connect_rooms failed\n");
        return 1;
    }
    if (place_staircases(dungeon)) {
        fprintf(stderr, "place_staircases failed\n");
        return 1;
    }

    // Pick the PC's spawn point
    uint8_t x, y;
    if (random_location(dungeon, &x, &y)) RETURN_ERROR("failed to place PC within dungeon");
    dungeon->pc.x = x;
    dungeon->pc.y = y;
    dungeon->cells[x][y].character = &(dungeon->pc);
    dungeon->pc.display = '@';
    dungeon->pc.monster = NULL;
    dungeon->pc.type = CHARACTER_PC;
    dungeon->pc.speed = 10;
    dungeon->pc.dead = 0;

    return 0;
}

// This struct is identical to the one in the Assignment 1.01 solution
// code, Piazza post @80.
typedef struct queue_node {
    int x, y;
    struct queue_node *next;
} queue_node;

// The Gaussian values ripped from Assignment 1.01's solution code.
int gaussian[5][5] = {
    {  1,  4,  7,  4,  1 },
    {  4, 16, 26, 16,  4 },
    {  7, 26, 41, 26,  7 },
    {  4, 16, 26, 16,  4 },
    {  1,  4,  7,  4,  1 }
};

// This function is partially based on 'smooth_hardness' provided in
// the Assignment 1.01 solution code, Piazza post @80.
// I might rewrite it before 1.03, but we'll see.
int fill_stone(dungeon *dungeon) {
    uint8_t x, y;
    queue_node *head, *tail, *temp;

    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            dungeon->cells[x][y].type = CELL_TYPE_STONE;
            dungeon->cells[x][y].hardness = 0;
            dungeon->cells[x][y].mutable = 1;
        }
    }

    int i;
    // Picks a random hardness and places it in a single random cell
    // STONE_SEED_COUNT times, enqueuing them along the way.
    int step = 255 / STONE_SEED_COUNT - 1;
    for (i = 0; i < STONE_SEED_COUNT; i++) {
        // Since we've just initialized everything to 0 and we have an
        // 80x21 grid, this can't fail. Though it can technically run
        // forever if we're really unlucky. Oh well.
        do {
            x = rand() % dungeon->width;
            y = rand() % dungeon->height;
        } while (dungeon->cells[x][y].hardness);

        dungeon->cells[x][y].hardness = (i == 0 ? 1 : i * step);
        if (i == 0) {
            head = malloc(sizeof (*head));
            tail = head;
        }
        else {
            tail->next = malloc(sizeof (*tail));
            tail = tail->next;
        }
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
    }

    // Diffuses values out until every cell is filled.
    uint8_t ix, iy;
    while (head) {
        x = head->x;
        y = head->y;
        i = dungeon->cells[x][y].hardness;

        for (ix = x - 1; ix <= x + 1; ix++) {
            for (iy = y - 1; iy <= y + 1; iy++) {
                if (ix == x && iy == y) continue;
                if (ix >= 0 && ix < dungeon->width && iy >= 0 && iy < dungeon->height
                    && !dungeon->cells[ix][iy].hardness) {
                    dungeon->cells[ix][iy].hardness = i;
                    tail->next = malloc(sizeof (*tail));
                    tail = tail->next;
                    tail->next = NULL;
                    tail->x = ix;
                    tail->y = iy;
                }
            }
        }
        temp = head;
        head = head->next;
        free(temp);
    }

    // Applies a gaussian convolution to smooth it out.
    int s, t, p, q;
    for (i = 0; i < GAUSSIAN_CONVOLUTION_COUNT; i++) {
        for (x = 0; x < dungeon->width; x++) {
            for (y = 0; y < dungeon->height; y++) {
                for (s = t = p = 0; p < 5; p++) {
                    for (q = 0; q < 5; q++) {
                        if (y + (p - 2) >= 0 && y + (p - 2) < dungeon->height &&
                            x + (q - 2) >= 0 && x + (q - 2) < dungeon->width) {
                            s += gaussian[p][q];
                            t += dungeon->cells[x + (q - 2)][y + (p - 2)].hardness * gaussian[p][q];
                        }
                    }
                }

                dungeon->cells[x][y].hardness = t / s;
            }
        }
    }

    return 0;
}

void fill_outside(dungeon *dungeon) {
    int i;
    for (i = 0; i < dungeon->width; i++) {
        dungeon->cells[i][0].type = CELL_TYPE_STONE;
        dungeon->cells[i][0].hardness = 255;
        dungeon->cells[i][0].mutable = 0;
        dungeon->cells[i][dungeon->height - 1].type = CELL_TYPE_STONE;
        dungeon->cells[i][dungeon->height - 1].hardness = 255;
        dungeon->cells[i][dungeon->height - 1].mutable = 0;
    }
    for (i = 1; i < dungeon->height - 1; i++) {
        dungeon->cells[0][i].type = CELL_TYPE_STONE;
        dungeon->cells[0][i].hardness = 255;
        dungeon->cells[0][i].mutable = 0;
        dungeon->cells[dungeon->width - 1][i].type = CELL_TYPE_STONE;
        dungeon->cells[dungeon->width - 1][i].hardness = 255;
        dungeon->cells[dungeon->width - 1][i].mutable = 0;
    }
}

int create_rooms(dungeon *dungeon, int count, uint8_t min_width, uint8_t min_height, int size_randomness_max) {
    int i;
    int room_width, room_height;

    if (count > dungeon->max_room_count) {
        fprintf(stderr, "create_rooms caller wants %d rooms, but the max is %d\n", count, dungeon->max_room_count);
        return 1;
    }

    for (i = 0; i < count; i++) {
        room_width = min_width + (rand() % size_randomness_max);
        room_height = min_height + (rand() % size_randomness_max);

        if (create_room(dungeon, dungeon->rooms + i, room_width, room_height)) {
            return i < dungeon->min_room_count; // This is only an error if we make less than 6 rooms.
                                                 // Otherwise, we can silently fail to create more (the extra rooms are random anyway).
        }

        dungeon->room_count++;
    }
    return 0;
}

int create_room(dungeon *dungeon, room *room, uint8_t room_width, uint8_t room_height) {
    int x_offset = rand();
    int y_offset = rand();

    uint8_t ix, iy;
    uint8_t x, y;
    int16_t jx, jy;
    int placed = 0;
    
    for (ix = 0; ix < dungeon->width && !placed; ix++) {
        x = (ix + x_offset) % dungeon->width;
        for (iy = 0; iy < dungeon->height && !placed; iy++) {
            y = (iy + y_offset) % dungeon->height;

            // Make sure the area is clear.
            placed = 1;
            for (jx = x - 1; jx < x + room_width + 1 && placed; jx++) {
                for (jy = y - 1; jy < y + room_height + 1; jy++) {
                    if (jx < 0 || jx >= dungeon->width || jy < 0 || jy >= dungeon->height 
                        || dungeon->cells[jx][jy].type != CELL_TYPE_STONE
                        || !dungeon->cells[jx][jy].mutable) {
                        placed = 0;
                        break;
                    }
                }
            }

            // Then, only place it if the area is open.
            if (placed) {
                room->x0 = x;
                room->y0 = y;
                room->x1 = x + room_width - 1;
                room->y1 = y + room_height - 1;
                for (jx = x; jx < x + room_width; jx++) {
                    for (jy = y; jy < y + room_height; jy++) {
                        dungeon->cells[jx][jy].type = CELL_TYPE_ROOM;
                        dungeon->cells[jx][jy].hardness = 0;
                    }
                }
            }
        }
    }
    return !placed;
}

int connect_rooms(dungeon *dungeon) {
    // Each room will connect to its nearest room until every room is marked as visited.
    int i;
    int visited[dungeon->room_count];
    for (i = 0; i < dungeon->room_count; i++) visited[i] = 0;
    visited[0] = 1;

    room *a;
    room *b;
    room *current;
    uint8_t ax, bx, ay, by;
    double distance, max_distance;
    int done = 0;
    while (!done) {
        // Pick the first un-visited room.
        for (i = 1; i < dungeon->room_count; i++)
            if (!visited[i]) {
                a = dungeon->rooms + i;
                visited[i] = 1;
                break;
            }
        
        // Find the nearest visited room.
        b = NULL;
        max_distance = INFINITY;
        for (i = 0; i < dungeon->room_count; i++)
            if (dungeon->rooms + i != a && visited[i]) {
                current = dungeon->rooms + i;
                // Find the center points of each room.
                ax = (a->x0 + a->x1) / 2;
                ay = (a->y0 + a->y1) / 2;
                bx = (current->x0 + current->x1) / 2;
                by = (current->y0 + current->y1) / 2;
                distance = pow(ax - bx, 2) + pow(ay - by, 2);

                if (distance < max_distance) {
                    b = dungeon->rooms + i;
                    max_distance = distance;
                    visited[i] = 1;
                }
            }

        // If there's only one room, we never get a b assigned.
        // That means there's nothing to connect and we can exit.
        if (b == NULL) return 0;

        // Connect a random point from each room.
        ax = (a->x0 + (rand() % (a->x1 - a->x0)));
        ay = (a->y0 + (rand() % (a->y1 - a->y0)));
        bx = (b->x0 + (rand() % (b->x1 - b->x0)));
        by = (b->y0 + (rand() % (b->y1 - b->y0)));

        if (connect_points(dungeon, ax, ay, bx, by)) {
            return 1;
        }
        done = 1;
        for (i = 0; i < dungeon->room_count; i++)
            if (!visited[i]) {
                done = 0;
                break;
            }
    }
    return 0;
}

int connect_points(dungeon *dungeon, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    // We want to make these paths semi-random, since right angles are boring.
    // We know we're going to travel x_diff and y_diff overall -- just to mix
    // things up, we'll randomly switch between which (X or Y) we're moving
    // while drawing the path.
    int8_t x_direction = x1 - x0 >= 0 ? 1 : -1;
    int8_t y_direction = y1 - y0 >= 0 ? 1 : -1;

    int8_t direction;
    uint8_t x = x0;
    uint8_t y = y0;

    uint8_t x_poss, y_poss;

    while (x != x1 || y != y1) {
        direction = rand() % 2;

        // We cannot place outside the map.
        x_poss = x + x_direction >= 0 && x + x_direction < dungeon->width;
        y_poss = y + y_direction >= 0 && y + y_direction < dungeon->height;

        // We cannot place on immutable blocks.
        if (!dungeon->cells[x + x_direction][y].mutable) x_poss = 0;
        if (!dungeon->cells[x][y + y_direction].mutable) y_poss = 0;

        // We won't place any more if we're aligned with the room.
        if (x == x1) x_poss = 0;
        if (y == y1) y_poss = 0;

        // If both directions are impossible, this is an error.
        // In our normal room configuration, this is impossible regardless of what random choices
        // we generate, since each room is within the bounds of the mutable area of the room.
        if (!x_poss && !y_poss) {
            fprintf(stderr, "err: connect_points from (%d, %d) to (%d, %d) has no possible next location at (%d, %d)\n", x0, y0, x1, y1, x, y);
            return 1;
        }

        // Flip directions if we can't place in our desired direction.
        if (direction == 0 && !x_poss) direction = 1;
        if (direction == 1 && !y_poss) direction = 0;

        if (direction == 0) x += x_direction;
        else y += y_direction;

        if (dungeon->cells[x][y].type == CELL_TYPE_STONE) {
            dungeon->cells[x][y].type = CELL_TYPE_HALL;
            dungeon->cells[x][y].hardness = 0;
        }
    }
    return 0;
}

int place_staircases(dungeon *dungeon) {
    if (dungeon->room_count < 1) return -1;

    uint8_t tmpx, tmpy;
    
    // Pick a random location in a room for the up staircase.
    room room = dungeon->rooms[rand() % dungeon->room_count];
    if (place_in_room(dungeon, room, CELL_TYPE_UP_STAIRCASE, &tmpx, &tmpy)) return 1;

    // And again for down...
    room = dungeon->rooms[rand() % dungeon->room_count];
    if (place_in_room(dungeon, room, CELL_TYPE_DOWN_STAIRCASE, &tmpx, &tmpy)) return 1;
    return 0;
}

int place_in_room(dungeon *dungeon, room room, cell_type material, uint8_t *x_loc, uint8_t *y_loc) {
    uint8_t x, y;
    if (random_location_in_room(dungeon, room, &x, &y)) RETURN_ERROR("no available space in room");
    dungeon->cells[x][y].type = material;
    dungeon->cells[x][y].hardness = 0;
    return 0;
}

int random_location_in_room(dungeon *dungeon, room room, uint8_t *x_loc, uint8_t *y_loc) {
    uint8_t x_offset = rand();
    uint8_t y_offset = rand();
    uint8_t i, j, x, y;
    uint8_t room_width = room.x1 - room.x0;
    uint8_t room_height = room.y1 - room.y0;
    for (i = 0; i < room_width; i++) {
        x = room.x0 + (x_offset + i) % room_width;
        for (j = 0; j < room_height; j++) {
            y = room.y0 + (y_offset + j) % room_height;

            if (dungeon->cells[x][y].type == CELL_TYPE_ROOM) {
                // We will additionally check that we aren't obstructing a hallway,
                // since that may be annoying in the future.
                if ((x - 1 >= 0 && dungeon->cells[x - 1][y].type == CELL_TYPE_HALL) \
                    || (y - 1 >= 0 && dungeon->cells[x][y - 1].type == CELL_TYPE_HALL) \
                    || (x + 1 < dungeon->width && dungeon->cells[x + 1][y].type == CELL_TYPE_HALL) \
                    || (y + 1 < dungeon->height && dungeon->cells[x][y + 1].type == CELL_TYPE_HALL)) {
                        continue;
                    }

                // And, we can't place in an immutable cell.
                if (!dungeon->cells[x][y].mutable) continue;

                // Also, don't overwrite a character.
                if (dungeon->cells[x][y].character) continue;
                
                *x_loc = x;
                *y_loc = y;
                return 0;
            }
        }
    }

    return 1;
}

int random_location(dungeon *dungeon, uint8_t *x_loc, uint8_t *y_loc) {
    int room_offset = rand();
    int i;
    room *room;

    for (i = 0; i < dungeon->room_count; i++) {
        room = dungeon->rooms + ((i + room_offset) % dungeon->room_count);
        if (!random_location_in_room(dungeon, *room, x_loc, y_loc)) return 0;
    }
    RETURN_ERROR("no available space in dungeon");
}