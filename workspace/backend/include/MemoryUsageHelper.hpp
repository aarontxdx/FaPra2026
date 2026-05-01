#include "Building.hpp"
#include "AdminArea.hpp"
#include "Road.hpp"

#include <iostream>

namespace helper
{
    inline size_t memoryUsage(const Point &p)
    {
        return sizeof(p);
    }

    inline size_t memoryUsage(const Building &b)
    {
        size_t size = sizeof(b);

        size += b.housenumber.size();
        size += b.street.size();
        size += b.postcode.size();
        size += b.city.size();
        size += b.country.size();
        size += b.name.size();

        size += b.polygon.size() * sizeof(Point);

        return size;
    }

    inline size_t memoryUsage(const AdminArea &a)
    {
        size_t size = sizeof(a);

        size += a.name.size();
        size += a.boundary.size();

        for (const auto &ring : a.area)
        {
            size += ring.size() * sizeof(Point);
        }

        return size;
    }

    inline size_t memoryUsage(const Road &r)
    {
        size_t size = sizeof(r);

        size += r.name.size();
        size += r.nodes.size() * sizeof(Point);

        return size;
    }

    inline void printMemoryUsage(std::vector<Building> &buildings, std::vector<AdminArea> &adminAreas, std::vector<Road> &roads)
    {
        size_t totalBuildings = 0;
        size_t totalAdminAreas = 0;
        size_t totalRoads = 0;

        // Buildings
        for (const auto &b : buildings)
            totalBuildings += memoryUsage(b);

        // AdminAreas
        for (const auto &a : adminAreas)
            totalAdminAreas += memoryUsage(a);

        // Roads
        for (const auto &r : roads)
            totalRoads += memoryUsage(r);

        std::cout << "Buildings: " << buildings.size() << "\n"
                  << "Areas: " << adminAreas.size() << "\n"
                  << "Roads: " << roads.size() << "\n\n"
                  << "Memory usage:\n"
                  << "Buildings: " << totalBuildings / (1024.0 * 1024.0) << " MB\n"
                  << "Admin areas: " << totalAdminAreas / (1024.0 * 1024.0) << " MB\n"
                  << "Roads: " << totalRoads / (1024.0 * 1024.0) << " MB\n\n";
    }

    inline void printMemoryUsageBuildings(std::vector<Building> &buildings, const std::string &message)
    {
        size_t totalMemory = 0;

        for (const auto &b : buildings)
            totalMemory += memoryUsage(b);

        std::cout << message << "\nBuildings: " << buildings.size() << "\n"
                  << "Buildings: " << totalMemory / (1024.0 * 1024.0) << " MB\n";
    }
}