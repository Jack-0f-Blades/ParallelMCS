/**
 * Структуры и функции для вычисления порядка вырожденности вершин.
 * Используются при начальном упорядочении (InitialOrderingMCR).
 */
#ifndef DEGENERACY_TOOLS_H
#define DEGENERACY_TOOLS_H

#include <vector>
#include <list>

class NeighborListArray
{
public:
    NeighborListArray() : vertex(-1), earlierDegree(-1), laterDegree(-1), orderNumber(-1) {}

    int vertex;
    std::vector<int> earlier;
    int earlierDegree;
    std::vector<int> later;
    int laterDegree;
    int orderNumber;
};

int computeDegeneracy(std::vector<std::list<int>> const &adjList, int size);
int computeDegeneracy(std::vector<std::vector<int>> const &adjList, int size);

std::vector<NeighborListArray> computeDegeneracyOrderArray(std::vector<std::vector<int>> &adjArray, int size);
std::vector<NeighborListArray> computeDegeneracyOrderArrayForReverse(std::vector<std::vector<int>> &adjArray, int size);
std::vector<int> GetVerticesInDegeneracyOrder(std::vector<std::vector<int>> &adjArray);

#endif // DEGENERACY_TOOLS_H