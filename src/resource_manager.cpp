#include "resource_manager.h"
#include "macros.h"
#include "logger.h"

#include <algorithm>
#include <filesystem>


ResourceManager *ResourceManager::instance = nullptr;

ResourceManager::ResourceManager() {}
ResourceManager::~ResourceManager() {
    for (const auto &pair : visuals) {
        delete pair.second;
    }
    for (const auto &pair : music) {
        delete pair.second;
    }
}

void ResourceManager::load_visuals(std::string path) {
    std::filesystem::path dir(path);
    if (!std::filesystem::is_directory(dir))
        throw dungeon_exception(__PRETTY_FUNCTION__, "path is not a directory");

    Logger::debug(__FILE__, "loading visuals");
    add_visual_path(dir, "");

    if (!visuals.count(DEFAULT_VISUAL)) {
        throw dungeon_exception(__PRETTY_FUNCTION__, "default visual doesn't exist");
    }

    Logger::debug(__FILE__, "visuals loaded");
    loaded = true;
}

void ResourceManager::add_visual_path(const std::filesystem::path &dir, std::string prefix) {
    Logger::debug(__FILE__, "traversing visuals in " + dir.filename().string());
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            std::string filename = prefix + entry.path().filename().string();
            unsigned long dot = filename.find_last_of(".");
            if (dot != std::string::npos) {
                filename = filename.substr(0, dot);
            }
            ncpp::Visual *visual = new ncpp::Visual(entry.path().string().c_str());
            visuals[filename] = visual;
            Logger::info(__FILE__, "loaded visual " + filename);
        }
        else if (entry.is_directory()) {
            add_visual_path(entry, prefix + entry.path().filename().string() + "_");
        }
    }
}

ncpp::Visual *ResourceManager::get_visual(std::string name) {
    if (!loaded)
        throw dungeon_exception(__PRETTY_FUNCTION__, "no visuals loaded");
    try {
        return visuals.at(name);
    } catch (const std::out_of_range &e) {
        // we only want to log these once
        if (std::find(errored_visuals.begin(), errored_visuals.end(), name) == errored_visuals.end()) {
            errored_visuals.push_back(name);
            Logger::warn(__FILE__, "visual " + name + " does not exist");
        }
        return visuals.at(DEFAULT_VISUAL);
    }
}

void ResourceManager::load_music(std::string path) {
    std::filesystem::path dir(path);
    if (!std::filesystem::is_directory(dir))
        throw dungeon_exception(__PRETTY_FUNCTION__, "path is not a directory");

    Logger::debug(__FILE__, "loading music");
    add_music_path(dir, "");

    Logger::debug(__FILE__, "music loaded");
    loaded = true;
}

void ResourceManager::add_music_path(const std::filesystem::path &dir, std::string prefix) {
    Logger::debug(__FILE__, "traversing music in " + dir.filename().string());
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            std::string filename = prefix + entry.path().filename().string();
            unsigned long dot = filename.find_last_of(".");
            if (dot != std::string::npos) {
                filename = filename.substr(0, dot);
            }
            sf::Music *m = new sf::Music;
            if (!m->openFromFile(entry.path().string())) {
                throw dungeon_exception(__PRETTY_FUNCTION__, "failed to read music file " + entry.path().string());
            }
            m->setVolume(60);
            music[filename] = m;
            Logger::info(__FILE__, "loaded music " + filename);
        }
        else if (entry.is_directory()) {
            add_music_path(entry, prefix + entry.path().filename().string() + "_");
        }
    }
}

sf::Music *ResourceManager::get_music(std::string name) {
    if (!loaded)
        throw dungeon_exception(__PRETTY_FUNCTION__, "no music loaded");
    try {
        return music.at(name);
    } catch (const std::out_of_range &e) {
        // we only want to log these once
        if (std::find(errored_music.begin(), errored_music.end(), name) == errored_music.end()) {
            errored_music.push_back(name);
            Logger::warn(__FILE__, "music " + name + " does not exist");
        }
        return music.at(DEFAULT_VISUAL);
    }
}