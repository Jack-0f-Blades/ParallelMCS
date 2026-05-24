#include "ParallelRunner.h"
#include "TaskQueue.h"
#include "MaxSubgraphAlgorithm.h"
#include "Logger.h"
#include <thread>
#include <chrono>

ParallelRunner::ParallelRunner(MaxSubgraphAlgorithm* a, TaskQueue* q)
    : queue(q), algo(a) {}

void ParallelRunner::Start(int nThreads, std::list<std::list<int>>& cliques)
{
    threads.reserve(nThreads);
    for (int i = 0; i < nThreads; ++i) {
        threads.emplace_back(&ParallelRunner::Worker, this, std::ref(cliques));
    }
    LOG("ParallelRunner: started " + std::to_string(nThreads) + " worker threads");
}

void ParallelRunner::Wait()
{
    while (queue->pending_tasks() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    queue->Stop();
    for (auto& t : threads) t.join();
    LOG("ParallelRunner: all workers joined");
}

void ParallelRunner::Worker(std::list<std::list<int>>& cliques)
{
    while (true) {
        if (algo->IsTimedOut()) break;
        auto taskOpt = queue->try_pop();
        if (taskOpt.has_value()) {
            LOG("Worker: popped task, pending=" + std::to_string(queue->pending_tasks()));
            try {
                algo->RunFromTask(*taskOpt, cliques);
            } catch (const std::exception& e) {
                // можно залогировать ошибку, но не обязательно
            } catch (...) {
            }
            queue->taskDone();
            LOG("Worker: finished task, pending=" + std::to_string(queue->pending_tasks()));
        } else {
            if (queue->IsStopped()) break;
            std::this_thread::yield();
        }
    }
    LOG("Worker: exiting");
}