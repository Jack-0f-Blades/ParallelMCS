#include "StaticOrderMCS.h"
#include "OrderingTools.h"
#include <algorithm>
#include <iostream> 

using namespace std;

StaticOrderMCS::StaticOrderMCS(vector<vector<char>> const &vAdjacencyMatrix)
    : MCQ(vAdjacencyMatrix)
{
    SetName("static-order-mcs");
}

void StaticOrderMCS::InitializeOrder(vector<int> &P, vector<int> &vVertexOrder, vector<int> &vColors)
{
    size_t foundCliqueSize = 0;
    vector<int> initialCliqueVertices;
    OrderingTools::InitialOrderingMCR(m_AdjacencyMatrix, P, vColors, foundCliqueSize, initialCliqueVertices);
    m_uMaximumCliqueSize = foundCliqueSize;
    vVertexOrder = P;

    if (foundCliqueSize > 0 && !initialCliqueVertices.empty()) {
        m_initialClique = initialCliqueVertices;
        std::cout << "DEBUG: MCR found clique of size " << foundCliqueSize << std::endl;
    } else {
        std::cout << "DEBUG: MCR did NOT return clique vertices, size=" << foundCliqueSize << std::endl;
    }
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