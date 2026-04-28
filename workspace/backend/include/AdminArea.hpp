#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Point.hpp"

struct AdminArea
{
    std::string name;
    int admin_level;
    std::string boundary;

    std::vector<std::vector<Point>> area;

    int64_t id;
};