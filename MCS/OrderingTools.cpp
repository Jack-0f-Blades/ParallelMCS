#include "OrderingTools.h"
#include "CliqueColoringStrategy.h"
#include "GraphTools.h"
#include <iostream>   // для отладки, потом удалить
#include <vector>
#include <climits>
#include <list>
#include <algorithm>

using namespace std;

namespace OrderingTools {

void InitialOrderingMCR(vector<vector<char>> const &adjacencyMatrix,
                        vector<int> &vOrderedVertices,
                        vector<int> &vColoring,
                        size_t &cliqueSize,
                        vector<int>& outClique,
                        atomic<bool>* /*stopFlag*/)
{
    size_t n = adjacencyMatrix.size();
    if (n == 0) return;

    vector<vector<int>> adjacencyArray(n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (adjacencyMatrix[i][j])
                adjacencyArray[i].push_back(static_cast<int>(j));
        }
    }

    vOrderedVertices.assign(n, -1);
    vColoring.assign(n, -1);

    vector<list<int>> verticesByDegree(n);
    vector<list<int>::iterator> vertexLocator(n);
    vector<int> degree(n);

    size_t maxDegree = 0;
    for (size_t i = 0; i < n; ++i) {
        degree[i] = static_cast<int>(adjacencyArray[i].size());
        vertexLocator[i] = verticesByDegree[degree[i]].insert(verticesByDegree[degree[i]].end(), static_cast<int>(i));
        maxDegree = max(maxDegree, adjacencyArray[i].size());
    }

    int currentDegree = 0;
    int numVerticesRemoved = 0;

    while (numVerticesRemoved < static_cast<int>(n)) {
        if (!verticesByDegree[currentDegree].empty()) {
            int vertex = -1;

            if (verticesByDegree[currentDegree].size() > 1) {
                if ((n - numVerticesRemoved) == verticesByDegree[currentDegree].size()) {
                    // Регулярный хвост – применяем точную раскраску
                    vector<int> remainingVertices(verticesByDegree[currentDegree].begin(),
                                                  verticesByDegree[currentDegree].end());
                    size_t tailSize = remainingVertices.size();

                    // Если регулярный хвост – клика
                    if (static_cast<int>(tailSize) == currentDegree + 1) {
                        if (cliqueSize < tailSize) {
                            cliqueSize = tailSize;
                            // СОХРАНЯЕМ ВЕРШИНЫ КЛИКИ
                            outClique.clear();
                            outClique.insert(outClique.end(),
                                             remainingVertices.begin(),
                                             remainingVertices.begin() + tailSize);
                        }
                    }

                    vector<int> remainingColors(tailSize, 0);
                    CliqueColoringStrategy coloringStrategy(adjacencyMatrix);
                    coloringStrategy.Color(adjacencyMatrix, remainingVertices, remainingVertices, remainingColors);

                    int maxColor = 0;
                    size_t idx = 0;
                    for (; idx < tailSize; ++idx) {
                        vOrderedVertices[idx] = remainingVertices[idx];
                        vColoring[idx] = remainingColors[idx];
                        if (remainingColors[idx] > maxColor) maxColor = remainingColors[idx];
                    }

                    int const lastIndexWithSmallerColor = min(tailSize + maxDegree - maxColor, n - 1);
                    int currentColor = maxColor + 1;
                    while (idx <= static_cast<size_t>(lastIndexWithSmallerColor)) {
                        vColoring[idx] = currentColor;
                        ++currentColor;
                        ++idx;
                    }
                    while (idx < n) {
                        vColoring[idx] = static_cast<int>(maxDegree + 1);
                        ++idx;
                    }
                    return;
                } else {
                    // Разрыв связей по сумме степеней соседей
                    size_t minNeighborhoodDegree = ULONG_MAX;
                    int chosenVertex = verticesByDegree[currentDegree].front();
                    for (int candidate : verticesByDegree[currentDegree]) {
                        size_t neighborhoodDegree = 0;
                        for (int neighbor : adjacencyArray[candidate]) {
                            if (degree[neighbor] != -1) {
                                neighborhoodDegree += degree[neighbor];
                            }
                        }
                        if (neighborhoodDegree < minNeighborhoodDegree) {
                            minNeighborhoodDegree = neighborhoodDegree;
                            chosenVertex = candidate;
                        }
                    }
                    vertex = chosenVertex;
                    verticesByDegree[currentDegree].erase(vertexLocator[vertex]);
                }
            } else {
                vertex = verticesByDegree[currentDegree].front();
                verticesByDegree[currentDegree].pop_front();
            }

            vOrderedVertices[n - numVerticesRemoved - 1] = vertex;
            degree[vertex] = -1;

            for (int neighbor : adjacencyArray[vertex]) {
                if (degree[neighbor] != -1) {
                    verticesByDegree[degree[neighbor]].erase(vertexLocator[neighbor]);
                    --degree[neighbor];
                    if (degree[neighbor] != -1) {
                        vertexLocator[neighbor] = verticesByDegree[degree[neighbor]].insert(verticesByDegree[degree[neighbor]].end(), neighbor);
                    }
                }
            }

            ++numVerticesRemoved;
            currentDegree = 0;
        } else {
            ++currentDegree;
        }
    }
}

// Разреженная версия (заглушка)
void InitialOrderingMCR(vector<vector<int>> const &adjacencyArray,
                        vector<int> &vOrderedVertices,
                        vector<int> &vColoring,
                        size_t &cliqueSize,
                        vector<int>& outClique,
                        atomic<bool>* /*stopFlag*/)
{
    size_t n = adjacencyArray.size();
    vOrderedVertices.resize(n);
    vColoring.resize(n);
    cliqueSize = 0;
    outClique.clear();
}

} // namespace OrderingTools