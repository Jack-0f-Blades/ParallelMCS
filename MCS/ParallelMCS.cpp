#include "ParallelMCS.h"
#include "ParallelRunner.h"
#include "OrderingTools.h"
#include "CliqueColoringStrategy.h"
#include "Logger.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <algorithm>
#include <chrono>
#include "CliqueColoringStrategy.h"

static const double PROGRESS_LOG_INTERVAL = 2.0; // секунды

ParallelMCS::ParallelMCS(std::vector<std::vector<char>> const &vAdjacencyMatrix,
                         size_t queueCapacity,
                         int splitDepth,
                         size_t targetQueueSize,
                         size_t lowThreshold,
                         int nodesBetweenChecks,
                         int minTaskSize)
    : MCS(vAdjacencyMatrix),
      m_taskQueue(queueCapacity == 0 ? std::max<size_t>(200, vAdjacencyMatrix.size()) : queueCapacity),
      m_queueCapacity(queueCapacity == 0 ? std::max<size_t>(200, vAdjacencyMatrix.size()) : queueCapacity),
      m_targetQueueSize(targetQueueSize),
      m_queueLowThreshold(lowThreshold),
      m_maxSplitDepth(splitDepth),
      m_nodesBetweenChecks(nodesBetweenChecks),
      m_minTaskSize(minTaskSize)
{
    SetName("mcs-parallel");
    SetParallelMode(true);
    SetTaskQueue(&m_taskQueue);
    this->m_maxSplitDepth = m_maxSplitDepth;
    LOG("ParallelMCS created: queueCapacity=" + std::to_string(m_queueCapacity) +
        " splitDepth=" + std::to_string(m_maxSplitDepth) +
        " targetQueueSize=" + std::to_string(m_targetQueueSize) +
        " lowThreshold=" + std::to_string(m_queueLowThreshold));
}

ParallelMCS::~ParallelMCS()
{
    m_taskQueue.Stop();
}

