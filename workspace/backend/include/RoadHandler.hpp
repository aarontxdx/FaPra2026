#pragma once

#include "Road.hpp"

#include <osmium/handler.hpp>
#include <osmium/osm/way.hpp>

class RoadHandler : public osmium::handler::Handler
{
public:
    explicit RoadHandler(std::vector<Road> &out)
        : roads(out) {}

    void way(const osmium::Way &way) noexcept
    {
        const auto &tags = way.tags();

        if (!tags.has_key("highway"))
            return;

        Road road;

        if (tags.has_key("name:de"))
            road.name = tags.get_value_by_key("name:de");
        else if (tags.has_key("name"))
            road.name = tags.get_value_by_key("name", "");

        for (const auto &n : way.nodes())
        {
            if (!n.location().valid())
                return;
            road.nodes.push_back({n.location().lon(), n.location().lat()});
        }

        roads.push_back(std::move(road));
    }

private:
    std::vector<Road> &roads;
};