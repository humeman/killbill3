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
    ITEM_TYPE_KEY,
    ITEM_TYPE_STACK,
    ITEM_TYPE_UNKNOWN // This must always be the last one for counting.
} item_type_t;

class ItemDefinition {
    public:
        std::string name;
        std::string description;
        item_type_t type;
        int color;
        Dice *damage_bonus;
        Dice *dodge_bonus;
        Dice *defense_bonus;
        Dice *speed_bonus;
        bool artifact;
        bool artifact_created = false;
        int rarity;
        std::string floor_texture;
        std::string ui_texture;
};

class Item {
    private:
        Item *next;
        int color_count;
        uint8_t color_i = 0;

    public:
        ItemDefinition *definition;
        int dodge_bonus, defense_bonus, speed_bonus;

        Item(ItemDefinition *definition);
        ~Item();

        int get_damage();
        void add_to_stack(Item *item);
        Item *detach_stack();
        Item *next_in_stack();
        Item *remove_next_in_stack();
        bool is_stacked();
        uint8_t next_color();
        uint8_t current_color();
};

#endif
