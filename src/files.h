#ifndef FILES_H
#define FILES_H

#include <stdio.h>

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
#define READ_UINT32(var, description) \
    size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) RETURN_ERROR("the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be32toh(var); \
    if (debug) printf("debug: %s = %u\n", description, var);

#define READ_UINT16(var, description) \
    size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) RETURN_ERROR("the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be16toh(var); \
    if (debug) printf("debug: %s = %u\n", description, var);

#define READ_UINT8(var, description) \
    size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) RETURN_ERROR("the specified file is not a valid RLG327 file (file ended too early)"); \
    if (debug) printf("debug: %s = %u\n", description, var);

/**
 * Initializes a dungeon from a loaded file.
 * 
 * Params:
 *  - dungeon: Dungeon to initialize.
 *  - f: File to read from. Must be opened with 'r'.
 *  - debug: If non-zero, enables debug logging for the file's fields.
 * Returns: 0 if successful.
 */
int dungeon_init_from_file(dungeon *dungeon, FILE *f, int debug);

/**
 * Writes an initialized dungeon to a file.
 * 
 * Params:
 *  - dungeon: Dungeon to save.
 *  - f: File to write to. Must be opened with 'w'.
 * Returns: 0 if successful.
 */
int dungeon_save(dungeon *dungeon, FILE *f);

#endif