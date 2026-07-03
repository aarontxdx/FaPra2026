#pragma once

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
            void build(const std::vector<std::string> &items) override;
            std::vector<SearchHit> query(const std::string &q, int maxResults = 10) override;

        private:
            size_t n_;
            std::vector<std::string> records_;
            std::unordered_map<std::string, std::vector<size_t>> index_;
        };

    }
} // namespace geocoder::search
