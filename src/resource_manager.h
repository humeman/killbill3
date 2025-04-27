/**
 * A singleton resource manager to load single instances of all of the
 * game's textures.
 */
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <ncpp/NotCurses.hh>
#include <string>
#include <filesystem>
#include <vector>
#include <ncpp/Visual.hh>

#define DEFAULT_VISUAL "error"

class resource_manager_t {
    private:
        static resource_manager_t *instance;
    
    public:
        static resource_manager_t *get() {
            if (!instance) instance = new resource_manager_t();
            return instance;
        }
        static void destroy() {
            if (!instance) return;
            delete instance;
            instance = nullptr;
        }

    private:
        std::map<std::string, ncpp::Visual *> visuals;
        std::vector<std::string> errored_visuals;
        bool loaded = false;

        resource_manager_t();
        ~resource_manager_t();
        void add_visual_path(const std::filesystem::path &dir, std::string prefix);

    public:
        void load(std::string path);
        ncpp::Visual *get_visual(std::string name);
};

#endif