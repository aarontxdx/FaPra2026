#pragma once

#include "Point.hpp"

#include <string>
#include <vector>

struct Road
{
    std::string name;

    std::vector<Point> nodes;
};