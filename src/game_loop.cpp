#include <ncpp/NotCurses.hh>
#include <cstring>
#include <cstdlib>

#include "game.h"
#include "item.h"
#include "macros.h"
#include "dungeon.h"
#include "heap.h"
#include "character.h"
#include "ascii.h"
#include "pathfinding.h"
#include "message_queue.h"
#include "resource_manager.h"
#include "logger.h"
#include <bits/this_thread_sleep.h>

const std::string CELL_TYPES_TO_FLOOR_TEXTURES[] = {
    "floor_stone",
    "floor_room",
    "", // overridden (decoration)
    "floor_hall",
    "floor_up_staircase",
    "floor_down_staircase",
    "floor_stone",
    "floor_stone",
    "floor_stone"
};

const std::string WALL_TYPES_TO_FLOOR_TEXTURES[] = {
    "floor_wall_t",
    "floor_wall_l",
    "floor_wall_r",
    "floor_wall_b",
    "floor_wall_tl",
    "floor_wall_tr",
    "floor_wall_bl",
    "floor_wall_br",
    "floor_wall_endl",
    "floor_wall_endr",
    "floor_wall_endt",
    "floor_wall_endb",
    "floor_wall_t_l_l",
    "floor_wall_t_l_r",
    "floor_wall_t_b_t",
    "floor_wall_t_b_b",
    "floor_wall_single",
    "floor_wall_quad"
};

void Game::create_nc() {
    Logger::get()->off();
    nc = new ncpp::NotCurses();
    planes = new PlaneManager(nc);
}
void Game::end_nc() {
    if (nc) {
        nc->stop();
        delete nc;
        nc = nullptr;
    }
    Logger::get()->on();
}

void Game::run_game() {
    ncinput inp;
    MessageQueue::get()->add("--- &0&bKILL BILL 3&r ---");
    unsigned int x, i, io, ii;
    timespec ts;
    Monster *monst;
    render_frame(true);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += rand() % 3 + 1;
    while (true) {
        render_frame(false);
        while (!nc->get(&ts, &inp)) {
            // Pick a random monster to play some ambiance for :)
            io = rand();
            for (ii = 0; ii < (unsigned int) turn_queue.size(); ii++) {
                i = (io + ii) % turn_queue.size();
                if (turn_queue.at(i)->type() == CHARACTER_TYPE_MONSTER) {
                    monst = (Monster *) turn_queue.at(i);
                    if (monst->definition->ambiance.length() > 0) {
                        ResourceManager::get()->play_music(monst->definition->ambiance);
                        break;
                    }
                }
            }
            clock_gettime(CLOCK_MONOTONIC, &ts);
            ts.tv_sec += rand() % 7 + 2;
        }

        if (result != GAME_RESULT_RUNNING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            // Print our beautiful images
            ncpp::Plane *bsod_plane = planes->get("bsod", 0, 0, term_x, term_y);
            std::string frown, qr, stop;
            unsigned long frown_size = term_y / 4;
            unsigned long qr_size = MAX(5, term_y / 6);
            unsigned long y_start = (term_y - (2 * frown_size + 7)) / 2;
            bsod_plane->move_top();
            if (result == GAME_RESULT_LOSE) {
                NC_APPLY_COLOR((*bsod_plane), RGB_COLOR_WHITE, RGB_COLOR_BSOD);
                bsod_plane->set_fg_rgb(RGB_COLOR_WHITE);
                NC_CLEAR(*bsod_plane);
                bsod_plane->printf(y_start + frown_size + 1, 5, "Your PC ran into a problem and needs to restart. We're");
                bsod_plane->printf(y_start + frown_size + 2, 5, "just collecting some error info, and then we'll restart for");
                bsod_plane->printf(y_start + frown_size + 3, 5, "you.");
                frown = "ui_bsod_frown";
                qr = "ui_qr_bsod";
                stop = "YUO_LOOSED";
            }
            else {
                NC_APPLY_COLOR((*bsod_plane), RGB_COLOR_WHITE, RGB_COLOR_GSOD)
                NC_CLEAR(*bsod_plane);
                bsod_plane->set_fg_rgb(RGB_COLOR_WHITE);
                bsod_plane->printf(y_start + frown_size + 1, 5, "Your PC didn't run into a problem and doesn't need to");
                bsod_plane->printf(y_start + frown_size + 2, 5, "restart. We aren't collecting any error info, but you'll");
                bsod_plane->printf(y_start + frown_size + 3, 5, "have to sit through this loading bar anyway.");
                frown = "ui_bsod_smile";
                qr = "ui_qr_gsod";
                stop = "BILL_DIED";
            }
            ncpp::Plane *frown_plane = planes->get("bsod_frown", 5, y_start, 3 * frown_size / 4, frown_size);
            frown_plane->move_top();
            NC_DRAW(frown_plane, frown);
            ncpp::Plane *qr_plane = planes->get("qr_code", 5, y_start + frown_size + 8, qr_size * 2, qr_size);
            qr_plane->move_top();
            NC_DRAW(qr_plane, qr);
            bsod_plane->printf(y_start + frown_size + 8, 6 + qr_size * 2, "For more information about this issue and");
            bsod_plane->printf(y_start + frown_size + 9, 6 + qr_size * 2, "possible fixes, visit https://tecktip.today.");
            bsod_plane->printf(y_start + frown_size + 8 + qr_size - 2, 6 + qr_size * 2, "If you call a support person, give them this info:");
            bsod_plane->printf(y_start + frown_size + 8 + qr_size - 1, 6 + qr_size * 2, "%s", stop.c_str());
            x = 0;
            while (x <= 100) {
                x += rand() % 31;
                bsod_plane->printf(y_start + frown_size + 5, 5, "%d%% complete", x > 100 ? 100 : x);
                nc->render();
                std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 1501));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            return;
        }

        next_turn_ready = false;
        if (controls.count(inp.id) != 0) {
            auto ctrl = controls.at(inp.id);
            (this->*ctrl)();
        }
        if (game_exit) return;

        if (result == GAME_RESULT_RUNNING && next_turn_ready) {
            update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, IntPair{pc.x, pc.y});
            // Run the game until the PC's turn comes up again (or it dies)
            run_until_pc();
        }
        if (result != GAME_RESULT_RUNNING) {
            MessageQueue::get()->clear();
            MessageQueue::get()->add("&bGame over. Press any key to continue.");
        }
    }
    ncinput ni;
    nc->get(true, &ni);
}

