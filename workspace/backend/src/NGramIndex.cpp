#include "../include/Search/NGramIndex.hpp"
#include "../include/Utils/FuzzyUtils.hpp"

#include <sstream>
#include <set>
#include <algorithm>

using namespace geocoder::search;
using namespace geocoder::utils;

NGramIndex::NGramIndex(size_t n) : n_(n) {}

void NGramIndex::build(const std::vector<std::string> &items)
{
    records_.clear();
    index_.clear();
    records_.reserve(items.size());

    for (size_t id = 0; id < items.size(); ++id)
    {
        const auto &combined = items[id];
        std::string norm = normalize(combined);
        records_.push_back(norm);

        auto grams = ngrams(norm, n_);
        std::set<std::string> uniq(grams.begin(), grams.end());
        for (const auto &g : uniq)
            index_[g].push_back(id);
    }
}

std::vector<SearchHit> NGramIndex::query(const std::string &q, int maxResults)
{
    std::string qn = normalize(q);
    auto qgrams = ngrams(qn, n_);

    std::unordered_map<size_t, size_t> counts;
    for (const auto &g : qgrams)
    {
        auto it = index_.find(g);
        if (it == index_.end())
            continue;
        for (size_t id : it->second)
            counts[id]++;
    }

    std::vector<SearchHit> results;
    results.reserve(counts.size());
    for (const auto &kv : counts)
    {
        size_t id = kv.first;
        size_t common = kv.second;
        const auto &rec = records_[id];
        double overlap = static_cast<double>(common) / std::max<size_t>(1, std::max(qgrams.size(), ngrams(rec, n_).size()));
        int lev = levenshtein(qn, rec);
        size_t maxl = std::max(qn.size(), rec.size());
        double editScore = 1.0 - (static_cast<double>(lev) / static_cast<double>(std::max<size_t>(1, maxl)));
        if (editScore < 0.0)
            editScore = 0.0;
        double score = 0.6 * overlap + 0.4 * editScore;
        results.push_back({id, score});
    }

    std::sort(results.begin(), results.end(), [](const SearchHit &a, const SearchHit &b)
              { return a.score > b.score; });
    if (results.size() > static_cast<size_t>(maxResults))
        results.resize(maxResults);
    return results;
}
