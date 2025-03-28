#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "dungeon.h"
#include "macros.h"
#include <stdexcept>

#define STONE_SEED_COUNT 10
#define GAUSSIAN_CONVOLUTION_COUNT 2

dungeon_t_new::dungeon_t_new(uint8_t width, uint8_t height, int max_rooms) {
    int i, j;
    width = width;
    height = height;
    rooms = (room_t *) malloc(max_rooms * sizeof (room_t));
    max_room_count = (uint16_t) max_rooms;
    room_count = 0;
    min_room_count = 0;
    if (rooms == NULL) goto init_free;
    cells = (cell_t **) malloc(width * sizeof (cell_t *));
    if (cells == NULL) {
        goto init_free_rooms;
    }
    for (i = 0; i < width; i++) {
        cells[i] = (cell_t *) malloc(height * sizeof (cell_t));
        if (cells[i] == NULL) {
            for (j = 0; j < i; j++) free(cells[j]);
            goto init_free_cells;
        }
    }

    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            cells[i][j].type = CELL_TYPE_EMPTY;
            cells[i][j].hardness = 0;
            cells[i][j].attributes = 0;
            cells[i][j].character = NULL;
        }
    }

    pathfinding_no_tunnel = (uint32_t **) malloc(width * sizeof (uint32_t*));
    if (pathfinding_no_tunnel == NULL) {
        goto init_free_all_cells;
    }
    for (i = 0; i < width; i++) {
        pathfinding_no_tunnel[i] = (uint32_t *) malloc(height * sizeof (uint32_t));
        if (pathfinding_no_tunnel[i] == NULL) {
            for (j = 0; j < i; j++) free(pathfinding_no_tunnel[j]);
            goto init_free_pathfinding_no_tunnel;
        }
    }

    pathfinding_tunnel = (uint32_t **) malloc(width * sizeof (uint32_t*));
    if (pathfinding_tunnel == NULL) {
        goto init_free_all_pathfinding_no_tunnel;
    }
    for (i = 0; i < width; i++) {
        pathfinding_tunnel[i] = (uint32_t *) malloc(height * sizeof (uint32_t));
        if (pathfinding_tunnel[i] == NULL) {
            for (j = 0; j < i; j++) free(pathfinding_tunnel[j]);
            goto init_free_pathfinding_tunnel;
        }
    }
    if (heap_init(&turn_queue, sizeof (character*))) {
        goto init_free_all_pathfinding_tunnel;
    }

    pc.display = '@';
    pc.monster = NULL;
    pc.type = CHARACTER_PC;
    pc.speed = PC_SPEED;
    pc.dead = 0;

    return;

    init_free_all_pathfinding_tunnel:
    for (j = 0; j < width; j++) free(pathfinding_tunnel[j]);
    init_free_pathfinding_tunnel:
    free(pathfinding_tunnel);
    init_free_all_pathfinding_no_tunnel:
    for (j = 0; j < width; j++) free(pathfinding_no_tunnel[j]);
    init_free_pathfinding_no_tunnel:
    free(pathfinding_no_tunnel);
    init_free_all_cells:
    for (j = 0; j < width; j++) free(cells[j]);
    init_free_cells:
    free(cells);
    init_free_rooms:
    free(rooms);
    init_free:
    throw std::runtime_error("failed to allocate dungeon");
}

dungeon_t_new::~dungeon_t_new() {
    int i;
    for (i = 0; i < width; i++) {
        free(cells[i]);
        free(pathfinding_no_tunnel[i]);
        free(pathfinding_tunnel[i]);
    }
    free(cells);
    free(rooms);
    free(pathfinding_no_tunnel);
    free(pathfinding_tunnel); 
    heap_destroy(turn_queue);
}

void dungeon_t_new::write_pgm() {
    FILE* out;
    out = fopen("dungeon.pgm", "w");
    fprintf(out, "P5\n%u %u\n255\n", width, height);
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            fwrite(&(cells[x][y].hardness), sizeof (uint8_t), 1, out);
        }
    }
    fclose(out);
}

