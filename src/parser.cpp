#include "parser.h"
#include "character.h"
#include "macros.h"
#include "random.h"
#include "item.h"

#include <fstream>
#include <cstring>

void convert(void *item, std::string line, std::ifstream &input, parse_type_t type, int &line_i) {
    switch (type) {
        case PARSE_TYPE_STRING:
            write_to_string(item, line, input);
            break;
        case PARSE_TYPE_LONG_STRING:
            write_to_long_string(item, line, input, line_i);
            break;
        case PARSE_TYPE_INT:
            write_to_int(item, line, input);
            break;
        case PARSE_TYPE_RARITY:
            write_to_int(item, line, input);
            if (*((int *) item) < 1 || *((int *) item) > 100)
                throw dungeon_exception(__PRETTY_FUNCTION__, "rarities must be between 1 and 100");
            break;
        case PARSE_TYPE_DICE:
            write_to_dice(item, line, input);
            break;
        case PARSE_TYPE_CHAR:
            write_to_char(item, line, input);
            break;
        case PARSE_TYPE_MONSTER_ATTRIBUTES:
            write_to_monster_attributes(item, line, input);
            break;
        case PARSE_TYPE_COLOR:
            write_to_color(item, line, input);
            break;
        case PARSE_TYPE_ITEM_TYPE:
            write_to_item_type(item, line, input);
            break;
        case PARSE_TYPE_BOOL:
            write_to_bool(item, line, input);
            break;
    }
}

void write_to_string(void *item, std::string line, std::ifstream &input) {
    if (line.length() > 77)
        throw dungeon_exception(__PRETTY_FUNCTION__, "line is too long (max 77, got " + std::to_string(line.length()) + ")");
    std::string *str = (std::string *) item;
    *str += line;
}

void write_to_long_string(void *item, std::string line, std::ifstream &input, int &line_i) {
    std::string *buf = (std::string *) item;
    if (line.length() != 0) {
        *buf += line;
        *buf += "\n";
    }
    while (true) {
        if (!std::getline(input, line))
            throw dungeon_exception(__PRETTY_FUNCTION__, "unterminated long string in file");
        line_i++;

        if (line.length() > 77)
            throw dungeon_exception(__PRETTY_FUNCTION__, "line is too long (max 77, got " + std::to_string(line.length()) + ")");

        if (line == ".") break;
        *buf += line;
        *buf += "\n";
    }
    // https://stackoverflow.com/a/47702335
    if (!(*buf).empty())
        (*buf).pop_back();
}

void write_to_int(void *item, std::string line, std::ifstream &input) {
    int *num = (int *) item;
    *num = std::stoi(line);
}

void write_to_dice(void *item, std::string line, std::ifstream &input) {
    int base, dice, sides;
    unsigned long i, j;
    i = line.find('+');
    if (i == std::string::npos)
        throw dungeon_exception(__PRETTY_FUNCTION__, "dice definition must be formatted <base>+<dice>d<sides>, got " + line);
    base = std::stoi(line.substr(0, i));
    j = line.find('d', i);
    if (j == std::string::npos)
        throw dungeon_exception(__PRETTY_FUNCTION__, "dice definition must be formatted <base>+<dice>d<sides>, got " + line);
    dice = std::stoi(line.substr(i + 1, j));
    if (j + 1 >= line.length())
        throw dungeon_exception(__PRETTY_FUNCTION__, "dice definition must be formatted <base>+<dice>d<sides>, got " + line);
    sides = std::stoi(line.substr(j + 1, line.length()));

    dice_t *d = new dice_t(base, dice, sides);
    memcpy(item, &d, sizeof (dice_t *));
}

void write_to_char(void *item, std::string line, std::ifstream &input) {
    char *c = (char *) item;
    if (line.length() != 1)
        throw dungeon_exception(__PRETTY_FUNCTION__, "character type must include only one character");
    *c = line[0];
}

