#include "BuildingHandler.hpp"
#include <iostream>

BuildingHandler::BuildingHandler(std::vector<Building> &out)
    : buildings(out) {}

Point BuildingHandler::computeCentroid(const std::vector<Point> &poly) const
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

    if (A == 0.0)
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

void BuildingHandler::way(const osmium::Way &way) noexcept
{
    const auto &tags = way.tags();

    if (!tags.has_key("building"))
        return;

    if (!way.is_closed())
        return;

    std::vector<Point> poly;
    poly.reserve(way.nodes().size());

    for (const auto &n : way.nodes())
    {
        if (!n.location().valid())
            return;
        poly.push_back({n.location().lon(), n.location().lat()});
    }

    if (poly.size() < 4)
        return;

    Building b;
    b.centroid = computeCentroid(poly);

    b.housenumber = tags.get_value_by_key("addr:housenumber", "");
    b.street = tags.get_value_by_key("addr:street", "");
    b.postcode = tags.get_value_by_key("addr:postcode", "");
    b.city = tags.get_value_by_key("addr:city", "");
    b.country = tags.get_value_by_key("addr:country", "");

    if (tags.has_key("name:de"))
        b.name = tags.get_value_by_key("name:de");
    else if (tags.has_key("name"))
        b.name = tags.get_value_by_key("name");

    buildings.push_back(std::move(b));
}

void BuildingHandler::area(const osmium::Area &area) noexcept
{
    const auto &tags = area.tags();

    if (!tags.has_key("building"))
        return;

    std::vector<Point> poly;

    for (const auto &outer : area.outer_rings())
    {
        for (const auto &n : outer)
        {
            if (!n.location().valid())
                continue;
            poly.push_back({n.location().lon(), n.location().lat()});
        }
    }

    if (poly.size() < 4)
        return;

    Building b;
    b.centroid = computeCentroid(poly);

    b.housenumber = tags.get_value_by_key("addr:housenumber", "");
    b.street = tags.get_value_by_key("addr:street", "");
    b.postcode = tags.get_value_by_key("addr:postcode", "");
    b.city = tags.get_value_by_key("addr:city", "");
    b.country = tags.get_value_by_key("addr:country", "");

    if (tags.has_key("name:de"))
        b.name = tags.get_value_by_key("name:de");
    else if (tags.has_key("name"))
        b.name = tags.get_value_by_key("name");

    buildings.push_back(std::move(b));
}

void BuildingHandler::node(const osmium::Node &node) noexcept {}