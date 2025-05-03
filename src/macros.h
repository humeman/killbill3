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

#define REDRAW_TIMEOUT 500
#define NO_ACTION_TIMEOUT 2500

#define FOG_OF_WAR_DISTANCE 2
#define TELEPORT_POINTER '*'

// This is distinct from the regular ncurses COLOR_* macros since we can store
// multiple colors for one monster/item and I don't want an array of ints.
#define FLAG_COLOR_RED 0x01 // 0
#define FLAG_COLOR_GREEN 0x02 // 1
#define FLAG_COLOR_YELLOW 0x04 // 2
#define FLAG_COLOR_BLUE 0x08 // 3
#define FLAG_COLOR_MAGENTA 0x10 // 4
#define FLAG_COLOR_CYAN 0x20 // 5
#define FLAG_COLOR_WHITE 0x40 // 6
#define FLAG_COLOR_BLACK 0x80 // 7

#define DEFAULT_DUNGEON_PATH "assets/dungeon"
#define DEFAULT_MONSTER_PATH "assets/enemies.txt"
#define DEFAULT_ITEM_PATH "assets/items.txt"

#define HEARTS 15
#define INVENTORY_BOX_WIDTH 29
#define DETAILS_WIDTH 60
#define DETAILS_HEIGHT 12
#define CHEATER_MENU_WIDTH 40
#define CHEATER_MENU_HEIGHT 10

// The spec for generating items and monsters involves redrawing if the randomly chosen
// monster/item is invalid. This specifies a number of attempts beyond which it is considered
// impossible to get a valid definition. Could occur if, for example, we only have unique
// monsters and eventually run out.
#define MAX_GENERATION_ATTEMPTS 500

#define STRING(x) #x

typedef enum {
    RGB_COLOR_RED = 0xFF0000,
    RGB_COLOR_GREEN = 0x00FF00,
    RGB_COLOR_YELLOW = 0xFFFF00,
    RGB_COLOR_BLUE = 0x0000FF,
    RGB_COLOR_MAGENTA = 0xFF00FF,
    RGB_COLOR_CYAN = 0x00FFFF,
    RGB_COLOR_WHITE = 0xFFFFFF,
    RGB_COLOR_BLACK = 0x00,
    RGB_COLOR_GSOD = 0x62C554,
    RGB_COLOR_BSOD = 0x4764DE,
    RGB_COLOR_RED_DIM = 0x7F0000,
    RGB_COLOR_GREEN_DIM = 0x007F00,
    RGB_COLOR_YELLOW_DIM = 0x7F7F00,
    RGB_COLOR_BLUE_DIM = 0x00007F,
    RGB_COLOR_MAGENTA_DIM = 0x7F007F,
    RGB_COLOR_CYAN_DIM = 0x007F7F,
    RGB_COLOR_WHITE_DIM = 0x7F7F7F,
    RGB_COLOR_BLACK_DIM = 0x707070,
    RGB_COLOR_BSOD_DIM = 0x001A5D
} color_codes_t;

#define NC_APPLY_COLOR(plane, color_code, bg) { \
    (plane).set_fg_rgb(color_code); \
    if (bg >= 0) (plane).set_bg_rgb(bg); }

#define NC_APPLY_COLOR_BY_NUM(plane, num, bg) { \
    switch (num) { \
        case 0: NC_APPLY_COLOR(plane, RGB_COLOR_RED, bg); break; \
        case 1: NC_APPLY_COLOR(plane, RGB_COLOR_GREEN, bg); break; \
        case 2: NC_APPLY_COLOR(plane, RGB_COLOR_YELLOW, bg); break; \
        case 3: NC_APPLY_COLOR(plane, RGB_COLOR_BLUE, bg); break; \
        case 4: NC_APPLY_COLOR(plane, RGB_COLOR_MAGENTA, bg); break; \
        case 5: NC_APPLY_COLOR(plane, RGB_COLOR_CYAN, bg); break; \
        case 6: NC_APPLY_COLOR(plane, (bg == RGB_COLOR_WHITE ? RGB_COLOR_BLACK : RGB_COLOR_WHITE), bg); break; \
        case 7: NC_APPLY_COLOR(plane, (bg == RGB_COLOR_BLACK ? RGB_COLOR_WHITE : RGB_COLOR_BLACK), bg); break; } }

#define NC_APPLY_COLOR_BY_NUM_DIM(plane, num, bg) { \
    switch (num) { \
        case 0: NC_APPLY_COLOR(plane, RGB_COLOR_RED_DIM, bg); break; \
        case 1: NC_APPLY_COLOR(plane, RGB_COLOR_GREEN_DIM, bg); break; \
        case 2: NC_APPLY_COLOR(plane, RGB_COLOR_YELLOW_DIM, bg); break; \
        case 3: NC_APPLY_COLOR(plane, RGB_COLOR_BLUE_DIM, bg); break; \
        case 4: NC_APPLY_COLOR(plane, RGB_COLOR_MAGENTA_DIM, bg); break; \
        case 5: NC_APPLY_COLOR(plane, RGB_COLOR_CYAN_DIM, bg); break; \
        case 6: NC_APPLY_COLOR(plane, (bg == RGB_COLOR_WHITE_DIM ? RGB_COLOR_BLACK_DIM : RGB_COLOR_WHITE_DIM), bg); break; \
        case 7: NC_APPLY_COLOR(plane, (bg == RGB_COLOR_BLACK_DIM ? RGB_COLOR_WHITE_DIM : RGB_COLOR_BLACK_DIM), bg); break; } }

