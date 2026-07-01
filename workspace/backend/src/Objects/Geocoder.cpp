#include "Geocoder.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

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

    //*********************************************************/
    //******************* Tokenization ************************/
    //*********************************************************/

    void joinToken(std::vector<Token> &tokenList)
    {
        // TODO: join Token which belong together
    }

    //*********************************************************/
    //****************** Inverted Index ***********************/
    //*********************************************************/
}

Geocoder::Geocoder(std::vector<AdminArea> &adminAreas,
                   std::vector<Building> &buildings,
                   std::vector<Road> &roads)
    : mAdminAreas(adminAreas),
      mBuildings(buildings),
      mRoads(roads) {}

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

std::vector<QueryResult> Geocoder::findQuery(std::string &inputText)
{
    mQueryString = inputText;
    Geocoder::normalize(mQueryString);

    auto tokens = tokenize();

    std::unordered_map<SearchObject, int, SearchObjectHash, SearchObjectEqual> score;

    for (const auto &token : tokens)
    {
        if (token.empty())
            continue;

        const auto matches = extendedSearch(token);
        for (const auto &obj : matches)
        {
            score[obj] += 3;
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

void Geocoder::createReverseIndex()
{
    for (auto &area : mAdminAreas)
        index(area);

    for (auto &building : mBuildings)
        index(building);

    for (auto &road : mRoads)
        index(road);

    std::cout << mIndex.size() << std::endl;
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

std::vector<SearchObject>
Geocoder::extendedSearch(const std::string &token)
{
    std::vector<SearchObject> result;
    std::string norm = token;
    normalize(norm);

    if (norm.empty())
        return result;

    auto it = mIndex.find(norm);
    if (it != mIndex.end())
    {
        result.insert(result.end(), it->second.begin(), it->second.end());
    }

    for (const auto &[key, values] : mIndex)
    {
        if (key == norm)
            continue;

        if (key.rfind(norm, 0) == 0 || key.find(norm) != std::string::npos)
        {
            result.insert(result.end(), values.begin(), values.end());
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

void Geocoder::index(Road &r)
{
    addToken(r.name, &r);

    addToken(r.city, &r);

    addToken(
        geocoder::objects::toString(r.type),
        &r);
}