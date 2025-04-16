#include "item.h"
#include "random.h"
#include "macros.h"

char ITEM_TYPES[] = {
    [ITEM_TYPE_WEAPON] = '|',
    [ITEM_TYPE_OFFHAND] = ')',
    [ITEM_TYPE_RANGED] = '}',
    [ITEM_TYPE_ARMOR] = '[',
    [ITEM_TYPE_HELMET] = ']',
    [ITEM_TYPE_CLOAK] = '(',
    [ITEM_TYPE_GLOVES] = '{',
    [ITEM_TYPE_BOOTS] = '\\',
    [ITEM_TYPE_RING] = '=',
    [ITEM_TYPE_AMULET] = '"',
    [ITEM_TYPE_LIGHT] = '_',
    [ITEM_TYPE_SCROLL] = '~',
    [ITEM_TYPE_BOOK] = '?',
    [ITEM_TYPE_FLASK] = '!',
    [ITEM_TYPE_GOLD] = '$',
    [ITEM_TYPE_AMMUNITION] = '/',
    [ITEM_TYPE_FOOD] = ',',
    [ITEM_TYPE_WAND] = '-',
    [ITEM_TYPE_CONTAINER] = '%',
    [ITEM_TYPE_STACK] = '&',
    [ITEM_TYPE_UNKNOWN] = '*'
};

item_t::item_t(item_definition_t *definition) {
    if (definition->artifact) {
        if (definition->artifact_created)
            throw dungeon_exception(__PRETTY_FUNCTION__, "artifact already exists");
        definition->artifact_created = true;
    }

    this->definition = definition;
    this->hit_bonus = definition->hit_bonus->roll();
    this->dodge_bonus = definition->dodge_bonus->roll();
    this->defense_bonus = definition->defense_bonus->roll();
    this->weight = definition->weight->roll();
    this->speed_bonus = definition->speed_bonus->roll();
    this->attributes = definition->attributes->roll();
    this->value = definition->value->roll();
    this->next = NULL;
    color_count = 0;
    int i;
    int color_val = definition->color;
    for (i = 0; i < 8; i++) {
        if (color_val & 1) color_count++;
        color_val >>= 1;
    }
}

item_t::~item_t() {
    // Deletes all stacked items too
    if (next != NULL) {
        delete next;
    }
}

uint8_t item_t::next_color() {
    color_i = (color_i + 1) % color_count;
    return current_color();
}

uint8_t item_t::current_color() {
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

int item_t::get_damage() {
    return definition->damage_bonus->roll();
}

void item_t::add_to_stack(item_t *item) {
    item_t *current = this;
    while (current->next != NULL)
        current = current->next;
    current->next = item;
}

item_t *item_t::detach_stack() {
    item_t *removed = next;
    next = NULL;
    return removed;
}

item_t *item_t::remove_next_in_stack() {
    item_t *removed = next;
    if (removed != NULL) next = removed->next;
    else next = NULL;
    return removed;
}

item_t *item_t::next_in_stack() {
    return next;
}

bool item_t::is_stacked() {
    return next != NULL;
}

char item_t::current_symbol() {
    if (next != NULL) return ITEM_TYPES[ITEM_TYPE_STACK];
    else if (definition->type >= 0 && definition->type <= sizeof (ITEM_TYPES) / sizeof (ITEM_TYPES[0]) - 3)
        return ITEM_TYPES[definition->type];
    else return ITEM_TYPES[ITEM_TYPE_UNKNOWN];
}

char item_t::regular_symbol() {
    if (definition->type >= 0 && definition->type <= sizeof (ITEM_TYPES) / sizeof (ITEM_TYPES[0]) - 3)
        return ITEM_TYPES[definition->type];
    else return ITEM_TYPES[ITEM_TYPE_UNKNOWN];
}
