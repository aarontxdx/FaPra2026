#pragma once

#include "DataStructures/Grid.hpp"
#include "GeocoderObjects/Building.hpp"
#include "Utils/UtilFunctions.hpp"

#include <tuple>

#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/point_on_surface.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/osm/node.hpp>

namespace helper
{
    namespace bg = boost::geometry;

    using BoostPoint =
        bg::model::point<double, 2, bg::cs::cartesian>;

    /**
     * This function finds a point on a polygon
     * Used to find a representative Point per building
     *
     * @param poly the polygon to test
     *
     * @return Point on the given polygon
     */
    static Point boundingBoxCenter(const std::vector<Point> &poly)
    {
        if (poly.empty())
            return {};

        double minLat = std::numeric_limits<double>::max();
        double maxLat = std::numeric_limits<double>::lowest();
        double minLon = std::numeric_limits<double>::max();
        double maxLon = std::numeric_limits<double>::lowest();

        for (const auto &p : poly)
        {
            minLat = std::min(minLat, p.lat);
            maxLat = std::max(maxLat, p.lat);

            minLon = std::min(minLon, p.lon);
            maxLon = std::max(maxLon, p.lon);
        }

        return {
            (minLat + maxLat) * 0.5,
            (minLon + maxLon) * 0.5};
    }
}

class BuildingHandler : public osmium::handler::Handler
{
public:
    explicit BuildingHandler(std::vector<Building> &out)
        : buildings(out) {}

    std::tuple<Point, Point> getOBB()
    {
        return mGeocoderObjectBB;
    }

    void way(const osmium::Way &way) noexcept
    {
        const auto &tags = way.tags();

        if (!tags.has_key("building"))
            return;

        if (!way.is_closed())
            return;

        std::vector<Point> poly;
        poly.reserve(way.nodes().size());

        for (const auto &n : way.nodes())
        {
            if (!n.location().valid())
                return;

            poly.emplace_back(n.location().lat(), n.location().lon());
        }

        if (poly.size() < 4)
            return;

        Building b;

        b.centroid = helper::boundingBoxCenter(poly);

        helper::updateObjectBoundingBox(b.centroid, mGeocoderObjectBB);

        b.housenumber = tags.get_value_by_key("addr:housenumber", "");
        b.street = tags.get_value_by_key("addr:street", "");
        b.postcode = tags.get_value_by_key("addr:postcode", "");
        b.city = tags.get_value_by_key("addr:city", "");
        b.country = tags.get_value_by_key("addr:country", "");

        if (tags.has_key("name:de"))
            b.name = tags.get_value_by_key("name:de");
        else
            b.name = tags.get_value_by_key("name", "");

        b.name.shrink_to_fit();

        buildings.push_back(std::move(b));
    }

    void area(const osmium::Area &area) noexcept
    {
        const auto &tags = area.tags();

        if (!tags.has_key("building"))
            return;

        Building b;

        std::vector<Point> outer;

        for (const auto &ring : area.outer_rings())
        {
            for (const auto &node : ring)
            {
                if (!node.location().valid())
                    return;

                outer.emplace_back(
                    node.location().lat(),
                    node.location().lon());
            }

            break;
        }

        if (outer.size() < 4)
            return;

        b.centroid =
            helper::boundingBoxCenter(outer);

        helper::updateObjectBoundingBox(
            b.centroid,
            mGeocoderObjectBB);

        b.housenumber =
            tags.get_value_by_key(
                "addr:housenumber",
                "");

        b.street =
            tags.get_value_by_key(
                "addr:street",
                "");

        b.postcode =
            tags.get_value_by_key(
                "addr:postcode",
                "");

        b.city =
            tags.get_value_by_key(
                "addr:city",
                "");

        b.country =
            tags.get_value_by_key(
                "addr:country",
                "");

        if (tags.has_key("name:de"))
            b.name =
                tags.get_value_by_key(
                    "name:de");
        else
            b.name =
                tags.get_value_by_key(
                    "name",
                    "");

        buildings.push_back(
            std::move(b));
    }

private:
    std::vector<Building> &buildings;

    std::tuple<Point, Point> mGeocoderObjectBB{
        Point{
            std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity()},
        Point{
            -std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity()}};
};
