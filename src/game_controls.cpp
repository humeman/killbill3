#include "game.h"
#include "message_queue.h"

void Game::ctrl_move_n() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, 0, -1);
    } else {
        try_move(0, -1);
        next_turn_ready = true;
    }
}

void Game::ctrl_move_e() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, 1, 0);
    } else {
        try_move(1, 0);
        next_turn_ready = true;
    }
}

void Game::ctrl_move_w() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, -1, 0);
    } else {
        try_move(-1, 0);
        next_turn_ready = true;
    }
}

void Game::ctrl_move_s() {
    if (teleport_mode || look_mode) {
        move_coords(pointer, 0, 1);
    } else {
        try_move(0, 1);
        next_turn_ready = true;
    }
}

void Game::ctrl_quit() {
    game_exit = true;
}

void Game::ctrl_grab_item() {
    if (teleport_mode || look_mode) return;
    Item *target_item = item_map[pc.x][pc.y];
    if (target_item == NULL) {
        MessageQueue::get()->clear();
        MessageQueue::get()->add("&0&bThere's no item here!");
        return;
    }
    if (pc.inventory_size() >= MAX_CARRY_SLOTS) {
        MessageQueue::get()->clear();
        MessageQueue::get()->add("&0&bYour carry slots are full!");
        return;
    }
    if (target_item->is_stacked()) {
        item_map[pc.x][pc.y] = target_item->detach_stack();
    } else {
        item_map[pc.x][pc.y] = NULL;
    }
    pc.add_to_inventory(target_item);
    MessageQueue::get()->add("You picked up &" + std::to_string(
        target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
    next_turn_ready = true;
}

void Game::ctrl_inventory() {
    inventory_menu();
}

void Game::ctrl_cheater() {
    cheater_menu();
}

void Game::ctrl_ptr_confirm() {
    if (teleport_mode) {
        force_move(pointer);
        next_turn_ready = true;
        teleport_mode = false;
    }
    else if (look_mode) {
        ncpp::Plane *plane = planes->get("look");
        NC_HIDE(nc, *plane);
        look_mode = false;
    }
}

void Game::ctrl_esc() {
    if (teleport_mode) {
        teleport_mode = false;
    }
    else if (look_mode) {
        ncpp::Plane *plane = planes->get("look");
        NC_HIDE(nc, *plane);
        look_mode = false;
    }
}

void Game::ctrl_refresh() {
    unsigned int x, y;
    nc->refresh(x, y);
}

void Game::init_controls() {
    controls['w'] = &Game::ctrl_move_n;
    controls['d'] = &Game::ctrl_move_e;
    controls['s'] = &Game::ctrl_move_s;
    controls['a'] = &Game::ctrl_move_w;
    controls['Q'] = &Game::ctrl_quit;
    controls['f'] = &Game::ctrl_grab_item;
    controls['i'] = &Game::ctrl_inventory;
    controls['e'] = &Game::ctrl_inventory;
    controls['`'] = &Game::ctrl_cheater;
    controls[NCKEY_ENTER] = &Game::ctrl_ptr_confirm;
    controls[NCKEY_ESC] = &Game::ctrl_esc;
    controls['r'] = &Game::ctrl_refresh;
}