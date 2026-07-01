#include "GeocoderObjects/Building.hpp"
#include "GeocoderObjects/AdminArea.hpp"
#include "GeocoderObjects/Road.hpp"

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
        size_t size = 0;

        // --- complete object itself ---
        size += sizeof(Building);

        // --- dynamically allocated string memory ---
        size += b.name.capacity();

        size += b.country.capacity();
        size += b.state.capacity();
        size += b.county.capacity();
        size += b.city.capacity();
        size += b.postcode.capacity();

        size += b.housenumber.capacity();
        size += b.street.capacity();

        // --- polygon vector heap memory ---
        size += b.polygon.capacity() * sizeof(Point);

        // --- adminAreas vector heap memory ---
        size += b.adminAreas.capacity() * sizeof(AdminArea *);

        return size;
    }

    /**
     * calculates memory usage of a single administration area
     */
    inline size_t memoryUsage(const AdminArea &a)
    {
        size_t size = 0;

        // --- complete object itself ---
        size += sizeof(AdminArea);

        // --- dynamically allocated string memory ---
        size += a.name.capacity();

        size += a.country.capacity();
        size += a.state.capacity();
        size += a.county.capacity();
        size += a.city.capacity();
        size += a.postcode.capacity();

        size += a.postal_code.capacity();
        size += a.boundary.capacity();

        // --- outer vector heap memory ---
        size += a.area.capacity() * sizeof(std::vector<Point>);

        // --- inner ring heap memory ---
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
        size_t size = 0;

        // --- complete object itself ---
        size += sizeof(Road);

        // --- dynamically allocated string memory ---
        size += r.name.capacity();

        size += r.country.capacity();
        size += r.state.capacity();
        size += r.county.capacity();
        size += r.city.capacity();
        size += r.postcode.capacity();

        // --- nodes vector heap memory ---
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