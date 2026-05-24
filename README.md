# MCS – Maximum Clique Solver

Данный проект реализует алгоритмы поиска максимальной клики в неориентированном графе:
- **MCS** – последовательная версия (Tomita et al., 2010)
- **Parallel MCS** – параллельная версия с динамическим планированием задач
- **ILS (Iterated Local Search)** – эвристика для получения начальной нижней границы, может работать последовательно или параллельно

Алгоритмы автоматически выбирают плотное (матрица смежности) или разреженное (список смежности) представление в зависимости от плотности графа и его размера.

## Сборка

Для компиляции необходим компилятор с поддержкой C++17 и библиотекой pthread.

```bash
g++ -std=c++17 -O2 -pthread -DENABLE_LOGGING -o run_tests.exe \
    run_tests.cpp \
    Algorithm.cpp \
    MaxSubgraphAlgorithm.cpp \
    MCQ.cpp \
    StaticOrderMCS.cpp \
    MCS.cpp \
    CliqueColoringStrategy.cpp \
    SparseCliqueColoringStrategy.cpp \
    SparseMCS.cpp \
    SparseParallelMCS.cpp \
    OrderingTools.cpp \
    DegeneracyTools.cpp \
    GraphTools.cpp \
    Tools.cpp \
    ParallelMCS.cpp \
    ParallelRunner.cpp \
    TaskQueue.cpp \
    ILS_src/ILSSolver.cpp \
    Logger.cpp