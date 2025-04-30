#include "game.h"
#include "message_queue.h"

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

void game_t::ctrl_grab_item() {
    if (teleport_mode || look_mode) return;
    item_t *target_item = item_map[pc.x][pc.y];
    if (target_item == NULL) {
        message_queue_t::get()->clear();
        message_queue_t::get()->add("&0&bThere's no item here!");
        return;
    }
    if (pc.inventory_size() >= MAX_CARRY_SLOTS) {
        message_queue_t::get()->clear();
        message_queue_t::get()->add("&0&bYour carry slots are full!");
        return;
    }
    if (target_item->is_stacked()) {
        item_map[pc.x][pc.y] = target_item->detach_stack();
    } else {
        item_map[pc.x][pc.y] = NULL;
    }
    pc.add_to_inventory(target_item);
    message_queue_t::get()->add("You picked up &" + std::to_string(
        target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
    next_turn_ready = true;
}

void game_t::ctrl_inventory() {
    inventory_menu();
}

void game_t::ctrl_cheater() {
    cheater_menu();
}

void game_t::ctrl_ptr_confirm() {
    if (teleport_mode) {
        force_move(pointer);
        next_turn_ready = true;
        teleport_mode = false;
    }
    else if (look_mode) {
        look_mode = false;
    }
}

void game_t::ctrl_esc() {
    if (teleport_mode) {
        teleport_mode = false;
    }
    else if (look_mode) {
        planes.get("look")->move_bottom();
        look_mode = false;
    }
}

void game_t::init_controls() {
    controls['w'] = &game_t::ctrl_move_n;
    controls['d'] = &game_t::ctrl_move_e;
    controls['s'] = &game_t::ctrl_move_s;
    controls['a'] = &game_t::ctrl_move_w;
    controls['Q'] = &game_t::ctrl_quit;
    controls['f'] = &game_t::ctrl_grab_item;
    controls['i'] = &game_t::ctrl_inventory;
    controls['e'] = &game_t::ctrl_inventory;
    controls['`'] = &game_t::ctrl_cheater;
    controls[NCKEY_ENTER] = &game_t::ctrl_ptr_confirm;
    controls[NCKEY_ESC] = &game_t::ctrl_esc;
}