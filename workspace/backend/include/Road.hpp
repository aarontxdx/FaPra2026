#pragma once

#include "Point.hpp"

#include <string>
#include <vector>

enum class RoadType
{
    Motorway,
    Trunk,
    Primary,
    Secondary,
    Tertiary,
    Residential,
    Unclassified,
    Unknown
};

struct Road
{
    std::string name;
    RoadType type;
    int64_t id;

    std::vector<Point> nodes;
};

namespace geocoder::objects
{
    inline const char *toString(RoadType t)
    {
        switch (t)
        {
        case RoadType::Motorway:
            return "motorway";
        case RoadType::Trunk:
            return "trunk";
        case RoadType::Primary:
            return "primary";
        case RoadType::Secondary:
            return "secondary";
        case RoadType::Tertiary:
            return "tertiary";
        case RoadType::Residential:
            return "residential";
        case RoadType::Unclassified:
            return "unclassified";
        default:
            return "unknown";
        }
    }
}
