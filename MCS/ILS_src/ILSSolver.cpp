#include "ILSSolver.h"
#include <random>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <limits>
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>

// ============================================================================
//  ILS for Maximum Independent Set (MIS) – класс единый для обоих вариантов
// ============================================================================

class ILSMIS {
public:
    ILSMIS(int n, const std::vector<std::vector<int>>& adj)
        : n_(n), adj_(adj), conf_(n, 0), inS_(n, false)
    {
        pos_in_addable_.assign(n, -1);
        pos_in_swappable_.assign(n, -1);
    }

    bool is_independent_set(const std::vector<int>& S) const {
        std::unordered_set<int> Sset(S.begin(), S.end());
        for (int v : S) {
            for (int u : adj_[v]) {
                if (Sset.count(u)) return false;
            }
        }
        return true;
    }

    std::pair<std::vector<int>, double> run(int max_scans, std::mt19937& rng,
                                            const std::vector<int>* initial_solution = nullptr) {
        auto start = std::chrono::steady_clock::now();
        std::vector<int> S;
        if (initial_solution && !initial_solution->empty()) {
            rebuild_structures(*initial_solution);
            S = *initial_solution;
        } else {
            S = greedy_initial_min_degree();
        }

        int best_size = static_cast<int>(S.size());
        auto best_S = S;
        int k_current = std::max(1, best_size / 10);
        int consecutive_failures = 0;
        const double K_INCREASE_FACTOR = 1.2;
        const int MAX_K = best_size / 2;

        for (int iter = 0; iter < max_scans; ++iter) {
            ILSMIS working_copy(n_, adj_);
            working_copy.rebuild_structures(best_S);
            working_copy.perturb_incremental(rng, k_current);
            auto candidate = working_copy.full_local_search(rng);
            assert(is_independent_set(candidate));

            if (candidate.size() > best_size) {
                best_size = static_cast<int>(candidate.size());
                best_S = std::move(candidate);
                consecutive_failures = 0;
                k_current = std::max(1, best_size / 10);
            } else {
                ++consecutive_failures;
                k_current = static_cast<int>(k_current * K_INCREASE_FACTOR);
                if (k_current > MAX_K) k_current = MAX_K;
                if (k_current < 1) k_current = 1;
            }
        }

        auto end = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        return {best_S, elapsed};
    }

private:
    int n_;
    const std::vector<std::vector<int>>& adj_;
    std::vector<int> conf_;
    std::vector<bool> inS_;
    std::vector<int> addable_;
    std::vector<int> swappable_;
    std::vector<int> pos_in_addable_;
    std::vector<int> pos_in_swappable_;

    void add_to_addable(int v) {
        if (pos_in_addable_[v] != -1) return;
        pos_in_addable_[v] = static_cast<int>(addable_.size());
        addable_.push_back(v);
    }
    void remove_from_addable(int v) {
        int p = pos_in_addable_[v];
        if (p == -1) return;
        int last = addable_.back();
        addable_[p] = last;
        pos_in_addable_[last] = p;
        addable_.pop_back();
        pos_in_addable_[v] = -1;
    }
    void add_to_swappable(int v) {
        if (pos_in_swappable_[v] != -1) return;
        pos_in_swappable_[v] = static_cast<int>(swappable_.size());
        swappable_.push_back(v);
    }
    void remove_from_swappable(int v) {
        int p = pos_in_swappable_[v];
        if (p == -1) return;
        int last = swappable_.back();
        swappable_[p] = last;
        pos_in_swappable_[last] = p;
        swappable_.pop_back();
        pos_in_swappable_[v] = -1;
    }

    void add_vertex(int v) {
        assert(!inS_[v]);
        inS_[v] = true;
        remove_from_addable(v);
        remove_from_swappable(v);
        for (int w : adj_[v]) {
            if (!inS_[w]) {
                int old = conf_[w];
                conf_[w]++;
                if (old == 0) {
                    remove_from_addable(w);
                    add_to_swappable(w);
                } else if (old == 1) {
                    remove_from_swappable(w);
                }
            }
        }
    }

    void remove_vertex(int v) {
        assert(inS_[v]);
        inS_[v] = false;
        for (int w : adj_[v]) {
            if (!inS_[w]) {
                int old = conf_[w];
                conf_[w]--;
                if (old == 1) {
                    remove_from_swappable(w);
                    add_to_addable(w);
                } else if (old == 2) {
                    add_to_swappable(w);
                }
            }
        }
        int new_conf = 0;
        for (int u : adj_[v]) if (inS_[u]) new_conf++;
        conf_[v] = new_conf;
        if (new_conf == 0) add_to_addable(v);
        else if (new_conf == 1) add_to_swappable(v);
    }

    void rebuild_structures(const std::vector<int>& S) {
        std::fill(conf_.begin(), conf_.end(), 0);
        std::fill(inS_.begin(), inS_.end(), false);
        addable_.clear();
        swappable_.clear();
        std::fill(pos_in_addable_.begin(), pos_in_addable_.end(), -1);
        std::fill(pos_in_swappable_.begin(), pos_in_swappable_.end(), -1);
        for (int v : S) inS_[v] = true;
        for (int v = 0; v < n_; ++v) {
            if (inS_[v]) continue;
            int c = 0;
            for (int u : adj_[v]) if (inS_[u]) c++;
            conf_[v] = c;
            if (c == 0) add_to_addable(v);
            else if (c == 1) add_to_swappable(v);
        }
    }

