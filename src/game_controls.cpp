#include "game.h"

void game_t::ctrl_move_n() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, 0, -1);
    } else {
        try_move(0, -1);
        next_turn_ready = true;
    }
}

void game_t::ctrl_move_e() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, 1, 0);
    } else {
        try_move(1, 0);
        next_turn_ready = true;
    }
}

void game_t::ctrl_move_w() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, -1, 0);
    } else {
        try_move(-1, 0);
        next_turn_ready = true;
    }
}

void game_t::ctrl_move_s() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, 0, 1);
    } else {
        try_move(0, 1);
        next_turn_ready = true;
    }
}

void game_t::ctrl_quit() {
    game_exit = true;
}

void game_t::init_controls() {
    controls['w'] = &game_t::ctrl_move_n;
    controls['d'] = &game_t::ctrl_move_e;
    controls['s'] = &game_t::ctrl_move_s;
    controls['a'] = &game_t::ctrl_move_w;
    controls['Q'] = &game_t::ctrl_quit;
}