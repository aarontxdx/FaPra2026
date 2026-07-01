#include "PreProcessingUnit.hpp"

#include "Utils/MemoryUsageHelper.hpp"
#include "Utils/UtilFunctions.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/point_on_surface.hpp>
#include <chrono>

namespace
{
    /**
     * This function does a PIP test for a specific building
     * and preprocesses the labels of the given building
     */
    void buildingInPolygonTest(
        Building &building,
        const AdminHierarchy &hierarchy)
    {
        // postalcode
        auto postalCodes = helper::pointInPolygon(
            building.centroid,
            hierarchy.postalCodes);

        for (const auto *area : postalCodes)
        {
            if (building.postcode.empty())
                building.postcode = area->postal_code;
        }

        // country (level 2)
        if (building.country.empty())
        {
            auto countries = helper::pointInPolygon(
                building.centroid,
                hierarchy.adminAreaByLevel[2]);

            for (auto *area : countries)
            {
                building.country = area->name;
            }
        }

        // state (level 4)
        auto states = helper::pointInPolygon(
            building.centroid,
            hierarchy.adminAreaByLevel[4]);

        for (auto *area : states)
        {
            building.state = area->name;
        }

        // county (level 6)
        auto counties = helper::pointInPolygon(
            building.centroid,
            hierarchy.adminAreaByLevel[6]);

        for (auto *area : counties)
        {
            building.county = area->name;
        }

        // city (level 8)
        if (building.city.empty())
        {
            auto cities = helper::pointInPolygon(
                building.centroid,
                hierarchy.adminAreaByLevel[8]);

            for (auto *area : cities)
            {
                building.city = area->name;
            }
        }

        if (building.city.empty())
        {
            // district (level 9)
            auto districts = helper::pointInPolygon(
                building.centroid,
                hierarchy.adminAreaByLevel[9]);

            for (auto *area : districts)
            {
                building.city = area->name;
            }
        }
    }

    /**
     * PIP test for roads
     *
     * Determines administrative areas for a road
     */
    void roadInPolygonTest(
        Road &road,
        const AdminHierarchy &hierarchy)
    {
        // postalcode
        auto postalCodes = helper::pointInPolygon(
            road.nodes[0],
            hierarchy.postalCodes);

        for (const auto *area : postalCodes)
        {
            if (road.postcode.empty())
                road.postcode = area->postal_code;
        }

        // country (level 2)
        auto countries = helper::pointInPolygon(
            road.nodes[0],
            hierarchy.adminAreaByLevel[2]);

        for (const auto *area : countries)
        {
            road.country = area->name;
        }

        // state (level 4)
        auto states = helper::pointInPolygon(
            road.nodes[0],
            hierarchy.adminAreaByLevel[4]);

        for (const auto *area : states)
        {
            road.state = area->name;
        }

        // county (level 6)
        auto counties = helper::pointInPolygon(
            road.nodes[0],
            hierarchy.adminAreaByLevel[6]);

        for (const auto *area : counties)
        {
            road.county = area->name;
        }

        // city (level 8)
        auto cities = helper::pointInPolygon(
            road.nodes[0],
            hierarchy.adminAreaByLevel[8]);

        for (const auto *area : cities)
        {
            road.city = area->name;
        }

        // district (level 9)
        auto districts = helper::pointInPolygon(
            road.nodes[0],
            hierarchy.adminAreaByLevel[9]);

        for (const auto *area : districts)
        {
            if (road.city.empty())
            {
                road.city = area->name;
            }
        }
    }

    /**
     * This function updates the boundingbox for a given Point
     */
    void updateObjectBoundingBox(Point &centroid, std::tuple<Point, Point> &GeocoderObjectBB)
    {
        Point &minPoint = std::get<0>(GeocoderObjectBB);
        Point &maxPoint = std::get<1>(GeocoderObjectBB);

        minPoint.lat = std::min(minPoint.lat, centroid.lat);
        minPoint.lon = std::min(minPoint.lon, centroid.lon);
        maxPoint.lat = std::max(maxPoint.lat, centroid.lat);
        maxPoint.lon = std::max(maxPoint.lon, centroid.lon);
    }

