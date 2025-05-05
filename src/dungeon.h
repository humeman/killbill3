/**
 * Structures and functions useful for the generation of the dungeon data structure.
 *
 * Author: csenneff
 */

#ifndef DUNGEON_H
#define DUNGEON_H

#include <cstdint>
#include <cstdio>

#include "heap.h"


#define CELL_TYPES 8
typedef enum { // if modified, sync with CELL_TYPES_TO_FLOOR_TEXTURES, game_loop.cpp
    CELL_TYPE_STONE,
    CELL_TYPE_ROOM,
    CELL_TYPE_HALL,
    CELL_TYPE_UP_STAIRCASE,
    CELL_TYPE_DOWN_STAIRCASE,
    CELL_TYPE_EMPTY,
    CELL_TYPE_DEBUG,
    CELL_TYPE_HIDDEN
} cell_type_t;

// Separate to avoid having to rewrite a bunch of game logic to check for different varieties of stone
typedef enum {
    WALL_TYPE_T,
    WALL_TYPE_L,
    WALL_TYPE_R,
    WALL_TYPE_B,
    WALL_TYPE_TL,
    WALL_TYPE_TR,
    WALL_TYPE_BL,
    WALL_TYPE_BR,
    WALL_TYPE_ENDL,
    WALL_TYPE_ENDR,
    WALL_TYPE_ENDT,
    WALL_TYPE_ENDB,
    WALL_TYPE_T_L_L,
    WALL_TYPE_T_L_R,
    WALL_TYPE_T_B_T,
    WALL_TYPE_T_B_B,
    WALL_TYPE_SINGLE,
    WALL_TYPE_QUAD,
    WALL_TYPE_NONE
} wall_type_t;

class Room {
    public:
        uint8_t x0;
        uint8_t y0;
        uint8_t x1;
        uint8_t y1;


        std::string str() const {
            return "(" + std::to_string(x0) + ", " + std::to_string(y0) + ") to (" + std::to_string(x1) + ", " + std::to_string(y1) + ")";
        }

        friend std::ostream &operator<<(std::ostream &o, const Room &ip) {
            return o << ip.str();
        }

};

class Cell {
    public:
        cell_type_t type;
        uint8_t hardness;
        uint8_t attributes;
        wall_type_t wall_type;

};

typedef enum {
    CELL_ATTRIBUTE_IMMUTABLE = 0x01,
    CELL_ATTRIBUTE_UNLOCKED = 0x02,
    CELL_ATTRIBUTE_WALL = 0x04
} cell_attributes_t;

class QueueNode {
    public:
        int x;
        int y;
        QueueNode *next;

};

class IntPair {
    public:
        int x;
        int y;
        IntPair(int x, int y) {
            this->x = x;
            this->y = y;
        }
        IntPair() {
            this->x = 0;
            this->y = 0;
        }
        std::string str() const {
            return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
        }

        friend std::ostream &operator<<(std::ostream &o, const IntPair &ip) {
            return o << ip.str();
        }
        bool operator==(const IntPair &o) const;
};
class DungeonOptions {
    public:
        std::string name;
        IntPair nummon;
        IntPair numitems;
        IntPair rooms;
        IntPair size;
        std::string up_staircase = "";
        std::string down_staircase = "";
        std::vector<std::string> monsters;
        std::vector<std::string> items;
        std::string boss = "";
        std::string key = "";
        bool is_default = false;
};

#define IS_FLOOR(cell_type) (cell_type == CELL_TYPE_ROOM || cell_type == CELL_TYPE_HALL || cell_type == CELL_TYPE_UP_STAIRCASE || cell_type == CELL_TYPE_DOWN_STAIRCASE)

typedef enum {
    GAME_RESULT_RUNNING = 0,
    GAME_RESULT_WIN = 1,
    GAME_RESULT_LOSE = 2
} game_result_t;

class Dungeon {
    private:
        bool is_initalized;
        
    public:
        DungeonOptions *options = nullptr;
        uint8_t width;
        uint8_t height;
        std::vector<Room> rooms;
        Cell **cells;

        /**
         * Allocates memory for a dungeon. It still must be filled after creation
         *  to be usable.
         *
         * Params:
         * - width: Width of the dungeon
         * - height: Height of the dungeon
         * - max_rooms: Number of room objects to allocate
         */
        Dungeon(uint8_t width, uint8_t height, int max_rooms);
        Dungeon(DungeonOptions &options);
        ~Dungeon();

        /**
         * Writes a .pgm file for the dungeon hardness map for debugging purposes.
         * The file is saved as "dungeon.pgm".
         */
        void write_pgm();

