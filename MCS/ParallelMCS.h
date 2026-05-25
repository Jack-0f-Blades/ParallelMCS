#ifndef PARALLEL_MCS_H
#define PARALLEL_MCS_H

#include "MCS.h"
#include "TaskQueue.h"
#include <list>
#include <atomic>
#include <chrono>
#include <vector>

class ParallelMCS : public MCS
{
public:
    ParallelMCS(std::vector<std::vector<char>> const &vAdjacencyMatrix,
                size_t queueCapacity = 0,
                int splitDepth = 2,
                size_t targetQueueSize = 100,
                size_t lowThreshold = 20,
                int nodesBetweenChecks = 500,
                int minTaskSize = 10);

    virtual ~ParallelMCS();

    virtual long Run(std::list<std::list<int>> &cliques) override;

    void RunFromTask(const Task& t,
                     std::list<std::list<int>>& cliques) override;

protected:
    void RunRecursiveParallel(std::vector<int> &P,
                              std::vector<int> &vVertexOrder,
                              std::list<std::list<int>> &cliques,
                              std::vector<int> &vColors,
                              std::vector<int> &R,
                              int &depth,
                              long &nodeCount);

    bool checkTimeout();

private:
    TaskQueue m_taskQueue;
    size_t m_queueCapacity;
    size_t m_targetQueueSize;
    size_t m_queueLowThreshold;
    int    m_maxSplitDepth;
    int    m_nodesBetweenChecks;
    int    m_minTaskSize;

    std::chrono::steady_clock::time_point m_lastLogTime;

    // Thread‑local стеки для переиспользования памяти внутри каждого потока
    static thread_local std::vector<std::vector<int>> t_stackP;
    static thread_local std::vector<std::vector<int>> t_stackColors;
    static thread_local std::vector<std::vector<int>> t_stackOrder;
};

#endif // PARALLEL_MCS_H