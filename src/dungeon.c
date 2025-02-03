#include "dungeon.h"

int fill_dungeon(int width, int height, char dungeon[width][height]) {
    fill_outside(width, height, dungeon);
    return 0;
}

void fill_outside(int width, int height, char dungeon[width][height]) {
    int i;
    for (i = 0; i < width; i++) {
        dungeon[i][0] = STONE;
        dungeon[i][height - 1] = STONE;
    }
    for (i = 1; i < height - 1; i++) {
        dungeon[0][i] = STONE;
        dungeon[width - 1][i] = STONE;
    }
}