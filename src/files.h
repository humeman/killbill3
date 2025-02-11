#ifndef FILES_H
#define FILES_H

#include <stdio.h>

#include "dungeon.h"
#include "macros.h"

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
 *  - debug: If non-zero, enables debug logging for the file's fields.
 * Returns: 0 if successful.
 */
int dungeon_save(dungeon *dungeon, FILE *f, int debug);

#endif