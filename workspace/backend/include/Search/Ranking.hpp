#pragma once

#include "GeocoderObjects/SearchObject.hpp"
#include "Search/ObjectMatch.hpp"
#include "Search/MatchFeature.hpp"

#include <cstddef>
#include <unordered_set>

namespace geocoder::search
{
    class Ranking
    {
    public:
        Ranking() = default;

        /**
         * Evaluates the quality of the match between the query and the object.
         */
        double matchScore(
            const MatchFeatures &features) const;

        /**
         * Evaluates how well the object type matches the query.
         * Considers:
         * - Feature Prior
         * - Object Complexity
         * - Completeness
         */
        double objectScore(
            const MatchFeatures &features) const;

        /**
         * Combines the match score and object score into a final ranking score.
         */
        double finalScore(
            const MatchFeatures &features) const;

        double finalReverseScore(
            const ObjectMatch &match,
            std::size_t queryTokenCount) const;

    private:
        /**
         * Object priority
         *
         * Beispiel:
         * AdminArea 1.0
         * Road      0.9
         * Building  0.7
         */
        double featurePrior(
            const SearchObject &object) const;

        /**
         * How many information needs the given object to be fully specified?
         *
         * AdminArea = 1
         * Road       = 2
         * Building   = 3
         */
        std::size_t objectComplexity(
            const SearchObject &object) const;

        /**
         * Punishes objects that are not fully specified.
         */
        double completenessScore(
            const SearchObject &object,
            std::size_t queryTokenCount) const;

        double areaContextScore(
            const MatchFeatures &features) const;
    };

} // namespace geocoder::search