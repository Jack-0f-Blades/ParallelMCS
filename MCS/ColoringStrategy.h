/**
 * Базовый класс стратегии раскраски. Содержит виртуальные методы для раскраски и перекраски.
 */
#ifndef COLORING_STRATEGY_H
#define COLORING_STRATEGY_H

#include <vector>

class ColoringStrategy
{
public:
    virtual ~ColoringStrategy() {}
    virtual void Color(std::vector<std::vector<char>> const &adjacencyMatrix,
                       std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) = 0;
    virtual void Recolor(std::vector<std::vector<char>> const &adjacencyMatrix,
                         std::vector<int> const &vVertexOrder,
                         std::vector<int> &vVerticesToReorder,
                         std::vector<int> &vColors,
                         int const currentBestCliqueSize,
                         int const currentCliqueSize) {}
    virtual void Color(std::vector<std::vector<int>> const &adjacencyList,
                       std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) {}
};

#endif