void Game::run_until_pc() {
    Character *ch = NULL;
    Monster *monster;
    uint32_t priority;

    while (true) {
        ch = NULL;
        while (turn_queue.size() > 0 && ch == NULL) {
            priority = turn_queue.top_priority();
            ch = turn_queue.remove();
            // The dead flag here avoids us having to remove from the heap at an arbitrary location.
            // We just destroy it when it comes off the queue next.
            if (ch->dead) {
                if (ch != &pc) {
                    destroy_character(character_map, ch);
                    ch = NULL;
                }
            }
        }

        if (ch == NULL)
            throw dungeon_exception(__PRETTY_FUNCTION__, "turn queue is empty");

        // If this was the PC's turn, signal that back to the caller
        if (ch == &pc) {
            turn_queue.insert(&pc, priority + (1000 / pc.speed_bonus()));
            result = GAME_RESULT_RUNNING;
            return;
        }

        monster = (Monster *) ch;
        result = GAME_RESULT_RUNNING;
        monster->take_turn(dungeon, &pc, turn_queue, character_map, item_map, pathfinding_tunnel, pathfinding_no_tunnel, priority, result);
        if (antidmg) {
            pc.hp = pc.base_hp;
            pc.dead = false;
        }
        if (pc.dead) result = GAME_RESULT_LOSE;
        return;
    }
}