long ParallelMCS::Run(std::list<std::list<int>> &cliques)
{
    LOG("=== Starting Parallel MCS ===");
    m_StartTimePoint = std::chrono::steady_clock::now();
    m_LastLogTime = m_StartTimePoint;
    m_bTimedOut = false;

    coloringStrategy.SetStopFlag(&m_bTimedOut);

    std::vector<int> initialClique;
    {
        std::lock_guard<std::mutex> lock(m_bestMutex);
        initialClique = R;
    }

    // Инициализация порядка и раскраски
    std::vector<int> P;
    std::vector<int> vVertexOrder;
    std::vector<int> vColors;
    InitializeOrder(P, vVertexOrder, vColors);
    if (checkTimeout()) { m_taskQueue.Stop(); return m_uMaximumCliqueSize; }
    Color(vVertexOrder, P, vColors);
    vVertexOrder = P;

    {
        std::lock_guard<std::mutex> lock(m_bestMutex);
        if (!initialClique.empty() && initialClique.size() > m_uMaximumCliqueSize.load(std::memory_order_acquire)) {
            m_uMaximumCliqueSize.store(initialClique.size(), std::memory_order_release);
            if (!cliques.empty())
                cliques.back() = std::list<int>(initialClique.begin(), initialClique.end());
            LOG("Adopted ILS clique as starting best, size=" + std::to_string(initialClique.size()));
        }
        R.clear();
    }

    LOG("Initial best bound = " + std::to_string(m_uMaximumCliqueSize.load()));

    // Генерация начальных задач
    std::vector<int> currentP = P;
    std::vector<int> currentOrder = vVertexOrder;
    std::vector<int> currentColors = vColors;
    std::vector<int> localR;

    int generatedCount = 0;
    LOG("Generating initial tasks, targetQueueSize=" + std::to_string(m_targetQueueSize));
    while (!currentP.empty() && m_taskQueue.size_approx() < m_targetQueueSize) {
        if (checkTimeout()) { m_taskQueue.Stop(); return m_uMaximumCliqueSize; }
        int v = currentP.back();
        currentP.pop_back();
        std::vector<int> newOrder;
        GetNewOrderLocal(newOrder, currentOrder, currentP, v, localR);
        if (!newOrder.empty()) {
            std::vector<int> newP(newOrder.size());
            std::vector<int> newColors(newOrder.size());
            Color(newOrder, newP, newColors);
            if (checkTimeout()) { m_taskQueue.Stop(); return m_uMaximumCliqueSize; }
            size_t bound = newColors.empty() ? 0 : newColors.back();
            if (localR.size() + bound > m_uMaximumCliqueSize.load(std::memory_order_acquire)) {
                Task task;
                task.P = std::move(newP);
                task.order = std::move(newOrder);
                task.colors = std::move(newColors);
                task.clique = localR;
                task.depth = 1;
                if (m_taskQueue.try_push(task)) {
                    ++generatedCount;
                    LOG("Initial task created, queue size=" + std::to_string(m_taskQueue.size_approx()));
                } else break;
            }
        }
        ProcessOrderAfterRecursionLocal(currentOrder, currentP, currentColors, v, localR);
    }
    LOG("Initial tasks generated: " + std::to_string(generatedCount) + ", queue size=" + std::to_string(m_taskQueue.size_approx()));

    // Запуск воркеров
    ParallelRunner runner(this, &m_taskQueue);
    int threads = m_numThreads > 0 ? m_numThreads : 1;
    runner.Start(threads, cliques);

    // Основной поток тоже обрабатывает оставшиеся начальные вершины и следит за очередью
    long localNodeCount = 0;
    int localDepth = 0;
    std::vector<int> localR2;
    // Выполняем оставшиеся начальные вершины (если есть)
    while (!currentP.empty() || m_taskQueue.pending_tasks() > 0) {
        if (checkTimeout()) break;

        // Если очередь мала, добавляем новые задачи из оставшихся начальных вершин
        if (!currentP.empty() && m_taskQueue.size_approx() < m_queueLowThreshold) {
            int v = currentP.back();
            currentP.pop_back();
            std::vector<int> newOrder;
            GetNewOrderLocal(newOrder, currentOrder, currentP, v, localR2);
            if (!newOrder.empty()) {
                std::vector<int> newP(newOrder.size());
                std::vector<int> newColors(newOrder.size());
                Color(newOrder, newP, newColors);
                if (checkTimeout()) break;
                size_t bound = newColors.empty() ? 0 : newColors.back();
                if (localR2.size() + bound > m_uMaximumCliqueSize.load(std::memory_order_acquire)) {
                    Task task;
                    task.P = std::move(newP);
                    task.order = std::move(newOrder);
                    task.colors = std::move(newColors);
                    task.clique = localR2;
                    task.depth = 1;
                    if (m_taskQueue.try_push(task)) {
                        LOG("Main thread pushed additional initial task, queue size=" + std::to_string(m_taskQueue.size_approx()));
                    }
                }
            }
            ProcessOrderAfterRecursionLocal(currentOrder, currentP, currentColors, v, localR2);
            continue;
        }

        auto taskOpt = m_taskQueue.try_pop();
        if (taskOpt.has_value()) {
            LOG("Main thread popped task, pending=" + std::to_string(m_taskQueue.pending_tasks()));
            // Обрабатываем задачу в основном потоке
            std::vector<int> Ptask = taskOpt->P;
            std::vector<int> orderTask = taskOpt->order;
            std::vector<int> colorsTask = taskOpt->colors;
            std::vector<int> Rtask = taskOpt->clique;
            int depthTask = taskOpt->depth;
            RunRecursiveParallel(Ptask, orderTask, cliques, colorsTask, Rtask, depthTask, localNodeCount);
            m_taskQueue.taskDone();
            LOG("Main thread finished task, pending=" + std::to_string(m_taskQueue.pending_tasks()));
        } else {
            if (m_taskQueue.pending_tasks() == 0) break;
            std::this_thread::yield();
        }
    }

    m_taskQueue.Stop();
    runner.Wait();
    double totalTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_StartTimePoint).count();
    LOG("=== Finished Parallel MCS, max clique size=" + std::to_string(m_uMaximumCliqueSize.load()) +
        ", time=" + std::to_string(totalTime) + "s" +
        ", queueCapacity=" + std::to_string(m_queueCapacity));
    return m_uMaximumCliqueSize.load(std::memory_order_acquire);
}

void ParallelMCS::RunFromTask(const Task& t,
                              std::list<std::list<int>>& cliques)
{
    if (m_bTimedOut.load(std::memory_order_acquire)) return;
    std::vector<int> P = t.P;
    std::vector<int> vVertexOrder = t.order;
    std::vector<int> vColors = t.colors;
    std::vector<int> R = t.clique;
    int localDepth = t.depth;
    long localNodeCount = 0;
    LOG("Worker thread executing task: depth=" + std::to_string(localDepth) +
        " Rsize=" + std::to_string(R.size()) + " Psize=" + std::to_string(P.size()));
    RunRecursiveParallel(P, vVertexOrder, cliques, vColors, R, localDepth, localNodeCount);
    LOG("Worker thread task completed");
}

