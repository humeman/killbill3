#ifndef GAME_H
#define GAME_H

#include "dungeon.h"
#include "character.h"

// To split up the dungeon from the game controls and such, this class
// stores all of the critical info for the game.
class game_t {
    private:
        character_t pc;
        binary_heap_t *turn_queue;
        uint32_t **pathfinding_no_tunnel;
        uint32_t **pathfinding_tunnel;
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
        void fill_and_place_on(cell_type_t target_cell);
};

#endif