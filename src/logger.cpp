#include "logger.h"
#include <iostream>

const char *log_level_names[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR"
};

void Logger::log(log_level_t level, const char *location, std::string log) {
    if (!levels_enabled[level]) return;
    std::ostringstream rendered_log;
    rendered_log << log_level_names[level] << " @ " << location << ": " << log;
    std::string rendered_log_str = rendered_log.str();
    if (!is_on) {
        // Add to backlog
        if (backlog.size() >= max_backlog) {
            backlog.pop_front();
        }
        backlog.push_back(rendered_log_str);
        return;
    }
    std::cout << rendered_log_str << std::endl;
}

void Logger::off() {
    if (!is_on) return;
    log(LOG_LEVEL_INFO, "logger", "logger disabled");
    is_on = false;
}

void Logger::on() {
    if (is_on) return;
    is_on = true;
    std::string backlog_log;
    log(LOG_LEVEL_INFO, "logger", "beginning message replay");
    while (!backlog.empty()) {
        backlog_log = backlog.front();
        backlog.pop_front();
        std::cout << backlog_log << std::endl;
    }
    log(LOG_LEVEL_INFO, "logger", "end of message replay");
    log(LOG_LEVEL_INFO, "logger", "logger re-enabled");
}

void Logger::on(log_level_t level) {
    levels_enabled[level] = true;
}

void Logger::off(log_level_t level) {
    levels_enabled[level] = false;
}