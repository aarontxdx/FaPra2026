#include "Search/Ranking.hpp"

#include <algorithm>
#include <type_traits>

using namespace geocoder::search;

double Ranking::matchScore(
    const MatchFeatures &features) const
{
    double score = 0.0;

    if (features.exact)
        score += 0.45;

    if (features.prefix)
        score += 0.20;

    if (features.substring)
        score += 0.05;

    score += 0.20 * features.ngramScore;
    score += 0.10 * features.editScore;

    return std::clamp(score, 0.0, 1.0);
}

double Ranking::featurePrior(
    const SearchObject &object) const
{
    return std::visit([](auto *obj) -> double
                      {
                          using T = std::decay_t<decltype(obj)>;

                          if constexpr (std::is_same_v<T, AdminArea *>)
                          {
                              return 1.0;
                          }
                          else if constexpr (std::is_same_v<T, Road *>)
                          {
                              return 0.90;
                          }
                          else if constexpr (std::is_same_v<T, Building *>)
                          {
                              return 0.75;
                          }
                          else
                          {
                              return 0.5;
                          } },
                      object);
}

std::size_t Ranking::objectComplexity(
    const SearchObject &object) const
{
    return std::visit([](auto *obj) -> std::size_t
                      {
                          using T = std::decay_t<decltype(obj)>;

                          if constexpr (std::is_same_v<T, AdminArea *>)
                          {
                              return 1;
                          }
                          else if constexpr (std::is_same_v<T, Road *>)
                          {
                              return 2;
                          }
                          else if constexpr (std::is_same_v<T, Building *>)
                          {
                              return 3;
                          }
                          else
                          {
                              return 2;
                          } },
                      object);
}

double Ranking::completenessScore(
    const SearchObject &object,
    std::size_t queryTokenCount) const
{
    const auto complexity = objectComplexity(object);

    if (complexity == 0)
        return 1.0;

    double score =
        static_cast<double>(queryTokenCount) /
        static_cast<double>(complexity);

    return std::clamp(score, 0.0, 1.0);
}

double Ranking::objectScore(
    const MatchFeatures &features) const
{
    const double prior =
        featurePrior(features.object);

    const double completeness =
        completenessScore(
            features.object,
            features.queryTokenCount);

    /*
        prior: which object is this
        completeness: how many information needs this object to be fully specified
    */

    return std::clamp(
        0.6 * prior +
            0.4 * completeness,
        0.0,
        1.0);
}

double Ranking::finalScore(
    const MatchFeatures &features) const
{
    double match =
        matchScore(features);

    const double object =
        objectScore(features);

    if (features.matchedStreet &&
        features.matchedHouseNumber)
    {
        match *= 1.5;
    }

    return std::clamp(
        0.75 * match +
            0.25 * object,
        0.0,
        1.0);
}

double Ranking::finalReverseScore(
    const ObjectMatch &match,
    std::size_t queryTokenCount) const
{
    double coverage =
        static_cast<double>(
            match.matchedTokens.size()) /
        static_cast<double>(
            std::max<std::size_t>(
                1,
                queryTokenCount));

    double score =
        match.bestScore *
        (0.8 + 0.2 * coverage);

    if (match.matchedPostcode)
    {
        score += 0.10;
    }

    if (match.matchedArea)
    {
        score += 0.08;
    }

    if (match.matchedCity)
    {
        score += 0.08;
    }

    if (match.matchedCounty)
    {
        score += 0.05;
    }

    if (match.matchedState)
    {
        score += 0.03;
    }

    if (match.matchedStreet)
    {
        score += 0.10;
    }

    if (match.matchedHouseNumber)
    {
        score += 0.10;
    }

    if (match.matchedStreet &&
        match.matchedHouseNumber)
    {
        score += 0.15;
    }

    if (match.matchedArea &&
        match.matchedStreet &&
        match.matchedHouseNumber)
    {
        score += 0.10;
    }

    return std::clamp(
        score,
        0.0,
        1.0);
}