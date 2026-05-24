#include "Algorithm.h"

Algorithm::Algorithm(std::string const &name)
    : m_sName(name)
    , m_bQuiet(false)
{
}

Algorithm::~Algorithm() {}

void Algorithm::AddCallBack(std::function<void(std::list<int> const&)> callback)
{
    m_vCallBacks.push_back(callback);
}

void Algorithm::ExecuteCallBacks(std::list<int> const &vertexSet) const
{
    for (auto &function : m_vCallBacks)
        function(vertexSet);
}

void Algorithm::SetName(std::string const &name)
{
    m_sName = name;
}

std::string Algorithm::GetName() const
{
    return m_sName;
}

void Algorithm::SetQuiet(bool const quiet)
{
    m_bQuiet = quiet;
}

bool Algorithm::GetQuiet() const
{
    return m_bQuiet;
}