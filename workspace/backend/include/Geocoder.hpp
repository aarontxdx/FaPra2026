#include "GeocoderObjects/AdminArea.hpp"
#include "GeocoderObjects/Building.hpp"
#include "GeocoderObjects/Road.hpp"

#include <variant>
#include <unordered_map>

using SearchObject = std::variant<AdminArea *, Building *, Road *>;

using Token = std::string;

struct QueryResult
{
    SearchObject object;
    int score;
};

class Geocoder
{
public:
    Geocoder(std::vector<AdminArea> &adminAreas,
             std::vector<Building> &buildings,
             std::vector<Road> &roads);

    std::vector<QueryResult> findQuery(std::string &inputString);
    void createReverseIndex();

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
    std::vector<SearchObject> extendedSearch(const std::string &token);

    /**
     * add string text as key
     * and object object to the inverted index list
     */
    template <typename T>
    void addToken(
        std::string text,
        T *object)
    {
        if (text.empty())
            return;

        normalize(text);

        mIndex[text].push_back(object);
    }

    /**
     * add all attributes to the inverted index list
     */
    void index(AdminArea &adminArea);
    void index(Building &building);
    void index(Road &road);

    std::vector<AdminArea> mAdminAreas;
    std::vector<Building> mBuildings;
    std::vector<Road> mRoads;

    std::unordered_map<std::string, std::vector<SearchObject>> mIndex;

    // member reverse index

    std::string mQueryString;
};