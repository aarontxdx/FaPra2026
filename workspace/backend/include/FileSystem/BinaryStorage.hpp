#pragma once

#include "../GeocoderObjects/Building.hpp"
#include "../GeocoderObjects/Road.hpp"
#include "../GeocoderObjects/AdminArea.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

class BinaryStorage
{
public:
    static bool saveBinary(
        const std::string &filename,
        const std::vector<Building> &buildings,
        const std::vector<AdminArea> &adminAreas,
        const std::vector<Road> &roads);

    static bool loadBinary(
        const std::string &filename,
        std::vector<Building> &buildings,
        std::vector<AdminArea> &adminAreas,
        std::vector<Road> &roads);

private:
    static void writeString(
        std::ofstream &out,
        const std::string &value);

    static void readString(
        std::ifstream &in,
        std::string &value);

    static void writePoint(
        std::ofstream &out,
        const Point &p);

    static void readPoint(
        std::ifstream &in,
        Point &p);

    static void writePoints(
        std::ofstream &out,
        const std::vector<Point> &points);

    static void readPoints(
        std::ifstream &in,
        std::vector<Point> &points);

    static void writeAdminArea(
        std::ofstream &out,
        const AdminArea &area);

    static void readAdminArea(
        std::ifstream &in,
        AdminArea &area);

    static void writeBuilding(
        std::ofstream &out,
        const Building &building);

    static void readBuilding(
        std::ifstream &in,
        Building &building,
        const std::unordered_map<int64_t, AdminArea *> &adminAreaMap);

    static void writeRoad(
        std::ofstream &out,
        const Road &road);

    static void readRoad(
        std::ifstream &in,
        Road &road);
};