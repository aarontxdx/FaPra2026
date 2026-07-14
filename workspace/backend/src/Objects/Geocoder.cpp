#include "Geocoder.hpp"

#include <algorithm>
#include <cctype>

#include <chrono>
#include <iostream>
#include <omp.h>
#include <sstream>

using namespace geocoder::search;

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

    void replacePunctuation(std::string &str)
    {
        for (char &c : str)
        {
            switch (c)
            {
            case '.':
            case ',':
            case ';':
            case ':':
            case '!':
            case '?':
            case '\'':
            case '"':
                c = ' ';
                break;
            }
        }
    }

    void removeDuplicateSpaces(std::string &str)
    {
        std::string result;
        result.reserve(str.size());

        bool lastWasSpace = false;

        for (char c : str)
        {
            if (std::isspace(static_cast<unsigned char>(c)))
            {
                if (!lastWasSpace)
                {
                    result.push_back(' ');
                    lastWasSpace = true;
                }
            }
            else
            {
                result.push_back(c);
                lastWasSpace = false;
            }
        }

        str = std::move(result);
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

    replacePunctuation(textInput);

    removeDuplicateSpaces(textInput);

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

    int threads = omp_get_max_threads();

    std::vector<ReverseIndex> localIndexes(threads);

    /*
     * AdminAreas
     */
    std::cout << "ReverseIndex build for areas..." << std::endl;

#pragma omp parallel
    {
        int id = omp_get_thread_num();
        auto &local = localIndexes[id];

#pragma omp for schedule(static)
        for (int i = 0;
             i < static_cast<int>(mAdminAreas.size());
             i++)
        {
            index(
                mAdminAreas[i],
                local);
        }
    }

    /*
     * Buildings
     */
    std::cout << "ReverseIndex build for buildings..." << std::endl;

#pragma omp parallel
    {
        int id = omp_get_thread_num();
        auto &local = localIndexes[id];

#pragma omp for schedule(static)
        for (int i = 0;
             i < static_cast<int>(mBuildings.size());
             i++)
        {
            index(
                mBuildings[i],
                local);
        }
    }

    /*
     * Roads
     */
    std::cout << "ReverseIndex build for roads..." << std::endl;

#pragma omp parallel
    {
        int id = omp_get_thread_num();
        auto &local = localIndexes[id];

#pragma omp for schedule(static)
        for (int i = 0;
             i < static_cast<int>(mRoads.size());
             i++)
        {
            index(
                mRoads[i],
                local);
        }
    }

    /*
     * Merge local indexes
     */
    std::cout << "Merging ReverseIndex..." << std::endl;

    for (auto &local : localIndexes)
    {
        for (auto &[token, entries] : local)
        {
            auto &target = mIndex[token];

            target.insert(
                target.end(),
                entries.begin(),
                entries.end());
        }
    }

    auto end = std::chrono::steady_clock::now();

    auto totalTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    std::cout
        << "createReverseIndex total duration: "
        << totalTime.count()
        << " ms\n"
        << "total memoryUsage: "
        << memoryUsageReverseIndex() / 1024 / 1024
        << " MB\n"
        << std::endl;
}

