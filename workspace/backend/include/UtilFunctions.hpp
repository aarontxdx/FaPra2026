#pragma once

#include "Building.hpp"
#include "AddressNode.hpp"

#include <vector>
#include <string>

namespace helper
{
    inline Point computeCentroid(const std::vector<Point> &poly)
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

            double cross = p1.x * p2.y - p2.x * p1.y;

            A += cross;
            Cx += (p1.x + p2.x) * cross;
            Cy += (p1.y + p2.y) * cross;
        }

        A *= 0.5;

        if (std::abs(A) < 1e-12)
        {
            // Fallback: Mittelwert
            double sx = 0.0, sy = 0.0;
            for (const auto &p : poly)
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