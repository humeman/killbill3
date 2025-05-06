#include "item.h"
#include "random.h"
#include "macros.h"

Item::Item(ItemDefinition *definition) {
    if (definition->artifact) {
        definition->artifact_created = true;
    }

    this->definition = definition;
    this->dodge_bonus = definition->dodge_bonus->roll();
    this->defense_bonus = definition->defense_bonus->roll();
    this->speed_bonus = definition->speed_bonus->roll();
    this->next = NULL;
    color_count = 0;
    int i;
    int color_val = definition->color;
    for (i = 0; i < 8; i++) {
        if (color_val & 1) color_count++;
        color_val >>= 1;
    }
}

Item::~Item() {
    // Deletes all stacked items too
    if (next != NULL) {
        delete next;
    }
}

uint8_t Item::next_color() {
    color_i = (color_i + 1) % color_count;
    return current_color();
}

uint8_t Item::current_color() {
    int i;
    int found = -1;
    int color_val = definition->color;
    for (i = 0; i < 8; i++) {
        if (color_val & 1) found++;
        if (found == color_i) return i;
        color_val >>= 1;
    }
    throw dungeon_exception(__PRETTY_FUNCTION__, "did not find target color (was it modified?)");
}

int Item::get_damage() {
    return definition->damage_bonus->roll();
}

void Item::add_to_stack(Item *item) {
    Item *current = this;
    while (current->next != NULL)
        current = current->next;
    current->next = item;
}

Item *Item::detach_stack() {
    Item *removed = next;
    next = NULL;
    return removed;
}

Item *Item::remove_next_in_stack() {
    Item *removed = next;
    if (removed != NULL) next = removed->next;
    else next = NULL;
    removed->next = NULL;
    return removed;
}

Item *Item::next_in_stack() {
    return next;
}

bool Item::is_stacked() {
    return next != NULL;
}