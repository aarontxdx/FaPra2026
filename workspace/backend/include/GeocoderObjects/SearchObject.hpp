#pragma once

#include "Building.hpp"
#include "Road.hpp"
#include "AdminArea.hpp"
#include "ObjectType.hpp"

#include <variant>
#include <unordered_map>

namespace geocoder
{
    using SearchObject = std::variant<
        AdminArea *,
        Building *,
        Road *>;

    struct IndexEntry
    {
        SearchObject object;
        int weight;
    };

    struct NGramRecord
    {
        std::string token;
    };

    using ReverseIndex = std::unordered_map<std::string, std::vector<IndexEntry>>;
}