#pragma once
#include "objective.hpp"

namespace bee
{
class CollectObjective : public bee::Objective
{
public:
    CollectObjective(const std::string& a_name, int a_quantity, const std::string& a_handle, Team a_team);
    ~CollectObjective() = default;
    void Update() override;
    void StartObjective() override;
};
}  // namespace bee