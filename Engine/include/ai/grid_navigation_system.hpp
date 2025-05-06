#pragma once
#include "ai/navigation_path.hpp"
#include "core/ecs.hpp"
#include "navigation_grid.hpp"

namespace bee::ai
{
struct GridAgent
{
    GridAgent(float radiusToSet, float speedToSet, float heightToSet)
        : height(heightToSet), radius(radiusToSet), speed(speedToSet)
    {
    }

    void SetGoal(const glm::vec2& goalToSet, bool shouldRecomputePath = true);

    void ComputePath(::bee::ai::NavigationGrid const& grid, const glm::vec2& currentPos);
    void CalculateVerticalPosition(const glm::vec3& currentPos, float dt);
    void ComputePreferredVelocity(const glm::vec3& currentPos, float dt);

    const float height;
    const float radius;
    const float m_detectionRadius = radius * 5;
    float speed;
    glm::vec2 goal = {0.f, 0.f};
    glm::vec2 preferredVelocity = {0.f, 0.f};
    float verticalPosition = 0;
    bool recomputePath = false;
    bee::ai::NavigationPath path = {};
};

class GridNavigationSystem : public bee::System
{
public:
    GridNavigationSystem(float fixedDeltaTime, const bee::ai::NavigationGrid& grid);
    ~GridNavigationSystem() override{};
    void Update(float dt) override;
    bee::ai::NavigationGrid& GetGrid() { return m_grid; }
    void Render() override;

    // This function will work as intended if there is an entity with a TerrainDataComponent that exists.
    void UpdateFromTerrain();

private:
    void Separation(const float detectionRadius, const glm::vec2& currentPos, const glm::vec2& otherAgentPos, glm::vec2& prefferedVelocity) const;

    bee::ai::NavigationGrid m_grid{{0, 0}, 0, 0, 0};
    float m_fixedDeltaTime = 1.0f;
    float m_timeSinceLastFrame = 0.0f;
};
}  // namespace bee::ai