#pragma once

#include "quest_system/objectives/objective.hpp"

namespace bee
{
class TrainUnitsObjective : public bee::Objective
{
public:
    TrainUnitsObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle,Team a_team);
    ~TrainUnitsObjective() = default;

    void Update() override;
    void StartObjective() override;
};
}  // namespace bee
