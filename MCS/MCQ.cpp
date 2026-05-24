#include "MCQ.h"
#include "OrderingTools.h"
#include <iostream>

using namespace std;

MCQ::MCQ(vector<vector<char>> const &vAdjacencyMatrix)
    : MaxSubgraphAlgorithm("mcq"),
      m_AdjacencyMatrix(vAdjacencyMatrix),
      coloringStrategy(m_AdjacencyMatrix)
{
    coloringStrategy.SetStopFlag(&m_bTimedOut);

    R.reserve(m_AdjacencyMatrix.size());
    stackP.resize(m_AdjacencyMatrix.size() + 1);
    stackColors.resize(m_AdjacencyMatrix.size() + 1);
    stackOrder.resize(m_AdjacencyMatrix.size() + 1);
    for (size_t i = 0; i < stackP.size(); ++i) {
        stackP[i].reserve(m_AdjacencyMatrix.size());
        stackColors[i].reserve(m_AdjacencyMatrix.size());
        stackOrder[i].reserve(m_AdjacencyMatrix.size());
    }
}

void MCQ::InitializeOrder(vector<int> &P, vector<int> &vVertexOrder, vector<int> &vColors)
{
    size_t dummyCliqueSize = 0;
    InitialOrderingMCR(m_AdjacencyMatrix, P, vColors, dummyCliqueSize, &m_bTimedOut);
    m_uMaximumCliqueSize = dummyCliqueSize;
    vVertexOrder = P;
}

void MCQ::Color(vector<int> const &vVertexOrder, vector<int> &vVerticesToReorder, vector<int> &vColors)
{
    coloringStrategy.Color(m_AdjacencyMatrix, vVertexOrder, vVerticesToReorder, vColors);
}

void MCQ::GetNewOrderLocal(vector<int> &vNewVertexOrder, vector<int> &vVertexOrder,
                           vector<int> const &P, int const chosenVertex,
                           vector<int> &R_local)
{
    if (chosenVertex != -1) R_local.push_back(chosenVertex);
    vNewVertexOrder.clear();
    for (int const candidate : vVertexOrder)
    {
        if (candidate != chosenVertex && m_AdjacencyMatrix[chosenVertex][candidate])
            vNewVertexOrder.push_back(candidate);
    }
}

void MCQ::ProcessOrderAfterRecursionLocal(vector<int> &vVertexOrder, vector<int> &P,
                                          vector<int> &vColors, int const chosenVertex,
                                          vector<int> &R_local)
{
    if (chosenVertex != -1) R_local.pop_back();
}

void MCQ::ProcessOrderBeforeReturnLocal(vector<int> &vVertexOrder, vector<int> &P,
                                        vector<int> &vColors, vector<int> &R_local)
{
}