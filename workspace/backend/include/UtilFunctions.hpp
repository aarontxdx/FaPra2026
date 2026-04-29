#pragma once
#include "Building.hpp"

#include <vector>
#include <string>

namespace helper
{
    inline Point computeCentroid(const std::vector<Point> &poly)
    {
        double A = 0.0;
        double Cx = 0.0;
        double Cy = 0.0;

        const int n = static_cast<int>(poly.size());
        if (n == 0)
        {
            return {0.0, 0.0};
        }

        for (int i = 0; i < n; ++i)
        {
            const Point &p1 = poly[i];
            const Point &p2 = poly[(i + 1) % n];

            double cross = p1.x * p2.y - p2.x * p1.y;
            A += cross;
            Cx += (p1.x + p2.x) * cross;
            Cy += (p1.y + p2.y) * cross;
        }

        A *= 0.5;

        if (std::abs(A) < 1e-12)
        {
            double sx = 0.0, sy = 0.0;
            for (auto &p : poly)
            {
                sx += p.x;
                sy += p.y;
            }
            return {sx / n, sy / n};
        }

        Cx /= (6.0 * A);
        Cy /= (6.0 * A);

        return {Cx, Cy};
    }
}