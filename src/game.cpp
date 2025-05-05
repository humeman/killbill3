#include <cstring>
#include <cstdlib>

#include "game.h"
#include "macros.h"
#include "dungeon.h"
#include "heap.h"
#include "character.h"
#include "ascii.h"
#include "message_queue.h"
#include "pathfinding.h"
#include "logger.h"

parser_definition_t MONSTER_PARSE_RULES[] {
    {.name = "NAME", .offset = offsetof(MonsterDefinition, name), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "DESC", .offset = offsetof(MonsterDefinition, description), .type = PARSE_TYPE_LONG_STRING, .required = true},
    {.name = "COLOR", .offset = offsetof(MonsterDefinition, color), .type = PARSE_TYPE_COLOR, .required = true},
    {.name = "SPEED", .offset = offsetof(MonsterDefinition, speed), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "ABIL", .offset = offsetof(MonsterDefinition, abilities), .type = PARSE_TYPE_MONSTER_ATTRIBUTES, .required = true},
    {.name = "HP", .offset = offsetof(MonsterDefinition, hp), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "DAM", .offset = offsetof(MonsterDefinition, damage), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "TEXTURE_N", .offset = offsetof(MonsterDefinition, floor_texture_n), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "TEXTURE_E", .offset = offsetof(MonsterDefinition, floor_texture_e), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "TEXTURE_W", .offset = offsetof(MonsterDefinition, floor_texture_w), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "TEXTURE_S", .offset = offsetof(MonsterDefinition, floor_texture_s), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "UI_TEXTURE", .offset = offsetof(MonsterDefinition, ui_texture), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "RRTY", .offset = offsetof(MonsterDefinition, rarity), .type = PARSE_TYPE_RARITY, .required = true}
};

