#include "GeocoderObjects/AdminArea.hpp"
#include "GeocoderObjects/Building.hpp"
#include "GeocoderObjects/Road.hpp"
#include "GeocoderObjects/SearchObject.hpp"
#include "Search/NGramIndex.hpp"

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

    struct IndexEntry
    {
        SearchObject object;
        int weight;
    };

    /**
     * connect token which belong together
     */
    std::vector<IndexEntry> extendedSearch(const std::string &token);

    std::vector<QueryResult> searchReverseIndex(
        const std::string &query);

    std::vector<QueryResult> searchNGram(
        const std::string &query);

    std::vector<QueryResult> mergeResults(
        const std::vector<QueryResult> &first,
        const std::vector<QueryResult> &second);

    /**
     * add string text as key
     * and object object to the inverted index list
     */
    template <typename T>
    void addToken(
        std::string text,
        T *object,
        int weight = 1)
    {
        if (text.empty())
            return;

        normalize(text);

        mIndex[text].push_back(IndexEntry{object, weight});
    }

    /**
     * Get the memoryUsage of mIndex
     */
    size_t memoryUsageReverseIndex() const;

    /**
     * add all attributes to the inverted index list
     */
    void index(AdminArea &adminArea);
    void index(Building &building);
    void index(Road &road);

    std::vector<AdminArea> &mAdminAreas;
    std::vector<Building> &mBuildings;
    std::vector<Road> &mRoads;

    std::unordered_map<std::string, std::vector<IndexEntry>> mIndex;
    std::unique_ptr<geocoder::search::SearchIndex> mNGramIndex;

    std::string mQueryString;
};