    /**
     * insert Buildings into grid
     */
    void buildGrid(Grid &grid, std::vector<Building> &buildings)
    {
        for (auto &building : buildings)
        {
            grid.insert(&building);
        }
    }

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
            (int)std::round(p.lat * scale),
            (int)std::round(p.lon * scale)};
    }

    namespace bg = boost::geometry;

    using BoostPoint = bg::model::point<double, 2, bg::cs::cartesian>;

    /**
     * This function finds a point on a polygon
     * Used to find a representative Point per building
     *
     * @param poly the polygon to test
     *
     * @return Point on the given polygon
     *
     * TODO: Maybe I have to do a point in polygon test before setting a new centroid
     */
    Point representativePoint(const std::vector<Point> &poly)
    {
        namespace bg = boost::geometry;

        bg::model::polygon<BoostPoint> polygon;

        for (const auto &p : poly)
            bg::append(polygon.outer(), BoostPoint(p.lat, p.lon));

        bg::correct(polygon);

        BoostPoint result;
        bg::point_on_surface(polygon, result);

        return {result.get<0>(), result.get<1>()};
    }

    /**
     * This creates a graph from the given road group which is done by connecting end points.
     * It calculates the node degree and adjacence lists
     */
    void buildGraph(const std::vector<Road> &group,
                    std::vector<std::vector<int>> &adj,
                    std::vector<int> &degree)
    {
        int n = group.size();

        std::unordered_map<QPoint, std::vector<int>, QPointHash> endpointMap;

        for (int i = 0; i < n; i++)
        {
            const auto &r = group[i];
            endpointMap[toQ(r.nodes.front())].push_back(i);
            endpointMap[toQ(r.nodes.back())].push_back(i);
        }

        adj.assign(n, {});

        for (int i = 0; i < n; i++)
        {
            const auto &r = group[i];

            QPoint ends[2] = {
                toQ(r.nodes.front()),
                toQ(r.nodes.back())};

            for (auto &ep : ends)
            {
                for (int j : endpointMap[ep])
                {
                    if (i != j)
                        adj[i].push_back(j);
                }
            }

            std::sort(adj[i].begin(), adj[i].end());
            adj[i].erase(std::unique(adj[i].begin(), adj[i].end()), adj[i].end());
        }

        degree.resize(n);
        for (int i = 0; i < n; i++)
            degree[i] = adj[i].size();
    }

    /**
     * goes through the graph from a given startpoint and collects all
     * connected streets, which are not visited at the moment
     */
    std::vector<int> buildChain(int start,
                                const std::vector<std::vector<int>> &adj,
                                std::vector<bool> &visited)
    {
        std::vector<int> chain;

        int current = start;
        int prev = -1;

        while (true)
        {
            if (visited[current])
                break;

            visited[current] = true;
            chain.push_back(current);

            int next = -1;

            for (int nb : adj[current])
            {
                if (nb != prev)
                {
                    next = nb;
                    break;
                }
            }

            if (next == -1)
                break;

            prev = current;
            current = next;
        }

        return chain;
    }

    /**
     * connects multiple road segments according to their end nodes to a single connected road
     */
    std::vector<Point> mergeChainGeometry(const std::vector<int> &chain,
                                          const std::vector<Road> &group)
    {
        auto same = [](const Point &a, const Point &b)
        {
            return std::abs(a.lat - b.lat) < 1e-6 &&
                   std::abs(a.lon - b.lon) < 1e-6;
        };

        std::vector<Point> line;

        for (int idx : chain)
        {
            const auto &r = group[idx];

            if (line.empty())
            {
                line = r.nodes;
                continue;
            }

            if (same(line.back(), r.nodes.front()))
            {
                line.insert(line.end(), r.nodes.begin() + 1, r.nodes.end());
            }
            else if (same(line.back(), r.nodes.back()))
            {
                auto tmp = r.nodes;
                std::reverse(tmp.begin(), tmp.end());
                line.insert(line.end(), tmp.begin() + 1, tmp.end());
            }
            else if (same(line.front(), r.nodes.back()))
            {
                line.insert(line.begin(), r.nodes.begin(), r.nodes.end() - 1);
            }
            else if (same(line.front(), r.nodes.front()))
            {
                auto tmp = r.nodes;
                std::reverse(tmp.begin(), tmp.end());
                line.insert(line.begin(), tmp.begin(), tmp.end() - 1);
            }
        }

        return line;
    }

    /**
     * build and merge chains for the preprocessing of roads
     */
    void mergeChains(const std::vector<Road> &group,
                     const std::vector<std::vector<int>> &adj,
                     const std::vector<int> &degree,
                     const std::string &name,
                     RoadType type,
                     std::vector<Road> &result)
    {
        int n = group.size();
        std::vector<bool> visited(n, false);

        for (int i = 0; i < n; i++)
        {
            if (visited[i])
                continue;

            // build chains
            std::vector<int> chain = buildChain(i, adj, visited);

            // merge their geometry
            std::vector<Point> line = mergeChainGeometry(chain, group);

            if (line.empty())
                continue;

            // build the new roads
            Road merged;
            merged.name = name;
            merged.type = type;
            merged.id = group[i].id;
            merged.nodes = std::move(line);

            result.push_back(std::move(merged));
        }
    }

    /**
     * merge candidate roads together and safe them in a result vector
     *
     * @param group list of Roads that should be merged
     * @param name the name of the road that is tested
     * @param type the type of the road that is tested
     * @param result the list of resulting roads
     */
    void processGroup(std::vector<Road> &group,
                      const std::string &name,
                      RoadType type,
                      std::vector<Road> &result)
    {
        if (group.empty())
            return;

        std::vector<std::vector<int>> adj;
        std::vector<int> degree;

        buildGraph(group, adj, degree);
        mergeChains(group, adj, degree, name, type, result);
    }
} // namespace

