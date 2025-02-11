#ifndef MACROS_H
#define MACROS_H

#define RETURN_ERROR(message) { \
    fprintf(stderr, "err: %s\n", message); \
    return 1; \
}

#endif