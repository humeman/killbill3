#include <string.h>
#include <endian.h>
#include <stdint.h>

#include "files.h"
#include "dungeon.h"
#include "macros.h"


int dungeon_init_from_file(dungeon *dungeon, FILE *f, int debug) {
    size_t size;
    
    int header_size = strlen(FILE_HEADER);
    char header[header_size + 1];
    size = fread(header, sizeof (*header), header_size, f);
    if (size != strlen(FILE_HEADER)) RETURN_ERROR("the specified file is not an RLG327 file (file ended too early)");
    header[header_size] = 0; // ensures null byte to terminate
    if (debug) printf("debug: file header = %s\n", header);
    if (strcmp(FILE_HEADER, header)) RETURN_ERROR("the specified file is not an RLG327 file (header mismatch)");

    uint32_t version;
    READ_UINT32(version, "version");
    if (version != 0) RETURN_ERROR("this program is incompatible with the provided file's version");

    uint32_t file_size;
    READ_UINT32(file_size, "file size");

    uint8_t pc_x, pc_y;
    READ_UINT8(pc_x, "pc x");
    READ_UINT8(pc_y, "pc y");

    // An unfortunate consequence of the room count being stored after this is
    // that we need the room count to initialize the dungeon. This isn't ideal,
    // but will work.
    if (fseek(f, FILE_ROOM_COUNT_OFFSET, SEEK_SET)) RETURN_ERROR("unexpected error while reading file (could not seek to room count)");

    uint16_t room_count;
    READ_UINT16(room_count, "room count");

    if (dungeon_init(dungeon, DUNGEON_WIDTH, DUNGEON_HEIGHT, room_count)) RETURN_ERROR("could not initialize dungeon");

    if (fseek(f, FILE_MATRIX_OFFSET, SEEK_SET)) RETURN_ERROR("unexpected error while reading file (could not seek to dungeon matrix)");
    uint8_t hardness;
    uint8_t x, y;
    for (y = 0; y < DUNGEON_HEIGHT; y++) {
        for (x = 0; x < DUNGEON_WIDTH; x++) {
            READ_UINT8(hardness, "cell matrix");
            dungeon->cells[x][y].hardness = hardness;
            dungeon->cells[x][y].type = hardness == 0 ? CELL_TYPE_HALL : CELL_TYPE_STONE;
        }
    }
    if (fseek(f, FILE_ROOM_COUNT_OFFSET + sizeof(room_count) , SEEK_SET)) RETURN_ERROR("unexpected error while reading file (could not seek past room count)");

    int i;
    uint8_t x0, y0, width, height;
    for (i = 0; i < room_count; i++) {
        if (debug) printf("debug: reading room %d\n", i);
        READ_UINT8(x0, "room x0");
        READ_UINT8(y0, "room y0");
        READ_UINT8(width, "room width");
        READ_UINT8(height, "room height");

        dungeon->rooms[i].x0 = x0;
        dungeon->rooms[i].y0 = y0;
        dungeon->rooms[i].x1 = x0 + width - 1;
        dungeon->rooms[i].y1 = y0 + height - 1;

        for (x = dungeon->rooms[i].x0; x <= dungeon->rooms[i].x1; x++)
            for (y = dungeon->rooms[i].y0; y<= dungeon->rooms[i].y1; y++)
                dungeon->cells[x][y].type = CELL_TYPE_ROOM;
    }
    dungeon->room_count = room_count;

    uint16_t up_staircase_count;
    READ_UINT16(up_staircase_count, "up staircase count");
    for (i = 0; i < up_staircase_count; i++) {
        if (debug) printf("debug: reading up staircase %d\n", i);
        READ_UINT8(x, "up staircase x");
        READ_UINT8(y, "up staircase y");

        dungeon->cells[x][y].type = CELL_TYPE_UP_STAIRCASE;
    }
    uint16_t down_staircase_count;
    READ_UINT16(down_staircase_count, "down staircase count");
    for (i = 0; i < down_staircase_count; i++) {
        if (debug) printf("debug: reading down staircase %d\n", i);
        READ_UINT8(x, "down staircase x");
        READ_UINT8(y, "down staircase y");
        dungeon->cells[x][y].type = CELL_TYPE_DOWN_STAIRCASE;
    }

    dungeon->cells[pc_x][pc_y].type = CELL_TYPE_PC;
    
    return 0;
}

int dungeon_save(dungeon *dungeon, FILE *f) {
    return 1;
}
