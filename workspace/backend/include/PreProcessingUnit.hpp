#pragma once

#include "Building.hpp"
#include "Road.hpp"

#include <unordered_map>

/**
 * Represents a grouping identifier for roads based on
 * their name, type, and whether they have a valid name.
 */
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

/**
 * Provides a hash function for Key so it can be used efficiently
 * in hash-based containers like std::unordered_map.
 */
struct KeyHash
{
    size_t operator()(const Key &k) const
    {
        return std::hash<std::string>()(k.name) ^
               (std::hash<int>()((int)k.type) << 1) ^
               (std::hash<bool>()(k.hasName) << 2);
    }
};

/**
 * Represents a quantized (integer-based) version of a
 * geographic point for robust spatial comparisons.
 */
struct QPoint
{
    int x, y;

    bool operator==(const QPoint &o) const
    {
        return x == o.x && y == o.y;
    }
};

/**
 * Provides a hash function for QPoint to enable fast lookup in hash maps.
 */
struct QPointHash
{
    size_t operator()(const QPoint &p) const
    {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

/**
 * Converts a floating-point coordinate into a quantized integer point
 * to reduce precision issues when comparing locations.
 */
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
    /**
     * preprocessing of buildings
     *
     * Finds a more representative point for a building
     *
     * (currently there is no filter or function to give
     * a building more information, therefore many buildings
     * are unnamed and partly have no housenumber)
     *
     * @param buildings list of Building objects
     */
    void preprocessBuildings(std::vector<Building> &buildings);

    /**
     * preprocessing of roads
     *
     * merge roads that have the same name and type
     *
     * TODO: some of the roads doesn't get merged together
     * TODO: (find out if the issue is because of extraction or preprocessing)
     *
     * @param roads list of Road objects
     */
    void preprocessRoads(std::vector<Road> &roads);
};
