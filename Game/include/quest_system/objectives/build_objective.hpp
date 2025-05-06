#pragma once
#include "quest_system/objectives/objective.hpp"

namespace bee
{

class BuildObjective : public bee::Objective
{
public:
    BuildObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle, const Team a_team);
    ~BuildObjective() = default;

    void Update() override;
    void StartObjective() override;

private:  // functions
private:  // params
    int m_aliveEnemyUnits = -1;
};
}  // namespace bee