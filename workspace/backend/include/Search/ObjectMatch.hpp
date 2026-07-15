#pragma once

#include <string>
#include <unordered_set>

namespace geocoder
{
    struct ObjectMatch
    {
        double bestScore = 0.0;

        std::unordered_set<std::string> matchedTokens;

        bool matchedArea = false;

        bool matchedName = false;

        bool matchedCountry = false;
        bool matchedState = false;
        bool matchedCounty = false;
        bool matchedCity = false;
        bool matchedPostcode = false;

        bool matchedStreet = false;
        bool matchedHouseNumber = false;
    };
}