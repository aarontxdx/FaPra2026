#pragma once

#include "Point.hpp"
#include "GeocoderObject.hpp"

#include <vector>

struct Building : GeocoderObject
{
    std::vector<Point> polygon;
    Point centroid;

    std::string housenumber;
    std::string street;
    std::string postcode;
    std::string city;
    std::string country;
};
