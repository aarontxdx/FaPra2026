#include <httplib.h>
#include "PBFLoader.hpp"
#include "PreProcessingUnit.hpp"
#include <json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace
{
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

    // >0 = CW, <0 = CCW (je nach Formel – ggf. testen!)
    bool isClockwise(const std::vector<Point> &ring)
    {
        return ringArea(ring) > 0;
    }
}

int main(int argc, char *argv[])
{
    std::string pbf_file;

    const std::string DATA_REG_STUTTGART = "data/stuttgart-regbez-260409.osm.pbf";
    const std::string DATA_BW = "data/baden-wuerttemberg-260416.osm.pbf";

    if (argc > 1)
    {
        pbf_file = argv[1];
    }
    else
    {
        pbf_file = DATA_BW;
    };

    std::cout << "Starting server..." << std::endl;
    std::cout << "Loading Buildings..." << std::endl;
    PBFLoader loader;
    auto [buildings, adminAreas, roads] = loader.extractFile(pbf_file);
    std::cout << "\nLoading Buildings finished...\n\n\n";
    std::cout << "Preprocessing..." << std::endl;
    PreProcessingUnit preprocessing;
    std::cout << "Preprocessing finished....\n"
              << std::endl;

    httplib::Server svr;

    /**
     * This server function find all Buildings (up to a threshold) in the current portview
     *
     * @param threshold default =1000
     * @param minLat, minLon, maxLat, maxLon represent the current portview
     *
     * @return JSON-file with Buildings
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
                    double lat = b.centroid.y;
                    double lon = b.centroid.x;

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
     * This server function find all Buildings (up to a threshold) in the current portview
     *
     * @param threshold default =10
     * @param adminLevel default =2
     *
     * @return GeoJSON with admin area polygons
     */
    svr.Get("/loadAdminAreas", [&](const httplib::Request &req, httplib::Response &res)
            {
                int adminLevel = 2;
                int threshold = 10;

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

    svr.listen("0.0.0.0", 8080);
}