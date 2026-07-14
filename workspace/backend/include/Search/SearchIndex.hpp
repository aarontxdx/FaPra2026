#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <utility>

#include "GeocoderObjects/SearchObject.hpp"

namespace geocoder::search
{
    struct SearchHit
    {
        SearchObject object;
        double score;
    };

    class SearchIndex
    {
    public:
        virtual void build(
            const ReverseIndex &reverseIndex) = 0;

        virtual std::vector<MatchFeatures> query(
            const std::string &q,
            int maxResults = 10) = 0;

        virtual size_t memoryUsage() const = 0;

        virtual ~SearchIndex() = default;
    };
}