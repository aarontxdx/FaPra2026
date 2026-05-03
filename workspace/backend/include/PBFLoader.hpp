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
     *
     * @return list of Buildings, Administration Areas and Roads
     */
    std::tuple<std::vector<Building>, std::vector<AdminArea>, std::vector<Road>>
    extractFile(const std::string &path);
};
