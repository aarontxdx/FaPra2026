#include <httplib.h>
#include "PBFLoader.hpp"
#include <json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace
{
    void test()
    {
    }
}

int main()
{
    std::cout << "Starting server..." << std::endl;
    std::cout << "Loading Buildings..." << std::endl;
    PBFLoader loader;
    auto [buildings, adminAreas] = loader.extractFile("data/stuttgart-regbez-260409.osm.pbf");
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

    svr.Options("/loadAdminAreas", [&](const httplib::Request &, httplib::Response &res)
                {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
    res.status = 200; });

    svr.Get("/loadAdminAreas", [&](const httplib::Request &req, httplib::Response &res)
            {
                int adminLevel = 2;
                if (req.has_param("adminLevel")) {
                    adminLevel = std::stoi(req.get_param_value("adminLevel"));
                } });

    svr.listen("0.0.0.0", 8080);
}