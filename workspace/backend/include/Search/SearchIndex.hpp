#pragma once

#include <string>
#include <vector>
#include <cstddef>

#include "GeocoderObjects/Building.hpp"

namespace geocoder
{
    namespace search
    {

        struct SearchHit
        {
            size_t id;
            double score;
        };

        class SearchIndex
        {
        public:
            virtual void build(const std::vector<std::string> &items) = 0;
            virtual std::vector<SearchHit> query(const std::string &q, int maxResults = 10) = 0;
            virtual ~SearchIndex() = default;
        };

    }
} // namespace geocoder::search
