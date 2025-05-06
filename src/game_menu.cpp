#include "game.h"
#include "logger.h"
#include "resource_manager.h"
#include <bits/this_thread_sleep.h>
#include <ncpp/Visual.hh>

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
void Game::run(bool skip_intro) {
    if (!nc)
        throw dungeon_exception(__PRETTY_FUNCTION__, "run Game::create_nc()");
    ncpp::Plane *plane;
    try {
        // Dimensioning stuff...
        Logger::debug(__FILE__, "dimensioning terminal");
        nc->get_term_dim(term_y, term_x);
        if (term_x < 80 || term_y < 24) {
            throw dungeon_exception(__PRETTY_FUNCTION__, "terminal is too small (minimum 80x24)");
        }
        Logger::debug(__FILE__, "size is (" + std::to_string(term_x) + "x" + std::to_string(term_y) + ")");
    
        // Reserve 4 rows on the bottom, a line of gap, and a row on the top.
        unsigned int cell_rows = term_y - 5;
        // 3 rows, 6 cols each.
        cells_y = cell_rows / 3;
        cells_x = term_x / 6;
    
        // We'd like to center these on the X if there's more cells available.
        // Don't care as much about Y.
        unsigned int xoff = (term_x - cells_x * 6) / 2;

        /*
        Now we'll make the planes.
        This uses a 'plane manager', a simple key-value caching map from names to planes->
        Planes have been a bit annoying to deal with (and it's quite possible there's a better recommended way,
        but I did not spend the time to study the user manual and find out). If they're destroyed (go out of 
        scope), they do not appear when calling nc.render(). I also haven't found a way to just blit a
        visual to part of a plane, so I've had to make one for each possible image location/dimension.
        I had previously done this with a bunch of instance variables and arrays of planes, but with the number
        of UI elements this seemed very unsustainable. 
         */
        planes->get("top", 0, 0, term_x, 1);
        planes->get("bottom", 0, term_y - 4, term_x, 4);
        // These are dynamically allocated, so we should always use term_x/y and never refresh them.
        for (unsigned int x = 0; x < cells_x; x++) {
            for (unsigned int y = 0; y < cells_y; y++) {
                planes->get(CELL_NAME(x, y), x * 6 + xoff, y * 3 + 1, 6, 3);
            }
        }
        // 4 cols wide, 2 rows high. Center in the middle
        xoff = (planes->get("bottom")->get_dim_x() - 4 * HEARTS) / 2;
        unsigned int yoff = planes->get("bottom")->get_y();
        for (unsigned int x = 0; x < HEARTS; x++) {
            planes->get(HEART_NAME(x), x * 4 + xoff, yoff, 4, 2);
        }
        unsigned long item_count = ARRAY_SIZE(pc.equipment) + MAX_CARRY_SLOTS;
        // 2 cols wide, 1 row high.
        // 2 col gap for indicators.
        xoff = (planes->get("bottom")->get_dim_x() - 4 * item_count) / 2;
        yoff = planes->get("bottom")->get_y() + 2;
        for (unsigned int x = 0; x < item_count; x++) {
            planes->get(ITEM_NAME(x), x * 4 + xoff, yoff, 2, 1);
        }
        plane = planes->get("overlay", 0, 0, term_x, term_y);
        plane->move_bottom();

        plane = planes->get("inventory", 
            (term_x - 2 * INVENTORY_BOX_WIDTH - 1) / 2, 
            (term_y - 16 - DETAILS_HEIGHT) / 2, 
            DETAILS_WIDTH, 13 + DETAILS_HEIGHT);
        plane->move_bottom();

        plane = planes->get("look", 
            (term_x - DETAILS_WIDTH) / 2, 
            term_y - 6 - DETAILS_HEIGHT, 
            DETAILS_WIDTH, DETAILS_HEIGHT);
        plane->move_bottom();

        plane = planes->get("cheater", 
            (term_x - CHEATER_MENU_WIDTH) / 2, 
            (term_y - CHEATER_MENU_HEIGHT) / 2, 
            CHEATER_MENU_WIDTH, CHEATER_MENU_HEIGHT);
        plane->move_bottom();

        std::string map_name = run_menu(skip_intro);
        init_from_map(map_name);
        run_game();
    } catch (dungeon_exception &e) {
        planes->clear();
        ResourceManager::destroy();
        end_nc();
        throw dungeon_exception(__PRETTY_FUNCTION__, e, "game loop failed");
    }
    // } catch (std::exception &e) {
    //     planes->clear();
    //     ResourceManager::destroy();
    //     end_nc();
    //     Logger::error(__FILE__, "unknown exception during game loop");
    //     throw;
    // } catch (...) {
    //     planes->clear();
    //     ResourceManager::destroy();
    //     end_nc();
    //     Logger::error(__FILE__, "unknown exception during game loop");
    //     throw;
    // }
    planes->clear();
    ResourceManager::destroy();
    end_nc();
}


