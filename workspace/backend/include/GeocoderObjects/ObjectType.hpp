#pragma once

namespace geocoder
{
    enum class ObjectType
    {
        Unknown,

        Country,
        State,
        County,
        City,
        Area,

        Road,
        Building,
        Address,

        POI
    };
};