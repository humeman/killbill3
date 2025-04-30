#ifndef ITEM_H
#define ITEM_H

#include <string>

#include "dungeon.h"
#include "random.h"

typedef enum {
    ITEM_TYPE_WEAPON,  // Start of PC-equippable items
    ITEM_TYPE_HAT,     // If more are added, they must be between WEAPON and POCKET
    ITEM_TYPE_SHIRT,
    ITEM_TYPE_PANTS,
    ITEM_TYPE_SHOES,
    ITEM_TYPE_GLASSES,
    ITEM_TYPE_POCKET, // End of PC-equippable items
    ITEM_TYPE_STACK,
    ITEM_TYPE_UNKNOWN // This must always be the last one for counting.
} item_type_t;

class item_definition_t {
    public:
        std::string name;
        std::string description;
        item_type_t type;
        int color;
        dice_t *hit_bonus;
        dice_t *damage_bonus;
        dice_t *dodge_bonus;
        dice_t *defense_bonus;
        dice_t *weight;
        dice_t *speed_bonus;
        dice_t *attributes;
        dice_t *value;
        bool artifact;
        bool artifact_created = false;
        int rarity;
        std::string floor_texture;
        std::string ui_texture;
};

class item_t {
    private:
        item_t *next;
        int color_count;
        uint8_t color_i = 0;

    public:
        item_definition_t *definition;
        int hit_bonus, dodge_bonus, defense_bonus, weight, speed_bonus, attributes, value;

        item_t(item_definition_t *definition);
        ~item_t();

        int get_damage();
        void add_to_stack(item_t *item);
        item_t *detach_stack();
        item_t *next_in_stack();
        item_t *remove_next_in_stack();
        bool is_stacked();
        uint8_t next_color();
        uint8_t current_color();
};

#endif
