#include "../include/Search/NGramIndex.hpp"
#include "../include/Utils/FuzzyUtils.hpp"
#include "GeocoderObjects/SearchObject.hpp"
#include "Utils/UtilFunctions.hpp"

#include <algorithm>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace geocoder;
using namespace geocoder::search;
using namespace geocoder::utils;

NGramIndex::NGramIndex(size_t n) : n_(n) {}

void NGramIndex::build(
    const ReverseIndex &reverseIndex)
{
    reverseIndex_ = &reverseIndex;

    records_.clear();
    gram_counts_.clear();
    index_.clear();

    records_.reserve(
        reverseIndex.size());

    for (const auto &[token, entries] : reverseIndex)
    {
        if (helper::isNumberToken(token))
            continue;
        size_t id = records_.size();

        records_.push_back(
            {token});

        auto grams =
            ngrams(token, n_);

        gram_counts_.push_back(
            grams.size());

        std::set<std::string> unique(
            grams.begin(),
            grams.end());

        for (const auto &g : unique)
        {
            index_[g].push_back(id);
        }
    }
}

std::vector<MatchFeatures> NGramIndex::query(
    const std::string &q,
    int maxResults)
{
    if (maxResults <= 0)
        return {};

    std::string qn = normalize(q);

    auto qgrams = ngrams(qn, n_);

    if (qgrams.empty())
        return {};

    std::unordered_map<size_t, size_t> counts;

    /*
     * Finde passende NGram Records
     */
    for (const auto &g : qgrams)
    {
        auto it = index_.find(g);

        if (it == index_.end())
            continue;

        for (size_t id : it->second)
            counts[id]++;
    }

    std::vector<std::pair<size_t, double>> candidates;

    candidates.reserve(counts.size());

    /*
     * Berechne Ähnlichkeit der Tokens
     */
    for (const auto &[id, common] : counts)
    {
        const auto &token =
            records_[id].token;

        size_t recGramCount =
            gram_counts_[id];

        double overlap =
            static_cast<double>(common) /
            std::max<size_t>(
                1,
                std::max(
                    qgrams.size(),
                    recGramCount));

        candidates.emplace_back(
            id,
            overlap);
    }

    if (candidates.empty())
        return {};

    const size_t candidateBudget =
        std::min<size_t>(
            candidates.size(),
            std::max<size_t>(
                static_cast<size_t>(maxResults) * 50,
                200));

    std::partial_sort(
        candidates.begin(),
        candidates.begin() + candidateBudget,
        candidates.end(),
        [](const auto &a, const auto &b)
        {
            return a.second > b.second;
        });

    std::vector<MatchFeatures> results;

    /*
     * Erzeuge Matches aus ReverseIndex
     */
    for (size_t i = 0; i < candidateBudget; ++i)
    {
        size_t id =
            candidates[i].first;

        const auto &token =
            records_[id].token;

        double overlap =
            candidates[i].second;

        int lev =
            levenshtein(
                qn,
                token);

        size_t maxLength =
            std::max(
                qn.size(),
                token.size());

        double editScore =
            1.0 -
            (static_cast<double>(lev) /
             static_cast<double>(
                 std::max<size_t>(
                     1,
                     maxLength)));

        if (editScore < 0.0)
            editScore = 0.0;

        /*
         * Jetzt aus ReverseIndex holen
         */
        auto reverseIt =
            reverseIndex_->find(token);

        if (reverseIt == reverseIndex_->end())
            continue;

        for (const auto &entry : reverseIt->second)
        {
            MatchFeatures feature;

            feature.object =
                entry.object;

            feature.ngramScore =
                overlap;

            feature.editScore =
                editScore;

            /*
             * Wichtig für Ranking/Coverage
             */
            feature.matchedTokens.insert(
                q);

            results.push_back(
                feature);
        }
    }

    return results;
}

size_t geocoder::search::NGramIndex::memoryUsage() const
{
    size_t total = sizeof(NGramIndex);

    total += records_.capacity() *
             sizeof(NGramRecord);

    for (const auto &record : records_)
    {
        total += record.token.capacity();
    }

    total += gram_counts_.capacity() *
             sizeof(size_t);

    total += index_.size() *
             sizeof(decltype(index_)::value_type);

    for (const auto &[gram, postings] : index_)
    {
        total += gram.capacity();

        total += postings.capacity() *
                 sizeof(size_t);
    }

    return total;
}
