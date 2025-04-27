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

#define CELL_PLANE_AT(this, x, y) this->cell_planes[x * this->cells_y + y]
#define CELL_CACHE_FLOOR_AT(this, x, y) this->cell_cache[x * this->cells_y + y]
#define CELL_CACHE_HEARTS_AT(this, i) this->cell_cache[this->cells_x * this->cells_y + i]
#define CELL_CACHE_ITEMS_AT(this, i) this->cell_cache[this->cells_x * this->cells_y + HEARTS + i]
#define DRAW_TO_PLANE(plane, texture_name) { \
    (plane)->erase(); \
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
        NC_APPLY_COLOR(*(nc->get_stdplane()), RGB_COLOR_BLACK, 0x000001);
        for (unsigned int x = 0; x < term_x; x++) {
            for (unsigned int y = 0; y < term_y; y++) {
                nc->get_stdplane()->putc(y, x, ' ');
            }
        }
        NC_APPLY_COLOR(*(bottom_plane), RGB_COLOR_BLACK, 0x000001);
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

        logger_t::debug(__FILE__, "create cell cache");
        cell_cache.clear();
        cell_cache.reserve((cells_x * cells_y + HEARTS) * sizeof (std::string *));
        for (unsigned int x = 0; x < (cells_x * cells_y + HEARTS); x++) {
            cell_cache.push_back("");
        }

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
    delete top_plane;
    delete bottom_plane;
    resource_manager_t::destroy();
    end_nc();
    if (ex) {
        logger_t::error(__FILE__, "rethrowing game loop exception");
        throw *ex;
    }
}

void game_t::run_internal() {
    message_queue_t::get()->add("--- &0&bKILL BILL 3&r ---");
    render_frame(true);
    ncinput ni;
    nc->get(true, &ni);
}

void game_t::render_frame(bool complete_redraw) {
    // Status message.
    logger_t::debug(__FILE__, "top plane");
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
                        new_texture = PC_TEXTURE "_n";
                    }
                    else if (character_map[x][y]->type() == CHARACTER_TYPE_MONSTER) {
                        monst = (monster_t *) character_map[x][y];
                        new_texture = monst->definition->floor_texture_n;
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
    int hearts_filled = (int) (hp * HEARTS);
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
    
    logger_t::debug(__FILE__, "render");
    nc->render();
}