        /**
         * Fills a dungeon (2D array) with a randomly generated one.
         * Must have been created with the DungeonOptions constructor.
         */
        void fill();

        /**
         * Picks a random, unobstructred location in a room within a dungeon.
         *
         * Parameters:
         * - room: Room to find an open space in
         * Returns: The picked coordinates
         */
        IntPair random_location_in_room(Room *room);

        /**
         * Picks a random, unobstructred location in any room within a dungeon.
         *
         * Returns: The picked coordinates
         */
        IntPair random_location();

        /**
         * Fills this dungeon with the data read from an RLG327 file.
         *
         * Params:
         * - f: File pointer to read
         * - debug: If true, debug messages will be printed
         * - pc_coords: A pointer to coordinates that contain the PC's location
         */
        void fill_from_file(FILE *f, int debug, IntPair *pc_coords);

        /**
         * Saves this dungeon to an RLG327 file.
         *
         * Params:
         * - f: File pointer to write
         * - debug: If true, debug messages will be printed
         * - pc_coords: A pointer to coordinates that will be updated with
         *      the PC's location
         */
        void save_to_file(FILE *f, int debug, IntPair *pc_coords);

        void apply_walls();

    private:
        /**
         * Fills the dungeon with randomly-generated stone. Overwrites everything
         * while doing so -- only run on a blank dungeon.
         */
        void fill_stone();

        /**
         * Fills only the outside cell of a dungeon with stone.
         */
        void fill_outside();

        /**
         * Places several random rooms within the dungeon.
         *
         * Parameters:
         * - min_width: Minimum room width.
         * - min_height: Minimum room height.
         * - size_randomness_max: The maximum number that can be randomly
         *      added to either dimension of the room size.
         */
        void create_rooms(int count, uint8_t min_width, uint8_t min_height, int size_randomness_max);

        /**
         * Places a single room of a particular width and height somewhere in the dungeon.
         * This will attempt every possible location, returning a non-zero status code if
         * placement fails.
         *
         * Parameters:
         * - room: Room struct to update with coordinates
         * - room_width: Width of the room
         * - room_height: Height of the room
         */
        Room create_room(uint8_t room_width, uint8_t room_height);

        /**
         * Connects every room in the dungeon.
         */
        void connect_rooms();

        /**
         * Connects two points with a semi-random walkway.
         *
         * Parameters:
         * - x0: X point 0
         * - y0: Y point 0
         * - x1: X point 1
         * - y1: Y point 1
         */
        void connect_points(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

        /**
         * Places an up and down staircase somewhere on the map.
         * They will be placed randomly within the center of a room.
         * There must be at least 2 rooms.
         *
         * Returns: 0 if successful
         */
        void place_staircases();

        /**
         * Places a material randomly within a room.
         * Avoids overwriting existing non-floor materials, and additionally
         * will not place such that it obstructs a hallway.
         * The provided room must be in the dungeon or an error may be returned.
         *
         * Parameters:
         * - room: Room to place in
         * - material: Material to place
         * Returns: coordinates to location of placed material
         */
        IntPair place_in_room(Room *room, cell_type_t material);
};


// Texture direction to coordinates that must be set to coordinates that must be unset
// In order of priority
class WallTileIdentifier {
    public:
        std::vector<IntPair> floor;
        std::vector<IntPair> wall;
        std::vector<IntPair> stone;
        wall_type_t type;

        WallTileIdentifier(wall_type_t type, std::vector<IntPair> floor, std::vector<IntPair> wall, std::vector<IntPair> stone) {
            this->type = type;
            this->floor = floor;
            this->wall = wall;
            this->stone = stone;
        }

        bool applies(Dungeon *dungeon, IntPair coords) const {
            int x, y;
            Cell *cell;
            for (auto &offset : floor) {
                x = coords.x + offset.x;
                y = coords.y + offset.y;
                if (x < 0 || x >= dungeon->width || y < 0 || y >= dungeon->height) return false;
                cell = &dungeon->cells[x][y];
                if (!IS_FLOOR(cell->type)) return false;
            }
            for (auto &offset : wall) {
                x = coords.x + offset.x;
                y = coords.y + offset.y;
                if (x < 0 || x >= dungeon->width || y < 0 || y >= dungeon->height) return false;
                cell = &dungeon->cells[x][y];
                if (!(cell->attributes & CELL_ATTRIBUTE_WALL)) return false;
            }
            for (auto &offset : stone) {
                x = coords.x + offset.x;
                y = coords.y + offset.y;
                if (x < 0 || x >= dungeon->width || y < 0 || y >= dungeon->height) continue;
                cell = &dungeon->cells[x][y];
                if (IS_FLOOR(cell->type)) return false;
            }
            return true;
        }
};

#endif