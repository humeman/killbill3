/**
 * A singleton logger that holds its messages back while Notcurses is on,
 * printing them all out once it stops.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <ncpp/NotCurses.hh>
#include <string>
#include <ncpp/Visual.hh>
#include <filesystem>
#include <deque>

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR // last one for counting
} log_level_t;

class Logger {
    public:
        static Logger *get() {
            static Logger inst;
            return &inst;
        }

        static void debug(const char *location, std::string message) {
            get()->log(LOG_LEVEL_DEBUG, location, message);
        }

        static void info(const char *location, std::string message) {
            get()->log(LOG_LEVEL_INFO, location, message);
        }

        static void warn(const char *location, std::string message) {
            get()->log(LOG_LEVEL_WARN, location, message);
        }

        static void error(const char *location, std::string message) {
            get()->log(LOG_LEVEL_ERROR, location, message);
        }

    private:
        std::deque<std::string> backlog;
        bool is_on = true;
        unsigned long max_backlog = 1000;
        bool levels_enabled[LOG_LEVEL_ERROR + 1];

        Logger() {
            for (int i = 0; i <= LOG_LEVEL_ERROR; i++)
                levels_enabled[i] = true;
        }
        ~Logger() {};

    public:
        void log(log_level_t level, const char *location, std::string path);
        void on(log_level_t level);
        void off(log_level_t level);
        void on();
        void off();
};


#endif