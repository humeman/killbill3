#include "message_queue.h"
#include "macros.h"
#include "logger.h"


MessageQueue *MessageQueue::instance = nullptr;

std::string escape_col(std::string inp) {
    unsigned long n = 0;
    while ((n = inp.find('&', n)) != std::string::npos) {
        inp.replace(n, 1, "&&");
        n++;
    }
    return inp;
}

void MessageQueue::emit(ncpp::Plane &plane, bool sticky) {
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
    int x0 = (plane.get_dim_x() - count) / 2;
    if (x0 < 0) x0 = 0; // Overflow
    plane.cursor_move(0, x0);

    NC_RESET(plane);
    int disp_i = 0;
    active = false;
    int last_color = 7;
    for (i = 0; (c = message[i]) && disp_i < (int) (plane.get_dim_x()) - 3; i++) {
        if (c == '&') {
            if (active) {
                active = false;
                disp_i++;
                plane.putc(c);
            } else {
                // Activate the color code symbol, but don't count yet.
                active = true;
            }
        }
        else if (active) {
            active = false;

            // Find out what color code this is.
            if (c == 'b') {
                plane.styles_on(ncpp::CellStyle::Bold);
            } else if (c == 'd') {
                NC_APPLY_COLOR_BY_NUM_DIM(plane, last_color, RGB_COLOR_BLACK);
            } else if (c == 'r') {
                NC_RESET(plane);
            } else if (c >= '0' && c <= '7') {
                last_color = c - '0';
                NC_APPLY_COLOR_BY_NUM(plane, last_color, RGB_COLOR_BLACK);
            } else {
                std::string err = "invalid color format specifier '";
                err += c;
                err += "'";
                throw dungeon_exception(__PRETTY_FUNCTION__, err);
            }
        }
        else {
            disp_i++;
            plane.putc(c);
        }
    }
    if (disp_i >= (int) (plane.get_dim_x()) - 6) {
        // Cut off
        NC_APPLY_COLOR(plane, RGB_COLOR_WHITE_DIM, RGB_COLOR_BLACK);
        plane.cursor_move(plane.get_y(), plane.get_dim_x() - 6);
        plane.putstr("... ");
    }

    // If there's more to print, we can show that too
    if (!messages.empty() && !sticky) {
        NC_APPLY_COLOR(plane, RGB_COLOR_WHITE_DIM, RGB_COLOR_BLACK);
        plane.cursor_move(plane.get_y(), plane.get_dim_x() - 2);
        plane.putstr("->");
    }
}

void MessageQueue::drop() {
    if (!messages.empty())
        messages.pop_front();
}

void MessageQueue::clear() {
    messages.clear();
}

void MessageQueue::add(std::string message) {
    messages.push_back(message);
}

bool MessageQueue::empty() {
    return messages.empty();
}