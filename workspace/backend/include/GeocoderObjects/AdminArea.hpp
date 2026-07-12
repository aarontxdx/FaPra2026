#pragma once

#include "Point.hpp"
#include "GeocoderObject.hpp"

#include <array>
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

    std::array<const AdminArea *, 11> parentAreas{};

    int64_t id;

    AdminArea()
    {
        parentAreas.fill(nullptr);
    }
};