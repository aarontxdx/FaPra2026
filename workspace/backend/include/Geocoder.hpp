#include "GeocoderObjects/AdminArea.hpp"
#include "GeocoderObjects/Building.hpp"
#include "GeocoderObjects/Road.hpp"
#include "GeocoderObjects/SearchObject.hpp"
#include "Search/NGramIndex.hpp"
#include "Search/Ranking.hpp"
#include "Search/ObjectMatch.hpp"
#include "Search/MatchFeature.hpp"

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>

using Token = std::string;
using namespace geocoder;

struct QueryResult
{
    SearchObject object;
    double score;
};

enum class SearchMode
{
    ReverseIndex,
    NGram,
    Combined
};

class Geocoder
{
public:
    Geocoder(std::vector<AdminArea> &adminAreas,
             std::vector<Building> &buildings,
             std::vector<Road> &roads);

    std::vector<QueryResult> findQuery(
        std::string inputString,
        SearchMode mode = SearchMode::ReverseIndex);

    void createReverseIndex();
    void createNGramIndex();

private:
    /**
     * normalization of a given string
     *
     * @param textInput queryString given by the user
     */
    void normalize(std::string &textInput);

    /**
     * tokenize mQueryString
     * ! use after normalization
     *
     * @return token list
     */
    std::vector<Token> tokenize();

    /**
     * connect token which belong together
     */
    std::vector<Token> buildSearchTokens(
        const std::vector<Token> &tokens);

    void checkObjectMatch(
        ObjectMatch &match,
        const SearchObject &object,
        const std::string &token);

    void checkAreaContext(
        ObjectMatch &match,
        const SearchObject &object,
        const std::vector<SearchObject> &areas);

    void checkAreaContext(
        MatchFeatures &feature,
        const SearchObject &object,
        const std::vector<SearchObject> &areas);

    const std::vector<IndexEntry> *extendedSearch(const std::string &token);

    std::vector<QueryResult> searchReverseIndex(
        const std::string &query);

    std::vector<QueryResult> searchNGram(
        const std::string &query);

    std::vector<QueryResult> searchCombined(
        const std::string &input);

    void fillAttributeMatches(
        MatchFeatures &feature,
        const std::vector<std::string> &tokens);

    void fillAttributeMatches(
        ObjectMatch &match,
        const SearchObject &object,
        const std::vector<std::string> &tokens);

    std::vector<QueryResult> mergeResults(
        const std::vector<QueryResult> &first,
        const std::vector<QueryResult> &second);

    /**
     * add string text as key
     * and object object to the inverted index list
     */
    template <typename T>
    void addToken(
        ReverseIndex &index,
        std::string text,
        T *object,
        int weight = 1)
    {
        if (text.empty())
            return;

        normalize(text);

        index[text].push_back(
            IndexEntry{object, weight});
    }

    /**
     * Get the memoryUsage of mIndex
     */
    size_t memoryUsageReverseIndex() const;

    /**
     * add all attributes to the inverted index list
     */
    void index(
        AdminArea &area,
        ReverseIndex &index);
    void index(
        Building &b,
        ReverseIndex &index);
    void index(
        Road &r,
        ReverseIndex &index);

    std::vector<AdminArea> &mAdminAreas;
    std::vector<Building> &mBuildings;
    std::vector<Road> &mRoads;

    ReverseIndex mIndex;
    std::unique_ptr<geocoder::search::NGramIndex> mNGramIndex;

    std::string mQueryString;
};