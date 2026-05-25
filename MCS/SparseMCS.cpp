#include "SparseMCS.h"
#include "OrderingTools.h"
#include <algorithm>

using namespace std;

SparseMCS::SparseMCS(vector<vector<int>> const &adjacencyArray)
    : MaxSubgraphAlgorithm("sparse-mcs"),
      m_AdjacencyArray(adjacencyArray),
      coloringStrategy(adjacencyArray)
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
    vector<int> dummyCliqueVertices; // временный вектор для outClique
    OrderingTools::InitialOrderingMCR(m_AdjacencyArray, P, vColors, dummyCliqueSize, dummyCliqueVertices, &m_bTimedOut);
    m_uMaximumCliqueSize = dummyCliqueSize;
    vVertexOrder = P;
}

void SparseMCS::Color(vector<int> const &vVertexOrder, vector<int> &vVerticesToReorder, vector<int> &vColors)
{
    coloringStrategy.Color(m_AdjacencyArray, vVertexOrder, vVerticesToReorder, vColors);
}

void SparseMCS::GetNewOrderLocal(vector<int> &vNewVertexOrder,
                                 vector<int> &vVertexOrder,
                                 vector<int> const &P,
                                 int const chosenVertex,
                                 vector<int> &R_local)
{
    if (chosenVertex != -1) R_local.push_back(chosenVertex);
    vNewVertexOrder.clear();
    const auto &neighbors = m_AdjacencyArray[chosenVertex];
    for (int const vertex : vVertexOrder) {
        if (vertex != chosenVertex && binary_search(neighbors.begin(), neighbors.end(), vertex)) {
            vNewVertexOrder.push_back(vertex);
        }
    }
}

void SparseMCS::ProcessOrderAfterRecursionLocal(vector<int> &vVertexOrder,
                                                vector<int> &P,
                                                vector<int> &vColors,
                                                int const chosenVertex,
                                                vector<int> &R_local)
{
    if (chosenVertex == -1) return;
    auto it = find(vVertexOrder.begin(), vVertexOrder.end(), chosenVertex);
    if (it != vVertexOrder.end())
        vVertexOrder.erase(it);
    R_local.pop_back();
}

void SparseMCS::ProcessOrderBeforeReturnLocal(vector<int> &vVertexOrder,
                                              vector<int> &P,
                                              vector<int> &vColors,
                                              vector<int> &R_local)
{
    // Ничего не делаем
}