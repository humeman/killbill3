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
    if (attrs & MONSTER_ATTRIBUTE_INTELLIGENT) std::cout << "SMART "; \
    if (attrs & MONSTER_ATTRIBUTE_TELEPATHIC) std::cout << "TELE "; \
    if (attrs & MONSTER_ATTRIBUTE_TUNNELING) std::cout << "TUNNEL "; \
    if (attrs & MONSTER_ATTRIBUTE_ERRATIC) std::cout << "ERRATIC "; \
    if (attrs & MONSTER_ATTRIBUTE_GHOST) std::cout << "PASS "; \
    if (attrs & MONSTER_ATTRIBUTE_PICKUP) std::cout << "PICKUP "; \
    if (attrs & MONSTER_ATTRIBUTE_DESTROY) std::cout << "DESTROY "; \
    if (attrs & MONSTER_ATTRIBUTE_UNIQUE) std::cout << "UNIQ "; \
    if (attrs & MONSTER_ATTRIBUTE_BOSS) std::cout << "BOSS "; }

#define PRINT_COLORS(attrs) { \
    if (attrs & FLAG_COLOR_WHITE) std::cout << "WHITE "; \
    if (attrs & FLAG_COLOR_RED) std::cout << "RED "; \
    if (attrs & FLAG_COLOR_GREEN) std::cout << "GREEN "; \
    if (attrs & FLAG_COLOR_YELLOW) std::cout << "YELLOW "; \
    if (attrs & FLAG_COLOR_BLUE) std::cout << "BLUE "; \
    if (attrs & FLAG_COLOR_MAGENTA) std::cout << "MAGENTA "; \
    if (attrs & FLAG_COLOR_CYAN) std::cout << "CYAN "; \
    if (attrs & FLAG_COLOR_BLACK) std::cout << "BLACK "; }

#define PRINT_ItemYPE(attrs) { \
    if (attrs == ITEM_TYPE_WEAPON) std::cout << "WEAPON"; \
    else if (attrs == ITEM_TYPE_OFFHAND) std::cout << "OFFHAND"; \
    else if (attrs == ITEM_TYPE_RANGED) std::cout << "RANGED"; \
    else if (attrs == ITEM_TYPE_ARMOR) std::cout << "ARMOR"; \
    else if (attrs == ITEM_TYPE_HELMET) std::cout << "HELMET"; \
    else if (attrs == ITEM_TYPE_CLOAK) std::cout << "CLOAK"; \
    else if (attrs == ITEM_TYPE_GLOVES) std::cout << "GLOVES"; \
    else if (attrs == ITEM_TYPE_BOOTS) std::cout << "BOOTS"; \
    else if (attrs == ITEM_TYPE_RING) std::cout << "RING"; \
    else if (attrs == ITEM_TYPE_AMULET) std::cout << "AMULET"; \
    else if (attrs == ITEM_TYPE_LIGHT) std::cout << "LIGHT"; \
    else if (attrs == ITEM_TYPE_SCROLL) std::cout << "SCROLL"; \
    else if (attrs == ITEM_TYPE_BOOK) std::cout << "BOOK"; \
    else if (attrs == ITEM_TYPE_FLASK) std::cout << "FLASK"; \
    else if (attrs == ITEM_TYPE_GOLD) std::cout << "GOLD"; \
    else if (attrs == ITEM_TYPE_AMMUNITION) std::cout << "AMMUNITION"; \
    else if (attrs == ITEM_TYPE_FOOD) std::cout << "FOOD"; \
    else if (attrs == ITEM_TYPE_WAND) std::cout << "WAND"; \
    else if (attrs == ITEM_TYPE_CONTAINER) std::cout << "CONTAINER"; }


