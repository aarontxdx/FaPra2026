#pragma once

#include <string>

struct GeocoderObject
{
    std::string name;

    std::string country;
    std::string state;
    std::string county;
    std::string city;
    std::string postcode;
};