#include "Geocoder.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

#include <chrono>
#include <iostream>
#include <sstream>

namespace
{
    struct SearchObjectHash
    {
        size_t operator()(const SearchObject &obj) const
        {
            if (const auto *p = std::get_if<AdminArea *>(&obj))
            {
                return std::hash<AdminArea *>()(*p);
            }
            if (const auto *p = std::get_if<Building *>(&obj))
            {
                return std::hash<Building *>()(*p);
            }
            if (const auto *p = std::get_if<Road *>(&obj))
            {
                return std::hash<Road *>()(*p);
            }
            return 0;
        }
    };

    struct SearchObjectEqual
    {
        bool operator()(const SearchObject &lhs, const SearchObject &rhs) const
        {
            return lhs == rhs;
        }
    };

    //****************************************************************/
    //******************* String Preprocessing ***********************/
    //****************************************************************/
    void replaceAll(std::string &str, const std::string &from, const std::string &to)
    {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    uint32_t nextCodepoint(const std::string &s, size_t &i)
    {
        unsigned char c = s[i];

        if (c < 0x80)
        {
            return s[i++];
        }
        else if ((c >> 5) == 0x6) // 2-byte
        {
            uint32_t cp = ((c & 0x1F) << 6) |
                          (s[i + 1] & 0x3F);
            i += 2;
            return cp;
        }
        else if ((c >> 4) == 0xE) // 3-byte
        {
            uint32_t cp = ((c & 0x0F) << 12) |
                          ((s[i + 1] & 0x3F) << 6) |
                          (s[i + 2] & 0x3F);
            i += 3;
            return cp;
        }
        else if ((c >> 3) == 0x1E) // 4-byte
        {
            uint32_t cp = ((c & 0x07) << 18) |
                          ((s[i + 1] & 0x3F) << 12) |
                          ((s[i + 2] & 0x3F) << 6) |
                          (s[i + 3] & 0x3F);
            i += 4;
            return cp;
        }

        i++; // invalid byte fallback
        return 0xFFFD;
    }

    char foldCodepoint(uint32_t cp)
    {
        switch (cp)
        {
        case 0x00E4:
        case 0x00C4:
            return 'a'; // ä Ä
        case 0x00F6:
        case 0x00D6:
            return 'o'; // ö Ö
        case 0x00FC:
        case 0x00DC:
            return 'u'; // ü Ü
        case 0x00DF:
            return 'ss'; // ß (optional "ss")
        case 0x00E7:
        case 0x00C7:
            return 'c'; // ç Ç
        case 0x00F1:
        case 0x00D1:
            return 'n'; // ñ Ñ

        // accents
        case 0x00E0:
        case 0x00E1:
        case 0x00E2:
        case 0x00E3:
        case 0x00E5:
            return 'a';

        case 0x00F2:
        case 0x00F3:
        case 0x00F4:
        case 0x00F5:
            return 'o';

        case 0x00F9:
        case 0x00FA:
        case 0x00FB:
            return 'u';

        default:
            if (cp < 128)
                return static_cast<char>(cp);
            return '?'; // fallback
        }
    }
}

Geocoder::Geocoder(std::vector<AdminArea> &adminAreas,
                   std::vector<Building> &buildings,
                   std::vector<Road> &roads)
    : mAdminAreas(adminAreas),
      mBuildings(buildings),
      mRoads(roads),
      mNGramIndex(std::make_unique<geocoder::search::NGramIndex>(3))
{
}

void Geocoder::normalize(std::string &textInput)
{
    // lowercase
    std::transform(textInput.begin(), textInput.end(),
                   textInput.begin(),
                   [](unsigned char c)
                   {
                       return std::tolower(c);
                   });

    // ASCII folding
    std::string resultQuery;
    resultQuery.reserve(textInput.size());
    for (size_t i = 0; i < textInput.size();)
    {
        uint32_t cp = nextCodepoint(textInput, i);
        if (cp == 0x00DF)
        {
            resultQuery += "ss";
            continue;
        }

        resultQuery.push_back(foldCodepoint(cp));
    }
    textInput = std::move(resultQuery);

    // replace abbreviations
    replaceAll(textInput, "str.", "strasse");
    replaceAll(textInput, "str ", "strasse ");

    // replace punctuation marks
    textInput = std::regex_replace(textInput,
                                   std::regex("[.,;:!?'\"]"),
                                   " ");

    // replace multiple spaces
    textInput = std::regex_replace(textInput,
                                   std::regex("\\s+"),
                                   " ");

    // replace spaces at the end and the beginning
    if (!textInput.empty())
    {
        textInput.erase(0, textInput.find_first_not_of(' '));
        textInput.erase(textInput.find_last_not_of(' ') + 1);
    }
}

std::vector<QueryResult> Geocoder::findQuery(
    std::string input,
    SearchMode mode)
{
    normalize(input);

    switch (mode)
    {
    case SearchMode::ReverseIndex:
        return searchReverseIndex(input);

    case SearchMode::NGram:
        return searchNGram(input);

    case SearchMode::Combined:
    {
        auto reverse = searchReverseIndex(input);
        auto ngram = searchNGram(input);

        return mergeResults(reverse, ngram);
    }
    }

    return {};
}

void Geocoder::createReverseIndex()
{
    auto start = std::chrono::steady_clock::now();

    std::cout << "ReverseIndex build for areas..." << std::endl;
    for (auto &area : mAdminAreas)
        index(area);

    std::cout << "ReverseIndex build for buildings..." << std::endl;
    for (auto &building : mBuildings)
        index(building);

    std::cout << "ReverseIndex build for roads..." << std::endl;
    for (auto &road : mRoads)
        index(road);

    auto end = std::chrono::steady_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "createReverseIndex total duration: "
              << totalTime.count() << " ms\n"
              << std::endl;
}

void Geocoder::createNGramIndex()
{
    auto start = std::chrono::steady_clock::now();

    std::vector<std::pair<std::string, SearchObject>> records;

    records.reserve(
        mBuildings.size() +
        mRoads.size() +
        mAdminAreas.size());

    std::cout << "NGramIndex build for buildings..." << std::endl;

    for (auto &building : mBuildings)
    {
        std::ostringstream ss;

        if (!building.street.empty())
            ss << building.street << " ";

        if (!building.housenumber.empty())
            ss << building.housenumber << " ";

        if (!building.city.empty())
            ss << building.city;

        records.push_back({ss.str(),
                           &building});
    }

    std::cout << "NGramIndex build for roads..." << std::endl;

    for (auto &road : mRoads)
    {
        std::ostringstream ss;

        if (!road.name.empty())
            ss << road.name << " ";

        if (!road.city.empty())
            ss << road.city;

        records.push_back({ss.str(),
                           &road});
    }

    std::cout << "NGramIndex build for areas..." << std::endl;

    for (auto &area : mAdminAreas)
    {
        std::ostringstream ss;

        if (!area.name.empty())
            ss << area.name << " ";

        if (!area.city.empty())
            ss << area.city << " ";

        if (!area.state.empty())
            ss << area.state;

        records.push_back({ss.str(),
                           &area});
    }

    std::cout << "Creating NGram records: "
              << records.size()
              << std::endl;

    mNGramIndex->build(records);

    auto end = std::chrono::steady_clock::now();

    auto totalTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    std::cout << "createNGramIndex total duration: "
              << totalTime.count()
              << " ms\n"
              << std::endl;

    std::cout << "NGram memory usage: "
              << mNGramIndex->memoryUsage() / 1024 / 1024
              << " MB"
              << std::endl;
}

std::vector<Token> Geocoder::tokenize()
{
    std::vector<Token> tokens;
    std::istringstream stream(mQueryString);

    std::string token;
    while (stream >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}

std::vector<Geocoder::IndexEntry>
Geocoder::extendedSearch(const std::string &token)
{
    std::vector<IndexEntry> result;
    std::string norm = token;
    normalize(norm);

    if (norm.empty())
        return result;

    auto it = mIndex.find(norm);
    if (it != mIndex.end())
    {
        for (const auto &entry : it->second)
        {
            result.push_back({entry.object, entry.weight * 12 + 60});
        }
    }

    for (const auto &[key, values] : mIndex)
    {
        if (key == norm)
            continue;

        bool prefix = key.rfind(norm, 0) == 0;
        bool substring = key.find(norm) != std::string::npos;
        if (prefix || substring)
        {
            for (const auto &entry : values)
            {
                int bonus = entry.weight;
                if (prefix)
                    bonus += 12;
                else if (substring)
                    bonus += 5;
                result.push_back({entry.object, bonus});
            }
        }
    }

    return result;
}

void Geocoder::index(AdminArea &area)
{
    addToken(area.name, &area);

    addToken(area.country, &area);
    addToken(area.state, &area);
    addToken(area.county, &area);
    addToken(area.city, &area);

    addToken(area.postcode, &area);
}

void Geocoder::index(Building &b)
{
    addToken(b.name, &b);

    addToken(b.street, &b);
    addToken(b.housenumber, &b);

    addToken(b.country, &b);
    addToken(b.state, &b);
    addToken(b.county, &b);
    addToken(b.city, &b);
    addToken(b.postcode, &b);
}

std::vector<QueryResult> Geocoder::searchReverseIndex(
    const std::string &inputText)
{
    mQueryString = inputText;

    auto tokens = tokenize();

    std::unordered_map<SearchObject, double, SearchObjectHash, SearchObjectEqual> score;

    for (const auto &token : tokens)
    {
        if (token.empty())
            continue;

        const auto matches = extendedSearch(token);

        for (const auto &entry : matches)
        {
            score[entry.object] += entry.weight;
        }
    }

    std::vector<QueryResult> results;
    results.reserve(score.size());

    for (const auto &[obj, s] : score)
    {
        results.push_back({obj, s});
    }

    std::sort(results.begin(), results.end(),
              [](const QueryResult &a, const QueryResult &b)
              {
                  return a.score > b.score;
              });

    return results;
}

std::vector<QueryResult> Geocoder::searchNGram(
    const std::string &input)
{
    std::vector<QueryResult> results;
    auto hits =
        mNGramIndex->query(input, 20);
    for (const auto &hit : hits)
    {
        results.push_back(
            {hit.object,
             hit.score});
    }
    return results;
}

std::vector<QueryResult> Geocoder::mergeResults(
    const std::vector<QueryResult> &first,
    const std::vector<QueryResult> &second)
{
    std::unordered_map<SearchObject,
                       double,
                       SearchObjectHash,
                       SearchObjectEqual>
        scores;

    for (const auto &r : first)
        scores[r.object] += r.score;

    for (const auto &r : second)
        scores[r.object] += r.score;

    std::vector<QueryResult> result;

    for (auto &[obj, score] : scores)
    {
        result.push_back({obj, score});
    }

    std::sort(result.begin(),
              result.end(),
              [](auto &a, auto &b)
              {
                  return a.score > b.score;
              });

    return result;
}

void Geocoder::index(Road &r)
{
    addToken(r.name, &r);

    addToken(r.city, &r);

    addToken(
        geocoder::objects::toString(r.type),
        &r);
}

size_t Geocoder::memoryUsageReverseIndex() const
{
    size_t size = sizeof(*this);

    for (const auto &[token, entries] : mIndex)
    {
        size += token.capacity();

        size += entries.capacity() * sizeof(decltype(entries)::value_type);
    }

    return size;
}