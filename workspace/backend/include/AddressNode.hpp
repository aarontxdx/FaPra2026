#pragma once

#include "Point.hpp"

#include <string>

struct AddressNode
{
    Point location;

    std::string housenumber;
    std::string street;
    std::string postcode;
    std::string city;
    std::string country;
    std::string name;
};