#define NC_RESET(plane) { \
    NC_APPLY_COLOR(plane, RGB_COLOR_WHITE, RGB_COLOR_BLACK); \
    (plane).styles_off(ncpp::CellStyle::Bold); }

#define NC_CLEAR(plane) { \
    for (unsigned int y = 0; y < (plane).get_dim_y(); y++) { \
        (plane).cursor_move(y, 0); \
        for (unsigned int x = 0; x < (plane).get_dim_x(); x++) \
            (plane).putc(' '); \
        } \
    }

// This extra clear/render is because of a bug in either notcurses or my terminal.
// Just moving the plane down is not enough -- in certain window sizes,
// there are small gaps between lines that remain as the menu color.
// Rendering first, then re-rendering once it's moved down, fixes this.
#define NC_HIDE(nc, plane) { \
    NC_RESET(plane); \
    NC_CLEAR(plane); \
    nc->render(); \
    (plane).move_bottom(); }

#define NC_PRINT_CENTERED_AT(plane, x, y, msg, ...) { \
    int len = snprintf(NULL, 0, msg, ##__VA_ARGS__); \
    plane->printf(y, (x) - len / 2, msg, ##__VA_ARGS__); }

#define CELL_PLANE_AT(this, x, y) this->cell_planes[x * this->cells_y + y]
#define CELL_CACHE_FLOOR_AT(this, x, y) this->cell_cache[x * this->cells_y + y]
#define CELL_CACHE_HEARTS_AT(this, i) this->cell_cache[this->cells_x * this->cells_y + i]
#define CELL_CACHE_ITEMS_AT(this, i) this->cell_cache[this->cells_x * this->cells_y + HEARTS + i]
#define NC_DRAW(plane, texture_name) { \
    ncvisual_options vopts{}; \
    vopts.flags |= NCVISUAL_OPTION_NOINTERPOLATE | NCVISUAL_OPTION_ADDALPHA; \
    vopts.scaling = NCSCALE_STRETCH; \
    vopts.blitter = NCBLIT_PIXEL; \
    vopts.n = (plane)->to_ncplane(); \
    ResourceManager::get()->get_visual(texture_name)->blit(&vopts); }

#define CELL_NAME(x, y) "cell_" + std::to_string(x) + "_" + std::to_string(y)
#define HEART_NAME(x) "heart_" + std::to_string(x)
#define ITEM_NAME(x) "item_" + std::to_string(x)
#define INVENTORY_NAME(x, y) "inv_" + std::to_string(x) + "_" + std::to_string(y)

typedef enum colors {
    COLORS_FLOOR = 1,
    COLORS_STONE,
    COLORS_PC,
    COLORS_MONSTER,
    COLORS_OBJECT,
    COLORS_TEXT,
    COLORS_TEXT_RED,
    COLORS_MENU_TEXT,
    COLORS_MENU_TEXT_SELECTED,
    COLORS_FOG_OF_WAR_TERRAIN,
    COLORS_FLOOR_ANY,
    COLORS_MENU_ANY = COLORS_FLOOR_ANY + 8
} colors_t;

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

#define RAND_BETWEEN(low, high) (low) + (rand() % ((high) - (low))) 

#define ARRAY_SIZE(x) ((int) (sizeof(x) / sizeof(x[0])))

// These would be functions, but in the interest of saving on lines of code by avoiding any extra error checks,
// these are simple macros that expand out to the 3/4 lines necessary to read, convert, and validate data.

// Reads a uint32 from f into the variable specified in 'var'.
#define READ_UINT32(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be32toh(var); \
    Logger::debug(__FILE__, std::string(description) + " = " + std::to_string(var)); }

// Reads a uint16 from f into the variable specified in 'var'.
#define READ_UINT16(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not a valid RLG327 file (file ended too early)"); \
    var = be16toh(var); \
    Logger::debug(__FILE__, std::string(description) + " = " + std::to_string(var)); }

// Reads a uint8 from f into the variable specified in 'var'.
#define READ_UINT8(var, description, f, debug) { \
    size_t size = fread(&var, sizeof (var), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "the specified file is not a valid RLG327 file (file ended too early)"); \
    Logger::debug(__FILE__, std::string(description) + " = " + std::to_string(var)); }

// Writes the uint32 variable 'var' into f.
#define WRITE_UINT32(var, description, f, debug) { \
    uint32_t be = htobe32(var); \
    size_t size = fwrite(&be, sizeof (uint32_t), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "could not write to file"); \
    Logger::debug(__FILE__, std::string(description) + " = " + std::to_string(var) + ", wrote " + std::to_string(sizeof (var)) + " bytes"); }

// Writes the uint16 variable 'var' into f.
#define WRITE_UINT16(var, description, f, debug) { \
    uint16_t be = htobe16(var); \
    size_t size = fwrite(&be, sizeof (uint16_t), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "could not write to file"); \
    Logger::debug(__FILE__, std::string(description) + " = " + std::to_string(var) + ", wrote " + std::to_string(sizeof (var)) + " bytes"); }

// Writes the uint8 variable 'var' into f.
#define WRITE_UINT8(var, description, f, debug) { \
    size_t size = fwrite(&var, sizeof (uint8_t), 1, f); \
    if (size != 1) throw dungeon_exception(__PRETTY_FUNCTION__, "could not write to file"); \
    Logger::debug(__FILE__, std::string(description) + " = " + std::to_string(var) + ", wrote " + std::to_string(sizeof (var)) + " bytes"); }

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

#define PCEXTURE "characters_pc"

#endif