parser_definition_t ITEM_PARSE_RULES[] {
    {.name = "NAME", .offset = offsetof(ItemDefinition, name), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "DESC", .offset = offsetof(ItemDefinition, description), .type = PARSE_TYPE_LONG_STRING, .required = true},
    {.name = "TYPE", .offset = offsetof(ItemDefinition, type), .type = PARSE_TYPE_ITEM_TYPE, .required = true},
    {.name = "COLOR", .offset = offsetof(ItemDefinition, color), .type = PARSE_TYPE_COLOR, .required = true},
    {.name = "DAM", .offset = offsetof(ItemDefinition, damage_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "DODGE", .offset = offsetof(ItemDefinition, dodge_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "DEF", .offset = offsetof(ItemDefinition, defense_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "SPEED", .offset = offsetof(ItemDefinition, speed_bonus), .type = PARSE_TYPE_DICE, .required = true},
    {.name = "ART", .offset = offsetof(ItemDefinition, artifact), .type = PARSE_TYPE_BOOL, .required = true},
    {.name = "RRTY", .offset = offsetof(ItemDefinition, rarity), .type = PARSE_TYPE_RARITY, .required = true},
    {.name = "TEXTURE", .offset = offsetof(ItemDefinition, floor_texture), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "UI_TEXTURE", .offset = offsetof(ItemDefinition, ui_texture), .type = PARSE_TYPE_STRING, .required = true}
};

parser_definition_t DUNGEON_OPTIONS_PARSE_RULES[] {
    {.name = "NAME", .offset = offsetof(DungeonOptions, name), .type = PARSE_TYPE_STRING, .required = true},
    {.name = "NUMENEMIES", .offset = offsetof(DungeonOptions, nummon), .type = PARSE_TYPE_TUPLE, .required = true},
    {.name = "NUMITEMS", .offset = offsetof(DungeonOptions, numitems), .type = PARSE_TYPE_TUPLE, .required = true},
    {.name = "NUMROOMS", .offset = offsetof(DungeonOptions, rooms), .type = PARSE_TYPE_TUPLE, .required = true},
    {.name = "SIZE", .offset = offsetof(DungeonOptions, size), .type = PARSE_TYPE_TUPLE, .required = true},
    {.name = "UP", .offset = offsetof(DungeonOptions, up_staircase), .type = PARSE_TYPE_STRING, .required = false},
    {.name = "DOWN", .offset = offsetof(DungeonOptions, down_staircase), .type = PARSE_TYPE_STRING, .required = false},
    {.name = "ENEMIES", .offset = offsetof(DungeonOptions, monsters), .type = PARSE_TYPE_VECTOR_STRINGS, .required = true},
    {.name = "ITEMS", .offset = offsetof(DungeonOptions, items), .type = PARSE_TYPE_VECTOR_STRINGS, .required = true},
    {.name = "BOSS", .offset = offsetof(DungeonOptions, boss), .type = PARSE_TYPE_STRING, .required = false},
    {.name = "DEFAULT", .offset = offsetof(DungeonOptions, is_default), .type = PARSE_TYPE_BOOL, .required = false},
    {.name = "KEY", .offset = offsetof(DungeonOptions, key), .type = PARSE_TYPE_STRING, .required = false}
};

parser_definition_t VOICE_LINES_PARSE_RULES[] {
    {.name = "VALUE", .offset = offsetof(VoiceLines, value), .type = PARSE_TYPE_LONG_STRING, .required = true},
    {.name = "DURATION", .offset = offsetof(VoiceLines, duration), .type = PARSE_TYPE_INT, .required = true},
    {.name = "MUSIC", .offset = offsetof(VoiceLines, music), .type = PARSE_TYPE_STRING, .required = true}
};

char CHARACTERS_BY_CELL_TYPE[CELL_TYPES] = {
    [CELL_TYPE_STONE] = ' ',
    [CELL_TYPE_ROOM] = '.',
    [CELL_TYPE_HALL] = '#',
    [CELL_TYPE_UP_STAIRCASE] = '<',
    [CELL_TYPE_DOWN_STAIRCASE] = '>',
    [CELL_TYPE_EMPTY] = '!',
    [CELL_TYPE_DEBUG] = 'X',
    [CELL_TYPE_HIDDEN] = ' '
};

int COLORS_BY_CELL_TYPE[CELL_TYPES] = {
    [CELL_TYPE_STONE] = COLORS_STONE,
    [CELL_TYPE_ROOM] = COLORS_FLOOR,
    [CELL_TYPE_HALL] = COLORS_FLOOR,
    [CELL_TYPE_UP_STAIRCASE] = COLORS_OBJECT,
    [CELL_TYPE_DOWN_STAIRCASE] = COLORS_OBJECT,
    [CELL_TYPE_EMPTY] = COLORS_OBJECT,
    [CELL_TYPE_DEBUG] = COLORS_OBJECT,
    [CELL_TYPE_HIDDEN] = COLORS_STONE
};

std::string ITEM_TYPE_STRINGS[ITEM_TYPE_UNKNOWN + 1] = {
    "weapon",
    "hat",
    "shirt",
    "pants",
    "shoes",
    "glasses",
    "pocket",
    "keycard",
    "stack",
    "mysterious object"
};

Game::Game(int debug) {
    this->debug = debug; 
    monst_parser = new Parser<MonsterDefinition>(MONSTER_PARSE_RULES, sizeof (MONSTER_PARSE_RULES) / sizeof (MONSTER_PARSE_RULES[0]), "KILL BILL 3 ENEMY DESCRIPTION 1", "ENEMY", false);
    item_parser = new Parser<ItemDefinition>(ITEM_PARSE_RULES, sizeof (ITEM_PARSE_RULES) / sizeof (ITEM_PARSE_RULES[0]), "KILL BILL 3 ITEM DESCRIPTION 1", "ITEM", false);
    map_parser = new Parser<DungeonOptions>(DUNGEON_OPTIONS_PARSE_RULES, sizeof (DUNGEON_OPTIONS_PARSE_RULES) / sizeof (DUNGEON_OPTIONS_PARSE_RULES[0]), "KILL BILL 3 MAP DESCRIPTION 1", "MAP", false);
    vl_parser = new Parser<VoiceLines>(VOICE_LINES_PARSE_RULES, sizeof (VOICE_LINES_PARSE_RULES) / sizeof (VOICE_LINES_PARSE_RULES[0]), "KILL BILL 3 VOICELINE DESCRIPTION 1", "LINE", false);
    init_controls();
}

Game::~Game() {
    Character *character;
    while (turn_queue.size() > 0) {
        try {
            character = turn_queue.remove();
         } catch (std::exception &e) {
            // Nothing we can do here :shrug:
            Logger::error(__FILE__, "catastrophe: failed to remove from heap while destroying game");
        }
        if (character == &pc) continue;
        destroy_character(character_map, character);
    }

    int x, y;
    for (x = 0; x < dungeon->width; x++)
        for (y = 0; y < dungeon->height; y++)
            if (item_map[x][y]) delete item_map[x][y];

    int i;
    for (i = 0; i < dungeon->width; i++) free(character_map[i]);
    free(character_map);
    for (i = 0; i < dungeon->width; i++) free(item_map[i]);
    free(item_map);
    for (i = 0; i < dungeon->width; i++) free(pathfinding_tunnel[i]);
    free(pathfinding_tunnel);
    for (i = 0; i < dungeon->width; i++) free(pathfinding_no_tunnel[i]);
    free(pathfinding_no_tunnel);
    delete dungeon;

    for (const auto &e : monster_defs) {
        delete e.second->speed;
        delete e.second->damage;
        delete e.second->hp;
        delete e.second;
    }
    for (const auto &e : item_defs) {
        delete e.second->damage_bonus;
        delete e.second->dodge_bonus;
        delete e.second->defense_bonus;
        delete e.second->speed_bonus;
        delete e.second;
    }
    delete monst_parser;
    delete item_parser;
}

void Game::init_monster_defs(const char *path) {
    std::ifstream file(path);
    if (file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file");

    monst_parser->parse(monster_defs, file);
}

void Game::init_item_defs(const char *path) {
    std::ifstream file(path);
    if (file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file");

    item_parser->parse(item_defs, file);
}

void Game::init_maps(const char *path) {
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            std::ifstream file(entry.path());
            if (file.fail()) {
                throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file: " + entry.path().string());
            }
            std::string filename = entry.path().filename().string();
            std::string map_name = filename.substr(0, filename.find_last_of('.'));
            map_parser->parse(map_defs[map_name], file);
        }
    }
}

void Game::init_voice_lines(const char *path) {
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            std::ifstream file(entry.path());
            if (file.fail()) {
                throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file: " + entry.path().string());
            }
            std::string filename = entry.path().filename().string();
            std::string vl_name = filename.substr(0, filename.find_last_of('.'));
            vl_parser->parse(vl_defs[vl_name], file);
        }
    }
}


// void Game::init_from_file(const char *path) {
//     IntPair pc_coords;
//     FILE *f;
//     f = fopen(path, "rb");
//     if (f == NULL) {
//         throw dungeon_exception(__PRETTY_FUNCTION__, "could not open the specified file");
//     }
//     try {
//         this->dungeon->fill_from_file(f, debug, &pc_coords);
//     }
//     catch (dungeon_exception &e) {
//         fclose(f);
//         throw dungeon_exception(__PRETTY_FUNCTION__, e);
//     }
//     fclose(f);

//     // Place the PC now
//     pc.move_to(pc_coords, character_map);
//     pc.dead = false;
//     pc.display = '@';
//     pc.speed = PC_SPEED;
//     character_map[pc.x][pc.y] = &pc;

//     update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);

//     turn_queue.insert(&pc, 0);
//     is_initialized = true;
// }

void Game::apply_dungeon(DungeonFloor &floor, IntPair pc_coords) {
    // If there's a dungeon active now, we need to clean it up.
    Character *ch;
    // Empty out the turn queue (regardless in case we mess up somewhere)
    while (turn_queue.size() > 0) {
        ch = turn_queue.remove();
        if (ch->type() == CHARACTER_TYPE_MONSTER && ch->dead) {
            // Died between the last run and now. Delete it.
            destroy_character(character_map, ch);
        }
    }
    if (dungeon) {
        // Remove the PC from its old location
        if (character_map[pc.x][pc.y] == &pc) {
            character_map[pc.x][pc.y] = nullptr;
        }
    }
        
    // Update the dungeon feature references
    dungeon = floor.dungeon;
    character_map = floor.character_map;
    item_map = floor.item_map;
    pathfinding_tunnel = floor.pathfinding_tunnel;
    pathfinding_no_tunnel = floor.pathfinding_no_tunnel;

    // Toss everyone back in the turn queue
    unsigned int x, y;
    for (x = 0; x < dungeon->width; x++) {
        for (y = 0; y < dungeon->height; y++) {
            if (character_map[x][y]) {
                turn_queue.insert(character_map[x][y], 1000 / character_map[x][y]->speed);
            }
        }
    }
    turn_queue.insert(&pc, 0);
    
    // Move the PC to its new location
    pc.location_initialized = false;
    pc.move_to(pc_coords, character_map);

    // And update pathfinding
    update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);
}

