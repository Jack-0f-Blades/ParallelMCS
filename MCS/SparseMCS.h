#ifndef SPARSE_MCS_H
#define SPARSE_MCS_H

#include "MaxSubgraphAlgorithm.h"
#include "SparseCliqueColoringStrategy.h"
#include <vector>

class SparseMCS : public MaxSubgraphAlgorithm
{
public:
    SparseMCS(std::vector<std::vector<int>> const &adjacencyArray);

    virtual void Color(std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) override;

    virtual void InitializeOrder(std::vector<int> &P,
                                 std::vector<int> &vVertexOrder,
                                 std::vector<int> &vColors) override;

    virtual void GetNewOrderLocal(std::vector<int> &vNewVertexOrder,
                                  std::vector<int> &vVertexOrder,
                                  std::vector<int> const &P,
                                  int const chosenVertex,
                                  std::vector<int> &R_local) override;

    virtual void ProcessOrderAfterRecursionLocal(std::vector<int> &vVertexOrder,
                                                 std::vector<int> &P,
                                                 std::vector<int> &vColors,
                                                 int const chosenVertex,
                                                 std::vector<int> &R_local) override;

    virtual void ProcessOrderBeforeReturnLocal(std::vector<int> &vVertexOrder,
                                               std::vector<int> &P,
                                               std::vector<int> &vColors,
                                               std::vector<int> &R_local) override;

    virtual size_t getRepairCount() const override { return coloringStrategy.getRepairCount(); }

protected:
    std::vector<std::vector<int>> const &m_AdjacencyArray;
    SparseCliqueColoringStrategy coloringStrategy;
};

#endif // SPARSE_MCS_H