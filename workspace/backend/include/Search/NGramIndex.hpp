#pragma once

#include "Ranking.hpp"
#include "SearchIndex.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace geocoder
{
    namespace search
    {

        class NGramIndex : public SearchIndex
        {
        public:
            explicit NGramIndex(size_t n = 3);
            void build(
                const std::vector<std::pair<std::string, SearchObject>> &items) override;

            std::vector<MatchFeatures> query(
                const std::string &q,
                int maxResults = 10);

            /**
             * @return approximate memory usage in bytes
             */
            size_t memoryUsage() const;

        private:
            struct NGramRecord
            {
                std::string text;
                SearchObject object;
            };

            size_t n_;
            std::vector<NGramRecord> records_;
            std::vector<size_t> gram_counts_;
            std::unordered_map<std::string, std::vector<size_t>> index_;
        };

    }
} // namespace geocoder::search
