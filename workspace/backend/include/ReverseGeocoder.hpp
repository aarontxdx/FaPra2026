#pragma once

#include "AdminArea.hpp"
#include "Building.hpp"
#include "Grid.hpp"
#include "Road.hpp"

#include <optional>

class ReverseGeocoder
{
public:
    ReverseGeocoder(const std::vector<Building> &buildings,
                    const std::vector<AdminArea> &adminAreas,
                    const std::vector<Road> &roads,
                    const Grid &grid);

    /**
     * find the nearest building to a given point
     *
     * @param lat
     * @param lon
     *
     * @return nearest building to (lat|lon)
     */
    Building findNearestBuilding(double lat, double lon);

private:
    const std::vector<Building> m_buildings;
    const std::vector<AdminArea> m_adminAreas;
    const std::vector<Road> m_roads;
    const Grid m_grid;
};