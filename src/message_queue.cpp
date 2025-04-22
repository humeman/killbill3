#include "message_queue.h"
#include "macros.h"

#include <ncurses.h>


std::string escape_col(std::string inp) {
    unsigned long n = 0;
    while ((n = inp.find('&', n)) != std::string::npos) {
        inp.replace(n, 1, "&&");
        n++;
    }
    return inp;
}

void message_queue_t::emit(int y, bool sticky) {
    if (messages.empty()) return;
    std::string message = messages.front();
    if (!sticky)
        messages.pop_front();

    // These can include embedded color codes (0-7), formatted as &<code>.
    // Ripped that one straight from Minecraft servers. :)
    // &b is bold, &d is dim. Any color code does attrset() to reset all other modifiers.
    // && will be an actual & symbol.

    // We need to find the real length to center the message.
    int i;
    char c;
    bool active = false;
    int count = 0;
    for (i = 0; (c = message[i]); i++) {
        if (c == '&') {
            if (active) {
                // Double &, which prints as one & (escaped).
                active = false;
                count++;
            } else {
                // Activate the color code symbol, but don't count yet.
                active = true;
            }
        }
        else if (active) {
            // This is the color code. Deactivate, don't count.
            active = false;
        }
        else {
            // Regular char.
            count++;
        }
    }

    // We know the length now -- find where on the screen that is.
    int x0 = (WIDTH - count) / 2;
    if (x0 < 0) x0 = 0; // Overflow

    move(y, x0);
    attrset(COLOR_PAIR(COLORS_TEXT));
    int disp_i = 0;
    active = false;
    for (i = 0; (c = message[i]) && disp_i < WIDTH - 3; i++) {
        if (c == '&') {
            if (active) {
                active = false;
                disp_i++;
                addch('&');
            } else {
                // Activate the color code symbol, but don't count yet.
                active = true;
            }
        }
        else if (active) {
            active = false;
            // Find out what color code this is.
            if (c == 'b') {
                attron(A_BOLD);
            } else if (c == 'd') {
                attron(A_DIM);
            } else if (c == 'r') {
                attrset(COLOR_PAIR(COLORS_TEXT));
            } else if (c >= '0' && c <= '7') {
                attrset(COLOR_PAIR(COLORS_FLOOR_ANY + (c - '0')));
            } else {
                std::string err = "invalid color format specifier '";
                err += c;
                err += "'";
                throw dungeon_exception(__PRETTY_FUNCTION__, err);
            }
        }
        else {
            disp_i++;
            addch(c);
        }
    }
    if (disp_i >= WIDTH - 6) {
        // Cut off
        attrset(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        mvprintw(y, WIDTH - 6, "...");
    }

    // If there's more to print, we can show that too
    if (!messages.empty() && !sticky) {
        attrset(COLOR_PAIR(COLORS_TEXT) | A_DIM);
        mvprintw(y, WIDTH - 2, "->");
    }
}

void message_queue_t::drop() {
    if (!messages.empty())
        messages.pop_front();
}

void message_queue_t::clear() {
    messages.clear();
}

void message_queue_t::add(std::string message) {
    messages.push_back(message);
}
