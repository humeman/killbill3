/**
 * Simple macros used throughout the project.
 * 
 * Author: csenneff
 */

#ifndef MACROS_H
#define MACROS_H

#define RETURN_ERROR(message) { \
    fprintf(stderr, "err: %s\n", message); \
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