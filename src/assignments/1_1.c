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

int main(int argc, char* argv[]) {
    int x, y;
    dungeon dungeon;

    srand(time(NULL));

    int draw_border = 0; 
    if (argc > 1) {
        if (strcmp(argv[1], "-b") == 0) draw_border = 1;
    }

    if (dungeon_init(&dungeon, DUNGEON_WIDTH, DUNGEON_HEIGHT, ROOM_MIN_COUNT + ROOM_COUNT_MAX_RANDOMNESS)) {
        fprintf(stderr, "err: could not allocate memory for dungeon\n");
        return 1;
    }
    if (fill_dungeon(&dungeon, ROOM_MIN_COUNT, ROOM_COUNT_MAX_RANDOMNESS, ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT, ROOM_MAX_RANDOMNESS)) {
        dungeon_destroy(&dungeon);
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

    return 0;
}