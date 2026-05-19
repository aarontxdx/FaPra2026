#pragma once

#include "AdminArea.hpp"
#include "Point.hpp"

#include <vector>

struct Building : GeocoderObject
{
    std::vector<Point> polygon;
    Point centroid;

    std::string housenumber;
    std::string street;

    std::string country;
    std::string state;
    std::string county;
    std::string city;
    std::string postcode;

    std::vector<AdminArea *> adminAreas;
};
