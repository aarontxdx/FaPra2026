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
        size_t size = 0;

        // --- Base class ---
        size += sizeof(GeocoderObject);
        size += b.name.capacity();

        // --- vector<Point> polygon ---
        size += sizeof(std::vector<Point>);
        size += b.polygon.capacity() * sizeof(Point);

        // --- centroid ---
        size += sizeof(b.centroid);

        // --- strings ---
        size += sizeof(std::string);
        size += b.housenumber.capacity();

        size += sizeof(std::string);
        size += b.street.capacity();

        size += sizeof(std::string);
        size += b.postcode.capacity();

        size += sizeof(std::string);
        size += b.city.capacity();

        size += sizeof(std::string);
        size += b.country.capacity();

        return size;
    }

    /**
     * calculates memory usage of a single administration area
     */
    inline size_t memoryUsage(const AdminArea &a)
    {
        size_t size = 0;

        // --- Base class ---
        size += sizeof(GeocoderObject);
        size += a.name.capacity();

        // --- AdminArea primitive fields ---
        size += sizeof(a.admin_level);
        size += sizeof(a.id);
        size += a.postal_code.capacity();

        // tuple BB (2 Points)
        size += sizeof(std::get<0>(a.bb));
        size += sizeof(std::get<1>(a.bb));

        // --- boundary string ---
        size += sizeof(std::string);
        size += a.boundary.capacity();

        // --- area (vector<vector<Point>>) ---
        size += sizeof(std::vector<std::vector<Point>>);
        size += a.area.capacity() * sizeof(std::vector<Point>);

        for (const auto &ring : a.area)
        {
            size += sizeof(std::vector<Point>);
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

        // --- Base class ---
        size += sizeof(GeocoderObject);
        size += r.name.capacity();

        // --- Road fields ---
        size += sizeof(r.type);
        size += sizeof(r.id);

        // --- nodes vector ---
        size += sizeof(std::vector<Point>);
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