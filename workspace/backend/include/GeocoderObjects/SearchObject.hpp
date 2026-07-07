#pragma once

#include "Building.hpp"
#include "Road.hpp"
#include "AdminArea.hpp"

#include <variant>

namespace geocoder
{
    using SearchObject = std::variant<
        AdminArea *,
        Building *,
        Road *>;
}