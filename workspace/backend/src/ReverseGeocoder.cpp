#include "ReverseGeocoder.hpp"

#include <iostream>

namespace
{

}

ReverseGeocoder::ReverseGeocoder(const std::vector<Building> &buildings,
                                 const std::vector<AdminArea> &adminAreas,
                                 const std::vector<Road> &roads)
    : buildings(buildings),
      adminAreas(adminAreas),
      roads(roads) {}

GeocoderObject ReverseGeocoder::findNearestObject()
{
    std::cout << "Nearest Object !!!" << std::endl;
    return buildings[0];
}