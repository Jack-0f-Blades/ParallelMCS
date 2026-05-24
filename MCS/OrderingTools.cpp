#include "OrderingTools.h"
#include "CliqueColoringStrategy.h"   // для плотной раскраски
#include "SparseCliqueColoringStrategy.h" // для разреженной раскраски
#include <algorithm>
#include <climits>
#include <list>
#include <vector>
#include <atomic>
#include "CliqueColoringStrategy.h"
using namespace std;

// ----------------------------------------------------------------------
// Внутренняя функция для плотной матрицы (работает через список смежности,
// построенный из матрицы, чтобы использовать общий код)
// ----------------------------------------------------------------------
static void InitialOrderingMCR_DenseImpl(vector<vector<int>> const &adjacencyArray,
                                         vector<int> &vOrderedVertices,
                                         vector<int> &vColoring,
                                         size_t &cliqueSize,
                                         atomic<bool>* stopFlag)
{
    size_t const size = adjacencyArray.size();
    vOrderedVertices.resize(size);
    vColoring.resize(size);

    vector<int> degree(size);
    vector<list<int>> verticesByDegree(size);
    vector<list<int>::iterator> vertexLocator(size);
    size_t maxDegree = 0;

    for (size_t i = 0; i < size; ++i) {
        degree[i] = adjacencyArray[i].size();
        verticesByDegree[degree[i]].push_front(i);
        vertexLocator[i] = verticesByDegree[degree[i]].begin();
        if (degree[i] > maxDegree) maxDegree = degree[i];
    }

    int currentDegree = 0;
    int numRemoved = 0;
    int startRmin = -1;

    while (numRemoved < (int)size) {
        if (stopFlag && stopFlag->load(memory_order_relaxed)) return;

        if (startRmin == -1 &&
            (int)verticesByDegree[currentDegree].size() == (int)size - numRemoved &&
            currentDegree > 0) {
            startRmin = size - numRemoved;
        }

        if (!verticesByDegree[currentDegree].empty()) {
            int vertex = -1;
            // Если несколько вершин с одинаковой степенью – выбираем с минимальной суммой степеней соседей
            if (verticesByDegree[currentDegree].size() > 1) {
                size_t minNeighborSum = ULONG_MAX;
                int chosen = verticesByDegree[currentDegree].front();
                for (int cand : verticesByDegree[currentDegree]) {
                    size_t neighborSum = 0;
                    for (int nb : adjacencyArray[cand]) {
                        if (degree[nb] != -1) neighborSum += degree[nb];
                    }
                    if (neighborSum < minNeighborSum) {
                        minNeighborSum = neighborSum;
                        chosen = cand;
                    }
                }
                vertex = chosen;
                verticesByDegree[currentDegree].erase(vertexLocator[vertex]);
            } else {
                vertex = verticesByDegree[currentDegree].front();
                verticesByDegree[currentDegree].pop_front();
            }

            vOrderedVertices[size - numRemoved - 1] = vertex;
            degree[vertex] = -1;

            for (int nb : adjacencyArray[vertex]) {
                if (degree[nb] != -1) {
                    verticesByDegree[degree[nb]].erase(vertexLocator[nb]);
                    --degree[nb];
                    if (degree[nb] != -1) {
                        verticesByDegree[degree[nb]].push_front(nb);
                        vertexLocator[nb] = verticesByDegree[degree[nb]].begin();
                    }
                }
            }
            ++numRemoved;
            currentDegree = 0;
        } else {
            ++currentDegree;
        }
    }

    if (stopFlag && stopFlag->load(memory_order_relaxed)) return;

    // Если есть хвост регулярного подграфа (R_min) – переупорядочиваем его с помощью раскраски
    if (startRmin == -1) startRmin = size;
    if (startRmin > 0 && startRmin < (int)size) {
        vector<int> rminVertices(vOrderedVertices.begin() + (size - startRmin), vOrderedVertices.end());
        int nR = startRmin;
        // Строим подматрицу для R_min
        vector<vector<char>> subMatrix(nR, vector<char>(nR, 0));
        vector<int> oldToNew(size, -1);
        for (int i = 0; i < nR; ++i) oldToNew[rminVertices[i]] = i;
        for (int i = 0; i < nR; ++i) {
            int u = rminVertices[i];
            for (int v : adjacencyArray[u]) {
                if (oldToNew[v] != -1) subMatrix[i][oldToNew[v]] = 1;
            }
        }
        // Раскрашиваем этот подграф
        CliqueColoringStrategy coloring(subMatrix);
        coloring.SetStopFlag(stopFlag);
        vector<int> colorOrder(nR), colors(nR), lexOrder(nR);
        for (int i = 0; i < nR; ++i) lexOrder[i] = i;
        coloring.Color(subMatrix, lexOrder, colorOrder, colors);
        // Переставляем вершины R_min в соответствии с полученным порядком
        vector<int> sortedRmin;
        for (int idx : colorOrder) sortedRmin.push_back(rminVertices[idx]);
        copy(sortedRmin.begin(), sortedRmin.end(), vOrderedVertices.begin() + (size - startRmin));
    }

    // Заполняем цвета (простая эвристика – по порядку, но в MCR это не используется, оставляем для совместимости)
    for (size_t i = 0; i < size; ++i) {
        vColoring[i] = min((int)(i + 1), (int)maxDegree + 1);
    }
    cliqueSize = 0;
}

// ----------------------------------------------------------------------
// Публичная обёртка для плотной матрицы (преобразует в список смежности)
// ----------------------------------------------------------------------
void InitialOrderingMCR(vector<vector<char>> const &adjacencyMatrix,
                        vector<int> &vOrderedVertices,
                        vector<int> &vColoring,
                        size_t &cliqueSize,
                        atomic<bool>* stopFlag)
{
    size_t n = adjacencyMatrix.size();
    vector<vector<int>> adjArray(n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (adjacencyMatrix[i][j]) adjArray[i].push_back(j);
        }
    }
    InitialOrderingMCR_DenseImpl(adjArray, vOrderedVertices, vColoring, cliqueSize, stopFlag);
}

// ----------------------------------------------------------------------
// Публичная функция для разреженного графа (прямая работа со списками)
// ----------------------------------------------------------------------
void InitialOrderingMCR(vector<vector<int>> const &adjacencyArray,
                        vector<int> &vOrderedVertices,
                        vector<int> &vColoring,
                        size_t &cliqueSize,
                        atomic<bool>* stopFlag)
{
    // Используем ту же внутреннюю реализацию, что и для плотного случая,
    // потому что она уже работает со списками смежности.
    InitialOrderingMCR_DenseImpl(adjacencyArray, vOrderedVertices, vColoring, cliqueSize, stopFlag);
}