void write_to_monster_attributes(void *item, std::string line, std::ifstream &input) {
    int *attr = (int *) item;
    *attr = 0;
    // There can be multiple of these, separated by spaces.
    unsigned long i = 0;
    unsigned long j = 0;
    std::string cur;
    while (i != std::string::npos) {
        j = line.find(' ', i + 1);
        cur = line.substr(i == 0 ? 0 : i + 1, j - i - (i == 0 ? 0 : 1));
        i = j;
        if (cur == "SMART")
            *attr |= MONSTER_ATTRIBUTE_INTELLIGENT;
        else if (cur == "TELE")
            *attr |= MONSTER_ATTRIBUTE_TELEPATHIC;
        else if (cur == "ERRATIC")
            *attr |= MONSTER_ATTRIBUTE_ERRATIC;
        else if (cur == "TUNNEL")
            *attr |= MONSTER_ATTRIBUTE_TUNNELING;
        else if (cur == "PASS")
            *attr |= MONSTER_ATTRIBUTE_GHOST;
        else if (cur == "PICKUP")
            *attr |= MONSTER_ATTRIBUTE_PICKUP;
        else if (cur == "DESTROY")
            *attr |= MONSTER_ATTRIBUTE_DESTROY;
        else if (cur == "UNIQ")
            *attr |= MONSTER_ATTRIBUTE_UNIQUE;
        else if (cur == "BOSS")
            *attr |= MONSTER_ATTRIBUTE_BOSS;
        else
            throw dungeon_exception(__PRETTY_FUNCTION__, "unrecognized monster attribute " + cur);
    }
}

void write_to_color(void *item, std::string line, std::ifstream &input) {
    int *attr = (int *) item;
    *attr = 0;
    // There can be multiple of these, separated by spaces.
    unsigned long i = 0;
    unsigned long j = 0;
    std::string cur;
    while (i != std::string::npos) {
        j = line.find(' ', i + 1);
        cur = line.substr(i == 0 ? 0 : i + 1, j - i - (i == 0 ? 0 : 1));
        i = j;
        if (cur == "WHITE")
            *attr |= FLAG_COLOR_WHITE;
        else if (cur == "RED")
            *attr |= FLAG_COLOR_RED;
        else if (cur == "YELLOW")
            *attr |= FLAG_COLOR_YELLOW;
        else if (cur == "BLUE")
            *attr |= FLAG_COLOR_BLUE;
        else if (cur == "MAGENTA")
            *attr |= FLAG_COLOR_MAGENTA;
        else if (cur == "CYAN")
            *attr |= FLAG_COLOR_CYAN;
        else if (cur == "GREEN")
            *attr |= FLAG_COLOR_GREEN;
        else if (cur == "BLACK")
            *attr |= FLAG_COLOR_WHITE;
        else
            throw dungeon_exception(__PRETTY_FUNCTION__, "unrecognized color " + cur);
    }
}

void write_to_item_type(void *item, std::string line, std::ifstream &input) {
    item_type_t *type = (item_type_t *) item;
    if (line == "WEAPON") *type = ITEM_TYPE_WEAPON;
    else if (line == "OFFHAND") *type = ITEM_TYPE_OFFHAND;
    else if (line == "RANGED") *type = ITEM_TYPE_RANGED;
    else if (line == "ARMOR") *type = ITEM_TYPE_ARMOR;
    else if (line == "HELMET") *type = ITEM_TYPE_HELMET;
    else if (line == "CLOAK") *type = ITEM_TYPE_CLOAK;
    else if (line == "GLOVES") *type = ITEM_TYPE_GLOVES;
    else if (line == "BOOTS") *type = ITEM_TYPE_BOOTS;
    else if (line == "RING") *type = ITEM_TYPE_RING;
    else if (line == "AMULET") *type = ITEM_TYPE_AMULET;
    else if (line == "LIGHT") *type = ITEM_TYPE_LIGHT;
    else if (line == "SCROLL") *type = ITEM_TYPE_SCROLL;
    else if (line == "BOOK") *type = ITEM_TYPE_BOOK;
    else if (line == "FLASK") *type = ITEM_TYPE_FLASK;
    else if (line == "GOLD") *type = ITEM_TYPE_GOLD;
    else if (line == "AMMUNITION") *type = ITEM_TYPE_AMMUNITION;
    else if (line == "FOOD") *type = ITEM_TYPE_FOOD;
    else if (line == "WAND") *type = ITEM_TYPE_WAND;
    else if (line == "CONTAINER") *type = ITEM_TYPE_CONTAINER;
    else throw dungeon_exception(__PRETTY_FUNCTION__, "unrecognized item type " + line);
}

void write_to_bool(void *item, std::string line, std::ifstream &input) {
    bool *b = (bool *) item;
    if (line == "TRUE") *b = true;
    else if (line == "FALSE") *b = false;
    else throw dungeon_exception(__PRETTY_FUNCTION__, "bool must be TRUE or FALSE, not " + line);
}
