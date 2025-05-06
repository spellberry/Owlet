#include "physics/world.hpp"

#include "core/engine.hpp"
#include "core/geometry2d.hpp"
#include "core/transform.hpp"
#include "tools/tools.hpp"
#include "physics/physics_components.hpp"
#ifdef _DEBUG
#include "rendering/debug_render.hpp"
#endif
#ifdef BEE_INSPECTOR
#include "tools/inspector.hpp"
#endif

using namespace glm;
using namespace bee::geometry2d;
using namespace bee::physics;

bool CollisionCheckDiskDisk(const vec2& center1, float radius1, const vec2& center2, float radius2, CollisionData& result)
{
    if (bee::CompareFloats(center1.x, center2.x, 0.001f) && bee::CompareFloats(center1.y, center2.y, 0.001f))
    {
        // Arbitrarily set normal to (1, 0) if centers are the same
        result.normal = vec2(1.0f, 0.0f);
        result.depth = radius1 + radius2; // Depth is the sum of radii
        result.contactPoint = center1; // Contact point is any point on the common center
        return true;
    }

    // check for overlap
    vec2 diff(center1 - center2);
    float l2 = length2(diff);
    float r = radius1 + radius2;
    if (l2 >= r * r) return false;

    // compute collision details
    result.normal = normalize(diff);
    result.depth = r - sqrt(l2);
    result.contactPoint = center2 + result.normal * radius2;

    return true;
}

bool CollisionCheckDiskPolygon(const vec2& diskCenter, float diskRadius, const vec2& polygonPos, const Polygon& polygonPoints,
                               CollisionData& result)
{
    const vec2& nearest = GetNearestPointOnPolygonBoundary(diskCenter - polygonPos, polygonPoints) + polygonPos;
    vec2 diff(diskCenter - nearest);
    float l2 = length2(diff);
    if (l2 == 0.0f)
    {
        result.normal = vec2(0.0f, 0.0f);
        result.depth = diskRadius;
        result.contactPoint = nearest;
        return true;
    }

    if (IsPointInsidePolygon(diskCenter - polygonPos, polygonPoints))
    {
        result.normal = vec2(0.0f, 0.0f);
        result.depth = diskRadius;
        result.contactPoint = nearest;
        return true;
    }

    if (l2 >= diskRadius * diskRadius) return false;

    // compute collision details
    float l = sqrt(l2);

    result.normal = diff / l;
    result.depth = diskRadius - l;
    result.contactPoint = nearest;
    return true;
}

void World::ResolveCollision(const CollisionData& collision, Body& body1, Body& body2)
{
    // if both bodies are not dynamic, there's nothing left to do
    if (body1.GetType() != Body::Type::Dynamic && body2.GetType() != Body::Type::Dynamic) return;

    // displace the objects to resolve overlap
    float m1 = body1.GetInvMass();
    float m2 = body2.GetInvMass();
    float totalInvMass = m1 + m2;
    const vec2& dist = (collision.depth / totalInvMass) * collision.normal;

    if (body1.GetType() == Body::Type::Dynamic)
    {
        body1.SetPosition(body1.GetPosition() + dist * m1);
#ifdef _DEBUG
        Engine.DebugRenderer().AddLine(DebugCategory::Physics, vec3(body1.GetPosition(), 0.15f),
                                       vec3(body1.GetPosition() + collision.normal, 0.15f), vec4(1, 0, 0, 0));
#endif
    }
    if (body2.GetType() == Body::Type::Dynamic)
    {
        body2.SetPosition(body2.GetPosition() - dist * m2);
#ifdef _DEBUG
        Engine.DebugRenderer().AddLine(DebugCategory::Physics, vec3(body2.GetPosition(), 0.15f),
                                       vec3(body2.GetPosition() - collision.normal, 0.15f), vec4(1, 0, 0, 0));
#endif
    }

    // compute and apply impulses
    float dotProduct = glm::dot(body1.GetLinearVelocity() - body2.GetLinearVelocity(), collision.normal);
    if (dotProduct <= 0)
    {
        float restitution = std::min(body1.GetRestitution(), body2.GetRestitution());
        float j = -(1 + restitution) * dotProduct / totalInvMass;
        const vec2& impulse = j * collision.normal;

        if (body1.GetType() == Body::Type::Dynamic) body1.ApplyImpulse(impulse);
        if (body2.GetType() == Body::Type::Dynamic) body2.ApplyImpulse(-impulse);
    }
}

void World::RegisterCollision(CollisionData& collision, const Entity& entity1, Body& body1, const Entity& entity2, Body& body2)
{
    // store references to the entities in the CollisionData object
    collision.entity1 = entity1;
    collision.entity2 = entity2;

    // store collision data in both bodies, also if they are kinematic (implying custom collision resolution)
    if (body1.GetType() != Body::Type::Static) body1.AddCollisionData(collision);
    if (body2.GetType() != Body::Type::Static)
        body2.AddCollisionData(CollisionData{collision.entity2, collision.entity1, -collision.normal, collision.depth});
}

