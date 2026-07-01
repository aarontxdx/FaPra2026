#include "DataStructures/Grid.hpp"
#include "iostream"

Grid::Grid(double minLat, double minLon,
           double maxLat, double maxLon,
           double cellSize)
{
    initialize(minLat, minLon, maxLat, maxLon, cellSize);
}

void Grid::initialize(double minLat, double minLon,
                      double maxLat, double maxLon,
                      double cellSize)
{
    if (cellSize <= 0.0 || maxLat <= minLat || maxLon <= minLon)
        throw std::runtime_error("Invalid grid parameters");

    m_minLat = minLat;
    m_minLon = minLon;
    m_cellSize = cellSize;

    m_rows = static_cast<int>((maxLat - minLat) / cellSize) + 1;
    m_cols = static_cast<int>((maxLon - minLon) / cellSize) + 1;

    m_cells.clear();
    m_cells.resize(m_rows);

    for (int r = 0; r < m_rows; r++)
    {
        m_cells[r].resize(m_cols);
    }
}

void Grid::insert(Building *building)
{
    const double lat = building->centroid.lat;
    const double lon = building->centroid.lon;

    auto [row, col] = getCellCoords(lat, lon);

    if (!isValidCell(row, col))
        return;

    m_cells[row][col].buildings.push_back(building);
}

Grid::Cell &Grid::getCell(int row, int col)
{
    return m_cells[row][col];
}

const Grid::Cell &Grid::getCell(int row, int col) const
{
    return m_cells[row][col];
}

std::pair<int, int> Grid::getCellCoords(double lat, double lon) const
{
    return {
        latToRow(lat),
        lonToCol(lon)};
}

bool Grid::isValidCell(int row, int col) const
{
    return row >= 0 && row < m_rows &&
           col >= 0 && col < m_cols;
}

void Grid::getNeighborCells(int row, int col, int radius,
                            std::vector<Building *> &out) const
{
    for (int r = row - radius; r <= row + radius; r++)
    {
        for (int c = col - radius; c <= col + radius; c++)
        {
            if (!isValidCell(r, c))
                continue;

            const auto &cellBuildings = m_cells[r][c].buildings;

            out.insert(out.end(),
                       cellBuildings.begin(),
                       cellBuildings.end());
        }
    }
}

float Grid::memoryUsageMegaBytes() const
{
    std::size_t total = 0;

    // Grid object itself
    total += sizeof(Grid);

    // Outer vector storage
    total += m_cells.capacity() * sizeof(std::vector<Cell>);

    for (const auto &row : m_cells)
    {
        // Inner vector storage
        total += row.capacity() * sizeof(Cell);

        for (const auto &cell : row)
        {
            // Buildings pointer array
            total += cell.buildings.capacity() * sizeof(Building *);
        }
    }

    return total / 1000000.0;
}