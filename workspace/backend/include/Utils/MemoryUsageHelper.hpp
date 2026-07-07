#pragma once

#include "GeocoderObjects/Building.hpp"
#include "GeocoderObjects/AdminArea.hpp"
#include "GeocoderObjects/Road.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace helper
{

    // =======================================================
    // Utility
    // =======================================================

    inline double toMB(size_t bytes)
    {
        return static_cast<double>(bytes) / (1024.0 * 1024.0);
    }

    // =======================================================
    // Single Objects
    // =======================================================

    inline size_t memoryUsage(const Point &p)
    {
        return sizeof(Point);
    }

    inline size_t memoryUsage(const Building &b)
    {
        size_t size = sizeof(Building);

        size += b.name.capacity();

        size += b.country.capacity();
        size += b.state.capacity();
        size += b.county.capacity();
        size += b.city.capacity();
        size += b.postcode.capacity();

        size += b.housenumber.capacity();
        size += b.street.capacity();

        size += b.polygon.capacity() * sizeof(Point);
        size += b.adminAreas.capacity() * sizeof(AdminArea *);

        return size;
    }

    inline size_t memoryUsage(const AdminArea &a)
    {
        size_t size = sizeof(AdminArea);

        size += a.name.capacity();

        size += a.country.capacity();
        size += a.state.capacity();
        size += a.county.capacity();
        size += a.city.capacity();
        size += a.postcode.capacity();

        size += a.postal_code.capacity();
        size += a.boundary.capacity();

        size += a.area.capacity() * sizeof(std::vector<Point>);

        for (const auto &ring : a.area)
            size += ring.capacity() * sizeof(Point);

        return size;
    }

    inline size_t memoryUsage(const Road &r)
    {
        size_t size = sizeof(Road);

        size += r.name.capacity();

        size += r.country.capacity();
        size += r.state.capacity();
        size += r.county.capacity();
        size += r.city.capacity();
        size += r.postcode.capacity();

        size += r.nodes.capacity() * sizeof(Point);

        return size;
    }

    // =======================================================
    // Generic Vector Memory
    // =======================================================

    template <typename T>
    inline size_t memoryUsage(const std::vector<T> &vec)
    {
        size_t total = vec.capacity() * sizeof(T);

        for (const auto &e : vec)
            total += memoryUsage(e) - sizeof(T);

        return total;
    }

    // =======================================================
    // Generic Printer
    // =======================================================

    template <typename T>
    inline void printMemoryUsage(const std::vector<T> &vec,
                                 const std::string &name)
    {
        std::cout << "\n";
        std::cout << name << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Count  : " << vec.size() << "\n";
        std::cout << "Memory : "
                  << std::fixed
                  << std::setprecision(2)
                  << toMB(memoryUsage(vec))
                  << " MB\n";
    }

    // =======================================================
    // Complete Summary
    // =======================================================

    inline void printMemoryUsage(const std::vector<Building> &buildings,
                                 const std::vector<AdminArea> &adminAreas,
                                 const std::vector<Road> &roads)
    {
        size_t buildingMemory = memoryUsage(buildings);
        size_t adminMemory = memoryUsage(adminAreas);
        size_t roadMemory = memoryUsage(roads);

        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "Memory Summary\n";
        std::cout << "========================================\n\n";

        std::cout << std::left
                  << std::setw(18) << "Buildings"
                  << std::setw(12) << buildings.size()
                  << std::fixed << std::setprecision(2)
                  << toMB(buildingMemory) << " MB\n";

        std::cout << std::left
                  << std::setw(18) << "Admin Areas"
                  << std::setw(12) << adminAreas.size()
                  << toMB(adminMemory) << " MB\n";

        std::cout << std::left
                  << std::setw(18) << "Roads"
                  << std::setw(12) << roads.size()
                  << toMB(roadMemory) << " MB\n";

        std::cout << "----------------------------------------\n";

        std::cout << std::left
                  << std::setw(30) << "Total"
                  << toMB(buildingMemory + adminMemory + roadMemory)
                  << " MB\n\n";
    }

    inline void printMemoryUsageBuildings(
        const std::vector<Building> &buildings,
        const std::string &title)
    {
        std::cout << "\n"
                  << title << "\n";
        std::cout << "Count: " << buildings.size() << "\n";
        std::cout << "Memory: "
                  << memoryUsage(buildings) / (1024.0 * 1024.0)
                  << " MB\n";
    }

    inline void printMemoryUsageRoads(
        const std::vector<Road> &roads,
        const std::string &title)
    {
        std::cout << "\n"
                  << title << "\n";
        std::cout << "Count: " << roads.size() << "\n";
        std::cout << "Memory: "
                  << memoryUsage(roads) / (1024.0 * 1024.0)
                  << " MB\n";
    }

    inline void printMemoryUsageAdminAreas(
        const std::vector<AdminArea> &areas,
        const std::string &title)
    {
        std::cout << "\n"
                  << title << "\n";
        std::cout << "Count: " << areas.size() << "\n";
        std::cout << "Memory: "
                  << memoryUsage(areas) / (1024.0 * 1024.0)
                  << " MB\n";
    }
} // namespace helper