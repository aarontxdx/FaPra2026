#pragma once
#include <string>
#include <memory>
#include <osmium/osm/area.hpp>

#include "Point.hpp"

struct AdminArea
{
    std::string name;
    std::string admin_level;
    std::string boundary;

    std::vector<std::vector<Point>> area;
};