int main(int argc, char* argv[]) {
    char *home = getenv("HOME");
    if (home == NULL) throw dungeon_exception(__PRETTY_FUNCTION__, "env var 'HOME' is not set");
    std::string path_prefix(home);

    std::vector<MonsterDefinition *> monsters;

    std::ifstream monst_file(path_prefix + "/.rlg327/monster_desc.txt");
    if (monst_file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file ~/.rlg327/monster_desc.txt");

    parser_definition_t monst_definitions[] {
        {.name = "NAME", .offset = offsetof(MonsterDefinition, name), .type = PARSE_TYPE_STRING, .required = true},
        {.name = "DESC", .offset = offsetof(MonsterDefinition, description), .type = PARSE_TYPE_LONG_STRING, .required = true},
        {.name = "COLOR", .offset = offsetof(MonsterDefinition, color), .type = PARSE_TYPE_COLOR, .required = true},
        {.name = "SPEED", .offset = offsetof(MonsterDefinition, speed), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "ABIL", .offset = offsetof(MonsterDefinition, abilities), .type = PARSE_TYPE_MONSTER_ATTRIBUTES, .required = true},
        {.name = "HP", .offset = offsetof(MonsterDefinition, hp), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "DAM", .offset = offsetof(MonsterDefinition, damage), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "SYMB", .offset = offsetof(MonsterDefinition, symbol), .type = PARSE_TYPE_CHAR, .required = true},
        {.name = "RRTY", .offset = offsetof(MonsterDefinition, rarity), .type = PARSE_TYPE_RARITY, .required = true}
    };

    Parser<MonsterDefinition> monst_parser(monst_definitions, sizeof (monst_definitions) / sizeof (monst_definitions[0]),
        "RLG327 MONSTER DESCRIPTION 1", "MONSTER", true);

    monst_parser.parse(monsters, monst_file);

    std::cout << "========= MONSTERS =========" << std::endl;
    for (MonsterDefinition *monst : monsters) {
        std::cout << monst->name << std::endl;
        std::cout << monst->description << std::endl;
        PRINT_COLORS(monst->color);
        std::cout << std::endl;
        std::cout << *(monst->speed) << std::endl;
        PRINT_MONST_ATTRS(monst->abilities);
        std::cout << std::endl;
        std::cout << *(monst->hp) << std::endl;
        std::cout << *(monst->damage) << std::endl;
        std::cout << monst->symbol << std::endl;
        std::cout << monst->rarity << std::endl;
        delete monst->speed;
        delete monst->damage;
        delete monst->hp;
        delete monst;
        std::cout << std::endl;
    }


    std::cout << std::endl;
    std::vector<ItemDefinition *> items;

    std::ifstream item_file(path_prefix + "/.rlg327/object_desc.txt");
    if (item_file.fail())
        throw dungeon_exception(__PRETTY_FUNCTION__, "failed to open file ~/.rlg327/object_desc.txt");

    parser_definition_t item_definitions[] {
        {.name = "NAME", .offset = offsetof(ItemDefinition, name), .type = PARSE_TYPE_STRING, .required = true},
        {.name = "DESC", .offset = offsetof(ItemDefinition, description), .type = PARSE_TYPE_LONG_STRING, .required = true},
        {.name = "TYPE", .offset = offsetof(ItemDefinition, type), .type = PARSE_TYPE_ItemYPE, .required = true},
        {.name = "COLOR", .offset = offsetof(ItemDefinition, color), .type = PARSE_TYPE_COLOR, .required = true},
        {.name = "HIT", .offset = offsetof(ItemDefinition, hit_bonus), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "DAM", .offset = offsetof(ItemDefinition, damage_bonus), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "DODGE", .offset = offsetof(ItemDefinition, dodge_bonus), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "DEF", .offset = offsetof(ItemDefinition, defense_bonus), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "WEIGHT", .offset = offsetof(ItemDefinition, weight), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "SPEED", .offset = offsetof(ItemDefinition, speed_bonus), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "ATTR", .offset = offsetof(ItemDefinition, attributes), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "VAL", .offset = offsetof(ItemDefinition, value), .type = PARSE_TYPE_DICE, .required = true},
        {.name = "ART", .offset = offsetof(ItemDefinition, artifact), .type = PARSE_TYPE_BOOL, .required = true},
        {.name = "RRTY", .offset = offsetof(ItemDefinition, rarity), .type = PARSE_TYPE_RARITY, .required = true}
    };

    Parser<ItemDefinition> item_parser(item_definitions, sizeof (item_definitions) / sizeof (item_definitions[0]),
        "RLG327 OBJECT DESCRIPTION 1", "OBJECT", true);

    item_parser.parse(items, item_file);

    std::cout << "========= ITEMS =========" << std::endl;
    for (ItemDefinition *item : items) {
        std::cout << item->name << std::endl;
        std::cout << item->description << std::endl;
        PRINT_ItemYPE(item->type);
        std::cout << std::endl;
        PRINT_COLORS(item->color);
        std::cout << std::endl;
        std::cout << *(item->hit_bonus) << std::endl;
        std::cout << *(item->damage_bonus) << std::endl;
        std::cout << *(item->dodge_bonus) << std::endl;
        std::cout << *(item->defense_bonus) << std::endl;
        std::cout << *(item->weight) << std::endl;
        std::cout << *(item->speed_bonus) << std::endl;
        std::cout << *(item->attributes) << std::endl;
        std::cout << *(item->value) << std::endl;
        std::cout << (item->artifact ? "true" : "false") << std::endl;
        std::cout << item->rarity << std::endl << std::endl;
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
