#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "../dungeon.h"
#include "../macros.h"
#include "../game.h"
#include "../macros.h"
#include "../resource_manager.h"
#include "../logger.h"

typedef struct {
    bool debug;
} game_args_t;

int prepare_args(int argc, char* argv[], game_args_t &args);

int main(int argc, char* argv[]) {
    srand(time(NULL));

    game_args_t args = {.debug = false};
    if (prepare_args(argc, argv, args)) {
        return 1;
    }

    Game game(args.debug);

    game.init_maps("assets/maps");
    game.init_from_map("easy");

    if (args.debug) {
        game.dungeon->write_pgm();
        printf("debug: wrote hardness map to dungeon.pgm\n");
    }

    game.init_monster_defs("assets/enemies.txt");
    game.init_item_defs("assets/items.txt");
    
    game.random_monsters();
    game.random_items();

    game.create_nc();
    ResourceManager::get()->load("assets/textures");
    game.run();

    return 0;
}

int prepare_args(int argc, char* argv[], game_args_t &args) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) args.debug = true;
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [-d]\n", argv[0]);
            printf("  -d/--debug: enable debugging features\n  -h/--help: display this message\n");
            return 1;
        }
        else {
            throw dungeon_exception(__PRETTY_FUNCTION__, "unrecognized argument. run -h/--help for usage");
        }
    }
    return 0;
}