void Game::init_from_map(std::string map_name) {
    std::map<std::string, DungeonOptions *> map = map_defs[map_name];
    IntPair pc_coords;
    DungeonFloor *dungeon_floor;
    Dungeon *new_dungeon;
    bool default_found = false;
    unsigned int i;

    pc.dead = false;
    pc.display = '@';
    pc.speed = PC_SPEED;

    for (const auto &pair : map) {
        // This is pretty bad, but the dungeons are randomly generated.
        // There's always a possibility that we get really unlucky, and some
        // placement is impossible, so this will get retried if so rather
        // than crashing.
        for (i = 0; i < MAX_DUNGEON_GENERATION_ATTEMPTS; i++) {
            try {
                new_dungeon = new Dungeon(*(pair.second));
                new_dungeon->fill();
                break;
            } catch (dungeon_exception &e) {
                delete new_dungeon;
                new_dungeon = nullptr;
                Logger::debug(__FILE__, "failed to generate dungeon (attempt " + std::to_string(i) + "): " + std::string(e.what()));
            }
        }
        if (!new_dungeon) {
            throw dungeon_exception(__PRETTY_FUNCTION__, "failed to generate dungeon after " STRING(MAX_DUNGEON_GENERATION_ATTEMPTS) " attempts");
        }
        dungeon_floor = new DungeonFloor(pair.first, new_dungeon);
        random_monsters(new_dungeon, dungeon_floor->character_map);
        random_items(new_dungeon, dungeon_floor->item_map);

        if (pair.second->is_default) {
            if (default_found) throw dungeon_exception(__PRETTY_FUNCTION__, "multiple default dungeons found");
            default_found = true;
            // TODO: Place the PC on the left side of the room
            apply_dungeon(*dungeon_floor, new_dungeon->random_location());
        }

        dungeons.push_back(dungeon_floor);
    }
    if (!default_found) throw dungeon_exception(__PRETTY_FUNCTION__, "no default dungeon found");
}

