/*Кольцевой буфер фиксированной ёмкости для хранения задач. 
Поддерживает потокобезопасные try_push / try_pop с помощью мьютекса, а также атомарные счётчики count и pending.*/

#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <vector>
#include <optional>
#include <mutex>
#include <atomic>

struct Task
{
    std::vector<int> P;
    std::vector<int> order;
    std::vector<int> colors;
    std::vector<int> clique;
    int depth = 0;
};

class TaskQueue
{
public:
    explicit TaskQueue(size_t capacity);

    bool try_push(const Task& t);
    std::optional<Task> try_pop();
    void taskDone();

    size_t size_approx() const;
    size_t capacity() const;
    size_t pending_tasks() const;

    void Stop();
    bool IsStopped() const;

private:
    std::vector<Task> buffer;
    size_t head, tail;
    size_t cap;
    mutable std::mutex mtx;
    std::atomic<size_t> count;
    std::atomic<size_t> pending;
    std::atomic<bool> stopped;
};

#endif