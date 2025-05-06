#pragma once

#include <glm/vec2.hpp>

namespace bee::physics
{

/// <summary>
/// Stores the details of a single collision.
/// </summary>
/// 

struct Interactable
{
    int dummy = 0;
};
struct CollisionData
{
    /// <summary>
    /// The ID of the first entity involved in the collision.
    /// </summary>
    bee::Entity entity1;

    /// <summary>
    /// The ID of the second entity involved in the collision.
    /// </summary>
    bee::Entity entity2;

    /// <summary>
    /// The normal vector on the point of contact, pointing away from entity2's physics body.
    /// </summary>
    glm::vec2 normal;

    /// <summary>
    /// The penetration depth of the two physics bodies
    /// (before they were displaced to resolve overlap).
    /// </summary>
    float depth;

    /// <summary>
    /// The approximate point of contact of the collision, in world coordinates.
    /// </summary>
    glm::vec2 contactPoint;
};

/// <summary>
/// A polygon-shaped collider for physics.
/// </summary>
struct PolygonCollider
{
public:
    /// <summary>
    /// The boundary vertices of the polygon in local coordinates,
    /// i.e. relative to the object's rotation and center of mass.
    /// </summary>
    std::vector<glm::vec2> m_pts;
    PolygonCollider(const std::vector<glm::vec2>& pts) : m_pts(pts) {}
};

/// <summary>
/// A disk-shaped collider for physics.
/// </summary>
struct DiskCollider
{
public:
    float radius;
    DiskCollider(float radiusToSet) : radius(radiusToSet) {}
};

/// <summary>
/// Describes a physics body (with a mass and shape) that lives in the physics world.
/// Add it as a component to your ECS entities.
/// </summary>
class Body
{
    friend class World;

public:
    enum Type
    {
        /// <summary>
        /// Indicates a physics body that does not move and is not affected by forces, as if it has infinite mass.
        /// </summary>
        Static,

        /// <summary>
        /// Indicates a physics body that can move under the influence of forces.
        /// </summary>
        Dynamic,

        /// <summary>
        /// Indicates a physics body that is not affected by forces. It moves purely according to its velocity.
        /// </summary>
        Kinematic
    };

    Body(Type type, float mass, float restitution = 1.0f) : m_type(type), m_restitution(restitution)
    {
        m_invMass = mass == 0.f ? 0.f : (1.f / mass);
    }

    Type GetType() const { return m_type; }
    inline float GetInvMass() const { return m_invMass; }
    inline float GetRestitution() const { return m_restitution; }

    inline const glm::vec2& GetPosition() const { return m_position; }
    inline const glm::vec2& GetLinearVelocity() const { return m_linearVelocity; }

    inline void SetPosition(const glm::vec2& pos) { m_position = pos; }
    inline void SetLinearVelocity(const glm::vec2& vel) { m_linearVelocity = vel; }

    inline void AddForce(const glm::vec2& force)
    {
        if (m_type == Dynamic) m_force += force;
    }
    inline void ApplyImpulse(const glm::vec2& imp)
    {
        if (m_type == Dynamic) m_linearVelocity += imp * m_invMass;
    }

    inline void AddCollisionData(const CollisionData& data) { m_collisions.push_back(data); }
    inline void ClearCollisionData() { m_collisions.clear(); }
    inline const std::vector<CollisionData>& GetCollisionData() const { return m_collisions; }

private:
    inline void ClearForces() { m_force = {0.f, 0.f}; }

    inline void Update(float dt)
    {
        if (m_type == Dynamic) m_linearVelocity += m_force * m_invMass * dt;
        m_position += m_linearVelocity * dt;
    }

    Type m_type;
    float m_invMass;
    float m_restitution;

    glm::vec2 m_position = {0.f, 0.f};
    // float m_angle;
    glm::vec2 m_linearVelocity = {0.f, 0.f};
    // float m_angularVelocity = 0.f;
    glm::vec2 m_force = {0.f, 0.f};

    std::vector<CollisionData> m_collisions = {};
};

}  // namespace bee::physics