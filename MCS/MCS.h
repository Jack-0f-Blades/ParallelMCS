/* Реализация MCS (Tomita et al., 2010). 
Отличается от MCQ тем, что в методе Color вызывает Recolor (перекраску) для более точной верхней границы.*/

#ifndef MCS_H
#define MCS_H

#include "StaticOrderMCS.h"

class MCS : public StaticOrderMCS
{
public:
    MCS(std::vector<std::vector<char>> const &vAdjacencyMatrix);

    virtual void Color(std::vector<int> const &vVertexOrder,
                       std::vector<int> &vVerticesToReorder,
                       std::vector<int> &vColors) override;
};

#endif