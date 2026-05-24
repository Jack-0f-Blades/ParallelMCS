#include "SparseCliqueColoringStrategy.h"
#include <algorithm>
#include <iostream>

using namespace std;

SparseCliqueColoringStrategy::SparseCliqueColoringStrategy(vector<vector<int>> const &adjacencyArray)
    : m_AdjacencyArray(adjacencyArray),
      m_vVertexToColor(adjacencyArray.size(), -1),
      m_vNeighborColorCount(adjacencyArray.size(), 0),
      m_vbNeighbors(adjacencyArray.size(), false),
      m_vbConflictNeighbors(adjacencyArray.size(), false)
{
    m_vvVerticesWithColor.resize(adjacencyArray.size());
}

void SparseCliqueColoringStrategy::Color(vector<vector<int>> const &adjacencyArray,
                                         vector<int> const &vVertexOrder,
                                         vector<int> &vVerticesToReorder,
                                         vector<int> &vColors)
{
    if (vVertexOrder.empty()) return;
    size_t maxColor = 0;
    vVerticesToReorder.resize(vVertexOrder.size());
    vColors.resize(vVertexOrder.size());

    for (int const vertex : vVertexOrder) {
        if (m_stopFlag && m_stopFlag->load(memory_order_relaxed)) break;

        // Отмечаем цвета соседей
        for (int const neighbor : adjacencyArray[vertex]) {
            int c = m_vVertexToColor[neighbor];
            if (c != -1) m_vbNeighbors[c] = true;
        }
        // Ищем минимальный свободный цвет
        int color = 0;
        while (color < (int)m_vbNeighbors.size() && m_vbNeighbors[color]) ++color;
        // Присваиваем
        m_vvVerticesWithColor[color].push_back(vertex);
        m_vVertexToColor[vertex] = color;
        if (color > (int)maxColor) maxColor = color;
        // Снимаем отметки
        for (int const neighbor : adjacencyArray[vertex]) {
            int c = m_vVertexToColor[neighbor];
            if (c != -1) m_vbNeighbors[c] = false;
        }
    }

    int idx = 0;
    for (int c = 0; c <= (int)maxColor; ++c) {
        for (int v : m_vvVerticesWithColor[c]) {
            vVerticesToReorder[idx] = v;
            vColors[idx] = c + 1;
            ++idx;
            m_vVertexToColor[v] = -1;
        }
        m_vvVerticesWithColor[c].clear();
    }
}

void SparseCliqueColoringStrategy::Recolor(vector<vector<int>> const &adjacencyArray,
                                           vector<int> const &vVertexOrder,
                                           vector<int> &vVerticesToReorder,
                                           vector<int> &vColors,
                                           int const currentBestCliqueSize,
                                           int const currentCliqueSize)
{
    if (vVertexOrder.empty()) return;
    size_t maxColor = 0;
    int iBestCliqueDelta = currentBestCliqueSize - currentCliqueSize;

    for (int const vertex : vVertexOrder) {
        if (m_stopFlag && m_stopFlag->load(memory_order_relaxed)) break;

        // Отмечаем цвета соседей
        for (int const neighbor : adjacencyArray[vertex]) {
            int c = m_vVertexToColor[neighbor];
            if (c != -1) {
                m_vbNeighbors[c] = true;
                ++m_vNeighborColorCount[c];
            }
        }
        int color = 0;
        while (color < (int)m_vbNeighbors.size() && m_vbNeighbors[color]) ++color;

        m_vvVerticesWithColor[color].push_back(vertex);
        m_vVertexToColor[vertex] = color;
        if (color > (int)maxColor) maxColor = color;

        // Попытка ремонта
        if (color + 1 > iBestCliqueDelta && color == (int)maxColor) {
            if (Repair(vertex, color, iBestCliqueDelta)) {
                if (m_vvVerticesWithColor[maxColor].empty())
                    --maxColor;
            }
        }

        // Снимаем отметки
        for (int const neighbor : adjacencyArray[vertex]) {
            int c = m_vVertexToColor[neighbor];
            if (c != -1) {
                m_vbNeighbors[c] = false;
                m_vNeighborColorCount[c] = 0;
            }
        }
    }

    int idx = 0;
    for (int c = 0; c <= (int)maxColor; ++c) {
        for (int v : m_vvVerticesWithColor[c]) {
            vVerticesToReorder[idx] = v;
            vColors[idx] = c + 1;
            ++idx;
            m_vVertexToColor[v] = -1;
        }
        m_vvVerticesWithColor[c].clear();
    }
}

bool SparseCliqueColoringStrategy::HasConflict(int vertex, const vector<int>& verticesWithColor)
{
    for (int v : verticesWithColor)
        if (find(m_AdjacencyArray[vertex].begin(), m_AdjacencyArray[vertex].end(), v) != m_AdjacencyArray[vertex].end())
            return true;
    return false;
}

int SparseCliqueColoringStrategy::GetConflictingVertex(int vertex, const vector<int>& verticesWithColor)
{
    int conflict = -1;
    int count = 0;
    for (int v : verticesWithColor) {
        if (find(m_AdjacencyArray[vertex].begin(), m_AdjacencyArray[vertex].end(), v) != m_AdjacencyArray[vertex].end()) {
            conflict = v;
            ++count;
            if (count > 1) return -1;
        }
    }
    return conflict;
}

bool SparseCliqueColoringStrategy::Repair(int vertex, int color, int iBestCliqueDelta)
{
    for (int newColor = 0; newColor <= iBestCliqueDelta - 1; ++newColor) {
        int conflictingVertex = GetConflictingVertex(vertex, m_vvVerticesWithColor[newColor]);
        if (conflictingVertex < 0) continue;
        for (int nextColor = newColor + 1; nextColor <= iBestCliqueDelta; ++nextColor) {
            if (HasConflict(conflictingVertex, m_vvVerticesWithColor[nextColor]))
                continue;
            // Удаляем vertex из его текущего цвета
            auto &vecCurr = m_vvVerticesWithColor[color];
            vecCurr.erase(find(vecCurr.begin(), vecCurr.end(), vertex));
            // Удаляем conflictingVertex из newColor
            auto &vecNew = m_vvVerticesWithColor[newColor];
            vecNew.erase(find(vecNew.begin(), vecNew.end(), conflictingVertex));
            // Перемещаем
            vecNew.push_back(vertex);
            m_vvVerticesWithColor[nextColor].push_back(conflictingVertex);
            // Обновляем m_vVertexToColor
            m_vVertexToColor[vertex] = newColor;
            m_vVertexToColor[conflictingVertex] = nextColor;
            ++m_repairCount;
            return true;
        }
        return false;
    }
    return false;
}