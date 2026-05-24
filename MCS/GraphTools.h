/**
 * Утилиты для работы с графами: сортировка вершин по степеням, вычисление связных компонент, печать графа.
 */
#ifndef GRAPH_TOOLS_H
#define GRAPH_TOOLS_H

#include <vector>
#include <list>
#include <string>

class GraphTools
{
public:
    static std::vector<int> OrderVerticesByDegree(std::vector<std::vector<char>> const &adjacencyMatrix, bool const ascending);
    static std::vector<int> OrderVerticesByDegree(std::vector<std::vector<char>> const &adjacencyMatrix,
                                                std::vector<int> const &vDegree, bool const ascending);
    static std::vector<int> OrderVerticesByDegree(std::vector<std::vector<int>> const &adjacencyList, bool const ascending);

    static void PrintGraphInEdgesFormat(std::vector<std::vector<int>> const &adjacencyArray);
    static void PrintGraphInSNAPFormat(std::vector<std::vector<int>> const &adjacencyArray);

    static void ComputeConnectedComponents(std::vector<std::vector<int>> const &adjacencyList,
                                           std::vector<std::vector<int>> &vComponents);
};

#endif // GRAPH_TOOLS_H