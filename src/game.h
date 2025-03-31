#ifndef GAME_H
#define GAME_H

#include <ncurses.h>

#include "dungeon.h"
#include "character.h"

typedef enum colors {
    COLORS_FLOOR = 1,
    COLORS_STONE,
    COLORS_PC,
    COLORS_MONSTER,
    COLORS_OBJECT,
    COLORS_TEXT,
    COLORS_MENU_TEXT,
    COLORS_FOG_OF_WAR_TERRAIN
} colors_t;

typedef enum keybinds {
    KB_UP_LEFT_0 = '7',
    KB_UP_LEFT_1 = 'y',
    KB_UP_0 = '8',
    KB_UP_1 = 'k',
    KB_UP_RIGHT_0 = '9',
    KB_UP_RIGHT_1 = 'u',
    KB_RIGHT_0 = '6',
    KB_RIGHT_1 = 'l',
    KB_DOWN_RIGHT_0 = '3',
    KB_DOWN_RIGHT_1 = 'n',
    KB_DOWN_0 = '2',
    KB_DOWN_1 = 'j',
    KB_DOWN_LEFT_0 = '1',
    KB_DOWN_LEFT_1 = 'b',
    KB_LEFT_0 = '4',
    KB_LEFT_1 = 'h',
    KB_DOWN_STAIRS = '>',
    KB_UP_STAIRS = '<',
    KB_REST_0 = '5',
    KB_REST_1 = ' ',
    KB_REST_2 = '.',
    KB_MONSTERS = 'm',
    KB_SCROLL_UP = KEY_UP,
    KB_SCROLL_DOWN = KEY_DOWN,
    KB_ESCAPE = 27, // can't find a constant for this???
    KB_QUIT = 'Q',
    KB_TOGGLE_FOG = 'f',
    KB_TELEPORT = 'g',
    KB_TELEPORT_RANDOM = 'r'
} keybinds_t;

// To split up the dungeon from the game controls and such, this class
// stores all of the critical info for the game.
class game_t {
    private:
        pc_t pc;
        binary_heap_t *turn_queue;
        uint32_t **pathfinding_no_tunnel;
        uint32_t **pathfinding_tunnel;
        cell_type_t **seen_map;
        character_t ***character_map;
        bool is_initialized = false;
        int nummon = -1;
        int debug;
        char *message;
    
    public:
        dungeon_t *dungeon;

        game_t(int debug, uint8_t width, uint8_t height, int max_rooms);
        ~game_t();

        void init_from_file(char *path);
        void init_random();
        void random_monsters();

        void write_to_file(char *path);

        void override_nummon(int nummon);

        void run();

    private:
        void update_fog_of_war();
        void try_move(int x_offset, int y_offset);
        void move_coords(coordinates_t &coords, int x_offset, int y_offset);
        void force_move(coordinates_t dest);
        void fill_and_place_on(cell_type_t target_cell);
};

#endif