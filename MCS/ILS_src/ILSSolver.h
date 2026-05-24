/**
 * Обёртки для ILS (последовательного и параллельного).
 * Возвращают найденную клику (индексы 0‑based) и время ILS в секундах.
 */
#ifndef ILSSOLVER_H
#define ILSSOLVER_H

#include <vector>
#include <string>

struct ILSResult {
    std::vector<int> clique;
    double ils_time_sec = 0.0;
};

/// Последовательный ILS
ILSResult RunSequentialILS(const std::vector<std::vector<int>>& adj,
                           int n, int max_scans, unsigned int seed);

/// Параллельный ILS (все потоки кооперируются)
ILSResult RunParallelILS(const std::vector<std::vector<int>>& adj,
                         int n, int total_scans, int num_threads,
                         unsigned int base_seed);

#endif