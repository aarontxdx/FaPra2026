#include "PBFLoader.hpp"

#include <iostream>
#include <chrono>

#include "AreaHandler.hpp"
#include "BuildingHandler.hpp"
#include "RoadHandler.hpp"

#include "MemoryUsageHelper.hpp"

#include <osmium/io/any_input.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>

#include <osmium/version.hpp>
#include <osmium/io/reader.hpp>
#include <osmium/visitor.hpp>

#include <osmium/dynamic_handler.hpp>

using namespace osmium;

std::tuple<std::vector<Building>, std::vector<AdminArea>, std::vector<Road>>
PBFLoader::extractFile(const std::string &path)
{
    const bool DEBUG_MODE = true;

    std::cout << "Start load process\n";

    auto startTime = std::chrono::steady_clock::now();

    std::vector<Building> buildings;
    std::vector<AdminArea> adminAreas;
    std::vector<Road> roads;

    using index_type = osmium::index::map::SparseMemArray<
        osmium::unsigned_object_id_type, osmium::Location>;
    using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();

    AreaHandler area_handler{};
    area_handler.set_admin_vector(adminAreas);

    BuildingHandler building_handler{buildings};

    RoadHandler road_handler{roads};

    // Area-Assembler-Konfiguration
    const osmium::area::Assembler::config_type assembler_config;

    // filter
    osmium::TagsFilter filter{false};
    filter.add_rule(true, "boundary", "administrative");
    filter.add_rule(true, "building", "*");
    filter.add_rule(true, "highway", "motorway");
    filter.add_rule(true, "highway", "trunk");
    filter.add_rule(true, "highway", "primary");
    filter.add_rule(true, "highway", "secondary");
    filter.add_rule(true, "highway", "tertiary");
    filter.add_rule(true, "highway", "residential");
    filter.add_rule(true, "highway", "unclassified");

    // MultipolygonManager
    osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config, filter};

    // 1. Pass: Relationen sammeln
    {
        std::cerr << "Pass 1 (relations)...\n";
        auto input_file = osmium::io::File{path};
        osmium::relations::read_relations(input_file, mp_manager);
        std::cerr << "Pass 1 done\n";
    }

    // 2. Pass: Ways + Areas + NodeLocations + Dispatcher
    {
        std::cerr << "Pass 2 (mp_manager, buildings, roads)...\n";
        osmium::io::Reader reader{path};
        osmium::apply(reader, location_handler, building_handler, road_handler,
                      mp_manager.handler([&area_handler](osmium::memory::Buffer &&buffer)
                                         { osmium::apply(buffer, area_handler); }));
        reader.close();
        std::cerr << "Pass 2 done\n\n";
    }

    auto endTime = std::chrono::steady_clock::now();
    auto applyDuration =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    if (DEBUG_MODE)
    {
        helper::printMemoryUsage(buildings, adminAreas, roads);
    }

    std::cout << "Load process finished! \n\n";
    std::cout << "Total Load Time: " << applyDuration.count() << " s\n\n";

    return {buildings, adminAreas, roads};
}
