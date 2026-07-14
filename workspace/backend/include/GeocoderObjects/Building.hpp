#pragma once

#include "AdminArea.hpp"
#include "Point.hpp"

#include <vector>

struct Building : GeocoderObject
{
    Point centroid;

    std::string housenumber;
    std::string street;

    std::vector<AdminArea *> adminAreas;
};
