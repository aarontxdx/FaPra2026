#pragma once

#include "GeocoderObjects/AdminArea.hpp"
#include "Utils/UtilFunctions.hpp"

#include <osmium/handler.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/geom/wkt.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>

class AreaHandler : public osmium::handler::Handler
{
    osmium::geom::WKTFactory<> m_factory;

public:
    AreaHandler() : adminAreas(nullptr) {} // default constructor

    void set_admin_vector(std::vector<AdminArea> &vec)
    {
        adminAreas = &vec;
    }

    inline void area(const osmium::Area &area) noexcept
    {
        const auto &tags = area.tags();
        if (!tags.has_key("boundary"))
            return;

        auto boundary = tags.get_value_by_key("boundary");

        if (boundary != std::string("administrative") &&
            boundary != std::string("postal_code"))
            return;

        try
        {
            auto mp = m_factory.create_multipolygon(area);

            AdminArea admin;
            if (tags.has_key("name"))
            {
                admin.name = tags["name"];
            }

            if (tags.has_key("admin_level"))
            {
                admin.admin_level = std::stoi(tags["admin_level"]);
            }

            if (tags.has_key("boundary"))
            {
                admin.boundary = tags["boundary"];
            }
            if (tags.has_key("postal_code") && tags["postal_code"] != "")
            {
                admin.postal_code = tags["postal_code"];
            }

            admin.id = area.id();

            // BoundingBox
            double minLat = std::numeric_limits<double>::infinity();
            double maxLat = -std::numeric_limits<double>::infinity();
            double minLon = std::numeric_limits<double>::infinity();
            double maxLon = -std::numeric_limits<double>::infinity();

            for (const auto &outer : area.outer_rings())
            {
                std::vector<Point> outerRing;

                for (const auto &n : outer)
                {
                    const auto &loc = n.location();

                    double lat = loc.lat();
                    double lon = loc.lon();

                    outerRing.emplace_back(lat, lon);

                    // Update BB
                    minLat = std::min(minLat, lat);
                    maxLat = std::max(maxLat, lat);
                    minLon = std::min(minLon, lon);
                    maxLon = std::max(maxLon, lon);
                }

                admin.area.push_back(std::move(outerRing));

                for (const auto &inner : area.inner_rings(outer))
                {
                    std::vector<Point> innerRing;

                    for (const auto &n : inner)
                    {
                        const auto &loc = n.location();
                        innerRing.emplace_back(loc.lat(), loc.lon());
                    }

                    admin.area.push_back(std::move(innerRing));
                }
            }

            admin.bb = {Point{minLat, minLon}, Point{maxLat, maxLon}};

            adminAreas->push_back(std::move(admin));
        }
        catch (const osmium::geometry_error &e)
        {
            std::cout << "GEOMETRY ERROR: " << e.what() << "\n";
        }
    }

private:
    std::vector<AdminArea> *adminAreas;

    inline bool is_admin_area(const osmium::Area &area)
    {
        const auto &tags = area.tags();

        return tags.has_key("boundary") &&
               std::string(tags.get_value_by_key("boundary")) == "administrative";
    }
};