void World::UpdateCollisionDetection()
{
    CollisionData collision;

    const auto& view_disk = Engine.ECS().Registry.view<Body, DiskCollider>();
    const auto& view_polygon = Engine.ECS().Registry.view<Body, PolygonCollider>();

    for (const auto& [entity1, body1, disk1] : view_disk.each())
    {
        if (body1.GetType() == Body::Type::Static) continue;

        // --- disk-disk collisions
        for (const auto& [entity2, body2, disk2] : view_disk.each())
        {
            if (entity1 == entity2) continue;

            // avoid duplicate collision checks
            if (body2.GetType() != Body::Type::Static && entity1 > entity2) continue;

            if (CollisionCheckDiskDisk(body1.GetPosition(), disk1.radius, body2.GetPosition(), disk2.radius, collision))
            {
                ResolveCollision(collision, body1, body2);
                RegisterCollision(collision, entity1, body1, entity2, body2);
            }
        }

        // --- disk-polygon collisions
        for (const auto& [entity2, body2, polygon2] : view_polygon.each())
        {
            if (CollisionCheckDiskPolygon(body1.GetPosition(), disk1.radius, body2.GetPosition(), polygon2.m_pts, collision))
            {
                ResolveCollision(collision, body1, body2);
                RegisterCollision(collision, entity1, body1, entity2, body2);
            }
        }
    }
}

void World::Update(float dt)
{
    const auto& view = Engine.ECS().Registry.view<Body>();

    // determine how many frames to simulate
    m_hasExecutedFrame = false;
    m_timeSinceLastFrame += dt;
    while (m_timeSinceLastFrame >= m_fixedDeltaTime)
    {
        // clear collision data from previous frame
        for (const auto& [entity, body] : view.each())
        {
            body.ClearCollisionData();
        }

        // apply gravity
        if (m_gravity != vec2(0, 0))
        {
            for (const auto& [entity, body] : view.each())
            {
                if (body.GetType() == Body::Type::Dynamic) body.AddForce(m_gravity);
            }
        }

        // update velocity and position
        for (const auto& [entity, body] : view.each())
        {
            if (body.GetType() != Body::Type::Static) body.Update(m_fixedDeltaTime);
        }

        // collision detection and resolution
        UpdateCollisionDetection();

        // reset data for next frame
        for (const auto& [entity, body] : view.each())
        {
            body.ClearForces();
        }

        // update internal timer
        m_timeSinceLastFrame -= m_fixedDeltaTime;

        m_hasExecutedFrame = true;
    }

// debug rendering of physics objects
#ifdef _DEBUG

    std::vector<vec4> typeColors = {vec4(0, 1, 0, 1), vec4(1, 0, 1, 1), vec4(1, 0, 0, 1)};

    for (auto entity : view)
    {
        const auto& body = view.get<Body>(entity);
        const vec4& color = typeColors[body.GetType()];

        PolygonCollider* p = Engine.ECS().Registry.try_get<PolygonCollider>(entity);
        if (p != nullptr)
        {
            size_t n = p->m_pts.size();
            for (size_t i = 0; i < n; ++i)
            {
                Engine.DebugRenderer().AddCircle(bee::DebugCategory::Physics, vec3(p->m_pts[i] + body.GetPosition(), 0.01f),
                                                    0.1f, color);
                Engine.DebugRenderer().AddLine(bee::DebugCategory::Physics, vec3(p->m_pts[i] + body.GetPosition(), 0.01f),
                                                vec3(p->m_pts[(i + 1) % n] + body.GetPosition(), 0.01f), color);
            }
        }

        DiskCollider* d = Engine.ECS().Registry.try_get<DiskCollider>(entity);
        if (d != nullptr)
        {
            Engine.DebugRenderer().AddCircle(bee::DebugCategory::Physics, vec3(body.GetPosition(), 0.01f), d->radius,
                                                color);
        }
    }

#endif

    // synchronize transforms with physics bodies. 
    // Note: For now, we simply snap to the last known physics position. You can still add interpolation if you want.
    for (const auto& [entity, body, transform] : Engine.ECS().Registry.view<Body, Transform>().each())
    {
        if (body.GetType() != Body::Type::Static)
        {
            transform.Translation.x = body.GetPosition().x;
            transform.Translation.y = body.GetPosition().y;
        }
    }
}

#ifdef BEE_INSPECTOR

void World::Inspect(bee::Entity entity)
{
    auto* body = Engine.ECS().Registry.try_get<Body>(entity);
    if (body)
    {
        if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Engine.Inspector().Inspect("Body Position", body->m_position);
            Engine.Inspector().Inspect("Inv Mass", body->m_invMass);
            Engine.Inspector().Inspect("Restitution", body->m_restitution);
        }
    }
}

#endif