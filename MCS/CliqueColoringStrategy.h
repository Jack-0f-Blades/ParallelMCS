#ifndef CLIQUE_COLORING_STRATEGY_H
#define CLIQUE_COLORING_STRATEGY_H

#include "ColoringStrategy.h"
#include <vector>
#include <atomic>

class CliqueColoringStrategy : public ColoringStrategy
{
public:
    CliqueColoringStrategy(std::vector<std::vector<char>> const &adjacencyMatrix);
    virtual void Color(std::vector<std::vector<char>> const &adjacencyMatrix,
                       std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) override;

    virtual void Recolor(std::vector<std::vector<char>> const &adjacencyMatrix,
                         std::vector<int> const &vVertexOrder,
                         std::vector<int> &vVerticesToReorder,
                         std::vector<int> &vColors,
                         int const currentBestCliqueSize,
                         int const currentCliqueSize);

    bool HasConflict(int const vertex, std::vector<int> const &vVerticesWithColor);
    int GetConflictingVertex(int const vertex, std::vector<int> const &vVerticesWithColor);
    bool Repair(int const vertex, int const color, int const iBestCliqueDelta);

    void SetStopFlag(std::atomic<bool>* stopFlag) { m_stopFlag = stopFlag; }
    size_t getRepairCount() const { return m_repairCount; }

protected:
    std::vector<std::vector<char>> const &m_AdjacencyMatrix;
    std::vector<std::vector<int>> m_vvVerticesWithColor;
    std::atomic<bool>* m_stopFlag = nullptr;
    size_t m_repairCount = 0;
};

#endif