void ParallelMCS::RunRecursiveParallel(std::vector<int> &P,
                                       std::vector<int> &vVertexOrder,
                                       std::list<std::list<int>> &cliques,
                                       std::vector<int> &vColors,
                                       std::vector<int> &R,
                                       int &depth,
                                       long &nodeCount)
{
    CliqueColoringStrategy localColoring(m_AdjacencyMatrix);
    localColoring.SetStopFlag(&m_bTimedOut);

    // Для периодического лога внутри задачи (по времени)
    auto taskStartTime = std::chrono::steady_clock::now();
    auto lastLogTime = taskStartTime;

    while (!P.empty()) {
        if (m_bTimedOut.load(std::memory_order_acquire)) return;
        ++nodeCount;

        // Периодический лог прогресса (каждые 2 секунды) – только для демонстрации активности
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - taskStartTime).count();
        if (elapsed - std::chrono::duration<double>(now - lastLogTime).count() >= PROGRESS_LOG_INTERVAL) {
            lastLogTime = now;
            LOG("Worker progress: nodes=" + std::to_string(nodeCount) +
                " depth=" + std::to_string(depth) +
                " Rsize=" + std::to_string(R.size()) +
                " Psize=" + std::to_string(P.size()) +
                " bound=" + std::to_string(vColors.back()));
        }

        int bound = vColors.back();
        size_t currentBest = m_uMaximumCliqueSize.load(std::memory_order_acquire);
        if (R.size() + bound <= currentBest) {
            P.clear();
            return;
        }

        vColors.pop_back();
        int v = P.back();
        P.pop_back();

        std::vector<int> newOrder;
        GetNewOrderLocal(newOrder, vVertexOrder, P, v, R);

        if (!newOrder.empty()) {
            std::vector<int> newP(newOrder.size());
            std::vector<int> newColors(newOrder.size());
            localColoring.Recolor(m_AdjacencyMatrix, newOrder, newP, newColors,
                                  static_cast<int>(currentBest),
                                  static_cast<int>(R.size()));

            size_t newBound = newColors.empty() ? 0 : newColors.back();
            if (R.size() + newBound <= currentBest) {
                ProcessOrderAfterRecursionLocal(vVertexOrder, P, vColors, v, R);
                continue;
            }

            bool shouldPush = false;
            if (depth >= m_maxSplitDepth && (nodeCount % m_nodesBetweenChecks) == 0
                && !m_bTimedOut.load(std::memory_order_acquire)) {
                size_t qSize = m_taskQueue.size_approx();
                if (qSize < m_queueLowThreshold && newOrder.size() >= static_cast<size_t>(m_minTaskSize)) {
                    shouldPush = true;
                    LOG("Worker: queue size=" + std::to_string(qSize) + " below threshold, creating new task");
                }
            }

            if (shouldPush) {
                Task subTask;
                subTask.P = std::move(newP);
                subTask.order = std::move(newOrder);
                subTask.colors = std::move(newColors);
                subTask.clique = R;
                subTask.depth = depth + 1;
                if (m_taskQueue.try_push(subTask)) {
                    LOG("Worker: pushed new task, queue size=" + std::to_string(m_taskQueue.size_approx()));
                    ProcessOrderAfterRecursionLocal(vVertexOrder, P, vColors, v, R);
                    continue;
                }
            }

            ++depth;
            RunRecursiveParallel(newP, newOrder, cliques, newColors, R, depth, nodeCount);
            --depth;
        } else {
            size_t newSize = R.size();
            size_t oldBest = m_uMaximumCliqueSize.load(std::memory_order_acquire);
            while (newSize > oldBest) {
                if (m_uMaximumCliqueSize.compare_exchange_weak(oldBest, newSize,
                                                              std::memory_order_release,
                                                              std::memory_order_acquire)) {
                    std::lock_guard<std::mutex> lock(m_bestMutex);
                    if (!cliques.empty())
                        cliques.back() = std::list<int>(R.begin(), R.end());
                    LOG("New best clique found, size=" + std::to_string(newSize) +
                        " time=" + std::to_string(std::chrono::duration<double>(std::chrono::steady_clock::now() - m_StartTimePoint).count()) + "s");
                    break;
                }
                newSize = R.size();
            }
        }

        ProcessOrderAfterRecursionLocal(vVertexOrder, P, vColors, v, R);
    }
    P.clear();
}

bool ParallelMCS::checkTimeout()
{
    if (m_TimeOutSeconds <= 0) return false;
    auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - m_StartTimePoint).count();
    if (elapsed > m_TimeOutSeconds) {
        m_bTimedOut.store(true, std::memory_order_release);
        LOG("Timeout occurred");
        return true;
    }
    return false;
}