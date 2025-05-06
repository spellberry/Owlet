#pragma once
#include "quest_system/objectives/objective.hpp"

namespace bee
{

class KillObjective : public bee::Objective
{
public:
    KillObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle, const Team a_team);
    ~KillObjective() = default;

    void Update() override;
    void StartObjective() override;

private:  // functions
    
private:  // params
    int m_aliveTargetUnits = -1;
    int m_handleUnitCnt = -1;
};
}  // namespace bee