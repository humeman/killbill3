#include "plane_manager.h"

void plane_manager_t::clear() {
    for (const auto &pair : planes) {
        delete pair.second;
    }
    planes.clear();
}

ncpp::Plane *plane_manager_t::get(std::string name, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height) {
    ncpp::Plane *plane = nullptr;
    auto i = planes.find(name);
    if (i != planes.end()) {
        return i->second;
    } else {
        plane = new ncpp::Plane(height, width, y0, x0);
        planes[name] = plane;
        visual_cache[name] = "";
        return plane;
    }
}

ncpp::Plane *plane_manager_t::get(std::string name, ncpp::Plane *parent, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height) {
    ncpp::Plane *plane = nullptr;
    auto i = planes.find(name);
    if (i != planes.end()) {
        return i->second;
    } else {
        plane = new ncpp::Plane(parent, height, width, y0, x0);
        planes[name] = plane;
        visual_cache[name] = "";
        return plane;
    }
}

bool plane_manager_t::cache_set(std::string name, std::string texture) {
    if (visual_cache[name] == texture) return false;
    visual_cache[name] = texture;
    return true;
}

ncpp::Plane *plane_manager_t::get(std::string name) {
    return planes[name];
}

void plane_manager_t::release(std::string name) {
    auto i = planes.find(name);
    if (i != planes.end()) {
        delete i->second;
        planes.erase(i);
    }
}

void plane_manager_t::for_each(std::string prefix, void (*action)(ncpp::Plane *)) {
    for (const auto &pair : planes) {
        if (pair.first.substr(0, prefix.length()) == prefix) {
            action(pair.second);
        }
    }
}