void Game::render_frame(bool complete_redraw) {
    // Status message.
    ncpp::Plane *top_plane = planes->get("top");
    std::vector<std::string> dupes;
    top_plane->erase();
    if (teleport_mode) {
        if (MessageQueue::get()->empty())
            MessageQueue::get()->add("-- &4&bTELEPORT MODE&r --");
    }
    else if (look_mode) {
        if (MessageQueue::get()->empty())
            MessageQueue::get()->add("-- &3&bLOOK MODE&r --");
    }
    MessageQueue::get()->emit(*top_plane, false);

    // Dungeon.
    // Complete redraw means empty out all the cells.
    // That should really only ever happen on the first frame, since it removes
    // the benefit of any cell caching.
    int x, y;
    ncpp::Plane *plane;
    if (complete_redraw) {
        Logger::debug(__FILE__, "running complete redraw");
        for (x = 0; x < (int) cells_x; x++) {
            for (y = 0; y < (int) cells_y; y++) {
                plane = planes->get(CELL_NAME(x, y));
                NC_DRAW(plane, CELL_TYPES_TO_FLOOR_TEXTURES[CELL_TYPE_EMPTY]);
            }
        }
    }


    // We're centering the camera around the PC.
    int dungeon_x0 = pc.x - cells_x / 2;
    int dungeon_y0 = pc.y - cells_y / 2;

    if (teleport_mode || look_mode) {
        dungeon_x0 = pointer.x - cells_x / 2;
        dungeon_y0 = pointer.y - cells_y / 2;
    }
    
    // Unsurprisingly, drawing entire pictures to the terminal is really slow.
    // To mitigate this, we'll only draw any changed cells from frame-to-frame.
    // Easiest method here is to keep track of the texture name.
    int cell_x, cell_y;
    std::string new_texture;
    Monster *monst;
    std::string cell_name;
    for (x = dungeon_x0; x < (int) (dungeon_x0 + cells_x); x++) {
        for (y = dungeon_y0; y < (int) (dungeon_y0 + cells_y); y++) {
            new_texture = "";
            if (x < 0 || x >= dungeon->width || y < 0 || y >= dungeon->height) {
                new_texture = CELL_TYPES_TO_FLOOR_TEXTURES[CELL_TYPE_STONE];
            }
            else {
                // Find out what we're drawing here.
                // Only draw what the PC can see.
                if ((teleport_mode || look_mode) && pointer.x == x && pointer.y == y) {
                    new_texture = "characters_pointer";
                }
                // else if (!teleport_mode && !look_mode && !seethrough && !pc.has_los(dungeon, (IntPair) {(unsigned char) x, (unsigned char) y})) {
                //     new_texture = CELL_TYPES_TO_FLOOR_TEXTURES[CELL_TYPE_STONE];
                // }
                // Characters get first priority.
                else if (character_map[x][y]) {
                    if (character_map[x][y]->type() == CHARACTER_TYPE_PC) {
                        new_texture = std::string(PCEXTURE) + "_" + "nesw"[pc.direction];
                    }
                    else if (character_map[x][y]->type() == CHARACTER_TYPE_MONSTER) {
                        monst = (Monster *) character_map[x][y];
                        switch (monst->direction) {
                            case DIRECTION_NORTH:
                                new_texture = monst->definition->floor_texture_n;
                                break;
                            case DIRECTION_EAST:
                                new_texture = monst->definition->floor_texture_e;
                                break;
                            case DIRECTION_SOUTH:
                                new_texture = monst->definition->floor_texture_s;
                                break;
                            case DIRECTION_WEST:
                                new_texture = monst->definition->floor_texture_w;
                                break;
                        }
                    }
                }
                // Then items.
                else if (item_map[x][y]) {
                    new_texture = item_map[x][y]->definition->floor_texture;
                }
                // Then regular cells.
                else {
                    if (dungeon->cells[x][y].wall_type != WALL_TYPE_NONE) {
                        new_texture = WALL_TYPES_TO_FLOOR_TEXTURES[dungeon->cells[x][y].wall_type];
                    } else if (dungeon->cells[x][y].type == CELL_TYPE_DECORATION) {
                        new_texture = *dungeon->cells[x][y].decoration_texture;  
                    } else {
                        new_texture = CELL_TYPES_TO_FLOOR_TEXTURES[dungeon->cells[x][y].type];
                    }
                }
            }
            
            cell_x = x - dungeon_x0;
            cell_y = y - dungeon_y0;
            cell_name = CELL_NAME(cell_x, cell_y);
            plane = planes->get(cell_name);
            if (planes->cache_set(cell_name, new_texture) || complete_redraw) {
                if (new_texture.length() == 0) new_texture = CELL_TYPES_TO_FLOOR_TEXTURES[CELL_TYPE_EMPTY];
                NC_DRAW(plane, new_texture);
            }
        }
    }

    // The healthbar.
    ncpp::Plane *bottom_plane = planes->get("bottom");
    bottom_plane->erase();
    NC_RESET(*bottom_plane);
    // Find the percentage of health left
    float hp = (float) pc.hp / pc.base_hp;
    // We'll have 15 hearts rendered
    int hearts_filled = (int) ((hp < 0 ? 0 : hp) * HEARTS);
    if (hearts_filled == 0 && pc.hp > 0) hearts_filled = 1;
    int i;
    std::string plane_name;
    for (i = 0; i < hearts_filled; i++) {
        plane_name = HEART_NAME(i);
        if (planes->cache_set(plane_name, "ui_heart")) {
            plane = planes->get(plane_name);
            NC_DRAW(plane, "ui_heart");
        }
    }
    for (i = hearts_filled; i < HEARTS; i++) {
        plane_name = HEART_NAME(i);
        if (planes->cache_set(plane_name, "ui_heart_dead")) {
            plane = planes->get(plane_name);
            NC_DRAW(plane, "ui_heart_dead");
        }
    }

    // The inventory.
    std::string item_texture;
    NC_APPLY_COLOR(*bottom_plane, RGB_COLOR_WHITE_DIM, RGB_COLOR_BLACK);
    for (i = 0; i < MAX_CARRY_SLOTS; i++) {
        plane_name = ITEM_NAME(i);
        plane = planes->get(plane_name);
        item_texture = i < pc.inventory_size() ? pc.inventory_at(i)->definition->ui_texture : "items_empty";

        if (planes->cache_set(plane_name, item_texture)) {
            NC_DRAW(plane, item_texture);
        }
        bottom_plane->putc(2, plane->get_x() - 1, i == 9 ? '0' : '1' + i);
    }
    for (i = 0; i < ARRAY_SIZE(pc.equipment); i++) {
        plane_name = ITEM_NAME(i + MAX_CARRY_SLOTS);
        plane = planes->get(plane_name);
        item_texture = pc.equipment[i] ? pc.equipment[i]->definition->ui_texture : "items_empty";

        if (planes->cache_set(plane_name, item_texture)) {
            NC_DRAW(plane, item_texture);
        }
        bottom_plane->putc(2, plane->get_x() - 1, "asdfghjk"[i]);
    }

    if (look_mode) {
        plane = planes->get("look");
        if (character_map[pointer.x][pointer.y] && character_map[pointer.x][pointer.y]->type() == CHARACTER_TYPE_MONSTER) {
            plane->move_top();
            render_monster_details(plane, (Monster *) character_map[pointer.x][pointer.y], 0, 0, DETAILS_WIDTH, DETAILS_HEIGHT);
        }
        else if (item_map[pointer.x][pointer.y]) {
            plane->move_top();
            render_inventory_details(plane, item_map[pointer.x][pointer.y], 0, 0, DETAILS_WIDTH, DETAILS_HEIGHT);
        } else {
            NC_HIDE(nc, *plane);
        }
    }

    nc->render();
}

