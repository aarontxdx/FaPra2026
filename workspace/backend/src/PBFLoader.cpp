#include "PBFLoader.hpp"

#include <iostream>
#include <chrono>

#include "AreaHandler.hpp"
#include "BuildingHandler.hpp"

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

std::tuple<std::vector<Building>, std::vector<AdminArea>>
PBFLoader::extractFile(const std::string &path)
{
    std::cout << "Start load process\n";
    std::cout << LIBOSMIUM_VERSION_STRING << "\n";

    auto startTime = std::chrono::steady_clock::now();

    std::vector<Building> buildings;
    std::vector<AdminArea> adminAreas;

    using index_type = osmium::index::map::SparseMemArray<
        osmium::unsigned_object_id_type, osmium::Location>;
    using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

    index_type index;
    location_handler_type location_handler{index};
    location_handler.ignore_errors();

    AreaHandler area_handler{};
    area_handler.set_admin_vector(adminAreas);

    BuildingHandler building_handler{buildings};

    // Area-Assembler-Konfiguration
    const osmium::area::Assembler::config_type assembler_config;

    // filter
    osmium::TagsFilter filter{false};
    filter.add_rule(true, "boundary", "administrative");
    filter.add_rule(true, "boundary", "building");

    // MultipolygonManager
    osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config, filter};

    // 1. Pass: Relationen sammeln
    {
        std::cerr << "Pass 1...\n";
        auto input_file = osmium::io::File{path};
        osmium::relations::read_relations(input_file, mp_manager);
        std::cerr << "Pass 1 done\n";
    }

    // Output the amount of main memory used so far. All multipolygon relations
    // are in memory now.
    std::cerr << "Memory:\n";
    osmium::relations::print_used_memory(std::cerr, mp_manager.used_memory());

    // 2. Pass: Ways + Areas + NodeLocations + Dispatcher
    {
        std::cerr << "Pass 2...\n";
        osmium::io::Reader reader{path};
        osmium::apply(reader, location_handler, building_handler, mp_manager.handler([&area_handler](osmium::memory::Buffer &&buffer)
                                                                                     { osmium::apply(buffer, area_handler); }));
        reader.close();
        std::cerr << "Pass 2 done\n";
    }

    // print used Memory so far
    std::cerr << "Memory:\n";
    osmium::relations::print_used_memory(std::cerr, mp_manager.used_memory());

    auto endTime = std::chrono::steady_clock::now();
    auto applyDuration =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "Load process finished! \nBuildings: " << buildings.size() << "\n"
              << "Areas: " << adminAreas.size() << "\n";
    std::cout << "Total Load Time: " << applyDuration.count() << " s\n";

    return {buildings, adminAreas};
}
