#include "BinaryStorage.hpp"
#include "DataStructures/Grid.hpp"
#include "Debug/DebugHelper.hpp"
#include "Geocoder.hpp"
#include "PBFLoader.hpp"
#include "PreProcessingUnit.hpp"
#include "ReverseGeocoder.hpp"
#include "Utils/UtilFunctions.hpp"

#include <filesystem>
#include <json.hpp>
#include <iostream>
#include <chrono>

#include <httplib.h>

using json = nlohmann::json;
using namespace geocoder::objects;

namespace fs = std::filesystem;

namespace
{
    /**
     * Computes the signed area of a polygon ring (used for orientation tests)
     */
    double ringArea(const std::vector<Point> &ring)
    {
        double sum = 0.0;
        for (size_t i = 0; i < ring.size(); ++i)
        {
            auto [x1, y1] = ring[i];
            auto [x2, y2] = ring[(i + 1) % ring.size()];
            sum += (x2 - x1) * (y2 + y1);
        }
        return sum;
    }

    /**
     * Returns true if the polygon ring is oriented clockwise
     */
    bool isClockwise(const std::vector<Point> &ring)
    {
        return ringArea(ring) > 0;
    }
}

namespace
{
    double getObjectLat(const SearchObject &obj)
    {
        if (const auto *building = std::get_if<Building *>(&obj))
            return (*building)->centroid.lat;

        if (const auto *road = std::get_if<Road *>(&obj))
            return (*road)->nodes.empty() ? 0.0 : (*road)->nodes.front().lat;

        if (const auto *area = std::get_if<AdminArea *>(&obj))
        {
            if ((*area)->area.empty())
                return 0.0;

            double lat = 0.0;
            size_t count = 0;
            for (const auto &ring : (*area)->area)
            {
                for (const auto &point : ring)
                {
                    lat += point.lat;
                    ++count;
                }
            }
            return count == 0 ? 0.0 : lat / count;
        }

        return 0.0;
    }

    double getObjectLon(const SearchObject &obj)
    {
        if (const auto *building = std::get_if<Building *>(&obj))
            return (*building)->centroid.lon;

        if (const auto *road = std::get_if<Road *>(&obj))
            return (*road)->nodes.empty() ? 0.0 : (*road)->nodes.front().lon;

        if (const auto *area = std::get_if<AdminArea *>(&obj))
        {
            if ((*area)->area.empty())
                return 0.0;

            double lon = 0.0;
            size_t count = 0;
            for (const auto &ring : (*area)->area)
            {
                for (const auto &point : ring)
                {
                    lon += point.lon;
                    ++count;
                }
            }
            return count == 0 ? 0.0 : lon / count;
        }

        return 0.0;
    }

    std::string getObjectName(const SearchObject &obj)
    {
        if (const auto *building = std::get_if<Building *>(&obj))
        {
            return (*building)->name.empty() ? (*building)->street : (*building)->name;
        }
        if (const auto *road = std::get_if<Road *>(&obj))
            return (*road)->name;
        if (const auto *area = std::get_if<AdminArea *>(&obj))
            return (*area)->name;
        return {};
    }

    json makeSearchResultJson(const SearchObject &obj, double score)
    {
        json item;
        item["score"] = score;
        item["name"] = getObjectName(obj);
        item["type"] = std::holds_alternative<Building *>(obj) ? "building"
                       : std::holds_alternative<Road *>(obj)   ? "road"
                                                               : "admin_area";
        item["lat"] = getObjectLat(obj);
        item["lon"] = getObjectLon(obj);

        if (const auto *building = std::get_if<Building *>(&obj))
        {
            item["street"] = (*building)->street;
            item["housenumber"] = (*building)->housenumber;
            item["city"] = (*building)->city;
            item["postcode"] = (*building)->postcode;
            item["county"] = (*building)->county;
            item["state"] = (*building)->state;
            item["country"] = (*building)->country;
        }
        else if (const auto *road = std::get_if<Road *>(&obj))
        {
            item["city"] = (*road)->city;
            item["postcode"] = (*road)->postcode;
        }
        else if (const auto *area = std::get_if<AdminArea *>(&obj))
        {
            item["adminLevel"] = (*area)->admin_level;
            item["city"] = (*area)->city;
            item["postcode"] = (*area)->postcode;
            item["county"] = (*area)->county;
            item["state"] = (*area)->state;
            item["country"] = (*area)->country;
        }

        return item;
    }
}

