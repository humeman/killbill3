#ifndef RANDOM_H
#define RANDOM_H

#include <ostream>
class dice_t {
    public:
        int base, dice, sides;
        dice_t(int base, int dice, int sides) {
            this->base = base;
            this->dice = dice;
            this->sides = sides;
        }

        int roll() {
            int res = base;
            int i;
            for (i = 0; i < dice; i++)
                res += rand() % sides + 1;
            return res;
        }

        std::string str() const {
            return std::to_string(base) + "+" + std::to_string(dice) + "d" + std::to_string(sides);
        }

        friend std::ostream &operator<<(std::ostream &o, const dice_t &dice) {
            return o << dice.str();
        }
};

#endif