// void Game::write_to_file(const char *path) {
//     if (!is_initialized) throw dungeon_exception(__PRETTY_FUNCTION__, "game is not yet initialized");
//     FILE *f;
//     IntPair pc_coords;
//     f = fopen(path, "wb");
//     if (f == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "couldn't open file for writing");
//     pc_coords.x = pc.x;
//     pc_coords.y = pc.y;
//     try {
//         dungeon->save_to_file(f, debug, &pc_coords);
//     } catch (dungeon_exception &e) {
//         fclose(f);
//         throw dungeon_exception(__PRETTY_FUNCTION__, e);
//     }
//     fclose(f);
// }

void Game::random_monsters(Dungeon *t_dungeon, Character ***t_cmap) {
    if (monster_defs.size() == 0) throw dungeon_exception(__PRETTY_FUNCTION__, "no monster definitions are set");

    // Pick how many we want to generate.
    int count = RAND_BETWEEN(t_dungeon->options->nummon.x, t_dungeon->options->nummon.y);

    IntPair loc;
    int monster_i, attempts;
    Monster *monst;
    Character *ch;
    int i, j;
    bool allowed;
    std::string mid;
    for (i = 0; i < count; i++) {
        attempts = 0;
        while (attempts++ < MAX_GENERATION_ATTEMPTS) {
            monster_i = rand() % t_dungeon->options->monsters.size();
            mid = t_dungeon->options->monsters[monster_i];
            if (monster_defs[mid]->abilities & MONSTER_ATTRIBUTE_UNIQUE) {
                if (monster_defs[mid]->unique_slain) continue; // If we've already killed this type of unique monster
                // Or, if there's already one in the dungeon.
                allowed = true;
                for (j = 0; j < turn_queue.size(); j++) {
                    ch = turn_queue.at(j);
                    if (ch == &pc) continue;
                    if (&(((Monster *) ch)->definition) == &(monster_defs[mid])) {
                        allowed = false;
                        break;
                    }
                }
                if (!allowed) continue;
            }
            if (rand() % 100 >= monster_defs[mid]->rarity) continue;

            break;
        }
        if (attempts == MAX_GENERATION_ATTEMPTS)
            throw dungeon_exception(__PRETTY_FUNCTION__, "no available monster definitions to use after " STRING(MAX_GENERATION_ATTEMPTS) "rolls (all unique or really unlucky)");

        // Now we can make a monster from this definition.
        monst = new Monster(monster_defs[mid], t_dungeon->options->key.length() == 0 ? nullptr : item_defs[t_dungeon->options->key]);
        // Pick a location...
        try {
            loc = random_location_no_kill(t_dungeon, t_cmap);
        } catch (dungeon_exception &e) {
            delete monst; // The rest of the ones in the queue already will be cleared out by the game destructor.
            throw dungeon_exception(__PRETTY_FUNCTION__, e, "no available space in dungeon for monster placement");
        }
        monst->move_to(loc, t_cmap);
    }

    // Insert boss, if one is chosen
    if (t_dungeon->options->boss.length() > 0) {
        monst = new Monster(monster_defs[t_dungeon->options->boss], nullptr);
        try {
            // TODO: Spawn on right side of map
            loc = random_location_no_kill(t_dungeon, t_cmap);
        } catch (dungeon_exception &e) {
            throw dungeon_exception(__PRETTY_FUNCTION__, e, "no available space in dungeon for monster placement");
        }
        monst->move_to(loc, t_cmap);
    }
}

