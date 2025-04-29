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

#define CELL_PLANE_AT(this, x, y) this->cell_planes[x * this->cells_y + y]
#define CELL_CACHE_FLOOR_AT(this, x, y) this->cell_cache[x * this->cells_y + y]
#define CELL_CACHE_HEARTS_AT(this, i) this->cell_cache[this->cells_x * this->cells_y + i]
#define CELL_CACHE_ITEMS_AT(this, i) this->cell_cache[this->cells_x * this->cells_y + HEARTS + i]
#define DRAW_TO_PLANE(plane, texture_name) { \
    ncvisual_options vopts{}; \
    vopts.flags |= NCVISUAL_OPTION_NOINTERPOLATE; \
    vopts.scaling = NCSCALE_STRETCH; \
    vopts.blitter = NCBLIT_PIXEL; \
    vopts.n = (plane)->to_ncplane(); \
    resource_manager_t::get()->get_visual(texture_name)->blit(&vopts); }

const std::string CELL_TYPES_TO_FLOOR_TEXTURES[] = {
    "floor_stone",
    "floor_room",
    "floor_hall",
    "floor_up_staircase",
    "floor_down_staircase",
    "floor_stone",
    "floor_stone",
    "floor_stone"
};

#define HEARTS 15

void game_t::create_nc() {
    logger_t::get()->off();
    nc = new ncpp::NotCurses();
}
void game_t::end_nc() {
    if (nc) {
        nc->stop();
        delete nc;
        nc = nullptr;
    }
    logger_t::get()->on();
}

/**
 * The new fancy sixel interface will let the camera follow the PC around.
 * We'll have a fixed frame with the usual status message, health, and inventory,
 * then the middle of the screen will be the game itself (now with 4x2 cells, since that's approximately square).
 * 
 * Our problem is guestimating how big that middle interface should be. I think it's better to use the entire terminal
 * window we're given rather than sticking to just 80x24. But, we need to keep the PC centered within that and fit 
 * as many cells within there as possible, and the game drawing loop will need to reflect that dynamic area within
 * the dungeon.
 * 
 * To start, we need our Notcurses planes -- one for the top status message, one for the health/inventory, and then
 * the game window itself (where each cell gets a plane).
 */
