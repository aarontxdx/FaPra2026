#pragma once

#include "GeocoderObjects/Point.hpp"
#include "GeocoderObjects/AdminArea.hpp"

#include <vector>

namespace helper
{
    /**
     * computes a centroid for a given polygon
     *
     * TODO: Not working like expected. Have another look on this function
     */
    Point computeCentroid(const std::vector<Point> &poly);

    /**
     * Computes a point in polygon test for a given point and a list of AdminAreas
     *
     * @param point point to test
     * @param adminAreas polygons to test
     *
     * @return list of AdminArea pointers which include the given point
     */
    std::vector<const AdminArea *> pointInPolygon(
        const Point &point,
        const std::vector<AdminArea *> &areas);

    /**
     * This function updates the boundingbox for a given Point
     */
    void updateObjectBoundingBox(Point &centroid, std::tuple<Point, Point> &GeocoderObjectBB);
}