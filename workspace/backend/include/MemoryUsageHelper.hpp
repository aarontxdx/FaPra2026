#include "Building.hpp"
#include "AdminArea.hpp"
#include "Road.hpp"

namespace helper
{
    size_t memoryUsage(const Point &p)
    {
        return sizeof(p);
    }

    size_t memoryUsage(const Building &b)
    {
        size_t size = sizeof(b);

        size += b.housenumber.size();
        size += b.street.size();
        size += b.postcode.size();
        size += b.city.size();
        size += b.country.size();
        size += b.name.size();

        return size;
    }

    size_t memoryUsage(const AdminArea &a)
    {
        size_t size = sizeof(a);

        size += a.name.size();
        size += a.boundary.size();

        for (const auto &ring : a.area)
        {
            size += ring.size() * sizeof(Point);
        }

        return size;
    }

    size_t memoryUsage(const Road &r)
    {
        size_t size = sizeof(r);

        size += r.name.size();
        size += r.nodes.size() * sizeof(Point);

        return size;
    }
}