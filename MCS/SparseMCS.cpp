#include "SparseMCS.h"
#include "OrderingTools.h"
#include <iostream>

using namespace std;

SparseMCS::SparseMCS(vector<vector<int>> const &adjacencyArray)
    : MaxSubgraphAlgorithm("sparse-mcs"),
      m_AdjacencyArray(adjacencyArray),
      coloringStrategy(m_AdjacencyArray)
{
    coloringStrategy.SetStopFlag(&m_bTimedOut);

    R.reserve(m_AdjacencyArray.size());
    stackP.resize(m_AdjacencyArray.size() + 1);
    stackColors.resize(m_AdjacencyArray.size() + 1);
    stackOrder.resize(m_AdjacencyArray.size() + 1);
    for (size_t i = 0; i < stackP.size(); ++i) {
        stackP[i].reserve(m_AdjacencyArray.size());
        stackColors[i].reserve(m_AdjacencyArray.size());
        stackOrder[i].reserve(m_AdjacencyArray.size());
    }
}

void SparseMCS::InitializeOrder(vector<int> &P, vector<int> &vVertexOrder, vector<int> &vColors)
{
    size_t dummyCliqueSize = 0;
    InitialOrderingMCR(m_AdjacencyArray, P, vColors, dummyCliqueSize, &m_bTimedOut);
    m_uMaximumCliqueSize = dummyCliqueSize;
    vVertexOrder = P;
}

void SparseMCS::Color(vector<int> const &vVertexOrder,
                      vector<int> &vVerticesToReorder,
                      vector<int> &vColors)
{
    coloringStrategy.Recolor(m_AdjacencyArray, vVertexOrder, vVerticesToReorder, vColors,
                             static_cast<int>(m_uMaximumCliqueSize.load()),
                             static_cast<int>(R.size()));
}

void SparseMCS::GetNewOrderLocal(vector<int> &vNewVertexOrder,
                                 vector<int> &vVertexOrder,
                                 vector<int> const &P,
                                 int const chosenVertex,
                                 vector<int> &R_local)
{
    if (chosenVertex != -1) R_local.push_back(chosenVertex);
    vNewVertexOrder.clear();
    // Временный булевый массив для соседей chosenVertex
    vector<bool> isNeighbor(m_AdjacencyArray.size(), false);
    for (int nb : m_AdjacencyArray[chosenVertex]) isNeighbor[nb] = true;
    for (int candidate : vVertexOrder) {
        if (candidate != chosenVertex && isNeighbor[candidate])
            vNewVertexOrder.push_back(candidate);
    }
}

void SparseMCS::ProcessOrderAfterRecursionLocal(vector<int> &vVertexOrder,
                                                vector<int> &P,
                                                vector<int> &vColors,
                                                int const chosenVertex,
                                                vector<int> &R_local)
{
    if (chosenVertex != -1) R_local.pop_back();
}

void SparseMCS::ProcessOrderBeforeReturnLocal(vector<int> &vVertexOrder,
                                              vector<int> &P,
                                              vector<int> &vColors,
                                              vector<int> &R_local)
{
    // Ничего не делаем
}