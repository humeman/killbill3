#ifndef DECORATIONS_H
#define DECORATIONS_H

#include "dungeon.h"

typedef enum {
    DECORATION_SCHEME_LOBBY,
    DECORATION_SCHEME_SERVER_ROOM,
    DECORATION_SCHEME_CONFERENCE_ROOM,
    DECORATION_SCHEME_MONEY_ROOM,
    DECORATION_SCHEME_COMPUTER_ROOM
} decoration_scheme_t;


bool apply_scheme(decoration_scheme_t scheme, Dungeon *dungeon, Room *room);
decoration_scheme_t parse_scheme(std::string &name);

#endif