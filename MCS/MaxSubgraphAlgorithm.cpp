#include "MaxSubgraphAlgorithm.h"
#include "TaskQueue.h"
#include "Logger.h"
#include <iostream>
#include <sstream>
#include <thread>

using namespace std;

static const double LOG_INTERVAL_SEC = 2.0;   // печатать прогресс каждые 2 секунды

MaxSubgraphAlgorithm::MaxSubgraphAlgorithm(string const &name)
    : Algorithm(name),
      m_uMaximumCliqueSize(0),
      m_bTimedOut(false),
      m_parallelMode(false),
      m_numThreads(1),
      m_taskQueue(nullptr),
      m_maxSplitDepth(10),
      nodeCount(0),
      depth(0)
{
    m_StartTimePoint = chrono::steady_clock::now();
    m_LastLogTime = m_StartTimePoint;
    stackP.resize(2);
    stackColors.resize(2);
    stackOrder.resize(2);
}

MaxSubgraphAlgorithm::~MaxSubgraphAlgorithm() = default;

void MaxSubgraphAlgorithm::GetNewOrder(vector<int> &vNewVertexOrder,
                                       vector<int> &vVertexOrder,
                                       vector<int> const &P,
                                       int const chosenVertex)
{
    GetNewOrderLocal(vNewVertexOrder, vVertexOrder, P, chosenVertex, this->R);
}

void MaxSubgraphAlgorithm::ProcessOrderAfterRecursion(vector<int> &vVertexOrder,
                                                      vector<int> &P,
                                                      vector<int> &vColors,
                                                      int const chosenVertex)
{
    ProcessOrderAfterRecursionLocal(vVertexOrder, P, vColors, chosenVertex, this->R);
}

void MaxSubgraphAlgorithm::ProcessOrderBeforeReturn(vector<int> &vVertexOrder,
                                                    vector<int> &P,
                                                    vector<int> &vColors)
{
    ProcessOrderBeforeReturnLocal(vVertexOrder, P, vColors, this->R);
}

void MaxSubgraphAlgorithm::SetInitialClique(const vector<int>& clique)
{
    lock_guard<mutex> lock(m_bestMutex);
    R = clique;
    m_uMaximumCliqueSize = clique.size();
    if (!clique.empty())
        LOG("Initial clique from ILS, size=" + to_string(clique.size()));
}

void MaxSubgraphAlgorithm::SetTimeOut(double seconds)
{
    m_TimeOutSeconds = seconds;
}

bool MaxSubgraphAlgorithm::IsTimedOut() const
{
    return m_bTimedOut;
}

void MaxSubgraphAlgorithm::SetParallelMode(bool enable)
{
    m_parallelMode = enable;
}

void MaxSubgraphAlgorithm::SetNumThreads(int n)
{
    m_numThreads = n;
}

void MaxSubgraphAlgorithm::SetTaskQueue(TaskQueue* queue)
{
    m_taskQueue = queue;
}

long MaxSubgraphAlgorithm::Run(list<list<int>> &cliques)
{
    LOG("=== Starting MCS ===");
    vector<int> initialClique;
    {
        lock_guard<mutex> lock(m_bestMutex);
        initialClique = R;
    }

    vector<int> &P = stackP[0];
    vector<int> &vColors = stackColors[0];
    vector<int> &vVertexOrder = stackOrder[0];

    InitializeOrder(P, vVertexOrder, vColors);
    Color(vVertexOrder, P, vColors);
    vVertexOrder = P;

    {
        lock_guard<mutex> lock(m_bestMutex);
        if (!initialClique.empty() && initialClique.size() > m_uMaximumCliqueSize) {
            m_uMaximumCliqueSize = initialClique.size();
            if (!cliques.empty()) {
                cliques.back() = list<int>(initialClique.begin(), initialClique.end());
            }
            LOG("Adopted ILS clique as starting best, size=" + to_string(initialClique.size()));
        }
        R.clear();
    }

    nodeCount = 0;
    depth = 0;
    m_StartTimePoint = chrono::steady_clock::now();
    m_LastLogTime = m_StartTimePoint;

    LOG("Initial best bound = " + to_string(m_uMaximumCliqueSize.load()));
    RunRecursive(P, vVertexOrder, cliques, vColors);

    double totalTime = chrono::duration<double>(chrono::steady_clock::now() - m_StartTimePoint).count();
    LOG("=== Finished, max clique size=" + to_string(m_uMaximumCliqueSize.load()) +
        ", nodes=" + to_string(nodeCount) +
        ", time=" + to_string(totalTime) + "s" +
        ", repairs=" + to_string(getRepairCount()));
    return m_uMaximumCliqueSize;
}

void MaxSubgraphAlgorithm::RunRecursive(vector<int> &P,
                                        vector<int> &vVertexOrder,
                                        list<list<int>> &cliques,
                                        vector<int> &vColors)
{
    while (!P.empty())
    {
        nodeCount++;

        // Вычисляем прошедшее время и проверяем, пора ли логгировать прогресс
        auto now = chrono::steady_clock::now();
        double elapsed = chrono::duration<double>(now - m_StartTimePoint).count();
        double lastLogElapsed = chrono::duration<double>(m_LastLogTime - m_StartTimePoint).count();

        if (elapsed - lastLogElapsed >= LOG_INTERVAL_SEC) {
            m_LastLogTime = now;
            LOG("Progress: nodes=" + to_string(nodeCount) +
                " time=" + to_string(elapsed) + "s" +
                " best=" + to_string(m_uMaximumCliqueSize.load()) +
                " depth=" + to_string(depth) +
                " Rsize=" + to_string(R.size()) +
                " Psize=" + to_string(P.size()) +
                " bound=" + to_string(vColors.back()));
        }

        if (m_TimeOutSeconds > 0 && elapsed > m_TimeOutSeconds) {
            m_bTimedOut = true;
            LOG("Timeout occurred");
            return;
        }

        int bound = vColors.back();
        {
            lock_guard<mutex> lock(m_bestMutex);
            if ((int)R.size() + bound <= (int)m_uMaximumCliqueSize)
            {
                P.clear();
                return;
            }
        }

        vColors.pop_back();
        int v = P.back();
        P.pop_back();

        vector<int> newOrder;
        GetNewOrder(newOrder, vVertexOrder, P, v);

        if (!newOrder.empty())
        {
            vector<int> newP(newOrder.size());
            vector<int> newColors(newOrder.size());
            Color(newOrder, newP, newColors);

            depth++;
            RunRecursive(newP, newOrder, cliques, newColors);
            depth--;
        }
        else
        {
            lock_guard<mutex> lock(m_bestMutex);
            if ((int)R.size() > (int)m_uMaximumCliqueSize)
            {
                m_uMaximumCliqueSize = R.size();
                cliques.back() = list<int>(R.begin(), R.end());
                LOG("New best clique found, size=" + to_string(R.size()) +
                    " time=" + to_string(elapsed) + "s");
            }
        }

        ProcessOrderAfterRecursion(vVertexOrder, P, vColors, v);
    }
    P.clear();
}

void MaxSubgraphAlgorithm::RunFromTask(const Task& t,
                                       list<list<int>>& cliques)
{
    vector<int> P = t.P;
    vector<int> vVertexOrder = t.order;
    vector<int> vColors = t.colors;

    {
        lock_guard<mutex> lock(m_bestMutex);
        R = t.clique;
    }

    depth = t.depth;
    RunRecursive(P, vVertexOrder, cliques, vColors);
}

void MaxSubgraphAlgorithm::SetProgressCallback(ProgressCallback cb)
{
    m_progressCallback = cb;
}