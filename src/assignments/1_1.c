#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../dungeon.h"

#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21

int main(int argc, char* argv[]) {
    srand(time(NULL));
    int x, y;
    char dungeon[DUNGEON_WIDTH][DUNGEON_HEIGHT];
    // for debugging purposes, add an invalid character everywhere to note that
    // the cell isn't filled yet
    for (x = 0; x < DUNGEON_WIDTH; x++)
        for (y = 0; y < DUNGEON_HEIGHT; y++)
            dungeon[x][y] = '!';

    fill_dungeon(DUNGEON_WIDTH, DUNGEON_HEIGHT, dungeon);

    // print out the filled dungeon
    for (y = 0; y < DUNGEON_HEIGHT; y++) {
        for (x = 0; x < DUNGEON_WIDTH; x++)
            printf("%c", dungeon[x][y]);
        printf("\n");
    }
}