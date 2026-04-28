#pragma once
#include <vector>
#include <string>

#include "Point.hpp"

struct Building
{
    Point centroid;

    std::string housenumber;
    std::string street;
    std::string postcode;
    std::string city;
    std::string country;
    std::string name;
};
