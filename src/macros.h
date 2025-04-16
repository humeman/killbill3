/**
 * Simple macros used throughout the project.
 *
 * Author: csenneff
 */

#ifndef MACROS_H
#define MACROS_H

#include <string>

#define ROOM_MIN_COUNT 6
#define ROOM_COUNT_MAX_RANDOMNESS 4
#define ROOM_MIN_WIDTH 4
#define ROOM_MIN_HEIGHT 3
#define ROOM_MAX_RANDOMNESS 6
#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21
#define RANDOM_MONSTERS_MIN 5
#define RANDOM_MONSTERS_MAX 10
#define RANDOM_ITEMS_MIN 10
#define RANDOM_ITEMS_MAX 15
#define PC_SPEED 10
#define MAX_CARRY_SLOTS 10

#define FILE_HEADER "RLG327-S2025"
#define FILE_VERSION 0

#define FILE_MATRIX_OFFSET 22
#define FILE_ROOM_COUNT_OFFSET 1702

#define WIDTH 80
#define HEIGHT 24

#define FOG_OF_WAR_DISTANCE 2
#define TELEPORT_POINTER '*'

// This is distinct from the regular ncurses COLOR_* macros since we can store
// multiple colors for one monster/item and I don't want an array of ints.
#define FLAG_COLOR_RED 0x01
#define FLAG_COLOR_GREEN 0x02
#define FLAG_COLOR_YELLOW 0x04
#define FLAG_COLOR_BLUE 0x08
#define FLAG_COLOR_MAGENTA 0x10
#define FLAG_COLOR_CYAN 0x20
#define FLAG_COLOR_WHITE 0x40
#define FLAG_COLOR_BLACK 0x80

#define DEFAULT_DUNGEON_PATH "/.rlg327/dungeon"
#define DEFAULT_MONSTER_PATH "/.rlg327/monster_desc.txt"
#define DEFAULT_ITEM_PATH "/.rlg327/object_desc.txt"

// The spec for generating items and monsters involves redrawing if the randomly chosen
// monster/item is invalid. This specifies a number of attempts beyond which it is considered
// impossible to get a valid definition. Could occur if, for example, we only have unique
// monsters and eventually run out.
#define MAX_GENERATION_ATTEMPTS 500

#define STRING(x) #x

class dungeon_exception : public std::exception {
    private:
        std::string message;
    public:
        dungeon_exception(const char *function, std::string message) {
            this->message = "\n  - " + std::string(function) + ": " + message;
        }
        dungeon_exception(const char *function, dungeon_exception &e) {
            this->message = std::string(e.what()) + "\n  - " + std::string(function);
        }
        dungeon_exception(const char *function, dungeon_exception &e, std::string message) {
            this->message = std::string(e.what()) + "\n  - " + std::string(function) + ": " + message;
        }
        dungeon_exception(const char *function, const char *message) {
            this->message = "\n  - " + std::string(function) + ": " + std::string(message);
        }
        dungeon_exception(const char *function, dungeon_exception &e, const char *message) {
            this->message = std::string(e.what()) + "\n  - " + function + ": " + std::string(message);
        }

        const char *what() const noexcept override {
            return message.c_str();
        }
};


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
#define CLAMP(a, low, high) ((a < low) ? low : ((a > high) ? high : a))

// These would be functions, but in the interest of saving on lines of code by avoiding any extra error checks,
// these are simple macros that expand out to the 3/4 lines necessary to read, convert, and validate data.

// Reads a uint32 from f into the variable specified in 'var'.
#define READ_UINT32(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be32toh(var); \
    if (debug) printf("debug: %s = %u\n", description, var); }

// Reads a uint16 from f into the variable specified in 'var'.
#define READ_UINT16(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be16toh(var); \
    if (debug) printf("debug: %s = %u\n", description, var); }

// Reads a uint8 from f into the variable specified in 'var'.
#define READ_UINT8(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not a valid RLG327 file (file ended too early)"); \
    if (debug) printf("debug: %s = %u\n", description, var); }

// Writes the uint32 variable 'var' into f.
#define WRITE_UINT32(var, description, f, debug) { \
    uint32_t be = htobe32(var); \
    size_t size = fwrite(&be, sizeof (uint32_t), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "could not write to file"); \
    if (debug) printf("debug: %s = %u, wrote %ld bytes\n", description, var, sizeof (var)); }

// Writes the uint16 variable 'var' into f.
#define WRITE_UINT16(var, description, f, debug) { \
    uint16_t be = htobe16(var); \
    size_t size = fwrite(&be, sizeof (uint16_t), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "could not write to file"); \
    if (debug) printf("debug: %s = %u, wrote %ld bytes\n", description, var, sizeof (var)); }

// Writes the uint8 variable 'var' into f.
#define WRITE_UINT8(var, description, f, debug) { \
    size_t size = fwrite(&var, sizeof (uint8_t), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "could not write to file"); \
    if (debug) printf("debug: %s = %u, wrote %ld bytes\n", description, var, sizeof (var)); }

#define APPEND_MONST_ATTRS(attrs, str) { \
    if (attrs & MONSTER_ATTRIBUTE_BOSS) str += "*BOSS* "; \
    if (attrs & MONSTER_ATTRIBUTE_INTELLIGENT) str += "smart "; \
    if (attrs & MONSTER_ATTRIBUTE_TELEPATHIC) str += "telepathic "; \
    if (attrs & MONSTER_ATTRIBUTE_TUNNELING) str += "tunneling "; \
    if (attrs & MONSTER_ATTRIBUTE_ERRATIC) str += "erratic "; \
    if (attrs & MONSTER_ATTRIBUTE_GHOST) str += "ghost "; \
    if (attrs & MONSTER_ATTRIBUTE_PICKUP) str += "pickup "; \
    if (attrs & MONSTER_ATTRIBUTE_DESTROY) str += "destroy "; \
    if (attrs & MONSTER_ATTRIBUTE_UNIQUE) str += "unique "; }

#endif
