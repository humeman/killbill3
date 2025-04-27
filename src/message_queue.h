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
#include <ncpp/NotCurses.hh>

std::string escape_col(std::string inp);

class message_queue_t {
    private:
        static message_queue_t *instance;
    
    public:
        static message_queue_t *get() {
            if (!instance) instance = new message_queue_t();
            return instance;
        }
        static void destroy() {
            if (!instance) return;
            delete instance;
            instance = nullptr;
        }

    private:
        message_queue_t() {};
        ~message_queue_t() {};

        std::deque<std::string> messages;

    public:
        void emit(ncpp::Plane &plane, bool sticky);
        void drop();
        void clear();
        void add(std::string message);
};

#endif
