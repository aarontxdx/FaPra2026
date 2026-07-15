#pragma once

#include "GeocoderObjects/SearchObject.hpp"
#include <unordered_set>

namespace geocoder
{
    struct MatchFeatures
    {
        SearchObject object;

        bool exact = false;
        bool prefix = false;
        bool substring = false;

        double ngramScore = 0.0;
        double editScore = 0.0;

        std::size_t queryTokenCount = 0;
        std::size_t matchedQueryTokens = 0;

        std::unordered_set<std::string> matchedTokens;

        bool matchedStreet = false;
        bool matchedHouseNumber = false;

        bool matchedCity = false;
        bool matchedCounty = false;
        bool matchedState = false;
        bool matchedPostcode = false;
    };
}
