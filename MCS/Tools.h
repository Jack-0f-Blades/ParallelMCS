/**
 * Общие утилиты: функции сравнения, печати, чтения графа (включая DIMACS) и измерения времени.
 */
#ifndef TOOLS_H
#define TOOLS_H

#include <list>
#include <vector>
#include <string>
#include <ctime>

class Algorithm;

int sortComparator(int node1, int node2);

void printArray(int* array, int size);
void printArrayOfLinkedLists(std::vector<std::list<int>> const &listOfLists, int size);
void destroyCliqueResults(std::list<std::list<int>> &cliques);

std::vector<std::list<int>> readInGraphAdjList(int &n, int &m, std::string const &fileName);
std::vector<std::list<int>> readInGraphAdjListEdgesPerLine(int &n, int &m, std::string const &fileName);
std::vector<std::list<int>> readInGraphDimacs(int &n, int &m, std::string const &fileName);  // <-- новое

void RunAndPrintStats(Algorithm* pAlgorithm, std::list<std::list<int>> &cliques, bool const outputLatex);

namespace Tools
{
    void printInt(int integer);
    void printList(std::list<int> const &linkedList, void (*printFunc)(int));
    std::string GetTimeInSeconds(clock_t delta, bool brackets = true);
}

#endif // TOOLS_H