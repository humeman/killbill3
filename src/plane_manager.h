#ifndef PLANE_MANAGER_H
#define PLANE_MANAGER_H

#include <map>
#include <ncpp/NotCurses.hh>

class PlaneManager {
    private:
        std::map<std::string, ncpp::Plane *> planes;
        std::map<std::string, std::string> visual_cache;
        ncpp::NotCurses *nc;

    public:
        PlaneManager(ncpp::NotCurses *nc) {
            this->nc = nc;
        }
        ~PlaneManager() {
            clear();
        }
        
        ncpp::Plane *get(std::string name, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height);
        ncpp::Plane *get(std::string name, ncpp::Plane *parent, unsigned int x0, unsigned int y0, unsigned int width, unsigned int height);
        ncpp::Plane *get(std::string name);
        bool cache_set(std::string name, std::string texture);
        void release(std::string name);
        void for_each(std::string prefix, void (*action)(ncpp::NotCurses *, ncpp::Plane *));
        void clear();
};

#endif