#pragma once

#include "Point.hpp"
#include "AdminArea.hpp"

#include <vector>;

namespace helper
{
    /**
     * computes a centroid for a given polygon
     *
     * TODO: Not working like expected. Have another look on this function
     */
    Point computeCentroid(const std::vector<Point> &poly);

    /**
     * TODO: Add params to filter out areas befor the bb test (If building has a postal code for example)
     *
     * Computes a point in polygon test for a given point and a list of AdminAreas
     *
     * @param point point to test
     * @param adminAreas polygons to test
     *
     * @return list of AdminArea pointers which include the given point
     */
    std::vector<const AdminArea *> pointInPolygon(Point &point, std::vector<AdminArea> &adminAreas);
}