int main(int argc, char *argv[])
{
    std::string inputFile;
    std::string binaryOutput = "data/geocoder.bin";

    const std::string DATA_BW =
        "data/baden-wuerttemberg-260416.osm.pbf";

    if (argc > 1)
    {
        inputFile = argv[1];
    }
    else
    {
        inputFile = DATA_BW;
    }

    if (fs::path(inputFile).extension() == ".pbf" && argc > 2)
    {
        binaryOutput = argv[2];
    }

    std::cout << "Starting server..." << std::endl;

    PBFLoader loader;

    std::vector<Building> buildings;
    std::vector<AdminArea> adminAreas;
    std::vector<Road> roads;

    AdminHierarchy adminHierarchy;
    Grid grid{};

    Debug::DebugBreak();

    if (fs::path(inputFile).extension() == ".pbf")
    {
        std::cout << "Loading PBF file: "
                  << inputFile
                  << std::endl;

        loader.extractFile(
            buildings,
            adminAreas,
            roads,
            inputFile);

        std::cout
            << "\nFiles extracted..."
            << "\n\nPreprocessing Elements..."
            << std::endl;

        PreProcessingUnit preprocessing;

        preprocessing.preprocessAdminAreas(
            adminAreas,
            adminHierarchy);

        preprocessing.preprocessBuildings(
            buildings,
            adminHierarchy,
            grid);

        preprocessing.preprocessRoads(
            roads,
            adminHierarchy);

        std::cout
            << "\nPreprocessing finished..."
            << std::endl;

        std::cout
            << "Saving binary file: "
            << binaryOutput
            << std::endl;

        if (!BinaryStorage::saveBinary(
                binaryOutput,
                buildings,
                adminAreas,
                roads))
        {
            std::cerr
                << "Saving binary failed!"
                << std::endl;
            return 1;
        }

        std::cout
            << "Binary saved successfully"
            << std::endl;
    }
    else if (fs::path(inputFile).extension() == ".bin")
    {
        std::cout
            << "Loading binary file: "
            << inputFile
            << std::endl;

        if (!BinaryStorage::loadBinary(
                inputFile,
                buildings,
                adminAreas,
                roads))
        {
            std::cerr
                << "Failed loading binary file!"
                << std::endl;

            return 1;
        }

        std::cout
            << "Binary loaded successfully"
            << std::endl;

        std::cout
            << "Building Grid..."
            << std::endl;

        std::tuple<Point, Point> boundingBox(
            Point{
                std::numeric_limits<double>::infinity(),
                std::numeric_limits<double>::infinity()},
            Point{
                -std::numeric_limits<double>::infinity(),
                -std::numeric_limits<double>::infinity()});

        for (auto &building : buildings)
        {
            helper::updateObjectBoundingBox(
                building.centroid,
                boundingBox);
        }

        grid = Grid(
            std::get<0>(boundingBox).lat,
            std::get<0>(boundingBox).lon,
            std::get<1>(boundingBox).lat,
            std::get<1>(boundingBox).lon,
            0.1);

        for (auto &building : buildings)
        {
            grid.insert(&building);
        }

        std::cout
            << "Building AdminHierarchy..."
            << std::endl;

        PreProcessingUnit preprocessing;

        preprocessing.preprocessAdminAreas(
            adminAreas,
            adminHierarchy);
    }
    else
    {
        std::cerr
            << "Unknown file type. Use .pbf or .bin"
            << std::endl;

        return 1;
    }

    Debug::DebugBreak();

    ReverseGeocoder reverseGeocoder{buildings, adminAreas, roads, grid};

    std::cout << "Starting Geocoder...\n"
              << std::endl;

    Geocoder geocoder{adminAreas, buildings, roads};

    Debug::DebugBreak();

    geocoder.createReverseIndex();

    Debug::DebugBreak();

    geocoder.createNGramIndex();

    std::cout << "Reverse Geocoder and Geocoder started...\n\n"
              << "Server is running on http : // localhost:8080\n"
              << std::endl;

    httplib::Server svr;

    /**
     * This server function finds all Buildings (up to a threshold) in the current portview
     *
     * @param threshold default =1000
     * @param minLat, minLon, maxLat, maxLon represent the current portview
     *
     * @return JSON representation of Buildings
     */
    svr.Get("/loadBuildings", [&](const httplib::Request &req, httplib::Response &res)
            {
                if (!req.has_param("minLat") || !req.has_param("minLon") ||
                    !req.has_param("maxLat") || !req.has_param("maxLon"))
                {
                    res.status = 400;
                    res.set_content("Missing bbox parameters", "text/plain");
                    return;
                }

                double minLat = std::stod(req.get_param_value("minLat"));
                double minLon = std::stod(req.get_param_value("minLon"));
                double maxLat = std::stod(req.get_param_value("maxLat"));
                double maxLon = std::stod(req.get_param_value("maxLon"));   

                int threshold = 1000;
                if (req.has_param("threshold"))
                {
                    threshold = std::stoi(req.get_param_value("threshold"));
                }
                // cut building list
                json j = json::array();

                size_t count = 0;

                for (const auto &b : buildings)
                {
                    double lat = b.centroid.lat;
                    double lon = b.centroid.lon;

                    if (lat < minLat || lat > maxLat ||
                        lon < minLon || lon > maxLon)
                    {
                        continue;
                    }

                    json jb;
                    jb["centroid"] = {lon, lat};
                    jb["housenumber"] = b.housenumber;
                    jb["street"] = b.street;
                    jb["postcode"] = b.postcode;
                    jb["city"] = b.city;
                    jb["country"] = b.country;
                    jb["state"] = b.state;
                    jb["county"] = b.county;
                    jb["name"] = b.name;

                    j.push_back(jb);

                    count++;
                    if (count >= threshold)
                        break;
                }

                std::cout << "Writing json finished" << std::endl;

                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(j.dump(), "application/json"); });

    /**
     * This server function finds all administration areas
     *
     * TODO: No current Portview search applied at the moment
     *
     * @param threshold default =10
     * @param adminLevel default =2
     *
     * @return GeoJSON with admin area polygons
     */
    svr.Get("/loadAdminAreas", [&](const httplib::Request &req, httplib::Response &res)
            {
                int threshold = 10;
                int adminLevel = 2;

                if (req.has_param("adminLevel")) {
                    adminLevel = std::stoi(req.get_param_value("adminLevel"));
                }
                if (req.has_param("threshold"))
                {
                    threshold = std::stoi(req.get_param_value("threshold"));
                }

                std::ostringstream json;
                json << R"({"type":"FeatureCollection","features":[)";

                int count = 0;

                bool first_area = true;

                for (const auto& adminArea : adminAreas)
                {
                    if (adminArea.admin_level != adminLevel) continue;

                    if (count >= threshold) break;

                    if (!first_area) {
                        json << ",";
                    } else {
                        first_area = false;
                    }

                    json << R"({"type":"Feature","geometry":{"type":"Polygon","coordinates":[)";

                    for (size_t r = 0; r < adminArea.area.size(); ++r)
                    {
                        if (r > 0) json << ",";

                        auto ring = adminArea.area[r];

                        bool shouldBeClockwise = (r != 0); // Löcher = CW
                        if (isClockwise(ring) != shouldBeClockwise)
                        {
                            std::reverse(ring.begin(), ring.end());
                        }

                        json << "[";

                        for (size_t i = 0; i < ring.size(); ++i)
                        {
                            const auto& [lat, lon] = ring[i];
                            json << "[" << lon << "," << lat << "]";
                            if (i + 1 < ring.size()) json << ",";
                        }

                        json << "]";
                    }

                    json << R"(]},"properties":{"id":)"
                         << adminArea.id
                         << R"(,"name":")"
                         << adminArea.name
                         << R"("}})";

                    count++;
                }

                json << "]}";

                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(json.str(), "application/json"); });

    /**
     * This server function finds all Roads (up to a threshold) in the current portview
     *
     * @param threshold default =1000
     * @param minLat, minLon, maxLat, maxLon represent the current portview
     *
     * @return JSON representation of Roads
     */
    svr.Get("/loadStreets", [&](const httplib::Request &req, httplib::Response &res)
            {
                if (!req.has_param("minLat") || !req.has_param("minLon") ||
                    !req.has_param("maxLat") || !req.has_param("maxLon"))
                {
                    res.status = 400;
                    res.set_content("Missing bbox parameters", "text/plain");
                    return;
                }

                double minLat = std::stod(req.get_param_value("minLat"));
                double minLon = std::stod(req.get_param_value("minLon"));
                double maxLat = std::stod(req.get_param_value("maxLat"));
                double maxLon = std::stod(req.get_param_value("maxLon"));   

                int threshold = 1000;
                if (req.has_param("threshold"))
                {
                    threshold = std::stoi(req.get_param_value("threshold"));
                }

                json j;
                j["type"] = "FeatureCollection";
                j["features"] = json::array();

                int count = 0;

                for (const auto &s : roads)
                {
                    bool inside = false;

                    for (const auto &p : s.nodes)
                    {
                        if (p.lat >= minLat && p.lat <= maxLat &&
                            p.lon >= minLon && p.lon <= maxLon)
                        {
                            inside = true;
                            break;
                        }
                    }

                    if (!inside) continue;

                    json feature;
                    feature["type"] = "Feature";

                    // Geometrie
                    json coords = json::array();
                    for (const auto &p : s.nodes)
                    {
                        coords.push_back({p.lon, p.lat}); // [lon, lat]
                    }

                    feature["geometry"] = {
                        {"type", "LineString"},
                        {"coordinates", coords}
                    };

                    // Eigenschaften
                    feature["properties"] = {
                        {"name", s.name},
                        {"type", toString(s.type)},
                        {"id", s.id},
                        {"postcode", s.postcode},
                        {"city", s.city},
                        {"country", s.country},
                        {"state", s.state},
                        {"county", s.county}
                    };

                    j["features"].push_back(feature);

                    if (++count >= threshold)
                        break;
                }

                std::cout << "Writing streets json finished" << std::endl;

                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(j.dump(), "application/json"); });

    svr.Get("/reverseGeocodeBuilding",
            [&](const httplib::Request &req,
                httplib::Response &res)
            {
                if (!req.has_param("lat") ||
                    !req.has_param("lon"))
                {
                    res.status = 400;
                    res.set_content(
                        "Missing lat/lon parameters",
                        "text/plain");
                    return;
                }

                const double lat =
                    std::stod(req.get_param_value("lat"));

                const double lon =
                    std::stod(req.get_param_value("lon"));

                try
                {
                    Building nearestBuilding =
                        reverseGeocoder.findNearestBuilding(
                            lat,
                            lon);

                    json j;

                    j["queryPoint"] = {lon, lat};

                    j["name"] = nearestBuilding.name;

                    j["housenumber"] = nearestBuilding.housenumber;
                    j["street"] = nearestBuilding.street;
                    j["postcode"] = nearestBuilding.postcode;
                    j["city"] = nearestBuilding.city;
                    j["county"] = nearestBuilding.county;
                    j["state"] = nearestBuilding.state;
                    j["country"] = nearestBuilding.country;

                    j["centroid"] = {
                        nearestBuilding.centroid.lon,
                        nearestBuilding.centroid.lat};

                    res.set_header(
                        "Access-Control-Allow-Origin",
                        "*");

                    res.set_content(
                        j.dump(),
                        "application/json");
                }
                catch (const std::exception &e)
                {
                    res.status = 404;

                    res.set_content(
                        e.what(),
                        "text/plain");
                }
            });
    /**
     * Reverse geocoding endpoint
     *
     * Finds the Area in which the given point is
     *
     * @param lat
     * @param lon
     * @param adminLevel defines search level
     *
     * @return JSON representation of nearest object
     */
    svr.Get("/reverseGeocodeArea",
            [&](const httplib::Request &req,
                httplib::Response &res)
            {
                if (!req.has_param("lat") ||
                    !req.has_param("lon") ||
                    !req.has_param("adminLevel"))
                {
                    res.status = 400;
                    res.set_content("Missing parameters", "text/plain");
                    return;
                }

                const double lat = std::stod(req.get_param_value("lat"));
                const double lon = std::stod(req.get_param_value("lon"));
                int adminLevel = std::stoi(req.get_param_value("adminLevel"));

                Point p{lat, lon};

                const auto &candidates =
                    adminHierarchy.adminAreaByLevel[adminLevel];

                std::ostringstream json;

                json << R"({"type":"FeatureCollection","features":[)";

                std::vector<const AdminArea *> hits =
                    helper::pointInPolygon(p, candidates);

                while (adminLevel < 10 && hits.empty())
                {
                    adminLevel++;
                    const auto &candidates =
                        adminHierarchy.adminAreaByLevel[adminLevel];

                    hits = helper::pointInPolygon(p, candidates);
                }

                bool first = true;

                for (const auto *area : hits)
                {
                    if (!first)
                        json << ",";
                    first = false;

                    json << R"({"type":"Feature","geometry":{"type":"Polygon","coordinates":[)";

                    for (size_t r = 0; r < area->area.size(); ++r)
                    {
                        if (r > 0)
                            json << ",";

                        auto ring = area->area[r];

                        bool shouldBeClockwise = (r != 0);
                        if (isClockwise(ring) != shouldBeClockwise)
                        {
                            std::reverse(ring.begin(), ring.end());
                        }

                        json << "[";

                        for (size_t i = 0; i < ring.size(); ++i)
                        {
                            const auto &pt = ring[i];
                            json << "[" << pt.lon << "," << pt.lat << "]";

                            if (i + 1 < ring.size())
                                json << ",";
                        }

                        json << "]";
                    }

                    json << R"(]},"properties":{)";

                    json << R"("id":)" << area->id << ",";
                    json << R"("name":")" << area->name << R"("})";

                    json << "}";
                }

                json << "]}";

                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(json.str(), "application/json");
            });

    /**
     * Reverse geocoding endpoint
     *
     * Finds the nearest Building to a given point
     *
     * @param lat
     * @param lon
     *
     * @return JSON representation of nearest object
     */
    svr.Get("/geocode",
            [&](const httplib::Request &req,
                httplib::Response &res)
            {
                if (!req.has_param("query"))
                {
                    res.status = 400;
                    res.set_content(
                        "Missing query parameter",
                        "text/plain");
                    return;
                }

                std::string query =
                    req.get_param_value("query");

                size_t maxResults = 20;

                if (req.has_param("maxResults"))
                {
                    maxResults =
                        std::stoi(
                            req.get_param_value("maxResults"));
                }

                // Suchmodus vom Frontend übernehmen
                SearchMode mode = SearchMode::Combined;

                if (req.has_param("mode"))
                {
                    std::string modeParam =
                        req.get_param_value("mode");

                    if (modeParam == "reverse")
                    {
                        mode = SearchMode::ReverseIndex;
                    }
                    else if (modeParam == "ngram")
                    {
                        mode = SearchMode::NGram;
                    }
                    else if (modeParam == "combined")
                    {
                        mode = SearchMode::Combined;
                    }
                }

                auto start =
                    std::chrono::steady_clock::now();

                auto results =
                    geocoder.findQuery(
                        query,
                        mode);

                auto elapsed =
                    std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - start)
                        .count();

                json out;

                out["query"] = query;
                out["queryTimeMs"] = elapsed;
                out["mode"] =
                    mode == SearchMode::ReverseIndex ? "reverse" : mode == SearchMode::NGram ? "ngram"
                                                                                             : "combined";

                out["results"] = json::array();

                size_t count = 0;

                for (const auto &result : results)
                {
                    if (count++ >= maxResults)
                        break;

                    out["results"].push_back(
                        makeSearchResultJson(
                            result.object,
                            result.score));
                }

                res.set_header(
                    "Access-Control-Allow-Origin",
                    "*");

                res.set_content(
                    out.dump(),
                    "application/json");
            });

    svr.listen("0.0.0.0", 8080);
}