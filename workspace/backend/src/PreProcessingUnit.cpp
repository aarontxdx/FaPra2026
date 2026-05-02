#include "PreProcessingUnit.hpp"

#include "MemoryUsageHelper.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/point_on_surface.hpp>

namespace
{
    namespace bg = boost::geometry;

    using BoostPoint = bg::model::point<double, 2, bg::cs::cartesian>;

    Point representativePoint(const std::vector<Point> &poly)
    {
        namespace bg = boost::geometry;

        bg::model::polygon<BoostPoint> polygon;

        for (const auto &p : poly)
            bg::append(polygon.outer(), BoostPoint(p.x, p.y));

        bg::correct(polygon);

        BoostPoint result;
        bg::point_on_surface(polygon, result);

        return {result.get<0>(), result.get<1>()};
    }
}

void PreProcessingUnit::preprocessBuildings(std::vector<Building> &buildings)
{
    helper::printMemoryUsageBuildings(buildings, "Memory of Buildings before Preprocessing:");

    auto startTime = std::chrono::steady_clock::now();

    for (auto &building : buildings)
    {
        building.centroid = representativePoint(building.polygon);
        building.polygon.clear();
        building.polygon.shrink_to_fit();
    }

    auto endTime = std::chrono::steady_clock::now();
    auto applyDuration =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    std::cout << "\nTotal process Time Buildings: " << applyDuration.count() << " s\n";

    helper::printMemoryUsageBuildings(buildings, "Memory of Buildings after Preprocessing:");
}

void PreProcessingUnit::preprocessRoads(std::vector<Road> &roads)
{
    helper::printMemoryUsageRoads(roads, "Memory of Roads before Preprocessing:");

    auto startTime = std::chrono::steady_clock::now();

    // --------------------------------------------------
    // 1. Filter ungültige Roads
    // --------------------------------------------------
    std::vector<Road> valid;
    valid.reserve(roads.size());

    for (auto &r : roads)
    {
        if (r.nodes.size() >= 2)
            valid.push_back(std::move(r));
    }

    roads.clear();

    // --------------------------------------------------
    // 2. Split: named vs unnamed
    // --------------------------------------------------
    std::unordered_map<Key, std::vector<Road>, KeyHash> namedGroups;
    std::unordered_map<RoadType, std::vector<Road>> unnamedGroups;

    for (auto &r : valid)
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

    // --------------------------------------------------
    // 3. Ergebniscontainer (WICHTIG: vor Lambda!)
    // --------------------------------------------------
    std::vector<Road> result;

    // --------------------------------------------------
    // 4. Merge-Funktion
    // --------------------------------------------------
    auto processGroup = [&result](std::vector<Road> &group,
                                  const std::string &name,
                                  RoadType type)
    {
        int n = group.size();
        if (n == 0)
            return;

        std::unordered_map<QPoint, std::vector<int>, QPointHash> endpointMap;

        for (int i = 0; i < n; i++)
        {
            const auto &r = group[i];
            endpointMap[toQ(r.nodes.front())].push_back(i);
            endpointMap[toQ(r.nodes.back())].push_back(i);
        }

        std::vector<std::vector<int>> adj(n);

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

            // Duplikate entfernen
            std::sort(adj[i].begin(), adj[i].end());
            adj[i].erase(std::unique(adj[i].begin(), adj[i].end()), adj[i].end());
        }

        std::vector<int> degree(n);
        for (int i = 0; i < n; i++)
            degree[i] = adj[i].size();

        std::vector<bool> visited(n, false);

        auto same = [](const Point &a, const Point &b)
        {
            return std::abs(a.x - b.x) < 1e-6 &&
                   std::abs(a.y - b.y) < 1e-6;
        };

        for (int i = 0; i < n; i++)
        {
            if (visited[i])
                continue;

            std::vector<int> chain;
            int current = i;
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

            // Merge
            Road merged;
            merged.name = name;
            merged.type = type;
            merged.id = group[i].id;

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

            if (!line.empty())
            {
                merged.nodes = std::move(line);
                result.push_back(std::move(merged));
            }
        }
    };

    // --------------------------------------------------
    // 5. Alle Gruppen verarbeiten
    // --------------------------------------------------
    for (auto &[key, group] : namedGroups)
    {
        processGroup(group, key.name, key.type);
    }

    for (auto &[type, group] : unnamedGroups)
    {
        processGroup(group, "", type);
    }

    // --------------------------------------------------
    // 6. Ergebnis zurückschreiben
    // --------------------------------------------------
    roads = std::move(result);

    auto endTime = std::chrono::steady_clock::now();
    auto applyDuration =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\nTotal process Time Roads: " << applyDuration.count() << " s\n";

    helper::printMemoryUsageRoads(roads, "Memory of Roads after Preprocessing:");
}