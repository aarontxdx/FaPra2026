#include "BinaryStorage.hpp"

#include <iostream>

struct BinaryHeader
{
    char magic[8] = "GEOCODE";
    uint32_t version = 1;

    uint64_t buildings;
    uint64_t adminAreas;
    uint64_t roads;
};

void BinaryStorage::writeString(
    std::ofstream &out,
    const std::string &value)
{
    uint32_t size = static_cast<uint32_t>(value.size());

    out.write(
        reinterpret_cast<char *>(&size),
        sizeof(size));

    out.write(
        value.data(),
        size);
}

void BinaryStorage::readString(
    std::ifstream &in,
    std::string &value)
{
    uint32_t size;

    in.read(
        reinterpret_cast<char *>(&size),
        sizeof(size));

    value.resize(size);

    in.read(
        value.data(),
        size);
}

void BinaryStorage::writePoint(
    std::ofstream &out,
    const Point &p)
{
    out.write(
        reinterpret_cast<const char *>(&p.lat),
        sizeof(double));

    out.write(
        reinterpret_cast<const char *>(&p.lon),
        sizeof(double));
}

void BinaryStorage::readPoint(
    std::ifstream &in,
    Point &p)
{
    in.read(
        reinterpret_cast<char *>(&p.lat),
        sizeof(double));

    in.read(
        reinterpret_cast<char *>(&p.lon),
        sizeof(double));
}

void BinaryStorage::writePoints(
    std::ofstream &out,
    const std::vector<Point> &points)
{
    uint32_t size =
        static_cast<uint32_t>(points.size());

    out.write(
        reinterpret_cast<char *>(&size),
        sizeof(size));

    for (const auto &p : points)
    {
        writePoint(out, p);
    }
}

void BinaryStorage::readPoints(
    std::ifstream &in,
    std::vector<Point> &points)
{
    uint32_t size;

    in.read(
        reinterpret_cast<char *>(&size),
        sizeof(size));

    points.resize(size);

    for (auto &p : points)
    {
        readPoint(in, p);
    }
}

void BinaryStorage::writeAdminArea(
    std::ofstream &out,
    const AdminArea &area)
{
    // GeocoderObject
    writeString(out, area.name);

    writeString(out, area.country);
    writeString(out, area.state);
    writeString(out, area.county);
    writeString(out, area.city);
    writeString(out, area.postcode);

    // eigene Daten
    out.write(
        reinterpret_cast<const char *>(&area.admin_level),
        sizeof(area.admin_level));

    writeString(out, area.boundary);

    writeString(out, area.postal_code);

    // Polygone
    uint32_t rings =
        static_cast<uint32_t>(area.area.size());

    out.write(
        reinterpret_cast<char *>(&rings),
        sizeof(rings));

    for (const auto &ring : area.area)
    {
        writePoints(out, ring);
    }

    // Bounding Box
    auto [min, max] = area.bb;

    writePoint(out, min);
    writePoint(out, max);

    out.write(
        reinterpret_cast<const char *>(&area.id),
        sizeof(area.id));
}

void BinaryStorage::readAdminArea(
    std::ifstream &in,
    AdminArea &area)
{
    readString(in, area.name);

    readString(in, area.country);
    readString(in, area.state);
    readString(in, area.county);
    readString(in, area.city);
    readString(in, area.postcode);

    in.read(
        reinterpret_cast<char *>(&area.admin_level),
        sizeof(area.admin_level));

    readString(in, area.boundary);

    readString(in, area.postal_code);

    uint32_t rings;

    in.read(
        reinterpret_cast<char *>(&rings),
        sizeof(rings));

    area.area.resize(rings);

    for (auto &ring : area.area)
    {
        readPoints(in, ring);
    }

    Point min;
    Point max;

    readPoint(in, min);
    readPoint(in, max);

    area.bb =
        std::make_tuple(min, max);

    in.read(
        reinterpret_cast<char *>(&area.id),
        sizeof(area.id));
}

