#pragma once

#include "AdminArea.hpp"
#include "Building.hpp"
#include "Road.hpp"

#include <string>
#include <vector>

class PBFLoader
{
public:
    std::tuple<std::vector<Building>, std::vector<AdminArea>, std::vector<Road>>
    extractFile(const std::string &path);
};
