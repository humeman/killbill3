#ifndef RANDOM_H
#define RANDOM_H

#include <ostream>
class dice_t {
    private:
        int base, dice, size;

    public:
        dice_t(int base, int dice, int size) {
            this->base = base;
            this->dice = dice;
            this->size = size;
        }

        friend std::ostream &operator<<(std::ostream &o, const dice_t &die) {
            return o << "base=" << std::to_string(die.base) << ", dice=" << std::to_string(die.dice) << ", size=" << std::to_string(die.size);
        }
};

#endif
