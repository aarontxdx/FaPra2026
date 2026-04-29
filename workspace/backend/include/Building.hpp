#pragma once
#include <vector>
#include <string>

#include "Point.hpp"

struct Building
{
    std::vector<Point> polygon;
    Point centroid;

    std::string housenumber;
    std::string street;
    std::string postcode;
    std::string city;
    std::string country;
    std::string name;
};
