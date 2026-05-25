#include "MCS.h"
#include "ParallelMCS.h"
#include "SparseMCS.h"
#include "SparseParallelMCS.h"
#include "Tools.h"
#include "ILS_src/ILSSolver.h"
#include "Logger.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstring>
#include <random>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

using namespace std;

vector<string> getFilesInDir(const string& dirPath) {
    vector<string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    string pattern = dirPath + "\\*";
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            string name = findData.cFileName;
            if (name != "." && name != "..")
                files.push_back(name);
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(dirPath.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name != "." && name != "..")
                files.push_back(name);
        }
        closedir(dir);
    }
#endif
    return files;
}

int main(int argc, char* argv[]) {
    string mode = "mcs_par";
    vector<string> inputDirs;
    string singleFile;
    string outputFile = "results.txt";

    int timeoutSec = 5400;
    int numThreads = thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    size_t queueCapacity = 0;
    int splitDepth = 2;
    size_t targetQueueSize = 100;
    size_t lowThreshold = 20;
    int nodesBetweenChecks = 500;
    int minTaskSize = 10;

    int ilsScans = 1000;
    int ilsAttempts = 15;
    int ilsThreads = -1;
    unsigned int ilsSeed = std::random_device{}();
    bool useILS = false;

    bool enableLogging = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc)
            mode = argv[++i];
        else if (arg == "--dir" && i + 1 < argc)
            inputDirs.push_back(argv[++i]);
        else if (arg == "--input-file" && i + 1 < argc)
            singleFile = argv[++i];
        else if (arg == "--timeout" && i + 1 < argc)
            timeoutSec = stoi(argv[++i]);
        else if (arg.find("--timeout=") == 0)
            timeoutSec = stoi(arg.substr(strlen("--timeout=")));
        else if (arg == "--threads" && i + 1 < argc)
            numThreads = stoi(argv[++i]);
        else if (arg.find("--threads=") == 0)
            numThreads = stoi(arg.substr(strlen("--threads=")));
        else if (arg == "--output" && i + 1 < argc)
            outputFile = argv[++i];
        else if (arg.find("--output=") == 0)
            outputFile = arg.substr(strlen("--output="));
        else if (arg == "--queue-capacity" && i + 1 < argc)
            queueCapacity = stoi(argv[++i]);
        else if (arg.find("--queue-capacity=") == 0)
            queueCapacity = stoi(arg.substr(strlen("--queue-capacity=")));
        else if (arg == "--split-depth" && i + 1 < argc)
            splitDepth = stoi(argv[++i]);
        else if (arg.find("--split-depth=") == 0)
            splitDepth = stoi(arg.substr(strlen("--split-depth=")));
        else if (arg == "--target-queue-size" && i + 1 < argc)
            targetQueueSize = stoi(argv[++i]);
        else if (arg.find("--target-queue-size=") == 0)
            targetQueueSize = stoi(arg.substr(strlen("--target-queue-size=")));
        else if (arg == "--low-threshold" && i + 1 < argc)
            lowThreshold = stoi(argv[++i]);
        else if (arg.find("--low-threshold=") == 0)
            lowThreshold = stoi(arg.substr(strlen("--low-threshold=")));
        else if (arg == "--nodes-between-checks" && i + 1 < argc)
            nodesBetweenChecks = stoi(argv[++i]);
        else if (arg.find("--nodes-between-checks=") == 0)
            nodesBetweenChecks = stoi(arg.substr(strlen("--nodes-between-checks=")));
        else if (arg == "--min-task-size" && i + 1 < argc)
            minTaskSize = stoi(argv[++i]);
        else if (arg.find("--min-task-size=") == 0)
            minTaskSize = stoi(arg.substr(strlen("--min-task-size=")));
        else if (arg == "--ils-scans" && i + 1 < argc)
            ilsScans = stoi(argv[++i]);
        else if (arg.find("--ils-scans=") == 0)
            ilsScans = stoi(arg.substr(strlen("--ils-scans=")));
        else if (arg == "--ils-attempts" && i + 1 < argc)
            ilsAttempts = stoi(argv[++i]);
        else if (arg.find("--ils-attempts=") == 0)
            ilsAttempts = stoi(arg.substr(strlen("--ils-attempts=")));
        else if (arg == "--ils-threads" && i + 1 < argc)
            ilsThreads = stoi(argv[++i]);
        else if (arg.find("--ils-threads=") == 0)
            ilsThreads = stoi(arg.substr(strlen("--ils-threads=")));
        else if (arg == "--seed" && i + 1 < argc)
            ilsSeed = static_cast<unsigned int>(stoul(argv[++i]));
        else if (arg.find("--seed=") == 0)
            ilsSeed = static_cast<unsigned int>(stoul(arg.substr(strlen("--seed="))));
        else if (arg == "--log") {
            enableLogging = true;
        }
    }

    // Корректировка количества потоков
    int originalThreads = numThreads;
    if (numThreads > 1) {
        numThreads--;
    } else if (numThreads == 1) {
        cerr << "Warning: --threads=1 would give 0 worker threads. Using 1 worker thread (parallel code will run effectively sequential)." << endl;
        numThreads = 1;
    }
    if (numThreads < 1) numThreads = 1;

    if (ilsThreads > 1) {
        ilsThreads--;
    } else if (ilsThreads == 1) {
        cerr << "Warning: --ils-threads=1 would give 0 worker threads for ILS. Using 1 worker thread." << endl;
    } else if (ilsThreads == 0) {
        ilsThreads = 1;
    }

    if (enableLogging) {
        Logger::instance().setEnabled(true);
        cout << "Logging enabled, output to mcs_log.txt" << endl;
    }

    bool useParallel = false;
    if (mode == "mcs") {
        useILS = false;
        useParallel = false;
    } else if (mode == "mcs_par") {
        useILS = false;
        useParallel = true;
    } else if (mode == "ils_par+mcs") {
        useILS = true;
        useParallel = false;
        mode = "mcs";
    } else if (mode == "ils_par+mcs_par") {
        useILS = true;
        useParallel = true;
        mode = "mcs_par";
    } else {
        cerr << "Unknown mode. Supported: mcs, mcs_par, ils_par+mcs, ils_par+mcs_par" << endl;
        return 1;
    }

    if (useParallel) {
        cout << "Parallel MCS will use " << numThreads << " worker thread(s) (total threads = " << (numThreads + 1) << ")" << endl;
    }
    if (useILS) {
        int actualIlsWorkers = (ilsThreads > 0) ? ilsThreads : numThreads;
        cout << "Parallel ILS will use " << actualIlsWorkers << " worker thread(s) (total threads = " << (actualIlsWorkers + 1) << ")" << endl;
    }

    if (inputDirs.empty() && singleFile.empty()) {
        cerr << "No input provided. Use --dir <dir> or --input-file <file>" << endl;
        return 1;
    }

    vector<pair<string, string>> tasks;
    if (!singleFile.empty()) {
        size_t slash = singleFile.find_last_of("/\\");
        string name = (slash == string::npos) ? singleFile : singleFile.substr(slash + 1);
        tasks.emplace_back(name, singleFile);
    } else {
        for (auto& dir : inputDirs) {
            auto files = getFilesInDir(dir);
            for (auto& f : files)
                tasks.emplace_back(f, dir + "/" + f);
        }
    }

    ofstream out(outputFile);
    if (!out.is_open()) {
        cerr << "Cannot open output file: " << outputFile << endl;
        return 1;
    }

    int total = tasks.size();
    int success = 0, timeouts = 0;
    auto startTimeGlobal = chrono::steady_clock::now();
    auto checkGlobalTimeout = [&]() -> bool {
        if (timeoutSec <= 0) return false;
        return chrono::duration<double>(chrono::steady_clock::now() - startTimeGlobal).count() > timeoutSec;
    };

    for (int taskIdx = 0; taskIdx < total; ++taskIdx) {
        auto& [name, path] = tasks[taskIdx];
        cout << "[" << taskIdx+1 << "/" << total << "] " << name << " ... " << flush;

        int n = 0, m = 0;
        vector<list<int>> adjList = readInGraphDimacs(n, m, path);
        if (n == 0) {
            cout << "SKIP (invalid)" << endl;
            continue;
        }

        double density = (2.0 * m) / (static_cast<double>(n) * (n - 1));
        bool useSparse = (density < 0.00001) || (n > 2000);
        if (useSparse) {
            cout << "[sparse representation, density=" << density << "] " << flush;
            LOG("Selected sparse representation for " + name + ", density=" + to_string(density));
        } else {
            cout << "[dense representation, density=" << density << "] " << flush;
            LOG("Selected dense representation for " + name + ", density=" + to_string(density));
        }

        vector<vector<char>> adjMatrix;
        vector<vector<int>> adjArray;

        if (!useSparse) {
            adjMatrix.assign(n, vector<char>(n, 0));
            for (int u = 0; u < n; ++u)
                for (int v : adjList[u])
                    adjMatrix[u][v] = 1;
        } else {
            adjArray.resize(n);
            for (int u = 0; u < n; ++u) {
                adjArray[u].reserve(adjList[u].size());
                for (int v : adjList[u])
                    adjArray[u].push_back(v);
            }
        }

        vector<int> clique;
        double totalTime = 0.0;
        bool timedOut = false;
        double ilsTotalTime = 0.0;
        vector<int> bestIlsClique;

        auto startTask = chrono::steady_clock::now();

        // ILS
        if (useILS) {
            if (adjArray.empty()) {
                adjArray.resize(n);
                for (int u = 0; u < n; ++u) {
                    adjArray[u].clear();
                    for (int v = 0; v < n; ++v)
                        if (adjMatrix[u][v]) adjArray[u].push_back(v);
                }
            }
            int actualIlsThreads = (ilsThreads > 0) ? ilsThreads : numThreads;
            size_t bestSize = 0;
            cout << "ILS start (" << ilsAttempts << " attempts) " << flush;
            for (int attempt = 0; attempt < ilsAttempts; ++attempt) {
                if (checkGlobalTimeout()) break;
                unsigned int seed = ilsSeed + attempt * 1000003;
                ILSResult res = RunParallelILS(adjArray, n, ilsScans, actualIlsThreads, seed);
                ilsTotalTime += res.ils_time_sec;
                if (res.clique.size() > bestSize) {
                    bestSize = res.clique.size();
                    bestIlsClique = move(res.clique);
                }
                cout << "\r[" << taskIdx+1 << "/" << total << "] " << name
                     << " ILS " << (attempt+1) << "/" << ilsAttempts
                     << " best=" << bestSize << " time=" << fixed << setprecision(1) << ilsTotalTime << "s   " << flush;
            }
            cout << endl;
        }

        // Основной алгоритм
        size_t bestKnownSize = 0; // для хранения размера из GetBestSize(), если клика не получена
        if (!checkGlobalTimeout()) {
            double remainingTimeout = (timeoutSec > 0) ? max(0.0, (double)timeoutSec - ilsTotalTime) : 0.0;

            if (!useParallel) {
                if (useSparse) {
                    SparseMCS algo(adjArray);
                    algo.SetQuiet(true);
                    if (timeoutSec > 0) algo.SetTimeOut(remainingTimeout);
                    if (!bestIlsClique.empty()) algo.SetInitialClique(bestIlsClique);
                    list<list<int>> cliques;
                    cliques.emplace_back();
                    algo.Run(cliques);
                    timedOut = algo.IsTimedOut();
                    if (!cliques.back().empty())
                        clique.assign(cliques.back().begin(), cliques.back().end());
                    bestKnownSize = algo.GetBestSize();
                } else {
                    MCS algo(adjMatrix);
                    algo.SetQuiet(true);
                    if (timeoutSec > 0) algo.SetTimeOut(remainingTimeout);
                    if (!bestIlsClique.empty()) algo.SetInitialClique(bestIlsClique);
                    list<list<int>> cliques;
                    cliques.emplace_back();
                    algo.Run(cliques);
                    timedOut = algo.IsTimedOut();
                    if (!cliques.back().empty())
                        clique.assign(cliques.back().begin(), cliques.back().end());
                    bestKnownSize = algo.GetBestSize();
                }
            } else {
                if (useSparse) {
                    SparseParallelMCS algo(adjArray, queueCapacity, splitDepth,
                                           targetQueueSize, lowThreshold,
                                           nodesBetweenChecks, minTaskSize);
                    algo.SetQuiet(true);
                    if (timeoutSec > 0) algo.SetTimeOut(remainingTimeout);
                    algo.SetNumThreads(numThreads);
                    if (!bestIlsClique.empty()) algo.SetInitialClique(bestIlsClique);
                    list<list<int>> cliques;
                    cliques.emplace_back();
                    algo.Run(cliques);
                    timedOut = algo.IsTimedOut();
                    if (!cliques.back().empty())
                        clique.assign(cliques.back().begin(), cliques.back().end());
                    bestKnownSize = algo.GetBestSize();
                } else {
                    ParallelMCS algo(adjMatrix, queueCapacity, splitDepth,
                                     targetQueueSize, lowThreshold,
                                     nodesBetweenChecks, minTaskSize);
                    algo.SetQuiet(true);
                    if (timeoutSec > 0) algo.SetTimeOut(remainingTimeout);
                    algo.SetNumThreads(numThreads);
                    if (!bestIlsClique.empty()) algo.SetInitialClique(bestIlsClique);
                    list<list<int>> cliques;
                    cliques.emplace_back();
                    algo.Run(cliques);
                    timedOut = algo.IsTimedOut();
                    if (!cliques.back().empty())
                        clique.assign(cliques.back().begin(), cliques.back().end());
                    bestKnownSize = algo.GetBestSize();
                }
            }

            if (clique.empty() && !bestIlsClique.empty())
                clique = bestIlsClique;
        } else {
            timedOut = true;
            if (!bestIlsClique.empty())
                clique = bestIlsClique;
        }

        totalTime = chrono::duration<double>(chrono::steady_clock::now() - startTask).count();
        if (checkGlobalTimeout()) timedOut = true;

        if (timedOut) ++timeouts;
        else ++success;

        string modeStr = useILS ? (mode == "mcs" ? "mcs_ils" : "mcs_par_ils") : mode;
        if (useSparse) modeStr += "_sparse";

        // Определяем размер для вывода: либо из найденной клики, либо из bestKnownSize
        size_t displaySize = 0;
        if (!clique.empty()) displaySize = clique.size();
        else if (bestKnownSize > 0) displaySize = bestKnownSize;

        cout << "\r[" << taskIdx+1 << "/" << total << "] " << name
             << " | " << modeStr
             << " | size=" << (displaySize ? to_string(displaySize) : "?")
             << " | time=" << fixed << setprecision(2) << totalTime << "s"
             << (useILS ? (" | ils_time=" + to_string(ilsTotalTime) + "s") : "")
             << " | " << (timedOut ? "TIMEOUT" : "OK") << endl;

        out << name << " | " << modeStr << " | ";
        if (displaySize) out << displaySize;
        else out << "?";
        out << " | " << fixed << setprecision(5) << totalTime << "s"
            << " | " << fixed << setprecision(2) << ilsTotalTime << "s"
            << " | " << (timedOut ? "TIMEOUT" : "OK") << " | ";
        for (int v : clique) out << (v+1) << " ";
        out << "\n";
        out.flush();
    }

    out << "\nTotal: " << total << ", OK: " << success << ", TIMEOUT: " << timeouts << endl;
    cout << "\nDone. Results saved in " << outputFile << endl;
    return 0;
}