#include <ncurses.h>
#include <cstring>
#include <cstdlib>

#include "game.h"
#include "macros.h"
#include "dungeon.h"
#include "heap.h"
#include "character.h"
#include "ascii.h"
#include "pathfinding.h"

parser_definition_t MONSTER_PARSE_RULES[] {
    {.name = "NAME", .offset = offsetof(monster_definition_t, name), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "DESC", .offset = offsetof(monster_definition_t, description), .type = PARSE_TYPE_LONG_STRING, .required = true},
    {.name = "COLOR", .offset = offsetof(monster_definition_t, color), .type = PARSE_TYPE_COLOR, .required = true},
    {.name = "SPEED", .offset = offsetof(monster_definition_t, speed), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "ABIL", .offset = offsetof(monster_definition_t, abilities), .type = PARSE_TYPE_MONSTER_ATTRIBUTES, .required = true},
    {.name = "HP", .offset = offsetof(monster_definition_t, hp), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "DAM", .offset = offsetof(monster_definition_t, damage), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "SYMB", .offset = offsetof(monster_definition_t, symbol), .type = PARSE_TYPE_CHAR, .required = true},
    {.name = "RRTY", .offset = offsetof(monster_definition_t, rarity), .type = PARSE_TYPE_RARITY, .required = true}
};

