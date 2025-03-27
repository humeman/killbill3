#include <cstring>
#include <endian.h>
#include <cstdint>
#include <cstdlib>

#include "files.h"
#include "dungeon.h"
#include "macros.h"

#define FILE_HEADER "RLG327-S2025"
#define FILE_VERSION 0
#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21

#define FILE_MATRIX_OFFSET 22
#define FILE_ROOM_COUNT_OFFSET 1702

// These would be functions, but in the interest of saving on lines of code by avoiding any extra error checks,
// these are simple macros that expand out to the 3/4 lines necessary to read, convert, and validate data.

// Reads a uint32 from f into the variable specified in 'var'.
#define READ_UINT32(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) RETURN_ERROR("the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be32toh(var); \
    if (debug) printf("debug: %s = %u\n", description, var); }

// Reads a uint16 from f into the variable specified in 'var'.
#define READ_UINT16(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) RETURN_ERROR("the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be16toh(var); \
    if (debug) printf("debug: %s = %u\n", description, var); }

// Reads a uint8 from f into the variable specified in 'var'.
#define READ_UINT8(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) RETURN_ERROR("the specified file is not a valid RLG327 file (file ended too early)"); \
    if (debug) printf("debug: %s = %u\n", description, var); }

// Writes the uint32 variable 'var' into f.
#define WRITE_UINT32(var, description, f, debug) { \
    uint32_t be = htobe32(var); \
    size_t size = fwrite(&be, sizeof (uint32_t), 1, f); \
    if (size != 1) RETURN_ERROR("could not write to file"); \
    if (debug) printf("debug: %s = %u, wrote %ld bytes\n", description, var, sizeof (var)); }

// Writes the uint16 variable 'var' into f.
#define WRITE_UINT16(var, description, f, debug) { \
    uint16_t be = htobe16(var); \
    size_t size = fwrite(&be, sizeof (uint16_t), 1, f); \
    if (size != 1) RETURN_ERROR("could not write to file"); \
    if (debug) printf("debug: %s = %u, wrote %ld bytes\n", description, var, sizeof (var)); }

// Writes the uint8 variable 'var' into f.
#define WRITE_UINT8(var, description, f, debug) { \
    size_t size = fwrite(&var, sizeof (uint8_t), 1, f); \
    if (size != 1) RETURN_ERROR("could not write to file"); \
    if (debug) printf("debug: %s = %u, wrote %ld bytes\n", description, var, sizeof (var)); }

