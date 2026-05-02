#pragma once

#include "Building.hpp"
#include "Road.hpp"

#include <unordered_map>

struct Key
{
    std::string name;
    RoadType type;
    bool hasName;

    bool operator==(const Key &o) const
    {
        return type == o.type &&
               hasName == o.hasName &&
               name == o.name;
    }
};

struct KeyHash
{
    size_t operator()(const Key &k) const
    {
        return std::hash<std::string>()(k.name) ^
               (std::hash<int>()((int)k.type) << 1) ^
               (std::hash<bool>()(k.hasName) << 2);
    }
};

// 👉 quantisierte Punkte (verhindert float-Probleme)
struct QPoint
{
    int x, y;

    bool operator==(const QPoint &o) const
    {
        return x == o.x && y == o.y;
    }
};

struct QPointHash
{
    size_t operator()(const QPoint &p) const
    {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

static QPoint toQ(const Point &p)
{
    const double scale = 1e6;
    return {
        (int)std::round(p.x * scale),
        (int)std::round(p.y * scale)};
}

class PreProcessingUnit
{
public:
    void preprocessBuildings(std::vector<Building> &buildings);
    void preprocessRoads(std::vector<Road> &roads);

private:
    struct GridKey
    {
        int x, y;
        bool operator==(const GridKey &o) const { return x == o.x && y == o.y; }
    };

    struct GridHash
    {
        size_t operator()(const GridKey &k) const
        {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
        }
    };
};