void BinaryStorage::writeBuilding(
    std::ofstream &out,
    const Building &building)
{
    // GeocoderObject
    writeString(out, building.name);
    writeString(out, building.country);
    writeString(out, building.state);
    writeString(out, building.county);
    writeString(out, building.city);
    writeString(out, building.postcode);

    // Building own data
    writeString(out, building.housenumber);
    writeString(out, building.street);

    // Polygon
    writePoints(
        out,
        building.polygon);

    // Centroid
    writePoint(
        out,
        building.centroid);

    // AdminArea Referenzen
    uint32_t count =
        static_cast<uint32_t>(
            building.adminAreas.size());

    out.write(
        reinterpret_cast<char *>(&count),
        sizeof(count));

    for (const auto *area : building.adminAreas)
    {
        int64_t id = area->id;
        out.write(
            reinterpret_cast<char *>(&id),
            sizeof(id));
    }
}

void BinaryStorage::readBuilding(
    std::ifstream &in,
    Building &building,
    const std::unordered_map<int64_t, AdminArea *> &adminAreaMap)
{
    // GeocoderObject
    readString(in, building.name);
    readString(in, building.country);
    readString(in, building.state);
    readString(in, building.county);
    readString(in, building.city);
    readString(in, building.postcode);

    // Building Daten
    readString(
        in,
        building.housenumber);

    readString(
        in,
        building.street);

    // Polygon
    readPoints(
        in,
        building.polygon);

    // Centroid
    readPoint(
        in,
        building.centroid);

    // AdminArea Links
    uint32_t count;

    in.read(
        reinterpret_cast<char *>(&count),
        sizeof(count));

    building.adminAreas.clear();

    for (uint32_t i = 0; i < count; i++)
    {
        int64_t id;
        in.read(
            reinterpret_cast<char *>(&id),
            sizeof(id));
        auto it = adminAreaMap.find(id);
        if (it != adminAreaMap.end())
        {
            building.adminAreas.push_back(
                it->second);
        }
    }
}

static void writeRoadType(
    std::ofstream &out,
    RoadType type)
{
    int value = static_cast<int>(type);
    out.write(
        reinterpret_cast<const char *>(&value),
        sizeof(value));
}

static RoadType readRoadType(
    std::ifstream &in)
{
    int value;
    in.read(
        reinterpret_cast<char *>(&value),
        sizeof(value));

    return static_cast<RoadType>(value);
}

void BinaryStorage::writeRoad(
    std::ofstream &out,
    const Road &road)
{
    // GeocoderObject
    writeString(out, road.name);
    writeString(out, road.country);
    writeString(out, road.state);
    writeString(out, road.county);
    writeString(out, road.city);
    writeString(out, road.postcode);

    // Road specific
    writeRoadType(out, road.type);
    out.write(
        reinterpret_cast<const char *>(&road.id),
        sizeof(road.id));

    // Nodes
    size_t nodeCount = road.nodes.size();
    out.write(
        reinterpret_cast<const char *>(&nodeCount),
        sizeof(nodeCount));

    for (const auto &point : road.nodes)
    {
        out.write(
            reinterpret_cast<const char *>(&point.lat),
            sizeof(point.lat));

        out.write(
            reinterpret_cast<const char *>(&point.lon),
            sizeof(point.lon));
    }
}

void BinaryStorage::readRoad(
    std::ifstream &in,
    Road &road)
{
    // GeocoderObject
    readString(in, road.name);
    readString(in, road.country);
    readString(in, road.state);
    readString(in, road.county);
    readString(in, road.city);
    readString(in, road.postcode);

    // Road specific
    road.type = readRoadType(in);
    in.read(
        reinterpret_cast<char *>(&road.id),
        sizeof(road.id));

    // Nodes
    uint64_t nodeCount;
    in.read(
        reinterpret_cast<char *>(&nodeCount),
        sizeof(nodeCount));

    road.nodes.resize(
        static_cast<size_t>(nodeCount));

    for (auto &point : road.nodes)
    {
        in.read(
            reinterpret_cast<char *>(&point.lat),
            sizeof(point.lat));

        in.read(
            reinterpret_cast<char *>(&point.lon),
            sizeof(point.lon));
    }
}

