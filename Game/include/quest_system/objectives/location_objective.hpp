#pragma once
#include "quest_system/objectives/objective.hpp"

namespace bee
{

class LocationObjective : public bee::Objective
{
public:
    LocationObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle, const Team a_team, glm::vec3 position, float radius)
        : Objective(a_name, a_quantity, a_handle, a_team)
    {
        objectiveName = a_name;
        quantity = a_quantity;
        handle = a_handle;
        team = a_team;
        m_goToPosition = position;
        m_locationRadius = radius;
    }
    ~LocationObjective() = default;

    void Update() override;
    void Render();
    void StartObjective() override;

private:  // functions
private:  // params
    glm::vec3 m_goToPosition = {0.0f, 0.0f, 0.0f};
    float m_locationRadius = 3.0f;
};
}  // namespace bee