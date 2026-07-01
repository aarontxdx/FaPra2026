#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include "GeocoderObjects/Building.hpp"

class Grid
{
public:
    struct Cell
    {
        std::vector<Building *> buildings;
    };

    Grid() = default;

    Grid(double minLat, double minLon,
         double maxLat, double maxLon,
         double cellSize);

    void initialize(double minLat, double minLon,
                    double maxLat, double maxLon,
                    double cellSize);

    void insert(Building *building);

    Cell &getCell(int row, int col);

    const Cell &getCell(int row, int col) const;

    /**
     * @param lat
     * @param lon
     *
     * @return cell coords for given point
     */
    std::pair<int, int> getCellCoords(double lat, double lon) const;

    /**
     * @param row
     * @param col
     *
     * @return if the cell row|col exists
     */
    bool isValidCell(int row, int col) const;

    void getNeighborCells(int row, int col, int radius,
                          std::vector<Building *> &out) const;

    /**
     * @return memory usage of grid in MB
     */
    float memoryUsageMegaBytes() const;

private:
    double m_minLat = 0.0;
    double m_minLon = 0.0;
    double m_cellSize = 0.001;

    int m_rows = 0;
    int m_cols = 0;

    std::vector<std::vector<Cell>> m_cells;

    inline int latToRow(double lat) const
    {
        return static_cast<int>((lat - m_minLat) / m_cellSize);
    }

    inline int lonToCol(double lon) const
    {
        return static_cast<int>((lon - m_minLon) / m_cellSize);
    }
};