void Game::render_inventory_box(std::string title, std::string labels, std::string input_tip, unsigned int x0, unsigned int y0) {
    int height = labels.length() + 2;
    unsigned int i, x, y;
    ncpp::Plane *plane = planes->get("inventory");
    // Clear out the area.
    NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
    for (y = y0; y < y0 + labels.length() + 2; y++) {
        for (x = x0; x < x0 + INVENTORY_BOX_WIDTH; x++) {
            plane->putc(y, x, ' ');
        }
    }
    // Title...
    plane->styles_set(ncpp::CellStyle::Bold);
    NC_PRINT_CENTERED_AT(plane, x0 + INVENTORY_BOX_WIDTH / 2, y0, "%s", title.c_str());
    plane->styles_off(ncpp::CellStyle::Bold);
    // Input tip...
    NC_PRINT_CENTERED_AT(plane, x0 + INVENTORY_BOX_WIDTH / 2, y0 + height - 1, "%s", input_tip.c_str());
    // And the labels.
    for (i = 0; i < labels.length(); i++)
    plane->printf(y0 + 1 + i, x0, "%c", labels[i]);
}

void Game::render_inventory_item(Item *item, int i, bool selected, unsigned int x0, unsigned int y0) {
    ncpp::Plane *plane = planes->get("inventory");
    // one of the most incredible macro expansions the world will ever see
    NC_APPLY_COLOR_BY_NUM(*plane, item ? item->current_color() : RGB_COLOR_BLACK, (selected ? RGB_COLOR_BLACK : RGB_COLOR_WHITE));
    std::string plane_name = INVENTORY_NAME(x0, y0 + i);
    planes->release(plane_name); // I have no clue why I have to do this, but rendering completely craps out if we re-use these planes.
    if (item) {
        Logger::debug(__FILE__, "plane: " + plane_name);
        ncpp::Plane *item_plane = planes->get(plane_name, plane, x0 + 2, y0 + 1 + i, 2, 1);
        item_plane->move_top();
        NC_APPLY_COLOR(*item_plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
        item_plane->printf(0, 0, "  ");
        nc->render();
        NC_DRAW(item_plane, item->definition->ui_texture);
        nc->render();
        plane->printf(y0 + 1 + i, x0 + 5, "%s", item->definition->name.c_str());
    }
}

void Game::render_inventory_details(ncpp::Plane *plane, Item *item, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height) {
    unsigned int i, x, y;
    // Draw out a box, taking up the whole width of the screen at y0.
    // This determines how many lines of description we get until it cuts off.
    NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
    for (y = y0; y < y0 + height; y++) {
        for (x = x0; x < x0 + width; x++) {
            plane->putc(y, x, ' ');
        }
    }

    if (!item) return;

    plane->styles_set(ncpp::CellStyle::Bold);
    NC_PRINT_CENTERED_AT(plane, x0 + width / 2, y0, "%s", item->definition->name.c_str());
    plane->styles_off(ncpp::CellStyle::Bold);
    // Just printing out the whole string clears out the color on the remainder of the line when there's
    // a newline character. It also doesn't let us protect against overflow, so unfortunately, this'll be
    // by hand.
    x = 0;
    y = y0 + 1;
    std::string &desc = item->definition->description;
    for (i = 0; y < y0 + height - 2 && desc[i]; i++) {
        if (desc[i] == '\n') {
            x = 0;
            y++;
            continue;
        }
        plane->putc(y, x++, desc[i]);
    }
    // The last line will be the dice.
    NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK_DIM, RGB_COLOR_WHITE);
    std::string res = "";
    Dice *c;
    if (item->definition->artifact) res += "*ARTIFACT* ";
    c = item->definition->damage_bonus;
    if (c->base != 0 || c->dice != 0) res += "dmg: " + c->str() + " "; 
    if (item->defense_bonus != 0) res += "def: " + std::to_string(item->defense_bonus) + " "; 
    if (item->dodge_bonus != 0) res += "dodge: " + std::to_string(item->dodge_bonus) + " "; 
    if (item->speed_bonus != 0) res += "speed: " + std::to_string(item->speed_bonus) + " "; 
    if (res[res.length() - 1] == ' ') res = res.substr(0, res.length() - 1);
    NC_PRINT_CENTERED_AT(plane, x0 + width / 2, y0 + height - 1, "%s", res.c_str());
}

