#include "Grid.hpp"

void Grid::insert(Building *building)
{
    const double lat = building->centroid.y;
    const double lon = building->centroid.x;

    auto [row, col] = getCellCoords(lat, lon);

    if (!isValidCell(row, col))
        return;

    m_cells[row][col].buildings.push_back(building);
}