std::string Game::run_menu(bool skip_intro) {
    VoiceLines *line;
    unsigned int duration, step, x, y;
    bool skip = false;
    char c;
    ncinput inp;
    ncpp::Plane *overlay, *logo, *bg, *text;
    std::map<std::string, VoiceLines *> &lines = vl_defs["bill_intro"];
    unsigned long size, m_width, m_height, x_center, y_center;
    overlay = planes->get("overlay");
    if (!skip_intro) {
        // Making this one here since we need to know where it's at to draw the other stuff.
        // The Michaelsoft logo has an aspect ratio of 7 (14 in terminal cols) to 1. I'd like it to take up about 3/4 of the screen.
        // Find out how big that is.
        m_width = 3 * term_x / 4;
        // Scale this up to a multiple of 14.
        m_width += 14 - (m_width % 14);
        m_height = m_width / 14;
        // Find out where that goes to be centered.
        x_center = (term_x - m_width) / 2;
        y_center = (term_y - m_height) / 2;
        logo = planes->get("menu_michaelsoft", x_center, y_center, m_width, m_height);
        NC_DRAW(logo, "ui_michaelsoft");
        ResourceManager::get()->play_music("intro_bootup");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        nc->render();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        NC_HIDE(nc, *logo);
        planes->release("menu_michaelsoft");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        // The intro cutscene overlay will take up the left of the screen, however big that has to be.
        size = MIN(term_x / 2, term_y);
        bg = planes->get("menu_bg", 0, 0, size * 2, size);
        bg->move_top();
        NC_DRAW(bg, "ui_bg");
        nc->render();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        overlay->move_top();
        NC_APPLY_COLOR(*overlay, RGB_COLOR_RED_DIM, RGB_COLOR_BLACK);
        overlay->printf(term_y - 1, ncpp::NCAlign::Right, "press any key to skip...");

        // The text will go to the rightmost side of the screen.
        text = planes->get("menu_bg_text", term_x - 60, term_y / 2 - 1, 60, 2);
        text->move_top();
        NC_APPLY_COLOR(*text, RGB_COLOR_BLACK, RGB_COLOR_RED);
        text->styles_on(ncpp::CellStyle::Bold);

        // Draw the intro voicelines one-by-one
        while (nc->get(false, &inp)) {}
        for (unsigned int i = 0; i < lines.size(); i++) {
            line = lines[std::to_string(i)];
            if (line->duration < 1000) throw dungeon_exception(__PRETTY_FUNCTION__, "duration needs to be at least 1000ms");
            duration = line->duration - 500;
            step = duration / lines.size();
            if (step == 0) throw dungeon_exception(__PRETTY_FUNCTION__, "too many characters for duration");
            Logger::debug(__FILE__, "music: " + line->music);
            ResourceManager::get()->play_music(line->music);
            text->erase();
            y = 0;
            x = 0;
            for (unsigned int j = 0; (c = line->value[j]); j++) {
                if (nc->get(false, &inp)) {
                    skip = true;
                    break;
                }
                if (c == '\n') {
                    y++;
                    x = 0;
                } else {
                    text->putc(y, x++, c);
                    nc->render();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(step));
            }
            if (skip) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        if (!skip)
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        NC_HIDE(nc, *text);
        NC_HIDE(nc, *bg);
    }
    overlay->move_top();
    NC_APPLY_COLOR(*overlay, RGB_COLOR_RED_DIM, RGB_COLOR_BLACK);
    NC_CLEAR(*overlay);

    // Same deal with the logo, but this one's 4:1.
    m_width = 3 * term_x / 4;
    // Scale this up to a multiple of 8.
    m_width += 8 - (m_width % 8);
    m_height = m_width / 8;
    // Find out where that goes to be centered.
    x_center = (term_x - m_width) / 2;
    logo = planes->get("menu_logo", x_center, 2, m_width, m_height);
    logo->move_top();
    NC_DRAW(logo, "ui_logo");
    planes->release("menu_bg");
    unsigned long bg_height = term_y - 2 - m_height;
    bg = planes->get("menu_bg_2", 0, 2 + m_height, bg_height * 2, bg_height);
    bg->move_top();
    NC_DRAW(bg, "ui_bg_menu");
    
    overlay->printf(term_y - 3, ncpp::NCAlign::Right, "Kill Bill 3");
    overlay->printf(term_y - 2, ncpp::NCAlign::Right, "Created by Camden Senneff");
    overlay->printf(term_y - 1, ncpp::NCAlign::Right, "ISU/CS 3270, Spring 2025");

    NC_APPLY_COLOR(*overlay, RGB_COLOR_RED, RGB_COLOR_BLACK);
    overlay->styles_on(ncpp::CellStyle::Bold);
    overlay->styles_on(ncpp::CellStyle::Underline);
    overlay->printf(6 + m_height, ncpp::NCAlign::Center, "select a difficulty");
    overlay->styles_off(ncpp::CellStyle::Bold);
    overlay->styles_off(ncpp::CellStyle::Underline);
    int menu_i = 0, i;
    unsigned int options = map_defs.size();
    std::vector<std::string> map_names;
    for (i = 0; i < (int) options; i++) {
        for (const auto &pair : map_defs) {
            if (pair.first[0] - '0' == i) {
                map_names.push_back(pair.first);
                break;
            }
        }
    }
    while (true) {
        for (i = 0; i < (int) options; i++) {
            if (menu_i == i) {
                NC_APPLY_COLOR(*overlay, RGB_COLOR_BLACK, RGB_COLOR_RED);
                overlay->styles_on(ncpp::CellStyle::Bold);
            } else {
                NC_APPLY_COLOR(*overlay, RGB_COLOR_RED, RGB_COLOR_BLACK);
                overlay->styles_off(ncpp::CellStyle::Bold);
            }

            overlay->printf(7 + m_height + i, ncpp::NCAlign::Center, "%s", map_names[i].substr(1).c_str());
        }
        nc->render();

        nc->get(true, &inp);
        switch (inp.id) {
            case NCKEY_DOWN:
                menu_i++;
                if (menu_i >= (int) options) menu_i = 0;
                break;
            case NCKEY_UP:
                menu_i--;
                if (menu_i < 0) menu_i = options - 1;
                break;
            case NCKEY_ENTER:
                planes->release("menu_logo");
                planes->release("menu_bg_2");
                NC_HIDE(nc, *overlay);
                return map_names[menu_i];
            case NCKEY_ESC:
            case 'Q':
                return "";
        }
    }

    return "easy";
}