int dungeon_init_from_file(dungeon_t *dungeon, FILE *f, int debug) {
    size_t size;
    uint32_t version, file_size;
    uint8_t pc_x, pc_y, hardness, x, y, x0, y0, width, height;
    uint16_t room_count, down_staircase_count, up_staircase_count;
    int i;
    int header_size = strlen(FILE_HEADER);
    char header[header_size + 1];
    
    size = fread(header, sizeof (*header), header_size, f);
    if (size != strlen(FILE_HEADER)) RETURN_ERROR("the specified file is not an RLG327 file (file ended too early)");
    header[header_size] = 0; // ensures null byte to terminate
    if (debug) printf("debug: file header = %s\n", header);
    if (strcmp(FILE_HEADER, header)) RETURN_ERROR("the specified file is not an RLG327 file (header mismatch)");

    READ_UINT32(version, "version", f, debug);
    if (version != 0) RETURN_ERROR("this program is incompatible with the provided file's version");

    READ_UINT32(file_size, "file size", f, debug);

    READ_UINT8(pc_x, "pc x", f, debug);
    READ_UINT8(pc_y, "pc y", f, debug);

    // An unfortunate consequence of the room count being stored after this is
    // that we need the room count to initialize the dungeon. This isn't ideal,
    // but will work.
    if (fseek(f, FILE_ROOM_COUNT_OFFSET, SEEK_SET)) RETURN_ERROR("unexpected error while reading file (could not seek to room count)");

    READ_UINT16(room_count, "room count", f, debug);

    if (dungeon_init(dungeon, DUNGEON_WIDTH, DUNGEON_HEIGHT, MAX(room_count, ROOM_MIN_COUNT + ROOM_COUNT_MAX_RANDOMNESS))) RETURN_ERROR("could not initialize dungeon");

    if (fseek(f, FILE_MATRIX_OFFSET, SEEK_SET)) RETURN_ERROR("unexpected error while reading file (could not seek to dungeon matrix)");
    for (y = 0; y < DUNGEON_HEIGHT; y++) {
        for (x = 0; x < DUNGEON_WIDTH; x++) {
            READ_UINT8(hardness, "cell matrix", f, debug);
            dungeon->cells[x][y].hardness = hardness;
            dungeon->cells[x][y].type = hardness == 0 ? CELL_TYPE_HALL : CELL_TYPE_STONE;
        }
    }
    if (fseek(f, FILE_ROOM_COUNT_OFFSET + sizeof (room_count), SEEK_SET)) RETURN_ERROR("unexpected error while reading file (could not seek past room count)");

    for (i = 0; i < room_count; i++) {
        if (debug) printf("debug: reading room %d\n", i);
        READ_UINT8(x0, "room x0", f, debug);
        READ_UINT8(y0, "room y0", f, debug);
        READ_UINT8(width, "room width", f, debug);
        READ_UINT8(height, "room height", f, debug);

        dungeon->rooms[i].x0 = x0;
        dungeon->rooms[i].y0 = y0;
        dungeon->rooms[i].x1 = x0 + width - 1;
        dungeon->rooms[i].y1 = y0 + height - 1;

        for (x = dungeon->rooms[i].x0; x <= dungeon->rooms[i].x1; x++)
            for (y = dungeon->rooms[i].y0; y<= dungeon->rooms[i].y1; y++)
                dungeon->cells[x][y].type = CELL_TYPE_ROOM;
    }
    dungeon->room_count = room_count;

    READ_UINT16(up_staircase_count, "up staircase count", f, debug);
    for (i = 0; i < up_staircase_count; i++) {
        if (debug) printf("debug: reading up staircase %d\n", i);
        READ_UINT8(x, "up staircase x", f, debug);
        READ_UINT8(y, "up staircase y", f, debug);

        dungeon->cells[x][y].type = CELL_TYPE_UP_STAIRCASE;
    }
    READ_UINT16(down_staircase_count, "down staircase count", f, debug);
    for (i = 0; i < down_staircase_count; i++) {
        if (debug) printf("debug: reading down staircase %d\n", i);
        READ_UINT8(x, "down staircase x", f, debug);
        READ_UINT8(y, "down staircase y", f, debug);
        dungeon->cells[x][y].type = CELL_TYPE_DOWN_STAIRCASE;
    }

    dungeon->pc.x = pc_x;
    dungeon->pc.y = pc_y;
    dungeon->pc.dead = 0;
    dungeon->pc.display = '@';
    dungeon->pc.monster = NULL;
    dungeon->pc.type = CHARACTER_PC;
    dungeon->cells[pc_x][pc_y].character = &(dungeon->pc);
    
    return 0;
}

int dungeon_save(dungeon_t *dungeon, FILE *f, int debug) {
    uint32_t version, file_size;
    uint8_t width, height;
    int up_count, down_count, x, y;
    coordinates_t *up, *down;
    char header[] = FILE_HEADER;
    int header_size = strlen(header);
    size_t size = fwrite(header, sizeof (*header), header_size, f);

    if (size != (size_t) header_size) RETURN_ERROR("could not write to file");
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
            if (dungeon->cells[x][y].type == CELL_TYPE_UP_STAIRCASE) {
                up_count++;
                if (up == NULL)
                    up = (coordinates_t *) malloc(sizeof (*up));
                else
                    up = (coordinates_t *) realloc(up, up_count * sizeof (*up));
                up[up_count - 1].x = x;
                up[up_count - 1].y = y;
            } 
            else if (dungeon->cells[x][y].type == CELL_TYPE_DOWN_STAIRCASE) {
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
    file_size = 1708 + dungeon->room_count * 4 + up_count * 2 + down_count * 2;
    WRITE_UINT32(file_size, "file size", f, debug);

    WRITE_UINT8((dungeon->pc.x), "pc x", f, debug);
    WRITE_UINT8((dungeon->pc.y), "pc y", f, debug);

    for (y = 0; y < DUNGEON_HEIGHT; y++) {
        for (x = 0; x < DUNGEON_WIDTH; x++) {
            WRITE_UINT8((dungeon->cells[x][y].hardness), "cell matrix", f, debug);
        }
    }

    WRITE_UINT16((dungeon->room_count), "room count", f, debug);
    int i;
    for (i = 0; i < dungeon->room_count; i++) {
        if (debug) printf("debug: writing room %d\n", i);
        WRITE_UINT8((dungeon->rooms[i].x0), "room x0", f, debug);
        WRITE_UINT8((dungeon->rooms[i].y0), "room y0", f, debug);
        width = dungeon->rooms[i].x1 - dungeon->rooms[i].x0 + 1;
        height = dungeon->rooms[i].y1 - dungeon->rooms[i].y0 + 1;
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
    return 0;
}
