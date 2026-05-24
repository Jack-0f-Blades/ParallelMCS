#include "CliqueColoringStrategy.h"
#include <algorithm>
#include <iostream>

using namespace std;

CliqueColoringStrategy::CliqueColoringStrategy(vector<vector<char>> const &adjacencyMatrix)
    : m_AdjacencyMatrix(adjacencyMatrix), m_stopFlag(nullptr), m_repairCount(0)
{
    m_vvVerticesWithColor.resize(adjacencyMatrix.size());
    for (auto &vec : m_vvVerticesWithColor)
        vec.reserve(adjacencyMatrix.size());
}

void CliqueColoringStrategy::Color(vector<vector<char>> const &adjacencyMatrix,
                                   vector<int> const &vVertexOrder,
                                   vector<int> &vVerticesToReorder,
                                   vector<int> &vColors)
{
    if (vVertexOrder.empty()) return;
    size_t maxColor = 0;
    size_t const numVertices = vVertexOrder.size();
    vVerticesToReorder.resize(numVertices);
    vColors.resize(numVertices);

    for (int const vertex : vVertexOrder)
    {
        if (m_stopFlag && m_stopFlag->load(std::memory_order_relaxed))
            break;

        size_t color = 0;
        for (vector<int> const &verticesWithColor : m_vvVerticesWithColor)
        {
            bool hasNeighborWithColor = false;
            if (verticesWithColor.empty()) break;
            for (int const coloredVertex : verticesWithColor)
            {
                if (adjacencyMatrix[vertex][coloredVertex])
                {
                    hasNeighborWithColor = true;
                    break;
                }
            }
            if (!hasNeighborWithColor) break;
            color++;
        }
        m_vvVerticesWithColor[color].push_back(vertex);
        maxColor = max(maxColor, color);
    }

    int currentIndex = 0;
    for (int currentColor = 0; currentColor <= (int)maxColor; ++currentColor)
    {
        for (int const vertex : m_vvVerticesWithColor[currentColor])
        {
            vVerticesToReorder[currentIndex] = vertex;
            vColors[currentIndex] = currentColor + 1;
            currentIndex++;
        }
        m_vvVerticesWithColor[currentColor].clear();
    }
}

void CliqueColoringStrategy::Recolor(vector<vector<char>> const &adjacencyMatrix,
                                     vector<int> const &vVertexOrder,
                                     vector<int> &vVerticesToReorder,
                                     vector<int> &vColors,
                                     int const currentBestCliqueSize,
                                     int const currentCliqueSize)
{
    if (vVertexOrder.empty()) return;

    size_t maxColor = 0;
    int iBestCliqueDelta = currentBestCliqueSize - currentCliqueSize;

    for (int const vertex : vVertexOrder)
    {
        if (m_stopFlag && m_stopFlag->load(std::memory_order_relaxed))
            break;

        size_t color = 0;
        for (vector<int> const &verticesWithColor : m_vvVerticesWithColor)
        {
            bool hasNeighborWithColor = false;
            if (verticesWithColor.empty()) break;
            for (int const coloredVertex : verticesWithColor)
            {
                if (adjacencyMatrix[vertex][coloredVertex])
                {
                    hasNeighborWithColor = true;
                    break;
                }
            }
            if (!hasNeighborWithColor) break;
            color++;
        }

        m_vvVerticesWithColor[color].push_back(vertex);
        maxColor = max(maxColor, color);

        if (color + 1 > static_cast<size_t>(iBestCliqueDelta) && color == maxColor)
        {
            Repair(vertex, static_cast<int>(color), iBestCliqueDelta);
            if (m_vvVerticesWithColor[maxColor].empty())
                maxColor--;
        }
    }

    int currentIndex = 0;
    for (int currentColor = 0; currentColor <= (int)maxColor; ++currentColor)
    {
        for (int const vertex : m_vvVerticesWithColor[currentColor])
        {
            vVerticesToReorder[currentIndex] = vertex;
            vColors[currentIndex] = currentColor + 1;
            currentIndex++;
        }
        m_vvVerticesWithColor[currentColor].clear();
    }
}

bool CliqueColoringStrategy::HasConflict(int const vertex, vector<int> const &vVerticesWithColor)
{
    for (int v : vVerticesWithColor)
        if (m_AdjacencyMatrix[vertex][v]) return true;
    return false;
}

int CliqueColoringStrategy::GetConflictingVertex(int const vertex, vector<int> const &vVerticesWithColor)
{
    int conflicting = -1;
    int count = 0;
    for (int v : vVerticesWithColor)
    {
        if (m_AdjacencyMatrix[vertex][v])
        {
            conflicting = v;
            count++;
            if (count > 1) return -1;
        }
    }
    return conflicting;
}

// ИСПРАВЛЕННЫЙ Repair: удалён ошибочный return false после внутреннего цикла
bool CliqueColoringStrategy::Repair(int const vertex, int const color, int const iBestCliqueDelta)
{
    for (int newColor = 0; newColor <= iBestCliqueDelta - 1; newColor++)
    {
        int const conflictingVertex = GetConflictingVertex(vertex, m_vvVerticesWithColor[newColor]);
        if (conflictingVertex < 0) continue;

        for (int nextColor = newColor + 1; nextColor <= iBestCliqueDelta; nextColor++)
        {
            if (HasConflict(conflictingVertex, m_vvVerticesWithColor[nextColor]))
                continue;

            auto &vecColor = m_vvVerticesWithColor[color];
            vecColor.erase(find(vecColor.begin(), vecColor.end(), vertex));

            auto &vecNew = m_vvVerticesWithColor[newColor];
            vecNew.erase(find(vecNew.begin(), vecNew.end(), conflictingVertex));

            vecNew.push_back(vertex);
            m_vvVerticesWithColor[nextColor].push_back(conflictingVertex);

            ++m_repairCount;
            return true;
        }
        // НЕТ return false здесь – продолжаем перебирать newColor
    }
    return false;
}