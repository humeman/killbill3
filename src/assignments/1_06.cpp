#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "../dungeon.h"
#include "../pathfinding.h"
#include "../macros.h"
#include "../character.h"
#include "../game.h"
#include "../macros.h"

#define DEFAULT_PATH "/.rlg327/dungeon"

int prepare_args(int argc, char* argv[], int *read, int *write, int *debug, int *nummon, char **path);

int main(int argc, char* argv[]) {
    srand(time(NULL));

    int read = 0;
    int write = 0;
    int debug = 0;
    int nummon = -1;
    char *path = NULL;
    if (prepare_args(argc, argv, &read, &write, &debug, &nummon, &path)) {
        return 1;
    }
    
    Game game(debug, DUNGEON_WIDTH, DUNGEON_HEIGHT, ROOM_MIN_COUNT + ROOM_COUNT_MAX_RANDOMNESS);
    if (nummon >= 0) game.override_nummon(nummon);

    if (read) {
        game.init_from_file(path);
    }
    else {
        game.init_random();
    }

    if (write) {
        game.write_to_file(path);
    }

    if (debug) {
        game.dungeon->write_pgm();
        printf("debug: wrote hardness map to dungeon.pgm\n");
    }

    game.random_monsters();
    game.run();

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
    *path = (char *) malloc(sizeof (char) * (strlen(home) + strlen(DEFAULT_PATH) + 1));
    if (*path == NULL) {
        RETURN_ERROR("failed to allocate memory");
    }
    strcpy(*path, home);
    strcat(*path, DEFAULT_PATH);
    *nummon = -1;
    for (i = 1; i < argc; i++) {
        if (path_next) {
            free(*path);
            // This is unnecessary, but simplifies all the free()s later.
            *path = (char *) malloc(sizeof (char) * (strlen(argv[i]) + 1));
            if (*path == NULL) {
                RETURN_ERROR("failed to allocate memory\n");
            }
            strcpy(*path, argv[i]);
            path_next = 0;
            custom_path = 1;
        }
        else if (nummon_next) {
            // https://stackoverflow.com/questions/2024648/convert-a-string-to-int-but-only-if-really-is-an-int
            temp = strtol(argv[i], &err, 10);
            if (*err != '\0' || temp > INT32_MAX || temp < 0) {
                free(*path);
                RETURN_ERROR("-n/--nummon must be a positive int\n");
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
            free(*path);
            RETURN_ERROR("unrecognized argument: %s. run -h/--help for usage", argv[i]);
        }
    }
    if (path_next) {
        free(*path);
        RETURN_ERROR("specify a path after -p/--path");
    }

    if (nummon_next) {
        free(*path);
        RETURN_ERROR("specify a number of monsters after -n/--nummon");
    }

    // It's an error to specify a path without reading or writing
    if (custom_path && !*read && !*write) {
        free(*path);
        RETURN_ERROR("specify one of -l/--load, -w/--write with -p/--path");
    }
    return 0;
}