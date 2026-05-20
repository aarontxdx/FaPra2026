#pragma once

#include "AdminArea.hpp"
#include "Building.hpp"
#include "Grid.hpp"
#include "Road.hpp"

#include <unordered_map>
#include <cmath>

class PreProcessingUnit
{
public:
    /**
     * preprocessing of buildings
     *
     * Finds a more representative point for a building
     *
     * (currently there is no filter or function to give
     * a building more information, therefore many buildings
     * are unnamed and partly have no housenumber)
     *
     * @param buildings list of Building objects
     */
    void preprocessBuildings(
        std::vector<Building> &buildings,
        std::vector<AdminArea> &adminAreas,
        Grid &grid);

    /**
     * preprocessing of roads
     *
     * merge roads that have the same name and type
     *
     * TODO: some of the roads doesn't get merged together
     * TODO: (find out if the issue is because of extraction or preprocessing)
     *
     * @param roads list of Road objects
     */
    void preprocessRoads(std::vector<Road> &roads);
};
