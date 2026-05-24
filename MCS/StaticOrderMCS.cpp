#include "StaticOrderMCS.h"
#include "OrderingTools.h"
#include <algorithm>   // для std::find

using namespace std;

StaticOrderMCS::StaticOrderMCS(vector<vector<char>> const &vAdjacencyMatrix)
    : MCQ(vAdjacencyMatrix)
{
    SetName("static-order-mcs");
}

void StaticOrderMCS::InitializeOrder(vector<int> &P, vector<int> &vVertexOrder, vector<int> &vColors)
{
    size_t dummyCliqueSize = 0;
    InitialOrderingMCR(m_AdjacencyMatrix, P, vColors, dummyCliqueSize);
    m_uMaximumCliqueSize = dummyCliqueSize;
    vVertexOrder = P;
}

void StaticOrderMCS::GetNewOrderLocal(vector<int> &vNewVertexOrder, vector<int> &vVertexOrder,
                                      vector<int> const &P, int const chosenVertex,
                                      vector<int> &R_local)
{
    if (chosenVertex != -1) R_local.push_back(chosenVertex);
    vNewVertexOrder.clear();
    for (int const vertex : vVertexOrder)
    {
        if (vertex != chosenVertex && m_AdjacencyMatrix[chosenVertex][vertex])
            vNewVertexOrder.push_back(vertex);
    }
}

// ИСПРАВЛЕНО: добавлено удаление chosenVertex из vVertexOrder
void StaticOrderMCS::ProcessOrderAfterRecursionLocal(vector<int> &vVertexOrder,
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