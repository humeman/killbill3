#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "../dungeon.h"
#include "../pathfinding.h"
#include "../macros.h"
#include "../character.h"
#include "../game.h"
#include "../macros.h"

#define DEFAULT_DUNGEON_PATH "/.rlg327/dungeon"
#define DEFAULT_MONSTER_PATH "/.rlg327/monster_desc.txt"
#define DEFAULT_ITEM_PATH "/.rlg327/object_desc.txt"

int prepare_args(int argc, char* argv[], game_args_t &args);

typedef struct {
    bool read;
    bool write;
    bool debug;
    int nummon;
    std::string dungeon_path;
    std::string monster_path;
    std::string item_path;
} game_args_t;

int main(int argc, char* argv[]) {
    srand(time(NULL));

    game_args_t args;
    if (prepare_args(argc, argv, args)) {
        return 1;
    }
    
    game_t game(args.debug, DUNGEON_WIDTH, DUNGEON_HEIGHT, ROOM_MIN_COUNT + ROOM_COUNT_MAX_RANDOMNESS);
    if (args.nummon >= 0) game.override_nummon(args.nummon);

    if (args.read) {
        game.init_from_file(args.dungeon_path.c_str());
    }
    else {
        game.init_random();
    }

    if (args.write) {
        game.write_to_file(args.dungeon_path.c_str());
    }

    if (args.debug) {
        game.dungeon->write_pgm();
        printf("debug: wrote hardness map to dungeon.pgm\n");
    }

    game.init_monster_defs(args.monster_path.c_str());
    game.init_item_defs(args.item_path.c_str());

    game.random_monsters();
    game.random_items();

    game.run();

    return 0;
}

int prepare_args(int argc, char* argv[], game_args_t &args) {
    int i;
    bool d_path_next = false;
    bool m_path_next = false;
    bool i_path_next = false;
    bool custom_d_path = false;
    int nummon_next = false;
    long temp;
    char *err;
    char *home = getenv("HOME");
    args.dungeon_path += home;
    args.dungeon_path += DEFAULT_DUNGEON_PATH;
    args.monster_path += home;
    args.monster_path += DEFAULT_MONSTER_PATH;
    args.item_path += home;
    args.item_path += DEFAULT_ITEM_PATH;
    args.nummon = -1;
    for (i = 1; i < argc; i++) {
        if (d_path_next) {
            args.dungeon_path = argv[i];
            custom_d_path = true;
            d_path_next = 0;
        }
        if (m_path_next) {
            args.monster_path = argv[i];
            m_path_next = 0;
        }
        if (i_path_next) {
            args.item_path = argv[i];
            i_path_next = 0;
        }
        else if (nummon_next) {
            // https://stackoverflow.com/questions/2024648/convert-a-string-to-int-but-only-if-really-is-an-int
            temp = strtol(argv[i], &err, 10);
            if (*err != '\0' || temp > INT32_MAX || temp < 0)
                throw dungeon_exception(__PRETTY_FUNCTION__, "-n/--nummon must be a positive int\n");
            args.nummon = (int) temp;
            nummon_next = 0;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--load") == 0) args.read = true;
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--save") == 0) args.write = true;
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) args.debug = true;
        else if (strcmp(argv[i], "-dp") == 0 || strcmp(argv[i], "--dungeon-path") == 0) d_path_next = true;
        else if (strcmp(argv[i], "-mp") == 0 || strcmp(argv[i], "--monster-path") == 0) m_path_next = true;
        else if (strcmp(argv[i], "-ip") == 0 || strcmp(argv[i], "--item-path") == 0) i_path_next = true;
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nummon") == 0) nummon_next = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [-l] [-s] [-d] [-p <path>] [-n <nummon>] [-f <fps>]\n  -l/--load: load a rungeon\n  -s/--save: write a random dungeon\n", argv[0]);
            printf("  -p/--path: override path to load/save (default ~/.rlg327/dungeon)\n");
            printf("  -n/--nummon: override monster count (default 10)\n");
            printf("  -d/--debug: enable debugging features\n  -h/--help: display this message\n");
            return 1;
        }
        else {
            throw dungeon_exception(__PRETTY_FUNCTION__, "unrecognized argument. run -h/--help for usage");
        }
    }
    if (d_path_next || m_path_next || i_path_next)
        throw dungeon_exception(__PRETTY_FUNCTION__, "specify a path after -dp/--dungeon-path, -mp/--monster-path, or -ip/--item-path");

    if (nummon_next)
        throw dungeon_exception(__PRETTY_FUNCTION__, "specify a number of monsters after -n/--nummon");

    // It's an error to specify a path without reading or writing
    if (custom_d_path && !args.read && !args.write)
        throw dungeon_exception(__PRETTY_FUNCTION__, "specify one of -l/--load, -w/--write with -p/--path");
    return 0;
}