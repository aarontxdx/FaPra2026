#include <httplib.h>
#include "PBFLoader.hpp"
#include <json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace
{
    void PreProcessing()
    {
    }
}

int main()
{
    const std::string DATA_REG_STUTTGART = "data/stuttgart-regbez-260409.osm.pbf";
    const std::string DATA_BW = "data/baden-wuerttemberg-260416.osm.pbf";

    std::cout << "Starting server..." << std::endl;
    std::cout << "Loading Buildings..." << std::endl;
    PBFLoader loader;
    auto [buildings, adminAreas, roads] = loader.extractFile(DATA_BW);
    std::cout << "Loading Buildings finished..." << std::endl;
    std::cout << "Preprocessing..." << std::endl;
    std::cout << "Preprocessing finished...." << std::endl;

    httplib::Server svr;

    svr.Get("/loadBuildings", [&](const httplib::Request &req, httplib::Response &res)
            {
                int threshold = 1000;
                if (req.has_param("threshold"))
                {
                    threshold = std::stoi(req.get_param_value("threshold"));
                }
                // cut building list
                json j = json::array();

                // Anzahl begrenzen, aber buildings NICHT verändern
                size_t limit = std::min(buildings.size(), static_cast<size_t>(threshold));

                for (size_t i = 0; i < limit; i++)
                {
                    const auto &b = buildings[i];

                    json jb;
                    jb["centroid"] = {b.centroid.x, b.centroid.y};
                    jb["housenumber"] = b.housenumber;
                    jb["street"] = b.street;
                    jb["postcode"] = b.postcode;
                    jb["city"] = b.city;
                    jb["country"] = b.country;
                    jb["name"] = b.name;

                    j.push_back(jb);
                }

                std::cout << "Writing json finished" << std::endl;

                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_content(j.dump(), "application/json"); });

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

                        json << "[";

                        for (size_t i = 0; i < adminArea.area[r].size(); ++i)
                        {
                            const auto& [lat, lon] = adminArea.area[r][i];

                            // Leaflet/GeoJSON braucht: [lon, lat]
                            json << "[" << lon << "," << lat << "]";

                            if (i + 1 < adminArea.area[r].size())
                                json << ",";
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