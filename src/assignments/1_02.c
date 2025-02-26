#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../dungeon.h"
#include "../files.h"

#define ROOM_MIN_COUNT 6
#define ROOM_COUNT_MAX_RANDOMNESS 4
#define ROOM_MIN_WIDTH 4
#define ROOM_MIN_HEIGHT 3
#define ROOM_MAX_RANDOMNESS 6
#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21

#define DEFAULT_PATH "/.rlg327/dungeon"

int prepare_args(int argc, char* argv[], int *draw_border, int *read, int *write, int *debug, char **path);

int main(int argc, char* argv[]) {
    int x, y;
    dungeon dungeon;

    srand(time(NULL));

    int draw_border = 0; 
    int read = 0;
    int write = 0;
    int debug = 0;
    char *path = NULL;
    if (prepare_args(argc, argv, &draw_border, &read, &write, &debug, &path)) {
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
        printf("loaded dungeon from %s\n", path);
    }
    else {
        if (dungeon_init(&dungeon, DUNGEON_WIDTH, DUNGEON_HEIGHT, ROOM_MIN_COUNT + ROOM_COUNT_MAX_RANDOMNESS)) {
            fprintf(stderr, "err: could not allocate memory for dungeon\n");
            free(path);
            return 1;
        }

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

    // print out the filled dungeon
    if (draw_border) {
        printf("*");
        for (x = 0; x < dungeon.width; x++) printf("-");
        printf("*\n");
    }
    for (y = 0; y < dungeon.height; y++) {
        if (draw_border) printf("|");
        for (x = 0; x < dungeon.width; x++) {
            if (x == dungeon.pc_x && y == dungeon.pc_y) printf("%c", CELL_TYPE_PC);
            else printf("%c", dungeon.cells[x][y].type);
        }
        if (draw_border) printf("|");
        printf("\n");
    }
    if (draw_border) {
        printf("*");
        for (x = 0; x < dungeon.width; x++) printf("-");
        printf("*\n");
    }

    dungeon_destroy(&dungeon);
    free(path);

    return 0;
}

int prepare_args(int argc, char* argv[], int *draw_border, int *read, int *write, int *debug, char **path) {
    int i;
    int path_next = 0;
    int custom_path = 0;
    char* home = getenv("HOME");
    *path = malloc(sizeof (char) * (strlen(home) + strlen(DEFAULT_PATH) + 1));
    if (*path == NULL) {
        fprintf(stderr, "err: failed to allocate memory\n");
        return 1;
    }
    strcpy(*path, home);
    strcat(*path, DEFAULT_PATH);
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
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--load") == 0) *read = 1;
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--save") == 0) *write = 1;
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) *debug = 1;
        else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--border") == 0) *draw_border = 1;
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) path_next = 1;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [-lwbhd] [-p <path>]\n  -l/--load: load a rungeon\n  -s/--save: write a random dungeon\n", argv[0]);
            printf("  -b/--border: draw a border around the dungeon\n  -p/--path: override path to load/save ");
            printf("(default ~/.rlg327/dungeon)\n  -d/--debug: enable debugging features\n  -h/--help: display this message\n");
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

    // It's an error to specify a path without reading or writing
    if (custom_path && !*read && !*write) {
        fprintf(stderr, "err: specify one of -l/--load, -w/--write with -p/--path\n");
        free(*path);
        return 1;
    }
    return 0;
}