void dungeon_t_new::fill(int min_rooms, int room_count_randomness_max, int room_min_width, int room_min_height, int room_size_randomness_max, int debug) {
    uint8_t x, y;
    coordinates_t loc;

    fill_stone();
    room_count = 0;
    min_room_count = min_rooms;
    create_rooms(min_rooms + (rand() % room_count_randomness_max), room_min_width, room_min_height, room_size_randomness_max);
    fill_outside();
    place_staircases();

    // Pick the PC's spawn point
    loc = random_location();
    pc.x = x;
    pc.y = y;
    cells[x][y].character = &(pc);
    pc.dead = 0;
}

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
// Update from 1.06: I didn't :)
void dungeon_t_new::fill_stone() {
    uint8_t x, y, ix, iy;
    queue_node_t *head, *tail, *temp;
    int i, step, s, t, p, q;

    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            cells[x][y].type = CELL_TYPE_STONE;
            cells[x][y].hardness = 0;
            cells[x][y].attributes = 0;
            cells[x][y].character = NULL;
        }
    }

    // Picks a random hardness and places it in a single random cell
    // STONE_SEED_COUNT times, enqueuing them along the way.
    step = 255 / STONE_SEED_COUNT - 1;
    for (i = 0; i < STONE_SEED_COUNT; i++) {
        // Since we've just initialized everything to 0 and we have an
        // 80x21 grid, this can't fail. Though it can technically run
        // forever if we're really unlucky. Oh well.
        do {
            x = rand() % width;
            y = rand() % height;
        } while (cells[x][y].hardness);

        cells[x][y].hardness = (i == 0 ? 1 : i * step);
        if (i == 0) {
            head = (queue_node_t *) malloc(sizeof (*head));
            tail = head;
        }
        else {
            tail->next = (queue_node_t *) malloc(sizeof (*tail));
            tail = tail->next;
        }
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
    }

    // Diffuses values out until every cell is filled.
    while (head) {
        x = head->x;
        y = head->y;
        i = cells[x][y].hardness;

        for (ix = x - 1; ix <= x + 1; ix++) {
            for (iy = y - 1; iy <= y + 1; iy++) {
                if (ix == x && iy == y) continue;
                if (ix >= 0 && ix < width && iy >= 0 && iy < height
                    && !cells[ix][iy].hardness) {
                    cells[ix][iy].hardness = i;
                    tail->next = (queue_node_t *) malloc(sizeof (*tail));
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
    for (i = 0; i < GAUSSIAN_CONVOLUTION_COUNT; i++) {
        for (x = 0; x < width; x++) {
            for (y = 0; y < height; y++) {
                for (s = t = p = 0; p < 5; p++) {
                    for (q = 0; q < 5; q++) {
                        if (y + (p - 2) >= 0 && y + (p - 2) < height &&
                            x + (q - 2) >= 0 && x + (q - 2) < width) {
                            s += gaussian[p][q];
                            t += cells[x + (q - 2)][y + (p - 2)].hardness * gaussian[p][q];
                        }
                    }
                }

                cells[x][y].hardness = t / s;
            }
        }
    }
}

void dungeon_t_new::fill_outside() {
    int i;
    for (i = 0; i < width; i++) {
        cells[i][0].type = CELL_TYPE_STONE;
        cells[i][0].hardness = 255;
        cells[i][0].attributes |= CELL_ATTRIBUTE_IMMUTABLE;
        cells[i][height - 1].type = CELL_TYPE_STONE;
        cells[i][height - 1].hardness = 255;
        cells[i][height - 1].attributes |= CELL_ATTRIBUTE_IMMUTABLE;
    }
    for (i = 1; i < height - 1; i++) {
        cells[0][i].type = CELL_TYPE_STONE;
        cells[0][i].hardness = 255;
        cells[0][i].attributes |= CELL_ATTRIBUTE_IMMUTABLE;
        cells[width - 1][i].type = CELL_TYPE_STONE;
        cells[width - 1][i].hardness = 255;
        cells[width - 1][i].attributes |= CELL_ATTRIBUTE_IMMUTABLE;
    }
}

void dungeon_t_new::create_rooms(int count, uint8_t min_width, uint8_t min_height, int size_randomness_max) {
    int i;
    int room_width, room_height;

    if (count > max_room_count) {
        // Not sure what the conventions are here, but I want exceptions and hate sprintf :)
        // https://stackoverflow.com/a/5591169
        throw std::runtime_error("create_rooms caller wants " + std::to_string(count) + " rooms, but the max is " + std::to_string(max_room_count));
    }

    for (i = 0; i < count; i++) {
        room_width = min_width + (rand() % size_randomness_max);
        room_height = min_height + (rand() % size_randomness_max);

        try {
            create_room(rooms + i, room_width, room_height);
        }
        catch (std::runtime_error e) {
            if (i < min_room_count) throw std::runtime_error("failed to create the minimum number of rooms (full?)");
        }

        room_count++;
    }
}

void dungeon_t_new::create_room(room_t *room, uint8_t room_width, uint8_t room_height) {
    int x_offset = rand();
    int y_offset = rand();
    uint8_t ix, iy;
    uint8_t x, y;
    int16_t jx, jy;
    int placed = 0;
    
    for (ix = 0; ix < width && !placed; ix++) {
        x = (ix + x_offset) % width;
        for (iy = 0; iy < height && !placed; iy++) {
            y = (iy + y_offset) % height;

            // Make sure the area is clear.
            placed = 1;
            for (jx = x - 1; jx < x + room_width + 1 && placed; jx++) {
                for (jy = y - 1; jy < y + room_height + 1; jy++) {
                    if (jx < 0 || jx >= width || jy < 0 || jy >= height 
                        || cells[jx][jy].type != CELL_TYPE_STONE
                        || cells[jx][jy].attributes & CELL_ATTRIBUTE_IMMUTABLE) {
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
                        cells[jx][jy].type = CELL_TYPE_ROOM;
                        cells[jx][jy].hardness = 0;
                    }
                }
            }
        }
    }
    if (!placed) throw std::runtime_error("no space available to place room");
}

void dungeon_t_new::connect_rooms() {
    // Each room will connect to its nearest room until every room is marked as visited.
    int i;
    int visited[room_count];
    for (i = 0; i < room_count; i++) visited[i] = 0;
    visited[0] = 1;
    room_t *a;
    room_t *b;
    room_t *current;
    uint8_t ax, bx, ay, by;
    double distance, max_distance;
    int done = 0;

    while (!done) {
        // Pick the first un-visited room.
        for (i = 1; i < room_count; i++)
            if (!visited[i]) {
                a = rooms + i;
                visited[i] = 1;
                break;
            }
        
        // Find the nearest visited room.
        b = NULL;
        max_distance = INFINITY;
        for (i = 0; i < room_count; i++)
            if (rooms + i != a && visited[i]) {
                current = rooms + i;
                // Find the center points of each room.
                ax = (a->x0 + a->x1) / 2;
                ay = (a->y0 + a->y1) / 2;
                bx = (current->x0 + current->x1) / 2;
                by = (current->y0 + current->y1) / 2;
                distance = pow(ax - bx, 2) + pow(ay - by, 2);

                if (distance < max_distance) {
                    b = rooms + i;
                    max_distance = distance;
                    visited[i] = 1;
                }
            }

        // If there's only one room, we never get a b assigned.
        // That means there's nothing to connect and we can exit.
        if (b == NULL) return;

        // Connect a random point from each room.
        ax = (a->x0 + (rand() % (a->x1 - a->x0)));
        ay = (a->y0 + (rand() % (a->y1 - a->y0)));
        bx = (b->x0 + (rand() % (b->x1 - b->x0)));
        by = (b->y0 + (rand() % (b->y1 - b->y0)));

        connect_points(ax, ay, bx, by);
        done = 1;
        for (i = 0; i < room_count; i++)
            if (!visited[i]) {
                done = 0;
                break;
            }
    }
}

void dungeon_t_new::connect_points(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
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
        x_poss = x + x_direction >= 0 && x + x_direction < width;
        y_poss = y + y_direction >= 0 && y + y_direction < height;

        // We cannot place on immutable blocks.
        if (cells[x + x_direction][y].attributes & CELL_ATTRIBUTE_IMMUTABLE) x_poss = 0;
        if (cells[x][y + y_direction].attributes & CELL_ATTRIBUTE_IMMUTABLE) y_poss = 0;

        // We won't place any more if we're aligned with the room.
        if (x == x1) x_poss = 0;
        if (y == y1) y_poss = 0;

        // If both directions are impossible, this is an error.
        // In our normal room configuration, this is impossible regardless of what random choices
        // we generate, since each room is within the bounds of the mutable area of the room.
        if (!x_poss && !y_poss)
            throw std::runtime_error(
                "connect_points from ("
                + std::to_string(x0) + ", " + std::to_string(y0) + ") to (" + std::to_string(x1) + ", " + std::to_string(y1)
                + "has no possible next location at (" + std::to_string(x) + ", " + std::to_string(y) + ")");

        // Flip directions if we can't place in our desired direction.
        if (direction == 0 && !x_poss) direction = 1;
        if (direction == 1 && !y_poss) direction = 0;

        if (direction == 0) x += x_direction;
        else y += y_direction;

        if (cells[x][y].type == CELL_TYPE_STONE) {
            cells[x][y].type = CELL_TYPE_HALL;
            cells[x][y].hardness = 0;
        }
    }
}

void dungeon_t_new::place_staircases() {
    room_t room;
    if (room_count < 1) throw std::runtime_error("attempted to place but no rooms exist");

    // Pick a random location in a room for the up staircase.
    room = rooms[rand() % room_count];
    place_in_room(&room, CELL_TYPE_UP_STAIRCASE);

    // And again for down...
    room = rooms[rand() % room_count];
    place_in_room(&room, CELL_TYPE_DOWN_STAIRCASE);
}

coordinates_t dungeon_t_new::place_in_room(room_t *room, cell_type_t material) {
    coordinates_t coords = random_location_in_room(room);
    cells[coords.x][coords.y].type = material;
    cells[coords.x][coords.y].hardness = 0;
}

coordinates_t dungeon_t_new::random_location_in_room(room_t *room) {
    uint8_t x_offset = rand();
    uint8_t y_offset = rand();
    uint8_t i, j, x, y;
    uint8_t room_width = room->x1 - room->x0;
    uint8_t room_height = room->y1 - room->y0;
    coordinates_t coords;
    for (i = 0; i < room_width; i++) {
        x = room->x0 + (x_offset + i) % room_width;
        for (j = 0; j < room_height; j++) {
            y = room->y0 + (y_offset + j) % room_height;

            if (cells[x][y].type == CELL_TYPE_ROOM) {
                // We will additionally check that we aren't obstructing a hallway,
                // since that may be annoying in the future.
                if ((x - 1 >= 0 && cells[x - 1][y].type == CELL_TYPE_HALL) \
                    || (y - 1 >= 0 && cells[x][y - 1].type == CELL_TYPE_HALL) \
                    || (x + 1 < width && cells[x + 1][y].type == CELL_TYPE_HALL) \
                    || (y + 1 < height && cells[x][y + 1].type == CELL_TYPE_HALL)) {
                        continue;
                    }

                // And, we can't place in an immutable cell.
                if (cells[x][y].attributes & CELL_ATTRIBUTE_IMMUTABLE) continue;

                // Also, don't overwrite a character.
                if (cells[x][y].character) continue;
                
                coords.x = x;
                coords.y = y;
                return coords;
            }
        }
    }

    throw std::runtime_error("couldn't find a location to place in");
}

coordinates_t dungeon_t_new::random_location() {
    int room_offset = rand();
    int i;
    room_t *room;

    for (i = 0; i < room_count; i++) {
        room = rooms + ((i + room_offset) % room_count);
        try {
            return random_location_in_room(room);
        } catch (std::runtime_error e) {}
    }
    throw std::runtime_error("no available space in dungeon");
}
