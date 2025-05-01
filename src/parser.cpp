#include "parser.h"
#include "character.h"
#include "macros.h"
#include "random.h"
#include "item.h"

#include <fstream>
#include <cstring>
#include <sstream>

void convert(void *item, std::string line, std::ifstream &input, parse_type_t type, int &line_i) {
    if (type != PARSE_TYPE_LONG_STRING)
        trim(line);

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
        case PARSE_TYPE_ItemYPE:
            write_to_Itemype(item, line, input);
            break;
        case PARSE_TYPE_BOOL:
            write_to_bool(item, line, input);
            break;
        case PARSE_TYPE_TUPLE:
            write_to_tuple(item, line, input);
            break;
        case PARSE_TYPE_VECTOR_STRINGS:
            write_to_vector_strings(item, line, input, line_i);
            break;
    }
}

void write_to_string(void *item, std::string line, std::ifstream &input) {
    if (line.length() > 60)
        throw dungeon_exception(__PRETTY_FUNCTION__, "line is too long (max 60, got " + std::to_string(line.length()) + ")");
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

    Dice *d = new Dice(base, dice, sides);
    memcpy(item, &d, sizeof (Dice *));
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
    bool set = false;
    while (i != std::string::npos) {
        // Find the beginning of this word
        while (i < line.length() && (line[i] == ' ' || line[i] == '\t')) i++;
        // And the end
        j = i;
        while (j < line.length() && line[j] != ' ' && line[j] != '\t') j++;
        if (i == j) {
            if (!set) throw dungeon_exception(__PRETTY_FUNCTION__, "no colors provided");
            break;
        }
        cur = line.substr(i, j - i);
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
        set = true;
    }
}

void write_to_color(void *item, std::string line, std::ifstream &input) {
    int *attr = (int *) item;
    *attr = 0;
    // There can be multiple of these, separated by spaces.
    unsigned long i = 0;
    unsigned long j = 0;
    std::string cur;
    bool set = false;
    while (i != std::string::npos) {
        // Find the beginning of this word
        while (i < line.length() && (line[i] == ' ' || line[i] == '\t')) i++;
        // And the end
        j = i;
        while (j < line.length() && line[j] != ' ' && line[j] != '\t') j++;
        if (i == j) {
            if (!set) throw dungeon_exception(__PRETTY_FUNCTION__, "no attributes provided");
            break;
        }
        cur = line.substr(i, j - i);
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
            *attr |= FLAG_COLOR_BLACK;
        else
            throw dungeon_exception(__PRETTY_FUNCTION__, "unrecognized color " + cur);
        set = true;
    }
}

void write_to_Itemype(void *item, std::string line, std::ifstream &input) {
    item_type_t *type = (item_type_t *) item;
    if (line == "WEAPON") *type = ITEM_TYPE_WEAPON;
    else if (line == "HAT") *type = ITEM_TYPE_HAT;
    else if (line == "SHIRT") *type = ITEM_TYPE_SHIRT;
    else if (line == "PANTS") *type = ITEM_TYPE_PANTS;
    else if (line == "SHOES") *type = ITEM_TYPE_SHOES;
    else if (line == "GLASSES") *type = ITEM_TYPE_GLASSES;
    else if (line == "POCKET") *type = ITEM_TYPE_POCKET;
    else throw dungeon_exception(__PRETTY_FUNCTION__, "unrecognized item type " + line);
}

void write_to_bool(void *item, std::string line, std::ifstream &input) {
    bool *b = (bool *) item;
    if (line == "TRUE") *b = true;
    else if (line == "FALSE") *b = false;
    else throw dungeon_exception(__PRETTY_FUNCTION__, "bool must be TRUE or FALSE, not " + line);
}

void trim(std::string &string) {
    unsigned long i;
    if (string.length() == 0) return;
    for (i = 0; i < string.length() && (string[i] == ' ' || string[i] == '\t'); i++);
    if (string.length() == 0) return;
    string.erase(0, i);
    for (i = string.length() - 1; i >= 0 && (string[i] == ' ' || string[i] == '\t'); i--);
    string.erase(i + 1);
}

void write_to_vector_strings(void *item, std::string line, std::ifstream &input, int &line_i) {
    std::string res;
    write_to_long_string(&res, line, input, line_i);
    std::vector<std::string> *vec = (std::vector<std::string> *) item;
    std::string temp;
    std::istringstream stream(res);
    while (std::getline(stream, temp)) {
        vec->push_back(temp);
    }
}

void write_to_tuple(void *item, std::string line, std::ifstream &input) {
    unsigned int x, y;
    int i = line.find(' ');
    if (i == std::string::npos)
        throw dungeon_exception(__PRETTY_FUNCTION__, "coordinate pair definition must be formatted <x> <y>, got " + line);
    x = std::stoi(line.substr(0, i));
    y = std::stoi(line.substr(i + 1, line.length()));

    IntPair *d = (IntPair *) item;
    d->x = x;
    d->y = y;
}