void game_t::run() {
    if (!nc)
        throw dungeon_exception(__PRETTY_FUNCTION__, "run game_t::create_nc()");
    const dungeon_exception *ex = nullptr;
    ncpp::Plane *plane;
    try {
        // Dimensioning stuff...
        logger_t::debug(__FILE__, "dimensioning terminal");
        nc->get_term_dim(term_y, term_x);
        if (term_x < 60 || term_y < 20) {
            throw dungeon_exception(__PRETTY_FUNCTION__, "terminal is too small (minimum 60x20)");
        }
        logger_t::debug(__FILE__, "size is (" + std::to_string(term_x) + "x" + std::to_string(term_y) + ")");
    
        // Reserve 4 rows on the bottom, a line of gap, and a row on the top.
        unsigned int cell_rows = term_y - 5;
        // 3 rows, 6 cols each.
        cells_y = cell_rows / 3;
        cells_x = term_x / 6;
    
        // We'd like to center these on the X if there's more cells available.
        // Don't care as much about Y.
        xoff = (term_x - cells_x * 6) / 2;

        // Now we'll make the planes.
        logger_t::debug(__FILE__, "create top plane");
        top_plane = new ncpp::Plane(1, term_x, 0, 0);
        NC_RESET(*top_plane);
        logger_t::debug(__FILE__, "create bottom plane");
        bottom_plane = new ncpp::Plane(4, term_x, term_y - 4, 0);
        NC_RESET(*bottom_plane);

        // Apply real RGBA black
        NC_APPLY_COLOR(*(nc->get_stdplane()), RGB_COLOR_BLACK, RGB_COLOR_BLACK);
        for (unsigned int x = 0; x < term_x; x++) {
            for (unsigned int y = 0; y < term_y; y++) {
                nc->get_stdplane()->putc(y, x, ' ');
            }
        }
        NC_APPLY_COLOR(*(bottom_plane), RGB_COLOR_BLACK, RGB_COLOR_BLACK);
        bottom_plane->move(term_y - 4, 0);
        for (unsigned int x = bottom_plane->get_x(); x < bottom_plane->get_x() + bottom_plane->get_dim_x(); x++) {
            for (unsigned int y = bottom_plane->get_y(); y < bottom_plane->get_y() + bottom_plane->get_dim_y(); y++) {
                bottom_plane->putc(' ');
            }
        }

        // These are dynamically allocated, so we should always use term_x/y and never refresh them.
        logger_t::debug(__FILE__, "create cell planes");
        cell_planes.clear();
        cell_planes.reserve(cells_x * cells_y * sizeof (ncpp::Plane *));
        for (unsigned int x = 0; x < cells_x; x++) {
            for (unsigned int y = 0; y < cells_y; y++) {
                plane = new ncpp::Plane(3, 6, y * 3 + 1, x * 6 + xoff);
                NC_RESET(*plane);
                cell_planes.push_back(plane);
            }
        }
        logger_t::debug(__FILE__, "create health planes");
        health_planes.clear();
        health_planes.reserve(HEARTS * sizeof (ncpp::Plane *));
        // 4 cols wide, 2 rows high. Center in the middle
        xoff = (bottom_plane->get_dim_x() - 4 * HEARTS) / 2;

        for (unsigned int x = 0; x < HEARTS; x++) {
            plane = new ncpp::Plane(2, 4, bottom_plane->get_y(), x * 4 + xoff);
            NC_RESET(*plane);
            health_planes.push_back(plane);
        }
        logger_t::debug(__FILE__, "create item planes");
        item_planes.clear();
        unsigned long item_count = ARRAY_SIZE(pc.equipment) + MAX_CARRY_SLOTS;
        item_planes.reserve(item_count * sizeof (ncpp::Plane *));
        // 2 cols wide, 1 row high.
        // 2 col gap for indicators.
        xoff = (bottom_plane->get_dim_x() - 4 * item_count) / 2;

        for (unsigned int x = 0; x < item_count; x++) {
            plane = new ncpp::Plane(1, 2, bottom_plane->get_y() + 2, x * 4 + xoff);
            NC_RESET(*plane);
            item_planes.push_back(plane);
        }

        logger_t::debug(__FILE__, "create cell cache");
        cell_cache.clear();
        cell_cache.reserve((cells_x * cells_y + HEARTS) * sizeof (std::string *));
        for (unsigned int x = 0; x < (cells_x * cells_y + HEARTS + item_count); x++) {
            cell_cache.push_back("");
        }

        overlay_plane = new ncpp::Plane(term_y, term_x, 0, 0);
        NC_RESET(*overlay_plane);
        overlay_plane->move_bottom();

        run_internal();
    } catch (dungeon_exception &e) {
        ex = new dungeon_exception(__PRETTY_FUNCTION__, e, "game loop failed");
    } catch (std::exception &e) {
        ex = new dungeon_exception(__PRETTY_FUNCTION__, "game loop failed (" + std::string(e.what()) + ")");
    } catch (...) {
        ex = new dungeon_exception(__PRETTY_FUNCTION__, "game loop failed (unknown)");
    }

    ncpp::Plane *p;
    while (!cell_planes.empty()) {
        p = cell_planes.back();
        cell_planes.pop_back();
        delete p;
    }
    while (!health_planes.empty()) {
        p = health_planes.back();
        health_planes.pop_back();
        delete p;
    }
    while (!item_planes.empty()) {
        p = item_planes.back();
        item_planes.pop_back();
        delete p;
    }
    // Deleting the top or bottom plane blows up notcurses
    // (it segfaults itself). Slight memory leak :)
    resource_manager_t::destroy();
    end_nc();
    if (ex) {
        logger_t::error(__FILE__, "rethrowing game loop exception");
        throw *ex;
    }
}

