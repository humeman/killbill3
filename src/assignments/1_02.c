#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../dungeon.h"

#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21

#define ROOM_MIN_COUNT 6
#define ROOM_COUNT_MAX_RANDOMNESS 4
#define ROOM_MIN_WIDTH 4
#define ROOM_MIN_HEIGHT 3
#define ROOM_MAX_RANDOMNESS 6

#define DEFAULT_PATH "/.cs327/dungeon"

int main(int argc, char* argv[]) {
    int x, y, i;
    dungeon dungeon;

    srand(time(NULL));

    int draw_border = 0; 
    int read = 0;
    int write = 0;
    int path_next = 0;
    char* home = getenv("HOME");
    char* path = malloc(sizeof (char) * (strlen(home) + strlen(DEFAULT_PATH)) + 1);
    strcpy(path, home);
    strcat(path, DEFAULT_PATH);
    for (i = 1; i < argc; i++) {
        if (path_next) {
            free(path);
            path = argv[i];
            path_next = 0;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--load") == 0) read = 1;
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--save") == 0) write = 1;
        else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--border") == 0) draw_border = 1;
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) path_next = 1;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [-lwbph]\n  -l/--load: load a rungeon\n  -s/--save: write a random dungeon\n", argv[0]);
            printf("  -b/--border: draw a border around the dungeon\n  -p/--path: override path to load/save ");
            printf("(default ~/.cs327/dungeon)\n  -h/--help: display this message\n");
            free(path);
            return 0;
        }
        else {
            fprintf(stderr, "err: unrecognized argument: %s. run -h/--help for usage\n", argv[i]);
            free(path);
            return 1;
        }
    }
    if (path_next) {
        fprintf(stderr, "err: specify a path after -p/--path\n");
        free(path);
        return 1;
    }

    if ((!read && !write) || (read && write)) {
        fprintf(stderr, "err: must specify one of -l/--load, -w/--write\n");
        free(path);
        return 1;
    }

    if (dungeon_init(&dungeon, DUNGEON_WIDTH, DUNGEON_HEIGHT, ROOM_MIN_COUNT + ROOM_COUNT_MAX_RANDOMNESS)) {
        fprintf(stderr, "err: could not allocate memory for dungeon\n");
        free(path);
        return 1;
    }
    if (fill_dungeon(&dungeon, ROOM_MIN_COUNT, ROOM_COUNT_MAX_RANDOMNESS, ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT, ROOM_MAX_RANDOMNESS)) {
        dungeon_destroy(&dungeon);
        free(path);
        fprintf(stderr, "err: could not generate dungeon\n");
        return 1;
    }

    // print out the filled dungeon
    if (draw_border) {
        printf("*");
        for (x = 0; x < DUNGEON_WIDTH; x++) printf("-");
        printf("*\n");
    }
    for (y = 0; y < DUNGEON_HEIGHT; y++) {
        if (draw_border) printf("|");
        for (x = 0; x < DUNGEON_WIDTH; x++)
            printf("%c", dungeon.cells[x][y].type);
        if (draw_border) printf("|");
        printf("\n");
    }
    if (draw_border) {
        printf("*");
        for (x = 0; x < DUNGEON_WIDTH; x++) printf("-");
        printf("*\n");
    }

    dungeon_destroy(&dungeon);
    free(path);

    return 0;
}