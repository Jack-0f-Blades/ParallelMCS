#include "MCS.h"

MCS::MCS(std::vector<std::vector<char>> const &vAdjacencyMatrix)
    : StaticOrderMCS(vAdjacencyMatrix)
{
    SetName("mcs");
}

void MCS::Color(std::vector<int> const &vVertexOrder,
                std::vector<int> &vVerticesToReorder,
                std::vector<int> &vColors)
{
    coloringStrategy.Recolor(m_AdjacencyMatrix, vVertexOrder, vVerticesToReorder, vColors,
                             static_cast<int>(m_uMaximumCliqueSize.load()),
                             static_cast<int>(R.size()));
}