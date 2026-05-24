/**
 * Базовый класс для любого алгоритма.
 * Предоставляет имя, режим "тихого" вывода и механизм коллбэков,
 * которые вызываются при нахождении нового решения (клики).
 */
#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <string>
#include <vector>
#include <list>
#include <functional>

class Algorithm
{
public:
    Algorithm(std::string const &name);
    virtual ~Algorithm();

    void SetName(std::string const &name);
    std::string GetName() const;

    void AddCallBack(std::function<void(std::list<int> const&)> callback);
    void ExecuteCallBacks(std::list<int> const &vertexSet) const;

    void SetQuiet(bool const quiet);
    bool GetQuiet() const;

    virtual long Run(std::list<std::list<int>> &cliques) = 0;

private:
    std::string m_sName;
    bool m_bQuiet;
    std::vector<std::function<void(std::list<int> const&)>> m_vCallBacks;
};

#endif // ALGORITHM_H