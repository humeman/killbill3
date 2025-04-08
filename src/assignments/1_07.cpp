#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "../macros.h"
#include "../character.h"
#include "../macros.h"
#include "../parser.h"
#include "../item.h"

#define DEFAULT_PATH "/.rlg327/dungeon"

int prepare_args(int argc, char* argv[], int *read, int *write, int *debug, int *nummon, char **path);

#define PRINT_MONST_ATTRS(attrs) { \
    if (attrs & MONSTER_ATTRIBUTE_INTELLIGENT) std::cout << "smart "; \
    if (attrs & MONSTER_ATTRIBUTE_TELEPATHIC) std::cout << "telepathic "; \
    if (attrs & MONSTER_ATTRIBUTE_TUNNELING) std::cout << "tunneling "; \
    if (attrs & MONSTER_ATTRIBUTE_ERRATIC) std::cout << "erratic "; \
    if (attrs & MONSTER_ATTRIBUTE_GHOST) std::cout << "pass "; \
    if (attrs & MONSTER_ATTRIBUTE_PICKUP) std::cout << "pickup "; \
    if (attrs & MONSTER_ATTRIBUTE_DESTROY) std::cout << "destroy "; \
    if (attrs & MONSTER_ATTRIBUTE_UNIQUE) std::cout << "unique "; \
    if (attrs & MONSTER_ATTRIBUTE_BOSS) std::cout << "boss "; }


#define PRINT_COLORS(attrs) { \
    if (attrs & FLAG_COLOR_WHITE) std::cout << "white "; \
    if (attrs & FLAG_COLOR_RED) std::cout << "red "; \
    if (attrs & FLAG_COLOR_GREEN) std::cout << "green "; \
    if (attrs & FLAG_COLOR_YELLOW) std::cout << "yellow "; \
    if (attrs & FLAG_COLOR_BLUE) std::cout << "blue "; \
    if (attrs & FLAG_COLOR_MAGENTA) std::cout << "magenta "; \
    if (attrs & FLAG_COLOR_CYAN) std::cout << "cyan "; \
    if (attrs & FLAG_COLOR_BLACK) std::cout << "black "; }

#define PRINT_ITEM_TYPE(attrs) { \
    if (attrs == ITEM_TYPE_WEAPON) std::cout << "weapon"; \
    else if (attrs == ITEM_TYPE_OFFHAND) std::cout << "offhand"; \
    else if (attrs == ITEM_TYPE_RANGED) std::cout << "ranged"; \
    else if (attrs == ITEM_TYPE_ARMOR) std::cout << "armor"; \
    else if (attrs == ITEM_TYPE_HELMET) std::cout << "helmet"; \
    else if (attrs == ITEM_TYPE_CLOAK) std::cout << "cloak"; \
    else if (attrs == ITEM_TYPE_GLOVES) std::cout << "gloves"; \
    else if (attrs == ITEM_TYPE_BOOTS) std::cout << "boots"; \
    else if (attrs == ITEM_TYPE_RING) std::cout << "ring"; \
    else if (attrs == ITEM_TYPE_AMULET) std::cout << "amulet"; \
    else if (attrs == ITEM_TYPE_LIGHT) std::cout << "light"; \
    else if (attrs == ITEM_TYPE_SCROLL) std::cout << "scroll"; \
    else if (attrs == ITEM_TYPE_BOOK) std::cout << "book"; \
    else if (attrs == ITEM_TYPE_FLASK) std::cout << "flask"; \
    else if (attrs == ITEM_TYPE_GOLD) std::cout << "gold"; \
    else if (attrs == ITEM_TYPE_AMMUNITION) std::cout << "ammunition"; \
    else if (attrs == ITEM_TYPE_FOOD) std::cout << "food"; \
    else if (attrs == ITEM_TYPE_WAND) std::cout << "wand"; \
    else if (attrs == ITEM_TYPE_CONTAINER) std::cout << "container"; }


