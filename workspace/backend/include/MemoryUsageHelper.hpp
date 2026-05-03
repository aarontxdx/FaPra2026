#include "Building.hpp"
#include "AdminArea.hpp"
#include "Road.hpp"

#include <iostream>

namespace helper
{
    /**
     * calculates memory usage of a single point
     */
    inline size_t memoryUsage(const Point &p)
    {
        return sizeof(p);
    }

    /**
     * calculates memory usage of a single building
     */
    inline size_t memoryUsage(const Building &b)
    {
        size_t size = sizeof(b);

        size += b.housenumber.capacity();
        size += b.street.capacity();
        size += b.postcode.capacity();
        size += b.city.capacity();
        size += b.country.capacity();
        size += b.name.capacity();

        size += b.polygon.capacity() * sizeof(Point);

        size += sizeof(b.centroid);

        return size;
    }

    /**
     * calculates memory usage of a single administration area
     */
    inline size_t memoryUsage(const AdminArea &a)
    {
        size_t size = sizeof(a);

        size += a.name.capacity();
        size += a.boundary.capacity();

        size += a.area.capacity() * sizeof(std::vector<Point>);

        for (const auto &ring : a.area)
        {
            size += ring.capacity() * sizeof(Point);
        }

        return size;
    }

    /**
     * calculates memory usage of a single road
     */
    inline size_t memoryUsage(const Road &r)
    {
        size_t size = sizeof(r);

        size += r.name.capacity();

        size += r.nodes.capacity() * sizeof(Point);

        return size;
    }

    /**
     * print memory usage of buildings, administration areas and roads
     *
     * @param buildings list of buildings with unknown memory
     * @param adminAreas list of administration areas with unknown memory
     * @param roads list of roads with unknown memory
     */
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

    /**
     * print memory usage of buildings
     *
     * @param buildings list of buildings with unknown memory
     * @param message extra console output
     */
    inline void printMemoryUsageBuildings(std::vector<Building> &buildings, const std::string &message)
    {
        size_t totalMemory = 0;

        for (const auto &b : buildings)
            totalMemory += memoryUsage(b);

        std::cout << "\n"
                  << message << "\nBuildings: " << buildings.size() << "\n"
                  << "Buildings: " << totalMemory / (1024.0 * 1024.0) << " MB\n";
    }

    /**
     * print memory usage of roads
     *
     * @param roads list of roads with unknown memory
     * @param message extra console output
     */
    inline void printMemoryUsageRoads(std::vector<Road> &roads, const std::string &message)
    {
        size_t totalMemory = 0;

        for (const auto &r : roads)
            totalMemory += memoryUsage(r);

        std::cout << "\n"
                  << message << "\nRoads: " << roads.size() << "\n"
                  << "Roads: " << totalMemory / (1024.0 * 1024.0) << " MB\n";
    }
}