#ifndef GAME_H
#define GAME_H

#include "dungeon.h"
#include "character.h"
#include "parser.h"
#include "item.h"
#include <ncpp/NotCurses.hh>
#include <notcurses/nckeys.h>
#include "plane_manager.h"

// typedef enum keybinds {
//     KB_N = 'w',
//     KB_E = 'd',
//     KB_S = 's',
//     KB_W = 'a',
//     KB_INTERACT = ' ',
//     KB_REST = ' ',
//     KB_MONSTERS = 'm',
//     KB_SCROLL_UP = NCKEY_UP,
//     KB_SCROLL_DOWN = NCKEY_DOWN,
//     KB_SCROLL_LEFT = NCKEY_LEFT,
//     KB_SCROLL_RIGHT = NCKEY_RIGHT,
//     KB_ESCAPE = NCKEY_ESC,
//     KB_QUIT = 'Q',
//     KB_TOGGLE_FOG = 'f',
//     KB_TELEPORT = 'g',
//     KB_TELEPORT_RANDOM = 'r',
//     KB_EQUIP = 'e',
//     KB_UNEQUIP = 'u',
//     KB_DROP = 'q',
//     KB_EXPUNGE = 'x',
//     KB_INVENTORY = 'i',
//     KB_LOOK_MODE = 'L',
//     KB_LOOK_SELECT = 't',
//     KB_NEXT_MESSAGE = NCKEY_ENTER
// } keybinds_t;

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
        character_t ***character_map;
        item_t ***item_map;
        bool is_initialized = false;
        int nummon = -1;
        int numitems = -1;
        int debug;
        parser_t<monster_definition_t> *monst_parser;
        parser_t<item_definition_t> *item_parser;
        std::map<std::string, monster_definition_t *> monster_defs;
        std::map<std::string, item_definition_t *> item_defs;

        ncpp::NotCurses *nc = nullptr;
        plane_manager_t planes;
        
        unsigned int term_x, term_y, cells_x, cells_y;

        std::map<uint32_t, void (game_t::*)()> controls;
        bool next_turn_ready = false;
        bool teleport_mode = false;
        bool look_mode = false;
        bool game_exit = false;
        bool needs_redraw = false;
        bool seethrough = false;
        coordinates_t pointer;
        game_result_t result = GAME_RESULT_RUNNING;

    public:
        dungeon_t *dungeon;

        game_t(int debug, uint8_t width, uint8_t height, int max_rooms);
        ~game_t();

        void create_nc();
        void end_nc();

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
          */
        void run_until_pc();

        /**
          * Displays the monster menu.
          */
        void monster_menu(monster_t *initial_target);
        void inventory_menu();
        void cheater_menu();

        /**
         * Tries to move the PC to a new location.
         *
         * Params:
         * - x_offset: Number to add to the X coordinate
         * - y_offset: Number to add to the Y coordinate
         */
        void try_move(int x_offset, int y_offset);

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
        void force_move(coordinates_t dest);

        /**
         * Regenerates the dungeon, placing the PC on the first instance of
         *  a specified target cell.
         *
         * Params:
         * - target_cell: Cell type to place on
         */
        void fill_and_place_on(cell_type_t target_cell);

        void render_inventory_box(std::string title, std::string labels, std::string input_tip, unsigned int x0, unsigned int y0);
        void render_inventory_item(item_t *item, int i, bool selected, unsigned int x0, unsigned int y0);
        void render_inventory_details(ncpp::Plane *plane, item_t *item, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height);
        void render_monster_details(ncpp::Plane *plane, monster_t *monst, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height);

        void render_frame(bool complete_redraw);
        void init_controls();

        // Controls
        void ctrl_move_n();
        void ctrl_move_e();
        void ctrl_move_w();
        void ctrl_move_s();
        void ctrl_inventory();
        void ctrl_quit();
        void ctrl_grab_item();
        void ctrl_cheater();
        void ctrl_ptr_confirm();
        void ctrl_esc();

};

#endif