void Game::random_items(Dungeon *t_dungeon, Item ***t_imap) {
    if (item_defs.size() == 0) throw dungeon_exception(__PRETTY_FUNCTION__, "no item definitions are set");
    // Ripped for the most part from random_monsters().
    // Pick how many we want to generate.
    int count = RAND_BETWEEN(t_dungeon->options->numitems.x, t_dungeon->options->numitems.y);

    IntPair loc;
    int item_i, attempts;
    Item *item;
    std::string iid;
    for (int i = 0; i < count; i++) {
        attempts = 0;
        while (attempts++ < MAX_GENERATION_ATTEMPTS) {
            item_i = rand() % t_dungeon->options->items.size();
            iid = t_dungeon->options->items[item_i];
            if (item_defs[iid]->artifact && item_defs[iid]->artifact_created) continue;
            if (rand() % 100 >= item_defs[iid]->rarity) continue;

            break;
        }
        if (attempts == MAX_GENERATION_ATTEMPTS)
            throw dungeon_exception(__PRETTY_FUNCTION__, "no available item definitions to use after " STRING(MAX_GENERATION_ATTEMPTS) "rolls (all unique or really unlucky)");

        // Now we can make a monster from this definition.
        item = new Item(item_defs[iid]);
        // Pick a location...
        try {
            loc = t_dungeon->random_location();
        } catch (dungeon_exception &e) {
            delete item; // The rest of the ones in the queue already will be cleared out by the game destructor.
            throw dungeon_exception(__PRETTY_FUNCTION__, e, "no available space in dungeon for item placement");
        }
        if (t_imap[loc.x][loc.y]) {
            t_imap[loc.x][loc.y]->add_to_stack(item);
        } else {
            t_imap[loc.x][loc.y] = item;
        }
    }
}

void Game::move_coords(IntPair &coords, int x_offset, int y_offset) {
    int new_x = (int) coords.x + x_offset;
    int new_y = (int) coords.y + y_offset;
    if (new_x < 0) new_x = 0;
    if (new_x >= dungeon->width) new_x = dungeon->width - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= dungeon->height) new_y = dungeon->height - 1;
    coords.x = new_x;
    coords.y = new_y;
}

