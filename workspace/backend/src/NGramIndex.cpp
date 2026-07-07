#include "../include/Search/NGramIndex.hpp"
#include "../include/Utils/FuzzyUtils.hpp"

#include <sstream>
#include <set>
#include <algorithm>
#include <utility>

using namespace geocoder::search;
using namespace geocoder::utils;

NGramIndex::NGramIndex(size_t n) : n_(n) {}

void NGramIndex::build(const std::vector<std::string> &items)
{
    records_.clear();
    gram_counts_.clear();
    index_.clear();
    records_.reserve(items.size());
    gram_counts_.reserve(items.size());

    for (size_t id = 0; id < items.size(); ++id)
    {
        const auto &combined = items[id];
        std::string norm = normalize(combined);
        records_.push_back(norm);

        auto grams = ngrams(norm, n_);
        gram_counts_.push_back(grams.size());
        std::set<std::string> uniq(grams.begin(), grams.end());
        for (const auto &g : uniq)
            index_[g].push_back(id);
    }
}

std::vector<SearchHit> NGramIndex::query(const std::string &q, int maxResults)
{
    if (maxResults <= 0)
        return {};

    std::string qn = normalize(q);
    auto qgrams = ngrams(qn, n_);
    if (qgrams.empty())
        return {};

    std::unordered_map<size_t, size_t> counts;
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
    for (const auto &kv : counts)
    {
        size_t id = kv.first;
        size_t common = kv.second;
        const auto &rec = records_[id];
        size_t recGramCount = (id < gram_counts_.size()) ? gram_counts_[id] : ngrams(rec, n_).size();
        double overlap = static_cast<double>(common) / std::max<size_t>(1, std::max(qgrams.size(), recGramCount));
        candidates.emplace_back(id, overlap);
    }

    if (candidates.empty())
        return {};

    const size_t candidateBudget = std::min<size_t>(candidates.size(), std::max<size_t>(static_cast<size_t>(maxResults) * 8, 64));
    std::partial_sort(candidates.begin(), candidates.begin() + candidateBudget, candidates.end(),
                      [](const std::pair<size_t, double> &a, const std::pair<size_t, double> &b)
                      { return a.second > b.second; });

    std::vector<SearchHit> results;
    results.reserve(std::min<size_t>(candidateBudget, static_cast<size_t>(maxResults)));
    for (size_t i = 0; i < candidateBudget; ++i)
    {
        size_t id = candidates[i].first;
        const auto &rec = records_[id];
        const auto overlap = candidates[i].second;
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

size_t geocoder::search::NGramIndex::memoryUsage() const
{
    size_t total = sizeof(NGramIndex);

    // records_
    total += records_.capacity() * sizeof(std::string);
    for (const auto &s : records_)
        total += s.capacity();

    // gram_counts_
    total += gram_counts_.capacity() * sizeof(size_t);

    // unordered_map
    total += index_.size() * sizeof(decltype(index_)::value_type);

    for (const auto &[gram, postings] : index_)
    {
        total += gram.capacity();
        total += postings.capacity() * sizeof(size_t);
    }

    return total;
}
