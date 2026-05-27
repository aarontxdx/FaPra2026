#pragma once

#include "AdminArea.hpp"
#include "AdminHierarchy.hpp"
#include "Building.hpp"
#include "Grid.hpp"
#include "Road.hpp"

#include <unordered_map>
#include <cmath>

class PreProcessingUnit
{
public:
    /**
     * preprocessing of admin areas
     *
     * Creates a hierarchy for all areas
     *
     * @param adminAreas list of all admin areas
     * @param resultAdminAreas the resulting data structure
     */
    void preprocessAdminAreas(
        std::vector<AdminArea> &adminAreas,
        AdminHierarchy &adminHierarchy);

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
        AdminHierarchy &adminHierarchy,
        Grid &grid);

    /**
     * preprocessing of roads
     *
     * merge roads that have the same name and type
     *
     * @param roads list of Road objects
     */
    void preprocessRoads(std::vector<Road> &roads, AdminHierarchy &adminHierarchy);
};
