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

        std::string tag = tags.get_value_by_key("highway");

        RoadType type = parseRoadType(tag);

        // filter out ignored tags
        if (isIgnored(tag))
            return;

        Road road;
        road.type = type;
        road.id = way.id();

        if (tags.has_key("name:de"))
            road.name = tags.get_value_by_key("name:de");
        else if (tags.has_key("name"))
            road.name = tags.get_value_by_key("name");
        else
            road.name = "unknown";

        road.nodes.reserve(way.nodes().size());

        for (const auto &n : way.nodes())
        {
            if (!n.location().valid())
                return;

            road.nodes.push_back({n.location().lat(),
                                  n.location().lon()});
        }

        if (road.nodes.size() < 2)
            return;

        roads.push_back(std::move(road));
    }

private:
    std::vector<Road> &roads;

    // Filter
    bool isIgnored(const std::string &t)
    {
        return t == "service" ||
               t == "track" ||
               t == "path" ||
               t == "footway" ||
               t == "cycleway";
    }

    RoadType parseRoadType(const std::string &t)
    {
        if (t == "motorway")
            return RoadType::Motorway;
        if (t == "motorway_link")
            return RoadType::Motorway;

        if (t == "trunk")
            return RoadType::Trunk;
        if (t == "trunk_link")
            return RoadType::Trunk;

        if (t == "primary")
            return RoadType::Primary;
        if (t == "primary_link")
            return RoadType::Primary;

        if (t == "secondary")
            return RoadType::Secondary;
        if (t == "secondary_link")
            return RoadType::Secondary;

        if (t == "tertiary")
            return RoadType::Tertiary;
        if (t == "tertiary_link")
            return RoadType::Tertiary;

        if (t == "residential")
            return RoadType::Residential;
        if (t == "unclassified")
            return RoadType::Unclassified;

        return RoadType::Unknown;
    }
};