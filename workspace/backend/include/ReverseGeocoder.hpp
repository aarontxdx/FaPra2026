#pragma once

#include "AdminArea.hpp"
#include "Building.hpp"
#include "Road.hpp"

#include <optional>

class ReverseGeocoder
{
public:
    ReverseGeocoder(const std::vector<Building> &buildings, const std::vector<AdminArea> &adminAreas, const std::vector<Road> &roads);

    GeocoderObject findNearestObject();

private:
    const std::vector<Building> buildings;
    const std::vector<AdminArea> adminAreas;
    const std::vector<Road> roads;
};