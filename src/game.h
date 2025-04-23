#ifndef GAME_H
#define GAME_H

#include <ncurses.h>

#include "dungeon.h"
#include "character.h"
#include "parser.h"
#include "item.h"

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
    KB_SCROLL_LEFT = KEY_LEFT,
    KB_SCROLL_RIGHT = KEY_RIGHT,
    KB_ESCAPE = 27, // can't find a constant for this???
    KB_QUIT = 'Q',
    KB_TOGGLE_FOG = 'f',
    KB_TELEPORT = 'g',
    KB_TELEPORT_RANDOM = 'r',
    KB_PICKUP = ',',
    KB_EQUIP = 'w',
    KB_UNEQUIP = 't',
    KB_DROP = 'd',
    KB_EXPUNGE = 'x',
    KB_INVENTORY = 'i',
    KB_EQUIPMENT = 'e',
    KB_INSPECT_ITEM = 'I',
    KB_LOOK_MODE = 'L',
    KB_LOOK_SELECT = 't',
    KB_NEXT_MESSAGE = 10 // KEY_ENTER doesn't work here (weird)
} keybinds_t;

extern char CHARACTERS_BY_CELL_TYPE[CELL_TYPES];
extern int COLORS_BY_CELL_TYPE[CELL_TYPES];
extern std::string ITEM_TYPE_STRINGS[ITEM_TYPE_UNKNOWN + 1];

// To split up the dungeon from the game controls and such, this class
// stores all of the critical info for the game.
class game_t {
    private:
        pc_t pc;
        binary_heap_t<character_t *> turn_queue;
        uint32_t **pathfinding_no_tunnel;
        uint32_t **pathfinding_tunnel;
        char **seen_map;
        character_t ***character_map;
        item_t ***item_map;
        bool is_initialized = false;
        int nummon = -1;
        int numitems = -1;
        int debug;
        parser_t<monster_definition_t> *monst_parser;
        parser_t<item_definition_t> *item_parser;
        std::vector<monster_definition_t *> monster_defs;
        std::vector<item_definition_t *> item_defs;


    public:
        dungeon_t *dungeon;

        game_t(int debug, uint8_t width, uint8_t height, int max_rooms);
        ~game_t();

        /**
         * Reads a monster definition file into this game.
         *
         * Params:
         * - path: Path to the file
         */
        void init_monster_defs(const char *path);

        /**
         * Reads an item definition file into this game.
         *
         * Params:
         * - path: Path to the file
         */
        void init_item_defs(const char *path);

        /**
         * Initializes the game's dungeon from an RLG327 file.
         *
         * Params:
         * - path: The path to the file to read.
         */
        void init_from_file(const char *path);

        /**
         * Initializes the game with a random dungeon.
         */
        void init_random();

        /**
         * Adds randomized monsters, the count is the value of nummon (or random
         *  if that hasn't been set).
         */
        void random_monsters();

        /**
         * Adds randomized items.
         */
        void random_items();

        /**
         * Writes the game's dungeon to an RLG327 file.
         *
         * Params:
         * - path: The path to the file to write.
         */
        void write_to_file(const char *path);

        /**
         * Overrides the number of monsters used when calling random_monsters()
         *  or when the PC goes up/down staircases. By default, this is random.
         *
         * Params:
         * - nummon: New number of monsters per dungeon
         */
        void override_nummon(int nummon);

        /**
         * Overrides the number of items used when calling random_items()
         *  or when the PC goes up/down staircases. By default, this is random.
         *
         * Params:
         * - numitems: New number of items per dungeon
         */
        void override_numitems(int numitems);

        /**
         * Runs the game.
         */
        void run();

    private:
        /**
         * The internal game loop. This is wrapped by run() to handle ncurses
         *  initialization and cleanup.
         */
        void run_internal();

        /**
          * Takes the turns of everything in the turn queue until the PC's turn (or
          *  until it dies).
          *
          * Returns: Game result denoting the current state
          */
        game_result_t run_until_pc();

        /**
          * Displays the monster menu.
          */
        void monster_menu(monster_t *initial_target);
        void inventory_menu();

        /**
         * Updates the PC's fog of war map with what it currently sees.
         */
        void update_fog_of_war();

        /**
         * Tries to move the PC to a new location.
         *
         * Params:
         * - x_offset: Number to add to the X coordinate
         * - y_offset: Number to add to the Y coordinate
         */
        void try_move(game_result_t &result, int x_offset, int y_offset);

        /**
         * Changes the value of some coordinates, clamping each value to the
         *  size of the dungeon.
         *
         * Params:
         * - coords: Coordinates to update
         * - x_offset: Number to add to the X coordinate
         * - y_offset: Number to add to the Y coordinate
         */
        void move_coords(coordinates_t &coords, int x_offset, int y_offset);

        /**
         * Forces the PC to move to some new coordinates, regardless of if
         *  they represent a possible PC location.
         */
        void force_move(game_result_t &result, coordinates_t dest);

        /**
         * Regenerates the dungeon, placing the PC on the first instance of
         *  a specified target cell.
         *
         * Params:
         * - target_cell: Cell type to place on
         */
        void fill_and_place_on(cell_type_t target_cell);

        void render_inventory_box(std::string title, std::string labels, std::string input_tip, int x0, int y0);
        void render_inventory_item(item_t *item, int i, bool selected, int x0, int y0);
        void render_inventory_details(item_t *item, int y0);
};

#endif
