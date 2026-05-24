#ifndef ORDERING_TOOLS_H
#define ORDERING_TOOLS_H

#include <vector>
#include <cstddef>
#include <atomic>

/**
 * Построение начального порядка вершин и раскраски для алгоритмов MCS/MCQ.
 * Используется вырожденный порядок (degeneracy order) с разрывом связей
 * по сумме степеней соседей (neighbourhood degree).
 *
 * @param adjacencyMatrix  плотная матрица смежности (char)
 * @param vOrderedVertices выходной порядок вершин
 * @param vColoring        выходные цвета (номера цветов, начиная с 1)
 * @param cliqueSize       размер клики, найденной в процессе (может быть 0)
 * @param stopFlag         флаг для досрочного прерывания (опционально)
 */
void InitialOrderingMCR(std::vector<std::vector<char>> const &adjacencyMatrix,
                        std::vector<int> &vOrderedVertices,
                        std::vector<int> &vColoring,
                        size_t &cliqueSize,
                        std::atomic<bool>* stopFlag = nullptr);

/**
 * Построение начального порядка и раскраски для разреженного графа (список смежности).
 *
 * @param adjacencyArray   список смежности (вектор векторов int)
 * @param vOrderedVertices выходной порядок вершин
 * @param vColoring        выходные цвета
 * @param cliqueSize       размер клики, найденной в процессе
 * @param stopFlag         флаг для досрочного прерывания
 */
void InitialOrderingMCR(std::vector<std::vector<int>> const &adjacencyArray,
                        std::vector<int> &vOrderedVertices,
                        std::vector<int> &vColoring,
                        size_t &cliqueSize,
                        std::atomic<bool>* stopFlag = nullptr);

#endif // ORDERING_TOOLS_H