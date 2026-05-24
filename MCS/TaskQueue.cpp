#include "TaskQueue.h"
#include "Logger.h"

TaskQueue::TaskQueue(size_t capacity)
    : buffer(capacity), head(0), tail(0), cap(capacity),
      count(0), pending(0), stopped(false) {}

bool TaskQueue::try_push(const Task& t)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (stopped || count >= cap) return false;
    buffer[tail] = t;
    tail = (tail + 1) % cap;
    ++count;
    ++pending;
    LOG("TaskQueue: push, count=" + std::to_string(count.load()) +
        " pending=" + std::to_string(pending.load()));
    return true;
}

std::optional<Task> TaskQueue::try_pop()
{
    std::lock_guard<std::mutex> lock(mtx);
    if (count == 0) return std::nullopt;
    Task t = buffer[head];
    head = (head + 1) % cap;
    --count;
    LOG("TaskQueue: pop, count=" + std::to_string(count.load()) +
        " pending=" + std::to_string(pending.load()));
    return t;
}

void TaskQueue::taskDone()
{
    --pending;
    LOG("TaskQueue: taskDone, pending=" + std::to_string(pending.load()));
}

size_t TaskQueue::size_approx() const
{
    return count.load(std::memory_order_relaxed);
}

size_t TaskQueue::capacity() const
{
    return cap;
}

size_t TaskQueue::pending_tasks() const
{
    return pending.load(std::memory_order_relaxed);
}

void TaskQueue::Stop()
{
    stopped.store(true);
}

bool TaskQueue::IsStopped() const
{
    return stopped.load();
}