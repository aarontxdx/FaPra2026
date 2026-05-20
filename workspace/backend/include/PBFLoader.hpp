#pragma once

#include "AdminArea.hpp"
#include "Building.hpp"
#include "Road.hpp"

#include <string>
#include <vector>

class PBFLoader
{
public:
    /**
     * extract a given pbf file
     *
     * @param path file path of .pbf
     */
    void extractFile(std::vector<Building> &buildings,
                     std::vector<AdminArea> &adminAreas,
                     std::vector<Road> &roads, const std::string &path);
};
