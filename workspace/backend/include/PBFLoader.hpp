#pragma once

#include "AdminArea.hpp"
#include "Building.hpp"

#include <string>
#include <vector>

class PBFLoader
{
public:
    std::tuple<std::vector<Building>, std::vector<AdminArea>> extractFile(const std::string &path);
};
