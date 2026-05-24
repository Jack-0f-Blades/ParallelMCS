#ifndef STATIC_ORDER_MCS_H
#define STATIC_ORDER_MCS_H

#include "MCQ.h"
#include <vector>

class StaticOrderMCS : public MCQ
{
public:
    StaticOrderMCS(std::vector<std::vector<char>> const &vAdjacencyMatrix);

    virtual void InitializeOrder(std::vector<int> &P, std::vector<int> &vVertexOrder,
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
};

#endif