#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace geocoder::utils
{

    inline std::string normalize(const std::string &s)
    {
        std::string out;
        out.reserve(s.size());
        for (unsigned char ch : s)
        {
            char c = static_cast<char>(std::tolower(ch));
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            {
                out.push_back(c);
            }
            else if (c == ' ' || c == '\t' || c == '-')
            {
                out.push_back(' ');
            }
        }
        // collapse spaces
        std::string res;
        res.reserve(out.size());
        bool lastSpace = false;
        for (char c : out)
        {
            if (c == ' ')
            {
                if (!lastSpace)
                {
                    res.push_back(c);
                    lastSpace = true;
                }
            }
            else
            {
                res.push_back(c);
                lastSpace = false;
            }
        }
        // trim
        if (!res.empty() && res.front() == ' ')
            res.erase(res.begin());
        if (!res.empty() && res.back() == ' ')
            res.pop_back();
        return res;
    }

    inline std::vector<std::string> ngrams(const std::string &s, size_t n = 3)
    {
        std::vector<std::string> out;
        if (s.empty())
            return out;
        if (s.size() <= n)
        {
            out.push_back(s);
            return out;
        }
        for (size_t i = 0; i + n <= s.size(); ++i)
            out.emplace_back(s.substr(i, n));
        return out;
    }

    inline int levenshtein(const std::string &a, const std::string &b)
    {
        const size_t n = a.size();
        const size_t m = b.size();
        if (n == 0)
            return static_cast<int>(m);
        if (m == 0)
            return static_cast<int>(n);
        std::vector<int> prev(m + 1), cur(m + 1);
        for (size_t j = 0; j <= m; ++j)
            prev[j] = static_cast<int>(j);
        for (size_t i = 1; i <= n; ++i)
        {
            cur[0] = static_cast<int>(i);
            for (size_t j = 1; j <= m; ++j)
            {
                int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
            }
            std::swap(prev, cur);
        }
        return prev[m];
    }

} // namespace geocoder::utils