bool BinaryStorage::saveBinary(
    const std::string &filename,
    const std::vector<Building> &buildings,
    const std::vector<AdminArea> &adminAreas,
    const std::vector<Road> &roads)
{
    std::ofstream out(
        filename,
        std::ios::binary);

    if (!out.is_open())
    {
        std::cerr
            << "Cannot open binary file: "
            << filename
            << std::endl;

        return false;
    }

    BinaryHeader header{};

    std::memcpy(
        header.magic,
        "GEOCODE",
        7);

    header.version = 1;

    header.buildings =
        static_cast<uint64_t>(buildings.size());

    header.adminAreas =
        static_cast<uint64_t>(adminAreas.size());

    header.roads =
        static_cast<uint64_t>(roads.size());

    out.write(
        reinterpret_cast<const char *>(&header),
        sizeof(header));

    if (!out)
    {
        std::cerr
            << "Failed writing binary header"
            << std::endl;

        return false;
    }

    std::cout
        << "Saving admin areas: "
        << adminAreas.size()
        << std::endl;

    for (const auto &area : adminAreas)
    {
        writeAdminArea(
            out,
            area);

        if (!out)
        {
            std::cerr
                << "Failed writing admin area"
                << std::endl;
            return false;
        }
    }

    std::cout
        << "Saving buildings: "
        << buildings.size()
        << std::endl;

    for (const auto &building : buildings)
    {
        writeBuilding(
            out,
            building);

        if (!out)
        {
            std::cerr
                << "Failed writing building"
                << std::endl;
            return false;
        }
    }

    std::cout
        << "Saving roads: "
        << roads.size()
        << std::endl;

    for (const auto &road : roads)
    {
        writeRoad(
            out,
            road);

        if (!out)
        {
            std::cerr
                << "Failed writing road"
                << std::endl;
            return false;
        }
    }

    out.close();

    if (!out)
    {
        std::cerr
            << "Error closing binary file"
            << std::endl;
        return false;
    }

    std::cout
        << "Binary save successful: "
        << filename
        << std::endl;

    return true;
}

bool BinaryStorage::loadBinary(
    const std::string &filename,
    std::vector<Building> &buildings,
    std::vector<AdminArea> &adminAreas,
    std::vector<Road> &roads)
{
    std::ifstream in(
        filename,
        std::ios::binary);

    if (!in.is_open())
    {
        std::cerr
            << "Cannot open binary file: "
            << filename
            << std::endl;

        return false;
    }

    BinaryHeader header{};

    in.read(
        reinterpret_cast<char *>(&header),
        sizeof(header));

    if (!in)
    {
        std::cerr
            << "Failed reading binary header"
            << std::endl;

        return false;
    }

    if (std::string(header.magic) != "GEOCODE")
    {
        std::cerr
            << "Invalid binary format"
            << std::endl;

        return false;
    }

    if (header.version != 1)
    {
        std::cerr
            << "Unsupported binary version: "
            << header.version
            << std::endl;

        return false;
    }

    buildings.clear();
    adminAreas.clear();
    roads.clear();

    adminAreas.resize(
        static_cast<size_t>(header.adminAreas));

    std::cout
        << "Loading admin areas: "
        << adminAreas.size()
        << std::endl;

    for (auto &area : adminAreas)
    {
        readAdminArea(
            in,
            area);

        if (!in)
        {
            std::cerr
                << "Failed reading admin area"
                << std::endl;

            return false;
        }
    }

    std::unordered_map<int64_t, AdminArea *> adminAreaMap;

    for (auto &area : adminAreas)
    {
        adminAreaMap[area.id] =
            &area;
    }

    buildings.resize(
        static_cast<size_t>(header.buildings));

    std::cout
        << "Loading buildings: "
        << buildings.size()
        << std::endl;

    for (auto &building : buildings)
    {
        readBuilding(
            in,
            building,
            adminAreaMap);

        if (!in)
        {
            std::cerr
                << "Failed reading building"
                << std::endl;

            return false;
        }
    }

    roads.resize(
        static_cast<size_t>(header.roads));

    std::cout
        << "Loading roads: "
        << roads.size()
        << std::endl;

    for (auto &road : roads)
    {
        readRoad(
            in,
            road);

        if (!in)
        {
            std::cerr
                << "Failed reading road"
                << std::endl;

            return false;
        }
    }

    in.close();

    std::cout
        << "Binary load successful\n"
        << "AdminAreas: "
        << adminAreas.size()
        << "\nBuildings: "
        << buildings.size()
        << "\nRoads: "
        << roads.size()
        << std::endl;

    return true;
}