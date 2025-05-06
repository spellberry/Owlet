#pragma once

#include <glm/vec2.hpp>

#include "core/ecs.hpp"

namespace bee::physics
{
class Body;
struct CollisionData;

/// <summary>
/// System that handles the physics loop.
/// </summary>
class World : public bee::System
{
public:
    World(const float fixedDeltaTime) : m_fixedDeltaTime(fixedDeltaTime), m_timeSinceLastFrame(0), m_gravity(glm::vec2(0, 0)) {}
#ifdef BEE_INSPECTOR
    void Inspect(bee::Entity entity) override;
#endif

    void Update(float dt) override;
    void SetGravity(const glm::vec2& gravity) { m_gravity = gravity; }

    inline bool HasExecutedFrame() const { return m_hasExecutedFrame; }
    inline float GetFixedDeltaTime() const { return m_fixedDeltaTime; }

private:
    /// The fixed timestep (in seconds) for physics-related code.
    float m_fixedDeltaTime;
    float m_timeSinceLastFrame;
    bool m_hasExecutedFrame = false;

    glm::vec2 m_gravity;

    void ResolveCollision(const CollisionData& collision, Body& body1, Body& body2);
    void RegisterCollision(CollisionData& collision, const bee::Entity& entity1, Body& body1, const bee::Entity& entity2,
                           Body& body2);
    void UpdateCollisionDetection();
};
}  // namespace bee::physics