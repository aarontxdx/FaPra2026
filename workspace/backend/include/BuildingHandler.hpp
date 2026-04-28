#pragma once

#include "UtilFunctions.hpp"

#include <osmium/handler.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/osm/node.hpp>

class BuildingHandler : public osmium::handler::Handler
{
public:
    explicit BuildingHandler(std::vector<Building> &out)
        : buildings(out) {}

    void way(const osmium::Way &way) noexcept
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
        b.centroid = utilfunctions::computeCentroid(poly);

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

private:
    std::vector<Building> &buildings;
};
