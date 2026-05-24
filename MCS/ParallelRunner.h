/*Управляет пулом потоков-воркеров. Каждый воркер в цикле извлекает задачу из очереди и вызывает RunFromTask.*/


#ifndef PARALLEL_RUNNER_H
#define PARALLEL_RUNNER_H

#include <vector>
#include <thread>
#include <list>

class TaskQueue;
class MaxSubgraphAlgorithm;

class ParallelRunner
{
public:
    ParallelRunner(MaxSubgraphAlgorithm* algo, TaskQueue* queue);

    void Start(int numThreads, std::list<std::list<int>>& cliques);
    void Wait();

private:
    void Worker(std::list<std::list<int>>& cliques);

    TaskQueue* queue;
    MaxSubgraphAlgorithm* algo;
    std::vector<std::thread> threads;
};

#endif