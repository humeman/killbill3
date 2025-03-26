#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "../dungeon.h"
#include "../files.h"
#include "../pathfinding.h"
#include "../macros.h"
#include "../character.h"
#include "../game.h"
#include "../macros.h"

#define DEFAULT_PATH "/.rlg327/dungeon"

int prepare_args(int argc, char* argv[], int *read, int *write, int *debug, int *nummon, char **path);

int main(int argc, char* argv[]) {
    int x, y;
    dungeon dungeon;

    srand(time(NULL));

    int read = 0;
    int write = 0;
    int debug = 0;
    int nummon = -1;
    char *path = NULL;
    if (prepare_args(argc, argv, &read, &write, &debug, &nummon, &path)) {
        return 1;
    }

    FILE *f;
    if (read) {
        f = fopen(path, "rb");
        if (f == NULL) {
            fprintf(stderr, "err: could not open file %s\n", path);
            free(path);
            return 1;
        }
        if (dungeon_init_from_file(&dungeon, f, debug)) {
            fprintf(stderr, "err: could not load dungeon from file %s\n", path);
            free(path);
            fclose(f);
            return 1;
        }
        fclose(f);
    }
    else {
        if (dungeon_init(&dungeon, DUNGEON_WIDTH, DUNGEON_HEIGHT, ROOM_MIN_COUNT + ROOM_COUNT_MAX_RANDOMNESS)) {
            fprintf(stderr, "err: could not allocate memory for dungeon\n");
            free(path);
            return 1;
        }
        for (x = 0; x < dungeon.width; x++)
            for (y = 0; y < dungeon.height; y++)
                dungeon.cells[x][y].type = CELL_TYPE_STONE;

        if (fill_dungeon(&dungeon, ROOM_MIN_COUNT, ROOM_COUNT_MAX_RANDOMNESS, ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT, ROOM_MAX_RANDOMNESS, debug)) {
            fprintf(stderr, "err: could not generate dungeon\n");
            dungeon_destroy(&dungeon);
            free(path);
            return 1;
        }
    }

    if (write) {
        f = fopen(path, "wb");
        if (f == NULL) {
            fprintf(stderr, "err: could not open file %s\n", path);
            dungeon_destroy(&dungeon);
            free(path);
            return 1;
        }
        if (dungeon_save(&dungeon, f, debug)) {
            fprintf(stderr, "err: could not save dungeon to %s\n", path);
            dungeon_destroy(&dungeon);
            free(path);
            fclose(f);
            return 1;
        }
        fclose(f);
        printf("saved dungeon to %s\n", path);
    }

    if (debug) {
        write_dungeon_pgm(&dungeon);
        printf("debug: wrote hardness map to dungeon.pgm\n");
    }

    if (update_pathfinding(&dungeon)) {
        fprintf(stderr, "err: failed to generate pathfinding maps\n");
        dungeon_destroy(&dungeon);
        free(path);
        return 1;
    }

    character *character;
    uint32_t trash;
    // The PC will play if it's in the heap. We'll let it go first since it'll probably never win :)
    character = &(dungeon.pc);
    if (heap_insert(dungeon.turn_queue, (void *) &character, 0)) {
        fprintf(stderr, "err: failed to insert PC into heap\n");
        dungeon_destroy(&dungeon);
        free(path);
        return 1;
    }
    if (generate_monsters(&dungeon, nummon < 0 ? (rand() % (RANDOM_MONSTERS_MAX - RANDOM_MONSTERS_MIN + 1)) + RANDOM_MONSTERS_MIN : nummon)) {
        fprintf(stderr, "err: couldn't place monsters\n");
        dungeon_destroy(&dungeon);
        free(path);
        return 1;
    }

    if (game_start(&dungeon, nummon)) {
        dungeon_destroy(&dungeon);
        free(path);
        RETURN_ERROR("game loop failed");
    }

    while (heap_size(dungeon.turn_queue) > 0) {
        if (heap_remove(dungeon.turn_queue, (void *) &character, &trash)) {
            fprintf(stderr, "err: failed to remove from turn queue while cleaning up\n");
            dungeon_destroy(&dungeon);
            free(path);
            return 1;
        }
        if (character == &(dungeon.pc)) continue;
        destroy_character(&dungeon, character);
    }
    
    dungeon_destroy(&dungeon);
    free(path);

    return 0;
}

int prepare_args(int argc, char* argv[], int *read, int *write, int *debug, int *nummon, char **path) {
    int i;
    int path_next = 0;
    int nummon_next = 0;
    int custom_path = 0;
    long temp;
    char *err;
    char *home = getenv("HOME");
    *path = malloc(sizeof (char) * (strlen(home) + strlen(DEFAULT_PATH) + 1));
    if (*path == NULL) {
        fprintf(stderr, "err: failed to allocate memory\n");
        return 1;
    }
    strcpy(*path, home);
    strcat(*path, DEFAULT_PATH);
    *nummon = -1;
    for (i = 1; i < argc; i++) {
        if (path_next) {
            free(*path);
            // This is unnecessary, but simplifies all the free()s later.
            *path = malloc(sizeof (char) * (strlen(argv[i]) + 1));
            if (*path == NULL) {
                fprintf(stderr, "err: failed to allocate memory\n");
                return 1;
            }
            strcpy(*path, argv[i]);
            path_next = 0;
            custom_path = 1;
        }
        else if (nummon_next) {
            // https://stackoverflow.com/questions/2024648/convert-a-string-to-int-but-only-if-really-is-an-int
            temp = strtol(argv[i], &err, 10);
            if (*err != '\0' || temp > INT32_MAX || temp < 0) {
                fprintf(stderr, "err: -n/--nummon must be an int\n");
                free(*path);
                return 1;
            }
            *nummon = (int) temp;
            nummon_next = 0;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--load") == 0) *read = 1;
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--save") == 0) *write = 1;
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) *debug = 1;
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) path_next = 1;
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nummon") == 0) nummon_next = 1;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [-l] [-s] [-d] [-p <path>] [-n <nummon>] [-f <fps>]\n  -l/--load: load a rungeon\n  -s/--save: write a random dungeon\n", argv[0]);
            printf("  -p/--path: override path to load/save (default ~/.rlg327/dungeon)\n");
            printf("  -n/--nummon: override monster count (default 10)\n");
            printf("  -d/--debug: enable debugging features\n  -h/--help: display this message\n");
            free(*path);
            return 1;
        }
        else {
            fprintf(stderr, "err: unrecognized argument: %s. run -h/--help for usage\n", argv[i]);
            free(*path);
            return 1;
        }
    }
    if (path_next) {
        fprintf(stderr, "err: specify a path after -p/--path\n");
        free(*path);
        return 1;
    }

    if (nummon_next) {
        fprintf(stderr, "err: specify a number of monsters after -n/--nummon\n");
        free(*path);
        return 1;
    }

    // It's an error to specify a path without reading or writing
    if (custom_path && !*read && !*write) {
        fprintf(stderr, "err: specify one of -l/--load, -w/--write with -p/--path\n");
        free(*path);
        return 1;
    }
    return 0;
}