parser_definition_t ITEM_PARSE_RULES[] {
    {.name = "NAME", .offset = offsetof(item_definition_t, name), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "DESC", .offset = offsetof(item_definition_t, description), .type = PARSE_TYPE_LONG_STRING, .required = true},
    {.name = "TYPE", .offset = offsetof(item_definition_t, type), .type = PARSE_TYPE_ITEM_TYPE, .required = true},
    {.name = "COLOR", .offset = offsetof(item_definition_t, color), .type = PARSE_TYPE_COLOR, .required = true},
    {.name = "HIT", .offset = offsetof(item_definition_t, hit_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "DAM", .offset = offsetof(item_definition_t, damage_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "DODGE", .offset = offsetof(item_definition_t, dodge_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "DEF", .offset = offsetof(item_definition_t, defense_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "WEIGHT", .offset = offsetof(item_definition_t, weight), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "SPEED", .offset = offsetof(item_definition_t, speed_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "ATTR", .offset = offsetof(item_definition_t, attributes), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "VAL", .offset = offsetof(item_definition_t, value), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "ART", .offset = offsetof(item_definition_t, artifact), .type = PARSE_TYPE_BOOL, .required = true},
    {.name = "RRTY", .offset = offsetof(item_definition_t, rarity), .type = PARSE_TYPE_RARITY, .required = true}
};

game_t::game_t(int debug, uint8_t width, uint8_t height, int max_rooms) {
    int i, j;
    this->debug = debug;
    this->dungeon = new dungeon_t(width, height, max_rooms);

    pathfinding_no_tunnel = (uint32_t **) malloc(width * sizeof (uint32_t*));
    if (pathfinding_no_tunnel == NULL) {
        goto init_free;
    }
    for (i = 0; i < width; i++) {
        pathfinding_no_tunnel[i] = (uint32_t *) malloc(height * sizeof (uint32_t));
        if (pathfinding_no_tunnel[i] == NULL) {
            for (j = 0; j < i; j++) free(pathfinding_no_tunnel[j]);
            goto init_free_pathfinding_no_tunnel;
        }
    }

    pathfinding_tunnel = (uint32_t **) malloc(width * sizeof (uint32_t*));
    if (pathfinding_tunnel == NULL) {
        goto init_free_all_pathfinding_no_tunnel;
    }
    for (i = 0; i < width; i++) {
        pathfinding_tunnel[i] = (uint32_t *) malloc(height * sizeof (uint32_t));
        if (pathfinding_tunnel[i] == NULL) {
            for (j = 0; j < i; j++) free(pathfinding_tunnel[j]);
            goto init_free_pathfinding_tunnel;
        }
    }
    try {
        turn_queue = new binary_heap_t(sizeof (character_t *), NULL);
    } catch (std::exception &e) {
        goto init_free_all_pathfinding_tunnel;
    }

    character_map = (character_t ***) malloc(width * sizeof (character_t**));
    if (character_map == NULL) {
        goto init_free_heap;
    }
    for (i = 0; i < width; i++) {
        character_map[i] = (character_t **) malloc(height * sizeof (character_t*));
        if (character_map[i] == NULL) {
            for (j = 0; j < i; j++) free(character_map[j]);
            goto init_free_character_map;
        }
        for (j = 0; j < height; j++) character_map[i][j] = NULL;
    }

    seen_map = (cell_type_t **) malloc(width * sizeof (cell_type_t*));
    if (seen_map == NULL) {
        goto init_free_all_character_map;
    }
    for (i = 0; i < width; i++) {
        seen_map[i] = (cell_type_t *) malloc(height * sizeof (cell_type_t));
        if (seen_map[i] == NULL) {
            for (j = 0; j < i; j++) free(seen_map[j]);
            goto init_free_seen_map;
        }
        for (j = 0; j < height; j++) seen_map[i][j] = CELL_TYPE_HIDDEN;
    }

    if (!(message = (char *) malloc(1 + WIDTH))) {
        goto init_free_all_seen_map;
    }
    message[0] = '\0';

    monst_parser = new parser_t<monster_definition_t>(MONSTER_PARSE_RULES, sizeof (MONSTER_PARSE_RULES) / sizeof (MONSTER_PARSE_RULES[0]), "RLG327 MONSTER DESCRIPTION 1", "MONSTER", true);
    item_parser = new parser_t<item_definition_t>(ITEM_PARSE_RULES, sizeof (ITEM_PARSE_RULES) / sizeof (ITEM_PARSE_RULES[0]), "RLG327 OBJECT DESCRIPTION 1", "OBJECT", true);

    return;

    init_free_all_seen_map:
    for (j = 0; j < width; j++) free(seen_map[j]);
    init_free_seen_map:
    free(seen_map);
    init_free_all_character_map:
    for (j = 0; j < width; j++) free(character_map[j]);
    init_free_character_map:
    free(character_map);
    init_free_heap:
    delete turn_queue;
    init_free_all_pathfinding_tunnel:
    for (j = 0; j < width; j++) free(pathfinding_tunnel[j]);
    init_free_pathfinding_tunnel:
    free(pathfinding_tunnel);
    init_free_all_pathfinding_no_tunnel:
    for (j = 0; j < width; j++) free(pathfinding_no_tunnel[j]);
    init_free_pathfinding_no_tunnel:
    free(pathfinding_no_tunnel);
    init_free:
    throw dungeon_exception(__PRETTY_FUNCTION__, "failed to allocate dungeon");
}

game_t::~game_t() {
    character_t *character;
    while (turn_queue->size() > 0) {
        try {
            turn_queue->remove((void *) &character);
         } catch (std::exception &e) {
            // Nothing we can do here :shrug:
            fprintf(stderr, "err: catastrophe: failed to remove from heap while destroying game");
        }
        if (character == &pc) continue;
        destroy_character(character_map, character);
    }

    int i;
    delete turn_queue;
    free(message);
    for (i = 0; i < dungeon->width; i++) free(character_map[i]);
    free(character_map);
    for (i = 0; i < dungeon->width; i++) free(pathfinding_tunnel[i]);
    free(pathfinding_tunnel);
    for (i = 0; i < dungeon->width; i++) free(pathfinding_no_tunnel[i]);
    free(pathfinding_no_tunnel);
    for (i = 0; i < dungeon->width; i++) free(seen_map[i]);
    free(seen_map);
    delete dungeon;

    for (monster_definition_t *monst : monster_defs) {
        delete monst->speed;
        delete monst->damage;
        delete monst->hp;
        delete monst;
    }
    for (item_definition_t *item : item_defs) {
        delete item->hit_bonus;
        delete item->damage_bonus;
        delete item->dodge_bonus;
        delete item->defense_bonus;
        delete item->weight;
        delete item->speed_bonus;
        delete item->attributes;
        delete item->value;
        delete item;
    }
    delete monst_parser;
    delete item_parser;
}

void game_t::init_monster_defs(const char *path) {
    std::ifstream file(path);
    if (file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file");
    
    monst_parser->parse(monster_defs, file);
}


void game_t::init_item_defs(const char *path) {
    std::ifstream file(path);
    if (file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file");
    
    monst_parser->parse(monster_defs, file);
}

void game_t::init_from_file(const char *path) {
    coordinates_t pc_coords;
    character_t *pc_pt;
    FILE *f;
    f = fopen(path, "rb");
    if (f == NULL) {
        throw dungeon_exception(__PRETTY_FUNCTION__, "could not open the specified file");
    }
    try {
        this->dungeon->fill_from_file(f, debug, &pc_coords);
    }
    catch (dungeon_exception &e) {
        fclose(f);
        throw dungeon_exception(__PRETTY_FUNCTION__, e);
    }
    fclose(f);

    // Place the PC now
    pc.x = pc_coords.x;
    pc.y = pc_coords.y;
    pc.dead = 0;
    pc.display = '@';
    pc.speed = PC_SPEED;
    character_map[pc.x][pc.y] = &pc;

    update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);

    pc_pt = &pc;
    turn_queue->insert((void *) &pc_pt, 0);
    is_initialized = true;
}

void game_t::init_random() {
    coordinates_t pc_coords;
    character_t *pc_pt;
    dungeon->fill(ROOM_MIN_COUNT, ROOM_COUNT_MAX_RANDOMNESS, ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT, ROOM_MAX_RANDOMNESS, debug);

    pc_coords = dungeon->random_location();

    // Place the PC now
    pc.x = pc_coords.x;
    pc.y = pc_coords.y;
    pc.dead = 0;
    pc.display = '@';
    pc.speed = PC_SPEED;
    character_map[pc.x][pc.y] = &pc;

    update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);

    pc_pt = &pc;
    turn_queue->insert((void *) &pc_pt, 0);
    is_initialized = true;
}

void game_t::write_to_file(const char *path) {
    if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
    FILE *f;
    coordinates_t pc_coords;
    f = fopen(path, "wb");
    if (f == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "couldn't open file for writing");
    pc_coords.x = pc.x;
    pc_coords.y = pc.y;
    try {
        dungeon->save_to_file(f, debug, &pc_coords);
    } catch (dungeon_exception &e) {
        fclose(f);
        throw dungeon_exception(__PRETTY_FUNCTION__, e);
    }
    fclose(f);
}

void game_t::override_nummon(int nummon) {
    this->nummon = nummon;
}

void game_t::random_monsters() {
    if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
    if (monster_defs.size() == 0) throw dungeon_exception(__PRETTY_FUNCTION__, "no monster definitions are set");
    // generate_monsters(dungeon, turn_queue, character_map, nummon < 0 ? (rand() % (RANDOM_MONSTERS_MAX - RANDOM_MONSTERS_MIN + 1)) + RANDOM_MONSTERS_MIN : nummon);
}

void game_t::random_items() {
    if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
    if (item_defs.size() == 0) throw dungeon_exception(__PRETTY_FUNCTION__, "no item definitions are set");
}

void game_t::update_fog_of_war() {
    if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
    int x, y;
    for (x = pc.x - FOG_OF_WAR_DISTANCE; x <= pc.x + FOG_OF_WAR_DISTANCE; x++) {
        if (x < 0 || x >= dungeon->width) continue;
        for (y = pc.y - FOG_OF_WAR_DISTANCE; y <= pc.y + FOG_OF_WAR_DISTANCE; y++) {
            if (y < 0 || y >= dungeon->height) continue;
            seen_map[x][y] = dungeon->cells[x][y].type;
        }
    }
}

void game_t::try_move(int x_offset, int y_offset) {
    if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
    int new_x = pc.x + x_offset;
    int new_y = pc.y + y_offset;
    if (new_x < 0) new_x = 0;
    if (new_x >= dungeon->width) new_x = dungeon->width - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= dungeon->height) new_y = dungeon->height - 1;
    if (dungeon->cells[new_x][new_y].type == CELL_TYPE_STONE) {
        snprintf(message, 80, "There's stone in the way!");
    }
    else {
        if (character_map[new_x][new_y] != NULL) {
            // Kill the monster there
            character_map[new_x][new_y]->dead = 1;
            snprintf(message, 80, "You ate the %c in the way.", character_map[new_x][new_y]->display);
        }
        pc.move_to((coordinates_t) {(uint8_t) new_x, (uint8_t) new_y}, character_map);
        update_fog_of_war();
    }
}

void game_t::move_coords(coordinates_t &coords, int x_offset, int y_offset) {
    int new_x = (int) coords.x + x_offset;
    int new_y = (int) coords.y + y_offset;
    if (new_x < 0) new_x = 0;
    if (new_x >= dungeon->width) new_x = dungeon->width - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= dungeon->height) new_y = dungeon->height - 1;
    coords.x = new_x;
    coords.y = new_y;
}

void game_t::force_move(coordinates_t dest) {
    if (character_map[dest.x][dest.y] != NULL && character_map[dest.x][dest.y] != &pc) {
        // Kill the monster there
        character_map[dest.x][dest.y]->dead = 1;
        snprintf(message, 80, "The poor %c stood no chance. Cheater!", character_map[dest.x][dest.y]->display);
    }
    pc.move_to(dest, character_map);
    update_fog_of_war();
}

void game_t::fill_and_place_on(cell_type_t target_cell) {
    if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
    // Kill all the monsters (RIP)
    character_t *character;
    int x, y, placed;
    while (turn_queue->size() > 0) {
        turn_queue->remove((void *) &character);
        if (character == &pc) continue;
        destroy_character(character_map, character);
    }
    dungeon->fill(ROOM_MIN_COUNT, ROOM_COUNT_MAX_RANDOMNESS, ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT, ROOM_MAX_RANDOMNESS, 0);
    placed = 0;
    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            if (dungeon->cells[x][y].type == target_cell) {
                pc.move_to((coordinates_t) {(uint8_t) x, (uint8_t) y}, character_map);
                placed = 1;
                break;
            }
        }
    }
    if (!placed) throw dungeon_exception(__PRETTY_FUNCTION__, "no target cell present to place PC on");
    // Readd the PC and new monsters
    character = &pc;
    turn_queue->insert((void *) &character, 0);
    generate_monsters(dungeon, turn_queue, character_map, nummon < 0 ? (rand() % (RANDOM_MONSTERS_MAX - RANDOM_MONSTERS_MIN + 1)) + RANDOM_MONSTERS_MIN : nummon);
    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            seen_map[x][y] = CELL_TYPE_HIDDEN;
        }
    }
    update_fog_of_war();
}
