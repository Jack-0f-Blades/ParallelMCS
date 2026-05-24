#include "GraphTools.h"
#include <iostream>
#include <algorithm>
using namespace std;

vector<int> GraphTools::OrderVerticesByDegree(vector<vector<char>> const &adjacencyMatrix, bool const ascending)
{
    size_t n = adjacencyMatrix.size();
    vector<int> vDegree(n, 0);
    for (size_t u=0; u<n; ++u)
        for (size_t v=0; v<n; ++v)
            if (adjacencyMatrix[u][v]) vDegree[u]++;
    return OrderVerticesByDegree(adjacencyMatrix, vDegree, ascending);
}

vector<int> GraphTools::OrderVerticesByDegree(vector<vector<char>> const &adjacencyMatrix,
                                              vector<int> const &vDegree, bool const ascending)
{
    size_t n = adjacencyMatrix.size();
    size_t maxDeg = 0;
    for (int d : vDegree) maxDeg = max(maxDeg, (size_t)d);
    vector<list<int>> vlVerticesByDegree(maxDeg+1);
    for (size_t v=0; v<n; ++v)
        vlVerticesByDegree[vDegree[v]].push_back(v);

    vector<int> vOrderedVertices(n, -1);
    size_t index = 0;
    if (ascending)
    {
        for (size_t deg=0; deg<=maxDeg; ++deg)
            for (int vertex : vlVerticesByDegree[deg])
                vOrderedVertices[index++] = vertex;
    }
    else
    {
        for (size_t i=0; i<=maxDeg; ++i)
        {
            size_t deg = maxDeg - i;
            for (int vertex : vlVerticesByDegree[deg])
                vOrderedVertices[index++] = vertex;
        }
    }
    return vOrderedVertices;
}

vector<int> GraphTools::OrderVerticesByDegree(vector<vector<int>> const &adjacencyList, bool const ascending)
{
    size_t n = adjacencyList.size();
    vector<int> vDegree(n, 0);
    for (size_t u=0; u<n; ++u)
        vDegree[u] = adjacencyList[u].size();
    size_t maxDeg = 0;
    for (int d : vDegree) maxDeg = max(maxDeg, (size_t)d);
    vector<list<int>> vlVerticesByDegree(maxDeg+1);
    for (size_t v=0; v<n; ++v)
        vlVerticesByDegree[vDegree[v]].push_back(v);

    vector<int> vOrderedVertices(n, -1);
    size_t index = 0;
    if (ascending)
    {
        for (size_t deg=0; deg<=maxDeg; ++deg)
            for (int vertex : vlVerticesByDegree[deg])
                vOrderedVertices[index++] = vertex;
    }
    else
    {
        for (size_t i=0; i<=maxDeg; ++i)
        {
            size_t deg = maxDeg - i;
            for (int vertex : vlVerticesByDegree[deg])
                vOrderedVertices[index++] = vertex;
        }
    }
    return vOrderedVertices;
}

void GraphTools::PrintGraphInEdgesFormat(vector<vector<int>> const &adjacencyArray)
{
    cout << adjacencyArray.size() << endl;
    int edges = 0;
    for (auto &neigh : adjacencyArray) edges += neigh.size();
    cout << edges << endl;
    for (size_t i=0; i<adjacencyArray.size(); ++i)
        for (int nb : adjacencyArray[i])
            cout << i << " " << nb << endl;
}

void GraphTools::PrintGraphInSNAPFormat(vector<vector<int>> const &adjacencyArray)
{
    for (size_t i=0; i<adjacencyArray.size(); ++i)
    {
        cout << i << " ";
        for (int nb : adjacencyArray[i]) cout << nb << " ";
        cout << endl;
    }
}

void GraphTools::ComputeConnectedComponents(vector<vector<int>> const &adjacencyList,
                                            vector<vector<int>> &vComponents)
{
    size_t n = adjacencyList.size();
    vector<bool> evaluated(n, false);
    vComponents.clear();
    int componentCount = 1;
    for (size_t v=0; v<n; ++v)
    {
        if (!evaluated[v])
        {
            vComponents.resize(componentCount);
            // простая реализация поиска в ширину
            vector<int> queue;
            queue.push_back(v);
            evaluated[v] = true;
            while (!queue.empty())
            {
                int cur = queue.back(); queue.pop_back();
                vComponents.back().push_back(cur);
                for (int nb : adjacencyList[cur])
                {
                    if (!evaluated[nb])
                    {
                        evaluated[nb] = true;
                        queue.push_back(nb);
                    }
                }
            }
            componentCount++;
        }
    }
}