#include "ReverseGeocoder.hpp"

#include <iostream>

ReverseGeocoder::ReverseGeocoder(const std::vector<Building> &buildings,
                                 const std::vector<AdminArea> &adminAreas,
                                 const std::vector<Road> &roads,
                                 const Grid &grid)
    : m_buildings(buildings),
      m_adminAreas(adminAreas),
      m_roads(roads),
      m_grid(grid) {}

Building ReverseGeocoder::findNearestBuilding(double lat, double lon)
{
    Point queryPoint{lat, lon};

    // Find start cell
    auto [row, col] = m_grid.getCellCoords(lat, lon);

    Building *nearestBuilding = nullptr;
    double bestDistanceSquared =
        std::numeric_limits<double>::infinity();

    // Search radius expansion
    constexpr int MAX_RADIUS = 5;

    for (int radius = 0; radius <= MAX_RADIUS; radius++)
    {
        std::vector<Building *> candidates;

        m_grid.getNeighborCells(
            row,
            col,
            radius,
            candidates);

        // Skip empty radius
        if (candidates.empty())
            continue;

        for (Building *building : candidates)
        {
            const double dx =
                building->centroid.lon - lon;

            const double dy =
                building->centroid.lat - lat;

            const double distanceSquared =
                dx * dx + dy * dy;

            if (distanceSquared < bestDistanceSquared)
            {
                bestDistanceSquared = distanceSquared;
                nearestBuilding = building;
            }
        }

        // stop when we found something
        if (nearestBuilding != nullptr)
            break;
    }

    if (nearestBuilding == nullptr)
    {
        throw std::runtime_error(
            "No nearby object found");
    }

    return *nearestBuilding;
}