#include "Tools.h"
#include "Algorithm.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;

int sortComparator(int node1, int node2)
{
    if (node1 < node2) return -1;
    if (node1 > node2) return 1;
    return 0;
}

void printArray(int* array, int size)
{
    for (int i=0; i<size; i++)
        printf("%d ", array[i]);
    printf("\n");
}

void printArrayOfLinkedLists(vector<list<int>> const &arrayOfLists, int size)
{
    for (size_t i=0; i<arrayOfLists.size(); i++)
    {
        if (!arrayOfLists[i].empty())
        {
            printf("%d:", (int)i);
            for (int v : arrayOfLists[i])
                printf(" %d", v);
            printf("\n");
        }
    }
}

void destroyCliqueResults(list<list<int>> &cliques)
{
    cliques.clear();
}

vector<list<int>> readInGraphAdjList(int &n, int &m, string const &fileName)
{
    ifstream instream(fileName.c_str());
    if (!instream.good())
    {
        fprintf(stderr, "ERROR: Could not open file %s\n", fileName.c_str());
        exit(1);
    }

    instream >> n;
    if (!instream.good())
    {
        fprintf(stderr, "ERROR: Problem reading number of vertices in %s\n", fileName.c_str());
        exit(1);
    }

    vector<list<int>> adjList(n);
    int u, v;
    while (instream.good() && !instream.eof())
    {
        instream >> u >> v;
        if (instream.fail()) break;
        u--; v--;
        if (u >= 0 && u < n && v >= 0 && v < n && u != v)
        {
            adjList[u].push_back(v);
            m++;
        }
    }
    return adjList;
}

vector<list<int>> readInGraphAdjListEdgesPerLine(int &n, int &m, string const &fileName)
{
    // аналогично readInGraphAdjList, но в формате списка смежности
    ifstream instream(fileName.c_str());
    instream >> n >> m;
    vector<list<int>> adjList(n);
    for (int i=0; i<n; i++)
    {
        int deg;
        instream >> deg;
        adjList[i].resize(deg);
        for (int j=0; j<deg; j++)
        {
            int v;
            instream >> v;
            adjList[i].push_back(v-1);
        }
    }
    return adjList;
}

void RunAndPrintStats(Algorithm* pAlgorithm, list<list<int>> &cliques, bool const outputLatex)
{
    long result = pAlgorithm->Run(cliques);
    if (outputLatex)
        printf(" & %ld", result);
    else
        printf("Maximum clique size: %ld\n", result);
}

void Tools::printInt(int integer)
{
    printf("%d", integer);
}

void Tools::printList(list<int> const &linkedList, void (*printFunc)(int))
{
    int count = 0;
    for (int value : linkedList)
    {
        printFunc(value);
        if (++count < (int)linkedList.size())
            printf(" ");
    }
    printf("\n");
}

string Tools::GetTimeInSeconds(clock_t delta, bool brackets)
{
    stringstream strm;
    strm.precision(2);
    strm.setf(ios::fixed, ios::floatfield);
    double secs = (double)delta / (double)CLOCKS_PER_SEC;
    if (brackets)
        strm << "[" << secs << "s]";
    else
        strm << secs << "s";
    return strm.str();
}
/**
 * Чтение графа в формате DIMACS (и BHOSLIB).
 * Формат: заголовочные строки c ... , затем строка p edge <n> <m>,
 * затем строки e <u> <v>.
 * Возвращает список смежности.
 */
vector<list<int>> readInGraphDimacs(int &n, int &m, string const &fileName)
{
    ifstream instream(fileName.c_str());
    if (!instream.good())
    {
        fprintf(stderr, "ERROR: Could not open file %s\n", fileName.c_str());
        exit(1);
    }

    string line;
    n = 0;
    m = 0;
    vector<list<int>> adjList;

    while (getline(instream, line))
    {
        if (line.empty()) continue;
        if (line[0] == 'c') continue;          // комментарий
        if (line[0] == 'p')
        {
            // формат: p edge n m
            istringstream iss(line);
            string tmp, type;
            iss >> tmp >> type >> n >> m;
            adjList.resize(n);
        }
        else if (line[0] == 'e')
        {
            // формат: e u v
            istringstream iss(line);
            char e;
            int u, v;
            iss >> e >> u >> v;
            u--; v--;                          // перевод к 0-индексации
            if (u < 0 || u >= n || v < 0 || v >= n)
            {
                fprintf(stderr, "WARNING: invalid edge %d %d in file %s\n", u+1, v+1, fileName.c_str());
                continue;
            }
            adjList[u].push_back(v);
            adjList[v].push_back(u);
        }
    }

    return adjList;
}