void PreProcessingUnit::preprocessAdminAreas(
    std::vector<AdminArea> &adminAreas,
    AdminHierarchy &adminHierarchy)
{
    for (auto &area : adminAreas)
    {
        // postalcode has no admin level
        if (area.boundary == "postal_code")
        {
            adminHierarchy.postalCodes.push_back(&area);
            continue;
        }

        // admin level hierarchy
        int level = area.admin_level;

        if (level >= 0 && level < static_cast<int>(adminHierarchy.adminAreaByLevel.size()))
        {
            adminHierarchy.adminAreaByLevel[level].push_back(&area);
        }
    }
}

void PreProcessingUnit::preprocessBuildings(
    std::vector<Building> &buildings,
    AdminHierarchy &adminHierarchy,
    Grid &grid)
{
    helper::printMemoryUsageBuildings(buildings, "Memory of Buildings before Preprocessing:");

    auto startTimeTotal = std::chrono::steady_clock::now();

    // BB of all objects
    // TODO: This should also be applied to RoadHandler when these objects also count to my ReverseGeocoder
    std::tuple<Point, Point> GeocoderObjectBB(
        Point{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()},
        Point{-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()});

    for (auto &building : buildings)
    {
        building.centroid = representativePoint(building.polygon);
        updateObjectBoundingBox(building.centroid, GeocoderObjectBB);

        building.polygon.clear();
        building.polygon.shrink_to_fit();

        // TODO: For this PIP test sometimes it should be good to use not lat lon but use projections to x/y
        // TODO: (lat lon are not coordinates on a plane)
        // maybe use projections to x,y for every lat lon

        buildingInPolygonTest(building, adminHierarchy);
    }

    auto startTimeGridBuild = std::chrono::steady_clock::now();

    grid.initialize(std::get<0>(GeocoderObjectBB).lat,
                    std::get<0>(GeocoderObjectBB).lon,
                    std::get<1>(GeocoderObjectBB).lat,
                    std::get<1>(GeocoderObjectBB).lon,
                    0.1);

    buildGrid(grid, buildings);

    std::cout << "\nMemoryUsage grid: " << grid.memoryUsageMegaBytes() << " MB" << std::endl;

    auto endTime = std::chrono::steady_clock::now();
    auto applyDurationGridBuild =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTimeGridBuild);
    std::cout << "\nProcess time grid build: " << applyDurationGridBuild.count() << " s\n";

    auto applyDurationTotal =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTimeTotal);
    std::cout << "\nTotal process Time Buildings: " << applyDurationTotal.count() << " s\n";

    helper::printMemoryUsageBuildings(buildings, "Memory of Buildings after Preprocessing:");
}

void PreProcessingUnit::preprocessRoads(
    std::vector<Road> &roads,
    AdminHierarchy &adminHierarchy)
{
    helper::printMemoryUsageRoads(roads, "Memory of Roads before Preprocessing:");

    auto startTime = std::chrono::steady_clock::now();

    // 1. split roads into named and unnamed groups
    std::unordered_map<Key, std::vector<Road>, KeyHash> namedGroups;
    std::unordered_map<RoadType, std::vector<Road>> unnamedGroups;

    for (auto &r : roads)
    {
        if (!r.name.empty() && r.name != "unknown")
        {
            Key k{r.name, r.type, true};
            namedGroups[k].push_back(std::move(r));
        }
        else
        {
            unnamedGroups[r.type].push_back(std::move(r));
        }
    }

    std::vector<Road> result;

    // 2. process each of the two groups
    for (auto &[key, group] : namedGroups)
    {
        processGroup(group, key.name, key.type, result);
    }

    for (auto &[type, group] : unnamedGroups)
    {
        processGroup(group, "", type, result);
    }

    roads = std::move(result);

    for (auto &road : roads)
    {
        roadInPolygonTest(road, adminHierarchy);
    }

    auto endTime = std::chrono::steady_clock::now();
    auto applyDuration =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\nTotal process Time Roads: " << applyDuration.count() << " s\n";

    helper::printMemoryUsageRoads(roads, "Memory of Roads after Preprocessing:");
}