    std::vector<int> greedy_initial_min_degree() {
        std::vector<int> deg(n_);
        for (int i = 0; i < n_; ++i) deg[i] = static_cast<int>(adj_[i].size());
        std::vector<bool> alive(n_, true);
        std::vector<int> S;
        while (true) {
            int min_deg = std::numeric_limits<int>::max();
            int best_v = -1;
            for (int v = 0; v < n_; ++v) {
                if (alive[v] && deg[v] < min_deg) {
                    min_deg = deg[v];
                    best_v = v;
                }
            }
            if (best_v == -1) break;
            S.push_back(best_v);
            std::vector<int> to_remove;
            to_remove.push_back(best_v);
            for (int u : adj_[best_v]) if (alive[u]) to_remove.push_back(u);
            for (int u : to_remove) {
                if (!alive[u]) continue;
                alive[u] = false;
                for (int w : adj_[u]) if (alive[w]) deg[w]--;
            }
        }
        rebuild_structures(S);
        return S;
    }

    void perturb_incremental(std::mt19937& rng, int k) {
        std::vector<int> inS_list;
        int cur_sz = 0;
        for (int i = 0; i < n_; ++i) if (inS_[i]) {
            inS_list.push_back(i);
            cur_sz++;
        }
        if (k > cur_sz) k = cur_sz;
        if (k <= 0) return;

        std::shuffle(inS_list.begin(), inS_list.end(), rng);
        for (int i = 0; i < k; ++i) {
            int v = inS_list[i];
            if (inS_[v]) remove_vertex(v);
        }

        int added = 0;
        int max_attempts = 2 * k + 10;
        for (int attempt = 0; attempt < max_attempts && added < k; ++attempt) {
            if (addable_.empty()) break;
            std::uniform_int_distribution<int> dist(0, static_cast<int>(addable_.size()) - 1);
            int idx = dist(rng);
            int v = addable_[idx];
            if (!inS_[v] && conf_[v] == 0) {
                add_vertex(v);
                ++added;
            }
        }
    }

    bool try_1swap(std::mt19937& rng) {
        if (swappable_.empty()) return false;
        const int MAX_ATTEMPTS = 50;
        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(swappable_.size()) - 1);
            int idx = dist(rng);
            int v = swappable_[idx];
            if (conf_[v] != 1) continue;
            int u = -1;
            for (int w : adj_[v]) if (inS_[w]) { u = w; break; }
            if (u == -1) continue;
            remove_vertex(u);
            add_vertex(v);
            return true;
        }
        return false;
    }

    bool try_2swap(std::mt19937& rng) {
        if (swappable_.size() < 2) return false;
        const int MAX_ATTEMPTS = 100;
        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(swappable_.size()) - 1);
            int idx1 = dist(rng);
            int idx2 = dist(rng);
            if (idx1 == idx2) continue;
            int v1 = swappable_[idx1];
            int v2 = swappable_[idx2];
            if (conf_[v1] != 1 || conf_[v2] != 1) continue;
            int u1 = -1, u2 = -1;
            for (int w : adj_[v1]) if (inS_[w]) { u1 = w; break; }
            for (int w : adj_[v2]) if (inS_[w]) { u2 = w; break; }
            if (u1 == -1 || u2 == -1 || u1 == u2) continue;
            bool adjacent = false;
            for (int w : adj_[v1]) if (w == v2) { adjacent = true; break; }
            if (adjacent) continue;
            bool ok = true;
            for (int w : adj_[v1]) if (w == u2) { ok = false; break; }
            if (!ok) continue;
            for (int w : adj_[v2]) if (w == u1) { ok = false; break; }
            if (!ok) continue;
            remove_vertex(u1);
            remove_vertex(u2);
            add_vertex(v1);
            add_vertex(v2);
            return true;
        }
        return false;
    }

    bool improving_local_search(std::mt19937& rng) {
        bool improved = false;
        int stall = 0;
        const int MAX_STALL = 10;
        int steps = 0;
        const int MAX_STEPS = 500;

        while (stall < MAX_STALL && steps < MAX_STEPS) {
            steps++;
            bool progress = false;

            while (!addable_.empty()) {
                int v = addable_.back();
                add_vertex(v);
                improved = true;
                progress = true;
            }
            if (progress) {
                stall = 0;
                continue;
            }

            int old_size = current_size();
            if (try_1swap(rng)) {
                int new_size = current_size();
                if (new_size > old_size) {
                    improved = true;
                    stall = 0;
                    continue;
                } else {
                    stall++;
                    continue;
                }
            }

            old_size = current_size();
            if (try_2swap(rng)) {
                int new_size = current_size();
                if (new_size > old_size) {
                    improved = true;
                    stall = 0;
                    continue;
                } else {
                    stall++;
                    continue;
                }
            }

            stall++;
        }
        return improved;
    }

    bool random_plateau_move(std::mt19937& rng) {
        if (swappable_.empty()) return false;
        const int MAX_ATTEMPTS = 50;
        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
            std::uniform_real_distribution<double> prob(0.0, 1.0);
            bool try_one = (prob(rng) < 0.5);
            if (try_one) {
                if (try_1swap(rng)) return true;
            } else {
                if (try_2swap(rng)) return true;
            }
        }
        return false;
    }

    std::vector<int> full_local_search(std::mt19937& rng) {
        improving_local_search(rng);

        const int MAX_PLATEAU = 20;
        int last_size = current_size();

        for (int iter = 0; iter < MAX_PLATEAU; ++iter) {
            std::vector<int> snapshot;
            for (int i = 0; i < n_; ++i) if (inS_[i]) snapshot.push_back(i);

            if (!random_plateau_move(rng)) break;

            while (!addable_.empty()) {
                add_vertex(addable_.back());
            }
            improving_local_search(rng);

            int new_size = current_size();
            if (new_size > last_size) {
                last_size = new_size;
            } else if (new_size < last_size) {
                rebuild_structures(snapshot);
                break;
            }
        }

        std::vector<int> result;
        for (int i = 0; i < n_; ++i) if (inS_[i]) result.push_back(i);
        return result;
    }

    int current_size() const {
        return std::count(inS_.begin(), inS_.end(), true);
    }
};