void Game::render_monster_details(ncpp::Plane *plane, Monster *monst, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height) {
    unsigned int i, x, y;
    std::string desc;
    // Draw out a box, taking up the whole width of the screen at y0.
    // This determines how many lines of description we get until it cuts off.
    NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
    for (y = y0; y < y0 + height; y++) {
        for (x = x0; x < x0 + width; x++) {
            plane->putc(y, x, ' ');
        }
    }

    if (!monst) return;

    plane->styles_set(ncpp::CellStyle::Bold);
    NC_PRINT_CENTERED_AT(plane, x0 + width / 2, y0, "%s", monst->definition->name.c_str());
    plane->styles_off(ncpp::CellStyle::Bold);
    // Just printing out the whole string clears out the color on the remainder of the line when there's
    // a newline character. It also doesn't let us protect against overflow, so unfortunately, this'll be
    // by hand.
    x = 0;
    y = y0 + 1;
    desc = monst->definition->description;
    for (i = 0; y < height - 2 && desc[i]; i++) {
        if (desc[i] == '\n') {
            x = 0;
            y++;
            continue;
        }
        plane->putc(y, x++, desc[i]);
    }
    // The last line will be the dice.
    NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK_DIM, RGB_COLOR_WHITE);
    std::string res = "";
    if (monst->definition->abilities & MONSTER_ATTRIBUTE_BOSS) res += "*BOSS* ";
    if (monst->definition->abilities & MONSTER_ATTRIBUTE_INTELLIGENT) res += "smart ";
    if (monst->definition->abilities & MONSTER_ATTRIBUTE_GHOST) res += "ghost ";
    if (monst->definition->abilities & MONSTER_ATTRIBUTE_ERRATIC) res += "erratic ";
    if (monst->definition->abilities & MONSTER_ATTRIBUTE_TELEPATHIC) res += "telepathic ";
    if (monst->definition->abilities & MONSTER_ATTRIBUTE_TUNNELING) res += "tunneling ";
    // don't care too much about the others
    res += "dmg: " + monst->definition->damage->str() + " "; 
    res += "hp: " + std::to_string(monst->hp) + " "; 
    NC_PRINT_CENTERED_AT(plane, x0 + width / 2, y0 + height - 1, "%s", res.c_str());
}

bool str_contains_char(std::string str, char ch) {
    for (int i = 0; str[i]; i++) if (str[i] == ch) return true;
    return false;
}