void Geocoder::createNGramIndex()
{
    auto start = std::chrono::steady_clock::now();

    std::cout << "Creating NGramIndex..." << std::endl;

    mNGramIndex->build(mIndex);

    auto end = std::chrono::steady_clock::now();

    auto totalTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    std::cout
        << "createNGramIndex total duration: "
        << totalTime.count()
        << " ms\n"
        << "NGram memory usage: "
        << mNGramIndex->memoryUsage() / 1024 / 1024
        << " MB\n"
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

void Geocoder::index(
    AdminArea &area,
    ReverseIndex &index)
{
    addToken(index, area.name, &area);

    addToken(index, area.country, &area);
    addToken(index, area.state, &area);
    addToken(index, area.county, &area);
    addToken(index, area.city, &area);

    addToken(index, area.postcode, &area);
}

void Geocoder::index(
    Building &b,
    ReverseIndex &index)
{
    addToken(index, b.name, &b);

    addToken(index, b.street, &b);
    addToken(index, b.housenumber, &b);

    addToken(index, b.country, &b);
    addToken(index, b.state, &b);
    addToken(index, b.county, &b);
    addToken(index, b.city, &b);
    addToken(index, b.postcode, &b);
}

void Geocoder::index(
    Road &r,
    ReverseIndex &index)
{
    addToken(index, r.name, &r);

    addToken(index, r.city, &r);

    addToken(
        index,
        geocoder::objects::toString(r.type),
        &r);
}

std::vector<MatchFeatures>
Geocoder::extendedSearch(const std::string &token)
{
    std::vector<MatchFeatures> result;

    std::string norm = token;
    normalize(norm);

    if (norm.empty())
        return result;

    auto it = mIndex.find(norm);

    if (it != mIndex.end())
    {
        for (const auto &entry : it->second)
        {
            MatchFeatures feature;

            feature.object = entry.object;
            feature.exact = true;
            feature.prefix = true;
            feature.substring = true;

            result.push_back(feature);
        }
    }

    return result;
}

std::vector<QueryResult> Geocoder::searchReverseIndex(
    const std::string &inputText)
{
    mQueryString = inputText;

    auto tokens = tokenize();

    Ranking ranking;

    const std::size_t queryTokenCount = tokens.size();

    std::unordered_map<SearchObject,
                       std::vector<MatchFeatures>,
                       SearchObjectHash,
                       SearchObjectEqual>
        matches;

    for (const auto &token : tokens)
    {
        if (token.empty())
            continue;

        auto entries = extendedSearch(token);

        for (auto &entry : entries)
        {
            entry.queryTokenCount = tokens.size();
            entry.matchedTokens.insert(token);

            matches[entry.object].push_back(entry);
        }
    }

    std::vector<QueryResult> results;
    results.reserve(matches.size());

    for (auto &[object, features] : matches)
    {
        double bestScore = 0.0;

        std::unordered_set<std::string> uniqueMatchedTokens;

        for (auto &feature : features)
        {
            bestScore = std::max(
                bestScore,
                ranking.finalScore(feature));

            uniqueMatchedTokens.insert(
                feature.matchedTokens.begin(),
                feature.matchedTokens.end());
        }

        double coverage =
            static_cast<double>(uniqueMatchedTokens.size()) /
            static_cast<double>(
                std::max<std::size_t>(1, queryTokenCount));

        double score =
            bestScore * (0.8 + 0.2 * coverage);

        results.push_back(
            {object,
             score});
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
    std::unordered_map<
        SearchObject,
        MatchFeatures,
        SearchObjectHash,
        SearchObjectEqual>
        matches;

    mQueryString = input;

    std::vector<QueryResult> results;

    Ranking ranking;

    auto tokens = tokenize();

    auto features =
        mNGramIndex->query(input, 20);

    for (auto &feature : features)
    {
        auto it = matches.find(feature.object);

        if (it == matches.end())
        {
            matches.emplace(
                feature.object,
                feature);
        }
        else
        {
            auto &existing = it->second;

            // mehrere Token-Treffer zusammenführen
            existing.matchedTokens.insert(
                feature.matchedTokens.begin(),
                feature.matchedTokens.end());

            // bester NGram Score zählt
            existing.ngramScore =
                std::max(
                    existing.ngramScore,
                    feature.ngramScore);

            // bester Edit Score zählt
            existing.editScore =
                std::max(
                    existing.editScore,
                    feature.editScore);
        }
    }

    for (auto &[object, feature] : matches)
    {
        feature.queryTokenCount =
            tokens.size();

        feature.matchedQueryTokens =
            feature.matchedTokens.size();

        double score =
            ranking.finalScore(feature);

        double coverage =
            static_cast<double>(
                std::min(
                    feature.matchedQueryTokens,
                    feature.queryTokenCount)) /
            static_cast<double>(
                std::max<size_t>(
                    1,
                    feature.queryTokenCount));

        score *= (0.8 + 0.2 * coverage);

        results.push_back(
            {object,
             score});
    }

    std::sort(
        results.begin(),
        results.end(),
        [](const QueryResult &a,
           const QueryResult &b)
        {
            return a.score > b.score;
        });

    return results;
}

// TODO: Prüfe Hausnummern als ReverseIndex
void Geocoder::fillAttributeMatches(
    MatchFeatures &feature,
    const std::vector<std::string> &tokens)
{
    std::visit(
        [&](auto *obj)
        {
            using T = std::decay_t<decltype(*obj)>;

            if constexpr (std::is_same_v<T, Building>)
            {
                std::string street = obj->street;
                normalize(street);

                std::string housenumber = obj->housenumber;
                normalize(housenumber);

                for (const auto &token : tokens)
                {
                    if (!street.empty() &&
                        street.find(token) != std::string::npos)
                    {
                        feature.matchedStreet = true;
                    }

                    if (!housenumber.empty() &&
                        housenumber == token)
                    {
                        feature.matchedHouseNumber = true;
                    }
                }
            }
        },
        feature.object);
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
              [](const QueryResult &a, const QueryResult &b)
              {
                  return a.score > b.score;
              });

    return result;
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