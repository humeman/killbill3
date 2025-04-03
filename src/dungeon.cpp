#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "dungeon.h"
#include "macros.h"
#include <stdexcept>
#include <string.h>
#include "character.h"

#define STONE_SEED_COUNT 10
#define GAUSSIAN_CONVOLUTION_COUNT 2

#define ENSURE_INITIALIZED if (!is_initalized) throw dungeon_exception(__PRETTY_FUNCTION__, "dungeon is not initialized")

dungeon_t::dungeon_t(uint8_t width, uint8_t height, int max_rooms) {
    int i, j;
    this->width = width;
    this->height = height;
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
        }
    }

    is_initalized = false;

    return;

    init_free_cells:
    free(cells);
    init_free_rooms:
    free(rooms);
    init_free:
    throw dungeon_exception(__PRETTY_FUNCTION__, "failed to allocate dungeon");
}

dungeon_t::~dungeon_t() {
    int i;
    for (i = 0; i < width; i++) {
        free(cells[i]);
    }
    free(cells);
    free(rooms);
}

void dungeon_t::write_pgm() {
    ENSURE_INITIALIZED;
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

void dungeon_t::fill(int min_rooms, int room_count_randomness_max, int room_min_width, int room_min_height, int room_size_randomness_max, int debug) {
    fill_stone();
    room_count = 0;
    min_room_count = min_rooms;
    create_rooms(min_rooms + (rand() % room_count_randomness_max), room_min_width, room_min_height, room_size_randomness_max);
    connect_rooms();
    fill_outside();
    place_staircases();
    is_initalized = true;
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
void dungeon_t::fill_stone() {
    uint8_t x, y, ix, iy;
    queue_node_t *head, *tail, *temp;
    int i, step, s, t, p, q;

    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            cells[x][y].type = CELL_TYPE_STONE;
            cells[x][y].hardness = 0;
            cells[x][y].attributes = 0;
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

void dungeon_t::fill_outside() {
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

void dungeon_t::create_rooms(int count, uint8_t min_width, uint8_t min_height, int size_randomness_max) {
    int i;
    int room_width, room_height;

    if (count > max_room_count) {
        // Not sure what the conventions are here, but I want exceptions and hate sprintf :)
        // https://stackoverflow.com/a/5591169
        throw dungeon_exception(__PRETTY_FUNCTION__, "create_rooms caller wants " + std::to_string(count) + " rooms, but the max is " + std::to_string(max_room_count));
    }

    for (i = 0; i < count; i++) {
        room_width = min_width + (rand() % size_randomness_max);
        room_height = min_height + (rand() % size_randomness_max);

        try {
            create_room(rooms + i, room_width, room_height);
        }
        catch (dungeon_exception &e) {
            if (i < min_room_count) 
                throw dungeon_exception(__PRETTY_FUNCTION__, e, "failed to create the minimum number of rooms (full?)");
        }

        room_count++;
    }
}

void dungeon_t::create_room(room_t *room, uint8_t room_width, uint8_t room_height) {
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
    if (!placed) throw dungeon_exception(__PRETTY_FUNCTION__, "no space available to place room");
}

void dungeon_t::connect_rooms() {
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

void dungeon_t::connect_points(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
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
            throw dungeon_exception(
                __PRETTY_FUNCTION__,
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

void dungeon_t::place_staircases() {
    room_t room;
    if (room_count < 1) throw dungeon_exception(__PRETTY_FUNCTION__, "attempted to place but no rooms exist");

    // Pick a random location in a room for the up staircase.
    room = rooms[rand() % room_count];
    place_in_room(&room, CELL_TYPE_UP_STAIRCASE);

    // And again for down...
    room = rooms[rand() % room_count];
    place_in_room(&room, CELL_TYPE_DOWN_STAIRCASE);
}

coordinates_t dungeon_t::place_in_room(room_t *room, cell_type_t material) {
    coordinates_t coords = random_location_in_room(room);
    cells[coords.x][coords.y].type = material;
    cells[coords.x][coords.y].hardness = 0;
    return coords;
}

coordinates_t dungeon_t::random_location_in_room(room_t *room) {
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
                
                coords.x = x;
                coords.y = y;
                return coords;
            }
        }
    }

    throw dungeon_exception(__PRETTY_FUNCTION__, "couldn't find a location to place in");
}

coordinates_t dungeon_t::random_location() {
    int room_offset = rand();
    int i;
    room_t *room;

    for (i = 0; i < room_count; i++) {
        room = rooms + ((i + room_offset) % room_count);
        try {
            return random_location_in_room(room);
        } catch (dungeon_exception &e) {}
    }
    throw dungeon_exception(__PRETTY_FUNCTION__, "no available space in dungeon");
}

void dungeon_t::fill_from_file(FILE *f, int debug, coordinates_t *pc_coords) {
    size_t size;
    uint32_t version, file_size;
    uint8_t hardness, x, y, x0, y0;
    uint16_t down_staircase_count, up_staircase_count;
    int i;
    room_t *temp;
    int header_size = strlen(FILE_HEADER);
    char header[header_size + 1];
    
    size = fread(header, sizeof (*header), header_size, f);
    if (size != strlen(FILE_HEADER)) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not an RLG327 file (file ended too early)");
    header[header_size] = 0; // ensures null byte to terminate
    if (debug) printf("debug: file header = %s\n", header);
    if (strcmp(FILE_HEADER, header)) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not an RLG327 file (header mismatch)");

    READ_UINT32(version, "version", f, debug);
    if (version != 0) throw dungeon_exception(__PRETTY_FUNCTION__, "this program is incompatible with the provided file's version");

    READ_UINT32(file_size, "file size", f, debug);

    READ_UINT8(pc_coords->x, "pc x", f, debug);
    READ_UINT8(pc_coords->y, "pc y", f, debug);

    // An unfortunate consequence of the room count being stored after this is
    // that we need the room count to initialize the dungeon. This isn't ideal,
    // but will work.
    if (fseek(f, FILE_ROOM_COUNT_OFFSET, SEEK_SET)) dungeon_exception(__PRETTY_FUNCTION__, "unexpected error while reading file (could not seek to room count)");

    READ_UINT16(room_count, "room count", f, debug);

    // We may not have allocated enough rooms for this dungeon.
    // If so, we'll realloc that.
    if (max_room_count < room_count) {
        temp = (room_t *) realloc(rooms, room_count * sizeof (room_t));
        if (temp == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "failed to realloc room array");
        rooms = temp;
    }
    max_room_count = room_count;

    if (fseek(f, FILE_MATRIX_OFFSET, SEEK_SET)) throw dungeon_exception(__PRETTY_FUNCTION__, "unexpected error while reading file (could not seek to dungeon matrix)");
    for (y = 0; y < DUNGEON_HEIGHT; y++) {
        for (x = 0; x < DUNGEON_WIDTH; x++) {
            READ_UINT8(hardness, "cell matrix", f, debug);
            cells[x][y].hardness = hardness;
            cells[x][y].type = hardness == 0 ? CELL_TYPE_HALL : CELL_TYPE_STONE;
        }
    }
    if (fseek(f, FILE_ROOM_COUNT_OFFSET + sizeof (room_count), SEEK_SET)) throw dungeon_exception(__PRETTY_FUNCTION__, "unexpected error while reading file (could not seek past room count)");

    for (i = 0; i < room_count; i++) {
        if (debug) printf("debug: reading room %d\n", i);
        READ_UINT8(x0, "room x0", f, debug);
        READ_UINT8(y0, "room y0", f, debug);
        READ_UINT8(width, "room width", f, debug);
        READ_UINT8(height, "room height", f, debug);

        rooms[i].x0 = x0;
        rooms[i].y0 = y0;
        rooms[i].x1 = x0 + width - 1;
        rooms[i].y1 = y0 + height - 1;

        for (x = rooms[i].x0; x <= rooms[i].x1; x++)
            for (y = rooms[i].y0; y<= rooms[i].y1; y++)
                cells[x][y].type = CELL_TYPE_ROOM;
    }

    READ_UINT16(up_staircase_count, "up staircase count", f, debug);
    for (i = 0; i < up_staircase_count; i++) {
        if (debug) printf("debug: reading up staircase %d\n", i);
        READ_UINT8(x, "up staircase x", f, debug);
        READ_UINT8(y, "up staircase y", f, debug);

        cells[x][y].type = CELL_TYPE_UP_STAIRCASE;
    }
    READ_UINT16(down_staircase_count, "down staircase count", f, debug);
    for (i = 0; i < down_staircase_count; i++) {
        if (debug) printf("debug: reading down staircase %d\n", i);
        READ_UINT8(x, "down staircase x", f, debug);
        READ_UINT8(y, "down staircase y", f, debug);
        cells[x][y].type = CELL_TYPE_DOWN_STAIRCASE;
    }

    is_initalized = true;
}

void dungeon_t::save_to_file(FILE *f, int debug, coordinates_t *pc_coords) {
    ENSURE_INITIALIZED;
    uint32_t version, file_size;
    uint8_t width, height;
    int up_count, down_count, x, y;
    coordinates_t *up, *down;
    char header[] = FILE_HEADER;
    int header_size = strlen(header);
    size_t size = fwrite(header, sizeof (*header), header_size, f);

    if (size != (size_t) header_size) throw dungeon_exception(__PRETTY_FUNCTION__, "could not write to file");
    if (debug) printf("debug: header = %s, wrote %ld bytes\n", header, sizeof (*header) * header_size);

    version = FILE_VERSION;
    WRITE_UINT32(version, "version", f, debug);

    // File size calculation:
    // 1708 + r * 4 + u * 2 + d * 2
    up_count = 0;
    down_count = 0;
    up = NULL;
    down = NULL;
    for (x = 0; x < DUNGEON_WIDTH; x++) {
        for (y = 0; y < DUNGEON_HEIGHT; y++) {
            if (cells[x][y].type == CELL_TYPE_UP_STAIRCASE) {
                up_count++;
                if (up == NULL)
                    up = (coordinates_t *) malloc(sizeof (*up));
                else
                    up = (coordinates_t *) realloc(up, up_count * sizeof (*up));
                up[up_count - 1].x = x;
                up[up_count - 1].y = y;
            } 
            else if (cells[x][y].type == CELL_TYPE_DOWN_STAIRCASE) {
                down_count++;
                if (down == NULL)
                    down = (coordinates_t *) malloc(sizeof (*down));
                else
                    down = (coordinates_t *) realloc(down, down_count * sizeof (*down));
                down[down_count - 1].x = x;
                down[down_count - 1].y = y;
            }
        }
    }
    file_size = 1708 + room_count * 4 + up_count * 2 + down_count * 2;
    WRITE_UINT32(file_size, "file size", f, debug);

    WRITE_UINT8(pc_coords->x, "pc x", f, debug);
    WRITE_UINT8(pc_coords->y, "pc y", f, debug);

    for (y = 0; y < DUNGEON_HEIGHT; y++) {
        for (x = 0; x < DUNGEON_WIDTH; x++) {
            WRITE_UINT8((cells[x][y].hardness), "cell matrix", f, debug);
        }
    }

    WRITE_UINT16(room_count, "room count", f, debug);
    int i;
    for (i = 0; i < room_count; i++) {
        if (debug) printf("debug: writing room %d\n", i);
        WRITE_UINT8((rooms[i].x0), "room x0", f, debug);
        WRITE_UINT8((rooms[i].y0), "room y0", f, debug);
        width = rooms[i].x1 - rooms[i].x0 + 1;
        height = rooms[i].y1 - rooms[i].y0 + 1;
        WRITE_UINT8(width, "room width", f, debug);
        WRITE_UINT8(height, "room height", f, debug);
    }

    WRITE_UINT16(up_count, "up staircase count", f, debug);
    for (i = 0; i < up_count; i++) {
        if (debug) printf("debug: writing up staircase %d\n", i);
        WRITE_UINT8((up[i].x), "up staircase x", f, debug);
        WRITE_UINT8((up[i].y), "up staircase y", f, debug);
    }
    WRITE_UINT16(down_count, "down staircase count", f, debug);
    for (i = 0; i < down_count; i++) {
        if (debug) printf("debug: writing down staircase %d\n", i);
        WRITE_UINT8((down[i].x), "down staircase x", f, debug);
        WRITE_UINT8((down[i].y), "down staircase y", f, debug);
    }

    free(up);
    free(down);
}
