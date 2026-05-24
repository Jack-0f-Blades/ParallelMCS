/*Базовый класс для всех алгоритмов ветвей и границ (MCQ, MCS, ParallelMCS). 
Содержит общую логику рекурсивного обхода (RunRecursive), управление стеком (stackP, stackColors, stackOrder), 
тайм-ауты, счётчики узлов.*/

#ifndef MAX_SUBGRAPH_ALGORITHM_H
#define MAX_SUBGRAPH_ALGORITHM_H

#include "Algorithm.h"
#include <vector>
#include <list>
#include <ctime>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>

class TaskQueue;
struct Task;

class MaxSubgraphAlgorithm : public Algorithm
{
public:
    MaxSubgraphAlgorithm(std::string const &name);
    virtual ~MaxSubgraphAlgorithm();

    virtual long Run(std::list<std::list<int>> &cliques) override;

    virtual void Color(std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) = 0;

    virtual void InitializeOrder(std::vector<int> &P,
                                 std::vector<int> &vVertexOrder,
                                 std::vector<int> &vColors) = 0;

    virtual void GetNewOrderLocal(std::vector<int> &vNewVertexOrder,
                                  std::vector<int> &vVertexOrder,
                                  std::vector<int> const &P,
                                  int const chosenVertex,
                                  std::vector<int> &R_local) = 0;

    virtual void ProcessOrderAfterRecursionLocal(std::vector<int> &vVertexOrder,
                                                 std::vector<int> &P,
                                                 std::vector<int> &vColors,
                                                 int const chosenVertex,
                                                 std::vector<int> &R_local) = 0;

    virtual void ProcessOrderBeforeReturnLocal(std::vector<int> &vVertexOrder,
                                               std::vector<int> &P,
                                               std::vector<int> &vColors,
                                               std::vector<int> &R_local) = 0;

    virtual void GetNewOrder(std::vector<int> &vNewVertexOrder,
                             std::vector<int> &vVertexOrder,
                             std::vector<int> const &P,
                             int const chosenVertex);

    virtual void ProcessOrderAfterRecursion(std::vector<int> &vVertexOrder,
                                            std::vector<int> &P,
                                            std::vector<int> &vColors,
                                            int const chosenVertex);

    virtual void ProcessOrderBeforeReturn(std::vector<int> &vVertexOrder,
                                          std::vector<int> &P,
                                          std::vector<int> &vColors);

    void SetInitialClique(const std::vector<int>& clique);
    void SetTimeOut(double seconds);
    bool IsTimedOut() const;

    void SetParallelMode(bool enable);
    void SetNumThreads(int n);
    void SetTaskQueue(TaskQueue* queue);

    virtual void RunFromTask(const Task& t,
        std::list<std::list<int>>& cliques);

    using ProgressCallback =
        std::function<void(size_t nodes, size_t cliqueSize, double seconds)>;
    void SetProgressCallback(ProgressCallback cb);

    // Для получения статистики (переопределяется в MCQ)
    virtual size_t getRepairCount() const { return 0; }

protected:
    std::vector<int> R;
    std::atomic<size_t> m_uMaximumCliqueSize;

    std::mutex m_bestMutex;

    double m_TimeOutSeconds = 0;
    std::chrono::steady_clock::time_point m_StartTimePoint;
    std::chrono::steady_clock::time_point m_LastLogTime;   // для периодического лога
    std::atomic<bool> m_bTimedOut;

    bool m_parallelMode;
    int m_numThreads;
    TaskQueue* m_taskQueue;

    int m_maxSplitDepth;

    std::vector<std::vector<int>> stackP;
    std::vector<std::vector<int>> stackColors;
    std::vector<std::vector<int>> stackOrder;

    long nodeCount;
    int depth;

    ProgressCallback m_progressCallback;

    void RunRecursive(std::vector<int> &P,
                      std::vector<int> &vVertexOrder,
                      std::list<std::list<int>> &cliques,
                      std::vector<int> &vColors);
};

#endif