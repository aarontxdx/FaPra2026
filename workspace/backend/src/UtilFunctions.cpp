#include "UtilFunctions.hpp"

#include <iostream>
#include <string>
#include <tuple>

namespace
{
    bool pointInBoundingBox(Point &p, AdminArea &area)
    {
        return p.lat >= std::get<0>(area.bb).lat &&
               p.lat <= std::get<1>(area.bb).lat &&
               p.lon >= std::get<0>(area.bb).lon &&
               p.lon <= std::get<1>(area.bb).lon;
    }

    bool isPointInsidePolygon(const Point &p,
                              const std::vector<std::vector<Point>> &polygon)
    {
        bool inside = false;

        for (const std::vector<Point> &polygonPart : polygon)
        {
            size_t j = polygonPart.size() - 1;
            for (size_t i = 0; i < polygonPart.size(); ++i)
            {
                const Point &pi = polygonPart[i];
                const Point &pj = polygonPart[j];

                bool intersect =
                    ((pi.lat > p.lat) != (pj.lat > p.lat)) &&
                    (p.lon < (pj.lon - pi.lon) * (p.lat - pi.lat) /
                                     (pj.lat - pi.lat + 1e-12) +
                                 pi.lon);

                if (intersect)
                    inside = !inside;

                j = i;
            }
        }
        return inside;
    }
}

namespace helper
{
    Point computeCentroid(const std::vector<Point> &poly)
    {
        const size_t n = poly.size();
        if (n < 3)
            return {0.0, 0.0};

        double A = 0.0;
        double Cx = 0.0;
        double Cy = 0.0;

        for (size_t i = 0; i < n; ++i)
        {
            const auto &p1 = poly[i];
            const auto &p2 = poly[(i + 1) % n];

            double cross = p1.lat * p2.lon - p2.lat * p1.lon;

            A += cross;
            Cx += (p1.lon + p2.lon) * cross;
            Cy += (p1.lat + p2.lat) * cross;
        }

        A *= 0.5;

        if (std::abs(A) < 1e-12)
        {
            double sx = 0.0, sy = 0.0;
            for (const auto &p : poly)
            {
                sy += p.lat;
                sx += p.lon;
            }
            return {sx / n, sy / n};
        }

        Cx /= (6.0 * A);
        Cy /= (6.0 * A);

        return {Cx, Cy};
    }

    std::vector<const AdminArea *> pointInPolygon(Point &point, std::vector<AdminArea> &adminAreas)
    {
        std::vector<const AdminArea *> result;

        for (auto &area : adminAreas)
        {
            // BB test
            if (!pointInBoundingBox(point, area))
                continue;
            // polygon test
            if (isPointInsidePolygon(point, area.area))
            {
                result.push_back(&area);
            }
        }
        return result;
    }
}
