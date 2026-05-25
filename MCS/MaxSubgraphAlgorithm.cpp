#include "MaxSubgraphAlgorithm.h"
#include "TaskQueue.h"
#include "Logger.h"
#include <iostream>
#include <sstream>
#include <thread>

using namespace std;

static const double LOG_INTERVAL_SEC = 2.0;
static const int NODE_CHECK_INTERVAL = 10000;

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
    if (clique.size() > m_uMaximumCliqueSize.load(memory_order_acquire)) {
        m_uMaximumCliqueSize.store(clique.size(), memory_order_release);
        R = clique;
        LOG("Initial clique from ILS, size=" + to_string(clique.size()));
    }
}

void MaxSubgraphAlgorithm::SetTimeOut(double seconds)
{
    m_TimeOutSeconds = seconds;
}

bool MaxSubgraphAlgorithm::IsTimedOut() const
{
    return m_bTimedOut.load(memory_order_acquire);
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
    vector<int> initialClique = R; // копия

    vector<int> &P = stackP[0];
    vector<int> &vColors = stackColors[0];
    vector<int> &vVertexOrder = stackOrder[0];

    InitializeOrder(P, vVertexOrder, vColors);
    // Убрана лишняя раскраска:
    vVertexOrder = P;

    if (!initialClique.empty() && initialClique.size() > m_uMaximumCliqueSize.load(memory_order_acquire)) {
        m_uMaximumCliqueSize.store(initialClique.size(), memory_order_release);
        if (!cliques.empty()) {
            cliques.back() = list<int>(initialClique.begin(), initialClique.end());
        }
        LOG("Adopted ILS clique as starting best, size=" + to_string(initialClique.size()));
    }
    R.clear();

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
    return m_uMaximumCliqueSize.load();
}

void MaxSubgraphAlgorithm::RunRecursive(vector<int> &P,
                                        vector<int> &vVertexOrder,
                                        list<list<int>> &cliques,
                                        vector<int> &vColors)
{
    while (!P.empty())
    {
        ++nodeCount;

        if ((nodeCount % NODE_CHECK_INTERVAL) == 0) {
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
                m_bTimedOut.store(true, memory_order_release);
                LOG("Timeout occurred");
                return;
            }
        }

        int bound = vColors.back();
        if (R.size() + bound <= m_uMaximumCliqueSize.load(memory_order_relaxed)) {
            P.clear();
            return;
        }

        vColors.pop_back();
        int v = P.back();
        P.pop_back();

        vector<int> &newOrder = stackOrder[R.size() + 1];
        GetNewOrder(newOrder, vVertexOrder, P, v);

        if (!newOrder.empty())
        {
            vector<int> &newP = stackP[R.size() + 1];
            vector<int> &newColors = stackColors[R.size() + 1];
            newP.resize(newOrder.size());
            newColors.resize(newOrder.size());
            Color(newOrder, newP, newColors);

            // PREPRUNE: проверка перед рекурсией
            if (R.size() + newColors.back() > m_uMaximumCliqueSize.load(memory_order_relaxed)) {
                ++depth;
                RunRecursive(newP, newOrder, cliques, newColors);
                --depth;
            }
        }
        else
        {
            // Найдена новая клика
            size_t newSize = R.size();
            size_t oldBest = m_uMaximumCliqueSize.load(memory_order_acquire);
            while (newSize > oldBest) {
                if (m_uMaximumCliqueSize.compare_exchange_weak(oldBest, newSize,
                                                              memory_order_release,
                                                              memory_order_acquire)) {
                    // обновляем список клик (не требуется блокировка, т.к. однопоточный режим)
                    if (!cliques.empty())
                        cliques.back() = list<int>(R.begin(), R.end());
                    double elapsed = chrono::duration<double>(chrono::steady_clock::now() - m_StartTimePoint).count();
                    LOG("New best clique found, size=" + to_string(R.size()) +
                        " time=" + to_string(elapsed) + "s");
                    break;
                }
                newSize = R.size();
            }
        }

        ProcessOrderAfterRecursion(vVertexOrder, P, vColors, v);
    }

    ProcessOrderBeforeReturn(vVertexOrder, P, vColors);
    P.clear();
}

void MaxSubgraphAlgorithm::RunFromTask(const Task& t,
                                       list<list<int>>& cliques)
{
    vector<int> P = t.P;
    vector<int> vVertexOrder = t.order;
    vector<int> vColors = t.colors;

    R = t.clique;
    depth = t.depth;
    RunRecursive(P, vVertexOrder, cliques, vColors);
}

void MaxSubgraphAlgorithm::SetProgressCallback(ProgressCallback cb)
{
    m_progressCallback = cb;
}