void move_back(ncpp::NotCurses *nc, ncpp::Plane *plane) {
    NC_HIDE(nc, *plane);
}

void Game::inventory_menu() {
    bool inventory = true, expunge_confirm = false;
    int menu_i = -1;
    int i, j, swap_slot;
    ncinput inp;
    char ch;
    Item *item, *target_item;
    item_type_t type;
    int scroll_dir;
    int inv_count;
    int equip_count = ARRAY_SIZE(pc.equipment);
    ncpp::Plane *plane = planes->get("inventory");
    ncpp::Plane *top = planes->get("top");

    NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
    plane->styles_set(ncpp::CellStyle::Bold);
    plane->erase();
    plane->move_top();
    MessageQueue::get()->clear();
    while (true) {
        top->erase();
        plane->erase();
        if (MessageQueue::get()->empty()) {
            MessageQueue::get()->add("-- &5&bINVENTORY&r --");
        }
        MessageQueue::get()->emit(*top, false);
        inv_count = pc.inventory_size();
        // Check if the selected item is out of bounds.
        // If it is, we'll move to the next available item.
        if (inventory && (menu_i >= inv_count || menu_i < 0)) {
            if (inv_count > 0) {
                if (menu_i < 0) menu_i = 0;
                else menu_i = inv_count - 1;
            }
            else menu_i = -1;
        }
        else if (!inventory && (menu_i >= equip_count || menu_i < 0)) {
            if (menu_i < 0) scroll_dir = 0;
            else scroll_dir = 1;
            for (i = 0; i < equip_count; i++) {
                if (pc.equipment[i]) {
                    if (scroll_dir) menu_i = i;
                    else if (menu_i < 0) menu_i = i;
                }
            }
        }
        render_inventory_box("INVENTORY", "1234567890", inventory ? "^^^" : "", 0, 0);
        for (i = 0; i < inv_count; i++)
            render_inventory_item(pc.inventory_at(i), i, inventory && i == menu_i, 0, 0);
        for (i = inv_count; i < MAX_CARRY_SLOTS; i++)
            render_inventory_item(nullptr, i, false, 0, 0);
        render_inventory_box("EQUIPMENT", "asdfghjk  ", !inventory ? "^^^" : "", INVENTORY_BOX_WIDTH + 2, 0);
        for (i = 0; i < equip_count; i++) {
            if (pc.equipment[i]) render_inventory_item(pc.equipment[i], i, !inventory && i == menu_i, INVENTORY_BOX_WIDTH + 2, 0);
            else render_inventory_item(nullptr, i, false, INVENTORY_BOX_WIDTH + 2, 0);
        }
        if (menu_i < 0) item = NULL;
        else item = inventory ? pc.inventory_at(menu_i) : pc.equipment[menu_i];
        render_inventory_details(plane, item, 0, 13, DETAILS_WIDTH, DETAILS_HEIGHT);
        nc->render();
        scroll_dir = 0;
        target_item = nullptr;
        if (inventory) {
            if (menu_i < pc.inventory_size() && menu_i >= 0)
                target_item = pc.inventory_at(menu_i);
        } else {
            if (menu_i < ARRAY_SIZE(pc.equipment) && menu_i >= 0)
                target_item = pc.equipment[menu_i];
        }
        nc->get(true, &inp);
        switch (inp.id) {
            case NCKEY_DOWN:
                scroll_dir = 1;
                expunge_confirm = false;
                break;
            case NCKEY_UP:
                scroll_dir = -1;
                expunge_confirm = false;
                break;
            case NCKEY_LEFT:
            case NCKEY_RIGHT:
                inventory = !inventory;
                menu_i = 0;
                expunge_confirm = false;
                break;
            case 'i':
            case 'e':
            case NCKEY_ESC:
                planes->for_each("inv_", move_back);
                NC_HIDE(nc, *plane);
                expunge_confirm = false;
                return;
            case NCKEY_ENTER:
                expunge_confirm = false;
                if (!target_item) break;
                if (inventory) {
                    type = target_item->definition->type;
                    // Make sure it's equippable.
                    if (type < ITEM_TYPE_WEAPON || type > ITEM_TYPE_POCKET) {
                        MessageQueue::get()->clear();
                        MessageQueue::get()->add("&0&bYou can't equip &" + std::to_string(
                            target_item->current_color()) + escape_col(target_item->definition->name) + "&0&b!");
                        break;
                    }
                    swap_slot = type;
                    // Prefer the second ring slot if they're both full.
                    if (pc.equipment[type] != NULL && type == ITEM_TYPE_POCKET) {
                        swap_slot = type + 1;
                    }
                    // Remove that item from the inventory...
                    target_item = pc.remove_from_inventory(menu_i);
                    if (target_item->is_stacked()) throw dungeon_exception(__PRETTY_FUNCTION__, "invalid state: removed item is stacked");
                    // If the swap slot is empty, just add the item there.
                    if (pc.equipment[swap_slot] == NULL) {
                        MessageQueue::get()->add("You equipped &" + std::to_string(
                            target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                    } else {
                        // Then we have to move that item out to the inventory.
                        // This will reorder them, but it seems way more difficult than it's worth not to.
                        MessageQueue::get()->add("You equipped &"
                                + std::to_string(target_item->current_color())
                                + escape_col(target_item->definition->name) + "&r, swapping out &"
                                + std::to_string(pc.equipment[swap_slot]->current_color())
                                + escape_col(pc.equipment[swap_slot]->definition->name) + "&r.");
                        pc.add_to_inventory(pc.equipment[swap_slot]);
                    }
                    pc.equipment[swap_slot] = target_item;
                    inventory = false;
                    menu_i = swap_slot;
                } else {
                    if (pc.inventory_size() >= 10) {
                        MessageQueue::get()->clear();
                        MessageQueue::get()->add("&0&bYou have no carry slots open!");
                        break;
                    }
                    pc.add_to_inventory(target_item);
                    pc.equipment[menu_i] = NULL;
                    MessageQueue::get()->add("You unequipped &" + std::to_string(
                        target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                    inventory = true;
                    menu_i = pc.inventory_size() - 1;
                }
                break;
            case NCKEY_BACKSPACE:
                if (!target_item) break;
                if (!expunge_confirm) {
                    MessageQueue::get()->add("Press BACKSPACE again to expunge &" + std::to_string(
                        target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                    expunge_confirm = true;
                    break;
                }
                if (inventory) {
                    pc.remove_from_inventory(menu_i);
                } else {
                    pc.equipment[menu_i] = nullptr;
                }
                MessageQueue::get()->add("You expunged &" + std::to_string(
                    target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                delete target_item;
                expunge_confirm = false;
                break;
            case NCKEY_SPACE:
                expunge_confirm = false;
                if (!target_item) break;
                if (inventory) {
                    pc.remove_from_inventory(menu_i);
                } else {
                    pc.equipment[menu_i] = nullptr;
                }
                if (item_map[pc.x][pc.y]) {
                    item_map[pc.x][pc.y]->add_to_stack(target_item);
                } else {
                    item_map[pc.x][pc.y] = target_item;
                }
                MessageQueue::get()->add("You dropped &" + std::to_string(
                    target_item->current_color()) + escape_col(target_item->definition->name) + "&r.");
                break;
            default:
                expunge_confirm = false;
                ch = (char) inp.id;
                if ((unsigned int) ch == inp.id) {
                    // If it's one of our slot keys, use those
                    if (str_contains_char("0123456789", inp.id)) {
                        inventory = true;
                        i = inp.id == '0' ? 9 : inp.id - '1';
                        if (i >= inv_count) {
                            break;
                        }
                    }
                    else if (str_contains_char("asdfghjk", inp.id)) {
                        inventory = false;
                        i = inp.id - 'a';
                    }
                    menu_i = i;
                    break;
                }
                break;
        }
        if (scroll_dir != 0) {
            if (inventory) {
                menu_i += scroll_dir;
                if (menu_i >= inv_count) menu_i = 0;
                else if (menu_i < 0) menu_i = inv_count - 1;
            }
            else {
                // Seek to the next item
                i = menu_i;
                item = NULL;
                for (j = 0; j < equip_count && !item; j++) {
                    i += scroll_dir;
                    if (i < 0) i += equip_count;
                    i %= equip_count;
                    item = pc.equipment[i];
                }
                if (!item) menu_i = -1;
                else menu_i = i;
            }
        }
    }
}

#define CHEATER_OPT_PRINT(plane, opt, i, menu_i) { \
    if (menu_i == i) NC_APPLY_COLOR(*plane, RGB_COLOR_WHITE, RGB_COLOR_BLACK) \
    else NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE); \
    plane->printf(1 + i, 0, opt); }

void Game::cheater_menu() {
    int menu_i = 0;
    ncinput inp;
    int options = 7;
    unsigned int x, y;
    ncpp::Plane *plane = planes->get("cheater");
    ncpp::Plane *top = planes->get("top");

    plane->erase();
    plane->move_top();
    MessageQueue::get()->clear();
    while (true) {
        top->erase();
        plane->erase();
        MessageQueue::get()->emit(*top, false);

        NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
        for (x = 0; x < CHEATER_MENU_WIDTH; x++) {
            for (y = 0; y < CHEATER_MENU_HEIGHT; y++) {
                plane->putc(y, x, ' ');
            }
        }

        plane->styles_set(ncpp::CellStyle::Bold);
        plane->printf(0, ncpp::NCAlign::Center, "CHEATS");
        plane->styles_off(ncpp::CellStyle::Bold);

        CHEATER_OPT_PRINT(plane, "look", 0, menu_i);
        NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK_DIM, RGB_COLOR_WHITE);
        plane->printf(1, ncpp::NCAlign::Right, "->");

        CHEATER_OPT_PRINT(plane, "speed", 1, menu_i);
        if (speed) NC_APPLY_COLOR(*plane, RGB_COLOR_GREEN, RGB_COLOR_WHITE)
        else NC_APPLY_COLOR(*plane, RGB_COLOR_RED, RGB_COLOR_WHITE);
        plane->printf(2, ncpp::NCAlign::Right, speed ? "on" : "off");

        CHEATER_OPT_PRINT(plane, "anti-damage", 2, menu_i);
        if (antidmg) NC_APPLY_COLOR(*plane, RGB_COLOR_GREEN, RGB_COLOR_WHITE)
        else NC_APPLY_COLOR(*plane, RGB_COLOR_RED, RGB_COLOR_WHITE);
        plane->printf(3, ncpp::NCAlign::Right, antidmg ? "on" : "off");

        CHEATER_OPT_PRINT(plane, "give linux penguin", 3, menu_i);
        NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK_DIM, RGB_COLOR_WHITE);
        plane->printf(4, ncpp::NCAlign::Right, "->");

        CHEATER_OPT_PRINT(plane, "teleport", 4, menu_i);
        NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK_DIM, RGB_COLOR_WHITE);
        plane->printf(5, ncpp::NCAlign::Right, "->");

        CHEATER_OPT_PRINT(plane, "go upstairs", 5, menu_i);
        NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK_DIM, RGB_COLOR_WHITE);
        plane->printf(6, ncpp::NCAlign::Right, "->");

        CHEATER_OPT_PRINT(plane, "replenish health", 6, menu_i);
        NC_APPLY_COLOR(*plane, RGB_COLOR_BLACK_DIM, RGB_COLOR_WHITE);
        plane->printf(7, ncpp::NCAlign::Right, "->");

        nc->render();
        nc->get(true, &inp);
        switch (inp.id) {
            case NCKEY_DOWN:
                menu_i++;
                if (menu_i >= options) menu_i = 0;
                break;
            case NCKEY_UP:
                menu_i--;
                if (menu_i < 0) menu_i = options - 1;
                break;
            case '`':
            case NCKEY_ESC:
                NC_HIDE(nc, *plane);
                return;
            case NCKEY_ENTER:
                switch (menu_i) {
                    case 0:
                        look_mode = true;
                        pointer.x = pc.x;
                        pointer.y = pc.y;
                        NC_HIDE(nc, *plane);
                        return;
                    case 1:
                        if (speed) {
                            pc.speed -= 100;
                        } else {
                            pc.speed += 100;
                        }
                        speed = !speed;
                        break;
                    case 2:
                        antidmg = !antidmg;
                        break;
                    case 3:
                        if (pc.equipment[PC_SLOT_WEAPON]) {
                            delete pc.equipment[PC_SLOT_WEAPON];
                        }
                        pc.equipment[PC_SLOT_WEAPON] = new Item(item_defs["penguin"]);
                        break;
                    case 4:
                        teleport_mode = true;
                        pointer.x = pc.x;
                        pointer.y = pc.y;
                        NC_HIDE(nc, *plane);
                        return;
                    case 5:
                        for (auto &f : dungeons) {
                            if (dungeon->options->up_staircase == f->id) {
                                apply_dungeon(*f, random_location_no_kill(f->dungeon, f->character_map));
                                MessageQueue::get()->clear();
                                MessageQueue::get()->add("You magically teleport to &b" + f->dungeon->options->name + "&r.");
                                NC_HIDE(nc, *plane);
                                return;
                            }
                        }
                        break;
                    case 6:
                        pc.hp = pc.base_hp;
                        break;
                }
                break;
        }
    }
}