// ============================================================================
//  Вспомогательные функции (дополнение, вызов ILSMIS)
// ============================================================================

static std::vector<std::vector<int>> build_complement(int n, const std::vector<std::vector<int>>& adj) {
    std::vector<std::vector<bool>> adj_matrix(n, std::vector<bool>(n, false));
    for (int i = 0; i < n; ++i) {
        for (int j : adj[i]) {
            adj_matrix[i][j] = true;
            adj_matrix[j][i] = true;
        }
    }
    std::vector<std::vector<int>> complement(n);
    for (int i = 0; i < n; ++i) {
        complement[i].reserve(n - 1 - (int)adj[i].size());
        for (int j = 0; j < n; ++j) {
            if (i != j && !adj_matrix[i][j])
                complement[i].push_back(j);
        }
    }
    return complement;
}

static std::vector<int> ils_max_clique(const std::vector<std::vector<int>>& complement,
                                       int n, int max_scans, unsigned int seed,
                                       double* out_elapsed,
                                       const std::vector<int>* initial_solution = nullptr) {
    std::mt19937 rng(seed);
    ILSMIS mis_solver(n, complement);
    auto [indep_set, elapsed] = mis_solver.run(max_scans, rng, initial_solution);
    if (out_elapsed) *out_elapsed = elapsed;
    return indep_set;
}

// ============================================================================
//  Параллельная кооперативная версия (только для RunParallelILS)
// ============================================================================

static std::vector<int> cooperative_parallel_ils(int n,
                                                 const std::vector<std::vector<int>>& complement,
                                                 int total_scans,
                                                 int num_threads,
                                                 unsigned int base_seed) {
    std::atomic<int> remaining_scans(total_scans);
    std::mutex best_mutex;
    std::vector<int> global_best;

    auto worker = [&](int thread_id) {
        unsigned int seed = base_seed + thread_id * 1000003;
        while (true) {
            int my_scans = remaining_scans.fetch_sub(1);
            if (my_scans <= 0) break;

            std::vector<int> initial;
            {
                std::lock_guard<std::mutex> lock(best_mutex);
                if (!global_best.empty()) initial = global_best;
            }

            double elapsed;
            std::vector<int> clique;
            if (!initial.empty()) {
                clique = ils_max_clique(complement, n, 1, seed + my_scans, &elapsed, &initial);
            } else {
                clique = ils_max_clique(complement, n, 1, seed + my_scans, &elapsed, nullptr);
            }

            {
                std::lock_guard<std::mutex> lock(best_mutex);
                if (clique.size() > global_best.size()) {
                    global_best = std::move(clique);
                }
            }
            seed += 1000003;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t)
        threads.emplace_back(worker, t);
    for (auto& th : threads) th.join();

    return global_best;
}

// ============================================================================
//  Реализация обёрток из ILSSolver.h
// ============================================================================

ILSResult RunSequentialILS(const std::vector<std::vector<int>>& adj,
                           int n, int max_scans, unsigned int seed) {
    ILSResult res;
    auto comp = build_complement(n, adj);
    double elapsed = 0.0;
    res.clique = ils_max_clique(comp, n, max_scans, seed, &elapsed, nullptr);
    res.ils_time_sec = elapsed;
    return res;
}

ILSResult RunParallelILS(const std::vector<std::vector<int>>& adj,
                         int n, int total_scans, int num_threads,
                         unsigned int base_seed) {
    ILSResult res;
    auto comp = build_complement(n, adj);
    auto start = std::chrono::steady_clock::now();
    res.clique = cooperative_parallel_ils(n, comp, total_scans, num_threads, base_seed);
    auto end = std::chrono::steady_clock::now();
    res.ils_time_sec = std::chrono::duration<double>(end - start).count();
    return res;
}