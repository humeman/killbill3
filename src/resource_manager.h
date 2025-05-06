/**
 * A singleton resource manager to load single instances of all of the
 * game's textures.
 */
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <ncpp/NotCurses.hh>
#include <SFML/Audio.hpp>
#include <string>
#include <filesystem>
#include <vector>
#include <ncpp/Visual.hh>

#define DEFAULT_VISUAL "error"
#define DEFAULT_MUSIC "intro_0"

class ResourceManager {
    private:
        static ResourceManager *instance;
    
    public:
        static ResourceManager *get() {
            if (!instance) instance = new ResourceManager();
            return instance;
        }
        static void destroy() {
            if (!instance) return;
            delete instance;
            instance = nullptr;
        }

        bool quiet = false;

    private:
        std::map<std::string, ncpp::Visual *> visuals;
        std::map<std::string, sf::Music *> music;
        std::vector<std::string> errored_visuals;
        std::vector<std::string> errored_music;
        bool loaded = false;

        ResourceManager();
        ~ResourceManager();
        void add_visual_path(const std::filesystem::path &dir, std::string prefix);
        void add_music_path(const std::filesystem::path &dir, std::string prefix);

    public:
        void load_visuals(std::string path);
        void load_music(std::string path);
        ncpp::Visual *get_visual(std::string name);
        void play_music(std::string name);
};

#endif