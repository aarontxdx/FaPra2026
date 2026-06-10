#pragma once

#include "Point.hpp"
#include "GeocoderObject.hpp"

#include <vector>
#include <memory>
#include <tuple>

struct AdminArea : GeocoderObject
{
    int admin_level;
    std::string boundary;
    std::string postal_code;

    std::vector<std::vector<Point>> area;

    std::tuple<Point, Point> bb;

    int64_t id;
};