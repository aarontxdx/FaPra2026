#include "PreProcessingUnit.hpp"

#include "MemoryUsageHelper.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/point_on_surface.hpp>

namespace
{
    namespace bg = boost::geometry;

    using BoostPoint = bg::model::point<double, 2, bg::cs::cartesian>;

    Point representativePoint(const std::vector<Point> &poly)
    {
        namespace bg = boost::geometry;

        bg::model::polygon<BoostPoint> polygon;

        for (const auto &p : poly)
            bg::append(polygon.outer(), BoostPoint(p.x, p.y));

        bg::correct(polygon);

        BoostPoint result;
        bg::point_on_surface(polygon, result);

        return {result.get<0>(), result.get<1>()};
    }
}

void PreProcessingUnit::preprocessBuildings(std::vector<Building> &buildings)
{
    helper::printMemoryUsageBuildings(buildings, "Memory of Buildings before Preprocessing:");

    for (auto &building : buildings)
    {
        building.centroid = representativePoint(building.polygon);
        building.polygon.clear();
    }

    helper::printMemoryUsageBuildings(buildings, "Memory of Buildings after Preprocessing:");
}