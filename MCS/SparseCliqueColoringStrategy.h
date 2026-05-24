#ifndef SPARSE_CLIQUE_COLORING_STRATEGY_H
#define SPARSE_CLIQUE_COLORING_STRATEGY_H

#include "ColoringStrategy.h"
#include <vector>
#include <list>
#include <atomic>

class SparseCliqueColoringStrategy : public ColoringStrategy
{
public:
    SparseCliqueColoringStrategy(std::vector<std::vector<int>> const &adjacencyArray);
    
    // Реализация для разреженного списка смежности (используется в SparseMCS)
    virtual void Color(std::vector<std::vector<int>> const &adjacencyArray,
                       std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) override;
    
    // Перегрузка для плотной матрицы (не используется, но нужна для компиляции)
    virtual void Color(std::vector<std::vector<char>> const &adjacencyMatrix,
                       std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) override {}
    
    virtual void Recolor(std::vector<std::vector<int>> const &adjacencyArray,
                         std::vector<int> const &vVertexOrder,
                         std::vector<int> &vVerticesToReorder,
                         std::vector<int> &vColors,
                         int const currentBestCliqueSize,
                         int const currentCliqueSize);
    
    void SetStopFlag(std::atomic<bool>* flag) { m_stopFlag = flag; }
    size_t getRepairCount() const { return m_repairCount; }

private:
    bool HasConflict(int vertex, const std::vector<int>& verticesWithColor);
    int GetConflictingVertex(int vertex, const std::vector<int>& verticesWithColor);
    bool Repair(int vertex, int color, int iBestCliqueDelta);

    std::vector<std::vector<int>> const &m_AdjacencyArray;
    std::vector<std::vector<int>> m_vvVerticesWithColor;
    std::vector<int> m_vVertexToColor;
    std::vector<int> m_vNeighborColorCount;
    std::vector<bool> m_vbNeighbors;
    std::vector<bool> m_vbConflictNeighbors;
    std::atomic<bool>* m_stopFlag = nullptr;
    size_t m_repairCount = 0;
};

#endif