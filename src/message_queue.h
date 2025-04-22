/**
A simple, global implementation of a message queue.
This would be associated with the game_t instance, but I'd like to be able to print debugging
 messages even before that exists, so it's structured as a singleton.
*/

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <deque>
#include <string>
#include <vector>

std::string escape_col(std::string inp);

class message_queue_t {
    public:
        static message_queue_t *get() {
            static message_queue_t inst;
            return &inst;
        }

    private:
        message_queue_t() {};
        ~message_queue_t() {};

        std::deque<std::string> messages;

    public:
        void emit(int y, bool sticky);
        void drop();
        void clear();
        void add(std::string message);
};

#endif
