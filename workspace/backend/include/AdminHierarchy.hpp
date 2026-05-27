#include "AdminArea.hpp"
#include <array>

struct AdminHierarchy
{
    std::array<std::vector<AdminArea *>, 11> adminAreaByLevel;

    std::vector<AdminArea *> postalCodes;
};