int main(int argc, char* argv[]) {
    char *home = getenv("HOME");
    if (home == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "env var 'HOME' is not set");
    std::string path_prefix(home);

    std::vector<monster_definition_t *> monsters;

    std::ifstream monst_file(path_prefix + "/.rlg327/monster_desc.txt");
    if (monst_file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file ~/.rlg327/monster_desc.txt");

    parser_definition_t monst_definitions[] {
        {.name = "NAME", .offset = offsetof(monster_definition_t, name), .type = PARSE_TYPE_STRING, .required = true},
        {.name = "DESC", .offset = offsetof(monster_definition_t, description), .type = PARSE_TYPE_LONG_STRING, .required = true},
        {.name = "COLOR", .offset = offsetof(monster_definition_t, color), .type = PARSE_TYPE_COLOR, .required = true},
        {.name = "SPEED", .offset = offsetof(monster_definition_t, speed), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "ABIL", .offset = offsetof(monster_definition_t, abilities), .type = PARSE_TYPE_MONSTER_ATTRIBUTES, .required = false},
        {.name = "HP", .offset = offsetof(monster_definition_t, hp), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "DAM", .offset = offsetof(monster_definition_t, damage), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "SYMB", .offset = offsetof(monster_definition_t, symbol), .type = PARSE_TYPE_CHAR, .required = true},
        {.name = "RRTY", .offset = offsetof(monster_definition_t, rarity), .type = PARSE_TYPE_INT, .required = true}
    };

    parser_t<monster_definition_t> monst_parser(monst_definitions, sizeof (monst_definitions) / sizeof (monst_definitions[0]),
        "RLG327 MONSTER DESCRIPTION 1", "MONSTER");

    monst_parser.parse(monsters, monst_file);

    std::cout << "========= MONSTERS =========" << std::endl;
    for (monster_definition_t *monst : monsters) {
        std::cout << "name: " << monst->name << std::endl;
        std::cout << "description: " << monst->description << std::endl;
        std::cout << "color: ";
        PRINT_COLORS(monst->color);
        std::cout << std::endl;
        std::cout << "speed: " << *(monst->speed) << std::endl;
        std::cout << "abilities: ";
        PRINT_MONST_ATTRS(monst->abilities);
        std::cout << std::endl;
        std::cout << "hp: " << *(monst->hp) << std::endl;
        std::cout << "damage: " << *(monst->damage) << std::endl;
        std::cout << "symbol: " << monst->symbol << std::endl;
        std::cout << "rarity: " << monst->rarity << std::endl;
        delete monst->speed;
        delete monst->damage;
        delete monst->hp;
        delete monst;
        std::cout << "--------------------" << std::endl;
    }


    std::cout << std::endl << std::endl;
    std::vector<item_definition_t *> items;

    std::ifstream item_file(path_prefix + "/.rlg327/object_desc.txt");
    if (item_file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file ~/.rlg327/object_desc.txt");

    parser_definition_t item_definitions[] {
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
        {.name = "RRTY", .offset = offsetof(item_definition_t, rarity), .type = PARSE_TYPE_INT, .required = true}
    };

    parser_t<item_definition_t> item_parser(item_definitions, sizeof (item_definitions) / sizeof (item_definitions[0]),
        "RLG327 OBJECT DESCRIPTION 1", "OBJECT");

    item_parser.parse(items, item_file);

    std::cout << "========= ITEMS =========" << std::endl;
    for (item_definition_t *item : items) {
        std::cout << "--------------------" << std::endl;
        std::cout << "name: " << item->name << std::endl;
        std::cout << "description: " << item->description << std::endl;
        std::cout << "type: ";
        PRINT_ITEM_TYPE(item->type);
        std::cout << std::endl;
        std::cout << "color: ";
        PRINT_COLORS(item->color);
        std::cout << std::endl;
        std::cout << "hit bonus: " << *(item->hit_bonus) << std::endl;
        std::cout << "damage bonus: " << *(item->damage_bonus) << std::endl;
        std::cout << "dodge bonus: " << *(item->dodge_bonus) << std::endl;
        std::cout << "defense bonus: " << *(item->defense_bonus) << std::endl;
        std::cout << "weight: " << *(item->weight) << std::endl;
        std::cout << "speed bonus: " << *(item->speed_bonus) << std::endl;
        std::cout << "attributes: " << *(item->attributes) << std::endl;
        std::cout << "value: " << *(item->value) << std::endl;
        std::cout << "artifact: " << item->artifact << std::endl;
        std::cout << "rarity: " << item->rarity << std::endl;
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

    return 0;
}
