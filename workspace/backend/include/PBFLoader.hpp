#pragma once

#include "GeocoderObjects/AdminArea.hpp"
#include "GeocoderObjects/Building.hpp"
#include "GeocoderObjects/Road.hpp"

#include <string>
#include <vector>

class PBFLoader
{
public:
    /**
     * extract a given pbf file
     *
     * @param path file path of .pbf
     */
    std::tuple<Point, Point> extractFile(std::vector<Building> &buildings,
                                         std::vector<AdminArea> &adminAreas,
                                         std::vector<Road> &roads, const std::string &path);
};