void Game::try_move(int x_offset, int y_offset) {
    int damage;
    Monster *monst;
    Item *keycard;
    int i;
    int new_x = pc.x + x_offset;
    int new_y = pc.y + y_offset;
    if (new_x < 0) new_x = 0;
    if (new_x >= dungeon->width) new_x = dungeon->width - 1;
    if (new_y < 0) new_y = 0;
    if (new_y >= dungeon->height) new_y = dungeon->height - 1;
    if (dungeon->cells[new_x][new_y].type == CELL_TYPE_STONE) {
        MessageQueue::get()->clear();
        MessageQueue::get()->add("&0&bThere's stone in the way!");
    }
    else if (character_map[new_x][new_y] && character_map[new_x][new_y]->type() == CHARACTER_TYPE_MONSTER) {
            // If that second condition didn't hit, we're moving to our own location (or there are two PCs somehow).
            // Attack the monster there
            damage = pc.damage_bonus();
            monst = (Monster *) (character_map[new_x][new_y]);
            monst->damage(damage, result, dungeon, item_map, character_map);
            MessageQueue::get()->add(
                "You hit &" +
                std::to_string(monst->current_color()) +
                escape_col(monst->definition->name) +
                "&r for &b" + std::to_string(damage) + "&r"
                + (monst->hp <= 0 ? ", killing it" : (" (" + std::to_string(monst->hp) + " left)")) + ".");
    } else if (dungeon->cells[new_x][new_y].type == CELL_TYPE_UP_STAIRCASE) {
        // To go upstairs, they have to have a keycard
        if (!(dungeon->cells[new_x][new_y].attributes & CELL_ATTRIBUTE_UNLOCKED)) {
            keycard = nullptr;
            for (i = 0; !keycard && i < pc.inventory_size(); i++) {
                if (pc.inventory_at(i)->definition->type == ITEM_TYPE_KEY) {
                    keycard = pc.remove_from_inventory(i);
                }
            }
            if (!keycard) {
                MessageQueue::get()->clear();
                MessageQueue::get()->add("&0&bYou need a keycard to use these stairs!");
                return;
            }
            delete keycard;
            dungeon->cells[new_x][new_y].attributes |= CELL_ATTRIBUTE_UNLOCKED;
        }
        // Find the target dungeon floor
        for (const auto &new_dungeon : dungeons) {
            if (new_dungeon->id == dungeon->options->up_staircase) {
                // Swap to that dungeon
                // FIXME: PC location
                apply_dungeon(*new_dungeon, new_dungeon->dungeon->random_location());
                MessageQueue::get()->clear();
                MessageQueue::get()->add("You go up the stairs to &b" + new_dungeon->dungeon->options->name + "&r.");
                return;
            }
        }
        throw dungeon_exception(__PRETTY_FUNCTION__, "no target floor with ID " + dungeon->options->up_staircase);
    } else if (dungeon->cells[new_x][new_y].type == CELL_TYPE_DOWN_STAIRCASE) {
        // Find the target dungeon floor
        for (const auto &new_dungeon : dungeons) {
            if (new_dungeon->id == dungeon->options->down_staircase) {
                // Swap to that dungeon
                // FIXME: PC location
                apply_dungeon(*new_dungeon, new_dungeon->dungeon->random_location());
                MessageQueue::get()->clear();
                MessageQueue::get()->add("You go down the stairs to &b" + new_dungeon->dungeon->options->name + "&r.");
                return;
            }
        }
        throw dungeon_exception(__PRETTY_FUNCTION__, "no target floor with ID " + dungeon->options->up_staircase);
    } else {
        pc.move_to((IntPair) {(uint8_t) new_x, (uint8_t) new_y}, character_map);
    }
}

void Game::force_move(IntPair dest) {
    Monster *monst;
    if (character_map[dest.x][dest.y] && character_map[dest.x][dest.y]->type() == CHARACTER_TYPE_MONSTER) {
        monst = (Monster *) character_map[dest.x][dest.y];
        // Kill the monster there
        MessageQueue::get()->add(
            "You suddenly materialize above &" +
            std::to_string(monst->current_color()) +
            escape_col(monst->definition->name) +
            "&r, killing it instantly. Poor thing.");
        monst->die(result, dungeon, character_map, item_map);
    }
    pc.move_to(dest, character_map);
}
