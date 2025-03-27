/**
 * Simple macros used throughout the project.
 * 
 * Author: csenneff
 */

#ifndef MACROS_H
#define MACROS_H

#define ROOM_MIN_COUNT 6
#define ROOM_COUNT_MAX_RANDOMNESS 4
#define ROOM_MIN_WIDTH 4
#define ROOM_MIN_HEIGHT 3
#define ROOM_MAX_RANDOMNESS 6
#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21
#define RANDOM_MONSTERS_MIN 5
#define RANDOM_MONSTERS_MAX 10
#define PC_SPEED 10

#define RETURN_ERROR(message, ...) { \
    fprintf(stderr, "err: " message "\n", ##__VA_ARGS__); \
    return 1; \
}

#define PRINT_PADDED(message, width) { \
    int padding = (width - strlen(message)) / 2 - 1; \
    for (int i = 0; i < padding; i++) printf("="); \
    printf(" %s ", message); \
    for (int i = 0; i < padding; i++) printf("="); \
    if (strlen(message) % 2 == 1) printf("="); \
    printf("\n"); \
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif