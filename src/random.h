#ifndef RANDOM_H
#define RANDOM_H

#include <ostream>
class dice_t {
    private:
        int base, dice, sides;

    public:
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

        friend std::ostream &operator<<(std::ostream &o, const dice_t &dice) {
            return o << std::to_string(dice.base) << "+" << std::to_string(dice.dice) << "d" << std::to_string(dice.sides);
        }
};

#endif