void game_t::run_internal() {
    ncinput inp;
    // message_queue_t::get()->add("--- &0&bKILL BILL 3&r ---");
    unsigned int x, y;
    render_frame(true);
    while (true) {
        render_frame(false);
        nc->get(true, &inp);

        if (result != GAME_RESULT_RUNNING) {
            // Print our beautiful images
            if (result == GAME_RESULT_LOSE) {
                NC_APPLY_COLOR((*overlay_plane), RGB_COLOR_WHITE, RGB_COLOR_BSOD);
                overlay_plane->styles_set(ncpp::CellStyle::Bold);
                overlay_plane->move_top();
                for (y = 0; y < term_y; y++) {
                    for (x = 0; x < term_x; x++) {
                        overlay_plane->putc(y, x, ' ');
                    }
                }
                overlay_plane->printf(10, 2, "Your PC ran into a problem and needs to restart. We're");
                overlay_plane->printf(11, 2, "just collecting some error info, and then we'll restart for");
                overlay_plane->printf(12, 2, "you.");
            }
            else {
                NC_APPLY_COLOR((*overlay_plane), RGB_COLOR_WHITE, RGB_COLOR_BSOD)
                overlay_plane->styles_set(ncpp::CellStyle::Bold);
                overlay_plane->move_top();
                for (y = 0; y < term_y; y++) {
                    for (x = 0; x < term_x; x++) {
                        overlay_plane->putc(y, x, ' ');
                    }
                }
                overlay_plane->printf(10, 2, "Your PC ran into a problem and needs to restart. We're");
                overlay_plane->printf(11, 2, "just collecting some error info, and then we'll restart for");
                overlay_plane->printf(12, 2, "you.");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            x = 0;
            while (x <= 100) {
                x += rand() % 31;
                overlay_plane->printf(14, 2, "%d%% complete", x > 100 ? 100 : x);
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
            update_pathfinding(dungeon, pathfinding_no_tunnel, pathfinding_tunnel, &pc);
            // Run the game until the PC's turn comes up again (or it dies)
            run_until_pc();
            //fog = false;
        }
        if (result != GAME_RESULT_RUNNING) {
            //hide_fog_of_war = true;
            message_queue_t::get()->clear();
            message_queue_t::get()->add("&bGame over. Press any key to continue.");
        }
    }
    ncinput ni;
    nc->get(true, &ni);
}

void game_t::run_until_pc() {
    character_t *ch = NULL;
    monster_t *monster;
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

        monster = (monster_t *) ch;
        result = GAME_RESULT_RUNNING;
        monster->take_turn(dungeon, &pc, turn_queue, character_map, item_map, pathfinding_tunnel, pathfinding_no_tunnel, priority, result);
        message_queue_t::get()->add("pc health " + std::to_string(pc.hp));
        if (pc.dead) result = GAME_RESULT_LOSE;
        return;
    }
}

void game_t::render_frame(bool complete_redraw) {
    // Status message.
    top_plane->erase();
    message_queue_t::get()->emit(*top_plane, false);

    // Dungeon.
    // Complete redraw means empty out all the cells.
    // That should really only ever happen on the first frame, since it removes
    // the benefit of any cell caching.
    int x, y;
    if (complete_redraw) {
        logger_t::debug(__FILE__, "running complete redraw");
        for (x = 0; x < (int) cells_x; x++) {
            for (y = 0; y < (int) cells_y; y++) {
                CELL_CACHE_FLOOR_AT(this, x, y) = "";
                DRAW_TO_PLANE(CELL_PLANE_AT(this, x, y), CELL_TYPES_TO_FLOOR_TEXTURES[CELL_TYPE_EMPTY]);
            }
        }
    }


    // We're centering the camera around the PC.
    int dungeon_x0 = pc.x - cells_x / 2;
    int dungeon_y0 = pc.y - cells_y / 2;
    
    // Unsurprisingly, drawing entire pictures to the terminal is really slow.
    // To mitigate this, we'll only draw any changed cells from frame-to-frame.
    // Easiest method here is to keep track of the texture name.
    int cell_x, cell_y;
    std::string new_texture;
    monster_t *monst;
    for (x = dungeon_x0; x < (int) (dungeon_x0 + cells_x); x++) {
        if (x < 0 || x >= dungeon->width) continue;
        for (y = dungeon_y0; y < (int) (dungeon_y0 + cells_y); y++) {
            new_texture = "";
            if (x < 0 || x >= dungeon->width || y < 0 || y >= dungeon->height) {
                new_texture = CELL_TYPES_TO_FLOOR_TEXTURES[CELL_TYPE_STONE];
            }
            else {
                // Find out what we're drawing here.
                // Characters get first priority.
                if (character_map[x][y]) {
                    if (character_map[x][y]->type() == CHARACTER_TYPE_PC) {
                        new_texture = std::string(PC_TEXTURE) + "_" + "nesw"[pc.direction];
                    }
                    else if (character_map[x][y]->type() == CHARACTER_TYPE_MONSTER) {
                        monst = (monster_t *) character_map[x][y];
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
                    new_texture = CELL_TYPES_TO_FLOOR_TEXTURES[dungeon->cells[x][y].type];
                }
            }
            
            cell_x = x - dungeon_x0;
            cell_y = y - dungeon_y0;
            if (complete_redraw || new_texture != CELL_CACHE_FLOOR_AT(this, cell_x, cell_y)) {
                CELL_CACHE_FLOOR_AT(this, cell_x, cell_y) = new_texture;
                if (new_texture.length() == 0) new_texture = CELL_TYPES_TO_FLOOR_TEXTURES[CELL_TYPE_EMPTY];
                DRAW_TO_PLANE(CELL_PLANE_AT(this, cell_x, cell_y), new_texture);
            }
        }
    }

    // The healthbar.
    bottom_plane->erase();
    NC_RESET(*bottom_plane);
    // Find the percentage of health left
    float hp = (float) pc.hp / pc.base_hp;
    // We'll have 15 hearts rendered
    int hearts_filled = (int) ((hp < 0 ? 0 : hp) * HEARTS);
    if (hearts_filled == 0 && pc.hp > 0) hearts_filled = 1;
    int i;
    for (i = 0; i < hearts_filled; i++) {
        if (CELL_CACHE_HEARTS_AT(this, i) != "ui_heart") {
            CELL_CACHE_HEARTS_AT(this, i) = "ui_heart";
            DRAW_TO_PLANE(health_planes[i], "ui_heart");
        }
    }
    for (i = hearts_filled; i < HEARTS; i++) {
        if (CELL_CACHE_HEARTS_AT(this, i) != "ui_heart_dead") {
            CELL_CACHE_HEARTS_AT(this, i) = "ui_heart_dead";
            DRAW_TO_PLANE(health_planes[i], "ui_heart_dead");
        }
    }

    // The inventory.
    std::string item_texture;
    NC_APPLY_COLOR(*bottom_plane, RGB_COLOR_WHITE, RGB_COLOR_BLACK);
    NC_APPLY_DIM((*bottom_plane));
    for (i = 0; i < MAX_CARRY_SLOTS; i++) {
        item_texture = i < pc.inventory_size() ? pc.inventory_at(i)->definition->ui_texture : "items_empty";

        if (CELL_CACHE_ITEMS_AT(this, i) != item_texture) {
            CELL_CACHE_ITEMS_AT(this, i) = item_texture;
            DRAW_TO_PLANE(item_planes[i], item_texture);
        }
        bottom_plane->putc(2, item_planes[i]->get_x() - 1, i == 9 ? '0' : '1' + i);
    }
    for (i = 0; i < ARRAY_SIZE(pc.equipment); i++) {
        item_texture = pc.equipment[i] ? pc.equipment[i]->definition->ui_texture : "items_empty";

        if (CELL_CACHE_ITEMS_AT(this, MAX_CARRY_SLOTS + i) != item_texture) {
            CELL_CACHE_ITEMS_AT(this, MAX_CARRY_SLOTS + i) = item_texture;
            DRAW_TO_PLANE(item_planes[MAX_CARRY_SLOTS + i], item_texture);
        }
        bottom_plane->putc(2, item_planes[MAX_CARRY_SLOTS + i]->get_x() - 1, 'a' + i);
    }

    nc->render();
}

#define INVENTORY_BOX_WIDTH 25

void game_t::render_inventory_box(std::string title, std::string labels, std::string input_tip, int x0, int y0) {
    int height = labels.length() + 2;
    unsigned int i, x, y;
    // Clear out the area.
    NC_APPLY_COLOR(*overlay_plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
    for (y = y0; y < term_y; y++) {
        for (x = x0; x < INVENTORY_BOX_WIDTH; x++) {
            overlay_plane->putc(y, x, ' ');
        }
    }
    // Title...
    overlay_plane->styles_set(ncpp::CellStyle::Bold);
    NC_PRINT_CENTERED_AT(overlay_plane, x0 + INVENTORY_BOX_WIDTH / 2, y0, "%s", title.c_str());
    overlay_plane->styles_off(ncpp::CellStyle::Bold);
    // Input tip...
    NC_PRINT_CENTERED_AT(overlay_plane, x0 + INVENTORY_BOX_WIDTH / 2, y0 + height - 1, "%s", input_tip.c_str());
    // And the labels.
    for (i = 0; i < labels.length(); i++)
        overlay_plane->printf(y0 + 1 + i, x0, "%c", labels[i]);
}

void game_t::render_inventory_item(item_t *item, int i, bool selected, int x0, int y0) {
    NC_APPLY_COLOR_BY_NUM(*overlay_plane, item->current_color(), selected ? RGB_COLOR_BLACK : RGB_COLOR_WHITE);
    // mvaddch(y0 + 1 + i, x0 + 2, item->regular_symbol());
    // attroff(COLOR_PAIR(c) | A_BOLD);
    // Now the item title
    overlay_plane->printf(y0 + 1 + i, x0 + 4, "%s", item->definition->name.c_str());
}

void game_t::render_inventory_details(item_t *item, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height) {
    unsigned int i, x, y;
    std::string desc;
    // Draw out a box, taking up the whole width of the screen at y0.
    // This determines how many lines of description we get until it cuts off.
    NC_APPLY_COLOR(*overlay_plane, RGB_COLOR_BLACK, RGB_COLOR_WHITE);
    for (y = y0; y < y0 + height; y++) {
        for (x = x0; x < x0 + width; x++) {
            overlay_plane->putc(y, x, ' ');
        }
    }

    if (!item) return;

    overlay_plane->styles_set(ncpp::CellStyle::Bold);
    NC_PRINT_CENTERED_AT(overlay_plane, x0 + width / 2, y0, "%s", item->definition->name.c_str());
    overlay_plane->styles_off(ncpp::CellStyle::Bold);
    // Just printing out the whole string clears out the color on the remainder of the line when there's
    // a newline character. It also doesn't let us protect against overflow, so unfortunately, this'll be
    // by hand.
    x = 0;
    y = y0 + 1;
    desc = item->definition->description;
    for (i = 0; y < HEIGHT - 1 && desc[i]; i++) {
        if (desc[i] == '\n') {
            x = 0;
            y++;
            continue;
        }
        overlay_plane->putc(y, x++, desc[i]);
    }
    // The last line will be the dice.
    NC_APPLY_DIM((*overlay_plane));
    NC_PRINT_CENTERED_AT(overlay_plane, x0 + width / 2, y0 + height - 1, "%sdmg: %s, def: %s, speed: %s, dodge: %s",
        item->definition->artifact ? "*ARTIFACT* " : "",
        item->definition->damage_bonus->str().c_str(),
        item->definition->defense_bonus->str().c_str(),
        item->definition->speed_bonus->str().c_str(),
        item->definition->dodge_bonus->str().c_str());
}

bool str_contains_char(std::string str, char ch) {
    for (int i = 0; str[i]; i++) if (str[i] == ch) return true;
    return false;
}

void game_t::inventory_menu() {
    bool inventory = true;
    int menu_i = -1;
    int i, j, c;
    char ch;
    item_t *item;
    int scroll_dir;
    int inv_count = pc.inventory_size();
    int equip_count = ARRAY_SIZE(pc.equipment);

    while (true) {
        render_inventory_box("INVENTORY", "1234567890  ", "", 1, 1);
        for (i = 0; i < inv_count; i++)
            render_inventory_item(pc.inventory_at(i), i, inventory && i == menu_i, 1, 1);
        render_inventory_box("EQUIPMENT", "abcdefghijkl", "", 3 + INVENTORY_BOX_WIDTH, 1);
        for (i = 0; i < equip_count; i++)
            if (pc.equipment[i]) render_inventory_item(pc.equipment[i], i, !inventory && i == menu_i, 3 + INVENTORY_BOX_WIDTH, 1);
        if (menu_i < 0) item = NULL;
        else item = inventory ? pc.inventory_at(menu_i) : pc.equipment[menu_i];
        render_inventory_details(item, 16);

        attrset(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
        PRINT_REPEATED(0, 0, WIDTH, ' ');
        PRINTW_CENTERED_AT(WIDTH / 2, 0, "Enter an item, use the arrow keys, or press ESC.");
        c = getch();
        scroll_dir = 0;
        switch (c) {
            case KB_SCROLL_DOWN:
                scroll_dir = 1;
                break;
            case KB_SCROLL_UP:
                scroll_dir = -1;
                break;
            case KB_SCROLL_LEFT:
            case KB_SCROLL_RIGHT:
                inventory = !inventory;
                menu_i = 0;
                break;
            case KB_ESCAPE:
                return;
            default:
                ch = (char) c;
                if (ch == c) {
                    // If it's one of our slot keys, use those
                    if (str_contains_char(INVENTORY_SLOT_CHARS, c)) {
                        inventory = true;
                        i = c == '0' ? 9 : c - '1';
                        if (i >= inv_count) {
                            break;
                        }
                    }
                    else if (str_contains_char(EQUIP_SLOT_CHARS, c)) {
                        inventory = false;
                        i = c - 'a';
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
        if (inventory && menu_i >= inv_count) menu_i = -1;
        else if (!inventory && menu_i >= equip_count) menu_i = -1;
        attroff(COLOR_PAIR(COLORS_TEXT) | A_BOLD);
    }
}