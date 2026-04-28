#pragma once

#include "AdminArea.hpp"
#include "UtilFunctions.hpp"

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
        if (tags.get_value_by_key("boundary") != std::string("administrative"))
            return;
        try
        {
            auto mp = m_factory.create_multipolygon(area);

            AdminArea admin;
            if (area.tags().has_key("name"))
            {
                admin.name = area.tags()["name"];
            }

            if (area.tags().has_key("admin_level"))
            {
                admin.admin_level = area.tags()["admin_level"];
            }

            if (area.tags().has_key("boundary"))
            {
                admin.boundary = area.tags()["boundary"];
            }

            for (const auto &outer : area.outer_rings())
            {
                std::vector<Point> ring;

                for (const auto &n : outer)
                {
                    const auto &loc = n.location();
                    ring.emplace_back(loc.lat(), loc.lon());
                }

                admin.area.push_back(std::move(ring));
            }

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

    inline Point parse_point(const std::string::iterator::value_type &coord_str)
    {
        std::istringstream ss(coord_str);
        Point pt;
        ss >> pt.x >> pt.y;
        return pt;
    }
};