#pragma once

#include "Building.hpp"
#include "Road.hpp"

class PreProcessingUnit
{
public:
    void preprocessBuildings(std::vector<Building> &buildings);
    void preprocessRoads(std::vector<Road> &roads);
};