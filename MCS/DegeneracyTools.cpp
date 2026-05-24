#include "DegeneracyTools.h"
#include <algorithm>
#include <climits>
#include <cassert>
#include <iostream>
using namespace std;

static int computeDegeneracyFromArray(vector<vector<int>> &adjArray, int size) {
    vector<int> degree(size);
    vector<list<int>> verticesByDegree(size);
    vector<list<int>::iterator> vertexLocator(size);

    for (int i=0; i<size; i++) {
        degree[i] = adjArray[i].size();
        verticesByDegree[degree[i]].push_front(i);
        vertexLocator[i] = verticesByDegree[degree[i]].begin();
    }

    int currentDegree = 0;
    int removed = 0;
    int degeneracy = 0;
    while (removed < size) {
        if (!verticesByDegree[currentDegree].empty()) {
            degeneracy = max(degeneracy, currentDegree);
            int const vertex = verticesByDegree[currentDegree].front();
            verticesByDegree[currentDegree].erase(vertexLocator[vertex]);
            degree[vertex] = -1;
            for (int const neighbor : adjArray[vertex]) {
                if (degree[neighbor] != -1) {
                    verticesByDegree[degree[neighbor]].erase(vertexLocator[neighbor]);
                    degree[neighbor]--;
                    if (degree[neighbor] != -1) {
                        verticesByDegree[degree[neighbor]].push_front(neighbor);
                        vertexLocator[neighbor] = verticesByDegree[degree[neighbor]].begin();
                    }
                }
            }
            removed++;
            currentDegree = 0; // сброс
        } else {
            currentDegree++;
        }
    }
    return degeneracy;
}

int computeDegeneracy(vector<list<int>> const &adjList, int size) {
    vector<vector<int>> adjArray(size);
    for (int i=0; i<size; i++) {
        for (int v : adjList[i]) adjArray[i].push_back(v);
    }
    return computeDegeneracyFromArray(adjArray, size);
}

int computeDegeneracy(vector<vector<int>> const &adjList, int size) {
    return computeDegeneracyFromArray(const_cast<vector<vector<int>>&>(adjList), size);
}

vector<NeighborListArray> computeDegeneracyOrderArray(vector<vector<int>> &adjArray, int size) {
    vector<int> degree(size);
    vector<list<int>> verticesByDegree(size);
    vector<list<int>::iterator> vertexLocator(size);
    for (int i=0; i<size; i++) {
        degree[i] = adjArray[i].size();
        verticesByDegree[degree[i]].push_front(i);
        vertexLocator[i] = verticesByDegree[degree[i]].begin();
    }
    int currentDegree = 0;
    int removed = 0;
    vector<NeighborListArray> orderingArray(size);
    while (removed < size) {
        if (!verticesByDegree[currentDegree].empty()) {
            int const vertex = verticesByDegree[currentDegree].front();
            verticesByDegree[currentDegree].pop_front();
            orderingArray[vertex].vertex = vertex;
            orderingArray[vertex].orderNumber = removed;
            degree[vertex] = -1;
            vector<int> &neighborList = adjArray[vertex];
            int splitPoint = neighborList.size();
            for (int i=0; i<splitPoint; ++i) {
                int const neighbor = neighborList[i];
                if (degree[neighbor] != -1) {
                    verticesByDegree[degree[neighbor]].erase(vertexLocator[neighbor]);
                    neighborList[i] = neighborList[--splitPoint];
                    neighborList[splitPoint] = neighbor;
                    i--;
                    degree[neighbor]--;
                    if (degree[neighbor] != -1) {
                        verticesByDegree[degree[neighbor]].push_front(neighbor);
                        vertexLocator[neighbor] = verticesByDegree[degree[neighbor]].begin();
                    } else {
                        // earlier neighbor
                        orderingArray[vertex].earlier.push_back(neighbor);
                    }
                }
            }
            orderingArray[vertex].laterDegree = neighborList.size() - splitPoint;
            orderingArray[vertex].later.resize(neighborList.size() - splitPoint);
            orderingArray[vertex].earlierDegree = splitPoint;
            orderingArray[vertex].earlier.resize(splitPoint);
            for (int i=0; i<splitPoint; ++i) orderingArray[vertex].earlier[i] = neighborList[i];
            for (int i=splitPoint; i<(int)neighborList.size(); ++i) orderingArray[vertex].later[i-splitPoint] = neighborList[i];
            removed++;
            currentDegree = 0;
        } else {
            currentDegree++;
        }
    }
    return orderingArray;
}

vector<NeighborListArray> computeDegeneracyOrderArrayForReverse(vector<vector<int>> &adjArray, int size) {
    // аналогично но later упорядочены по возрастанию orderNumber
    auto ordering = computeDegeneracyOrderArray(adjArray, size);
    for (auto &nla : ordering) {
        sort(nla.later.begin(), nla.later.end(), [&](int a, int b) { return ordering[a].orderNumber < ordering[b].orderNumber; });
    }
    return ordering;
}

vector<int> GetVerticesInDegeneracyOrder(vector<vector<int>> &adjArray) {
    size_t size = adjArray.size();
    vector<int> result(size, -1);
    vector<int> degree(size);
    for (int i=0; i<(int)size; i++) degree[i] = adjArray[i].size();
    // просто возврат порядка, аналогично computeDegeneracyOrderArray
    auto ordering = computeDegeneracyOrderArray(adjArray, size);
    for (int i=0; i<(int)size; i++) result[i] = ordering[i].vertex;
    return result;
}