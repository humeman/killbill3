#ifndef GAME_H
#define GAME_H

#include "dungeon.h"
#include "character.h"
#include "parser.h"
#include "item.h"
#include <ncpp/NotCurses.hh>
#include <notcurses/nckeys.h>
#include "plane_manager.h"

extern char CHARACTERS_BY_CELL_TYPE[CELL_TYPES];
extern int COLORS_BY_CELL_TYPE[CELL_TYPES];
extern std::string ITEM_TYPE_STRINGS[ITEM_TYPE_UNKNOWN + 1];

class DungeonFloor {
  public:
    Dungeon *dungeon;
    Character ***character_map;
    Item ***item_map;
    std::string id;
    // It is completely unnecessary to have one for each dungeon, but they have arbitrary sizes now,
    // so to take the easy way out that's what I'm doing.
    uint32_t **pathfinding_tunnel;
    uint32_t **pathfinding_no_tunnel;

    DungeonFloor(std::string id, Dungeon *dungeon) {
      int i, j;
      this->dungeon = dungeon;
      character_map = (Character ***) malloc(dungeon->width * sizeof (Character**));
      if (character_map == NULL) {
          goto init_free;
      }
      for (i = 0; i < dungeon->width; i++) {
          character_map[i] = (Character **) malloc(dungeon->height * sizeof (Character*));
          if (character_map[i] == NULL) {
              for (j = 0; j < i; j++) free(character_map[j]);
              goto init_free_character_map;
          }
          for (j = 0; j < dungeon->height; j++) character_map[i][j] = NULL;
      }

      item_map = (Item ***) malloc(dungeon->width * sizeof (Item**));
      if (item_map == NULL) {
          goto init_free_all_character_map;
      }
      for (i = 0; i < dungeon->width; i++) {
          item_map[i] = (Item **) malloc(dungeon->height * sizeof (Item*));
          if (item_map[i] == NULL) {
              for (j = 0; j < i; j++) free(item_map[j]);
              goto init_free_item_map;
          }
          for (j = 0; j < dungeon->height; j++) item_map[i][j] = NULL;
      }

      pathfinding_no_tunnel = (uint32_t **) malloc(dungeon->width * sizeof (uint32_t*));
      if (pathfinding_no_tunnel == NULL) {
          goto init_free_all_item_map;
      }
      for (i = 0; i < dungeon->width; i++) {
          pathfinding_no_tunnel[i] = (uint32_t *) malloc(dungeon->height * sizeof (uint32_t));
          if (pathfinding_no_tunnel[i] == NULL) {
              for (j = 0; j < i; j++) free(pathfinding_no_tunnel[j]);
              goto init_free_pathfinding_no_tunnel;
          }
      }
  
      pathfinding_tunnel = (uint32_t **) malloc(dungeon->width * sizeof (uint32_t*));
      if (pathfinding_tunnel == NULL) {
          goto init_free_all_pathfinding_no_tunnel;
      }
      for (i = 0; i < dungeon->width; i++) {
          pathfinding_tunnel[i] = (uint32_t *) malloc(dungeon->height * sizeof (uint32_t));
          if (pathfinding_tunnel[i] == NULL) {
              for (j = 0; j < i; j++) free(pathfinding_tunnel[j]);
              goto init_free_pathfinding_tunnel;
          }
      }

      return;
      init_free_pathfinding_tunnel:
      free(pathfinding_tunnel);
      init_free_all_pathfinding_no_tunnel:
      for (j = 0; j < dungeon->width; j++) free(pathfinding_no_tunnel[j]);
      init_free_pathfinding_no_tunnel:
      free(pathfinding_no_tunnel);
      init_free_all_item_map:
      for (j = 0; j < dungeon->width; j++) free(item_map[j]);
      init_free_item_map:
      free(item_map);
      init_free_all_character_map:
      for (j = 0; j < dungeon->width; j++) free(character_map[j]);
      init_free_character_map:
      free(character_map);
      init_free:
      throw dungeon_exception(__PRETTY_FUNCTION__, "memory allocation failed");
    }
};

// To split up the dungeon from the game controls and such, this class
// stores all of the critical info for the game.
class Game {
    private:
        PC pc;
        BinaryHeap<Character *> turn_queue;
        uint32_t **pathfinding_no_tunnel;
        uint32_t **pathfinding_tunnel;
        Character ***character_map;
        Item ***item_map;
        bool is_initialized = false;
        int debug;
        Parser<MonsterDefinition> *monst_parser;
        Parser<ItemDefinition> *item_parser;
        Parser<DungeonOptions> *map_parser;
        std::map<std::string, MonsterDefinition *> monster_defs;
        std::map<std::string, ItemDefinition *> item_defs;
        std::map<std::string, std::map<std::string, DungeonOptions *>> map_defs;
        std::vector<DungeonFloor *> dungeons;

        ncpp::NotCurses *nc = nullptr;
        PlaneManager planes;
        
        unsigned int term_x, term_y, cells_x, cells_y;

        std::map<uint32_t, void (Game::*)()> controls;
        bool next_turn_ready = false;
        bool teleport_mode = false;
        bool look_mode = false;
        bool game_exit = false;
        bool needs_redraw = false;
        bool seethrough = false;
        IntPair pointer;
        game_result_t result = GAME_RESULT_RUNNING;


    public:
        Dungeon *dungeon = nullptr;

        Game(int debug);
        ~Game();

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

        void init_maps(const char *path);


        /**
         * Initializes the game's dungeon from an RLG327 file.
         *
         * Params:
         * - path: The path to the file to read.
         */
        //void init_from_file(const char *path);

        void init_from_map(std::string map_name);

        /**
         * Adds randomized monsters, the count is the value of nummon (or random
         *  if that hasn't been set).
         */
        void random_monsters();

        /**
         * Adds randomized items.
         */
        void random_items();

        void apply_dungeon(DungeonFloor &floor, IntPair pc_coords);

        /**
         * Writes the game's dungeon to an RLG327 file.
         *
         * Params:
         * - path: The path to the file to write.
         */
        //void write_to_file(const char *path);

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
        //void monster_menu(Monster *initial_target);
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
        void move_coords(IntPair &coords, int x_offset, int y_offset);

        /**
         * Forces the PC to move to some new coordinates, regardless of if
         *  they represent a possible PC location.
         */
        void force_move(IntPair dest);

        void render_inventory_box(std::string title, std::string labels, std::string input_tip, unsigned int x0, unsigned int y0);
        void render_inventory_item(Item *item, int i, bool selected, unsigned int x0, unsigned int y0);
        void render_inventory_details(ncpp::Plane *plane, Item *item, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height);
        void render_monster_details(ncpp::Plane *plane, Monster *monst, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height);

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
