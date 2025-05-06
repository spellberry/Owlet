#include "core/transform.hpp"

#include <entt/entity/helper.hpp>

#include "core/ecs.hpp"
#include "core/engine.hpp"

using namespace bee;
using namespace glm;

void Transform::SetParent(Entity parent)
{
    assert(Engine.ECS().Registry.valid(parent));
    auto& parentTransform = Engine.ECS().Registry.get<Transform>(parent);
    // We can always get the entity from the transform.
    const Entity entity = to_entity(Engine.ECS().Registry, *this);
    parentTransform.AddChild(entity);
    m_parent = parent;
}

void Transform::AddChild(Entity child)
{
    assert(Engine.ECS().Registry.valid(child));
    if (m_first == entt::null)
    {
        m_first = child;
    }
    else
    {
        auto itr = m_first;
        while (true)
        {
            auto& t = Engine.ECS().Registry.get<Transform>(itr);
            if (t.m_next == entt::null)
            {
                t.m_next = child;
                break;
            }
            itr = t.m_next;
        }
    }
}

void Transform::OnTransformCreate(entt::registry& registry, entt::entity entity)
{
}

void Transform::OnTransformDestroy(entt::registry& registry, entt::entity entity)
{
    // Delete all children of the entity.
    // Note: Functionality of this function was moved in DeleteEntity() in ecs.hpp
}

void bee::Decompose(const mat4& transform, vec3& translation, vec3& scale, quat& rotation)
{
    auto m44 = transform;
    translation.x = m44[3][0];
    translation.y = m44[3][1];
    translation.z = m44[3][2];

    scale.x = length(vec3(m44[0][0], m44[0][1], m44[0][2]));
    scale.y = length(vec3(m44[1][0], m44[1][1], m44[1][2]));
    scale.z = length(vec3(m44[2][0], m44[2][1], m44[2][2]));

    mat4 myrot(m44[0][0] / scale.x, m44[0][1] / scale.x, m44[0][2] / scale.x, 0, m44[1][0] / scale.y, m44[1][1] / scale.y,
               m44[1][2] / scale.y, 0, m44[2][0] / scale.z, m44[2][1] / scale.z, m44[2][2] / scale.z, 0, 0, 0, 0, 1);
    rotation = quat_cast(myrot);
}

// Iterator implementation
Transform::Iterator::Iterator(entt::entity ent) : m_current(ent) {}

// Iterator implementation
Transform::Iterator& Transform::Iterator::operator++()
{
    assert(Engine.ECS().Registry.valid(m_current));
    auto& t = Engine.ECS().Registry.get<Transform>(m_current);
    m_current = t.m_next;
    return *this;
}

// Iterator implementation
bool Transform::Iterator::operator!=(const Iterator& iterator) { return m_current != iterator.m_current; }

// Iterator implementation
Entity Transform::Iterator::operator*() { return m_current; }

// Transform implementation
inline glm::mat4 Transform::World() const
{
    const auto translation = glm::translate(glm::mat4(1.0f), Translation);
    const auto rotation = glm::toMat4(Rotation);
    const auto scale = glm::scale(glm::mat4(1.0f), Scale);
    if (m_parent == entt::null) return translation * rotation * scale;
    assert(Engine.ECS().Registry.valid(m_parent));
    const auto& parent = Engine.ECS().Registry.get<Transform>(m_parent);
    return parent.World() * translation * rotation * scale;
}

glm::vec3 Transform::WorldTranslation() const
{
    const glm::mat4 worldMatrix = World();
    return glm::vec3(worldMatrix[3][0], worldMatrix[3][1], worldMatrix[3][2]);
}

void bee::Transform::SubscribeToEvents()
{
    // We subscribe to the creation and destruction of the Transform component.
    Engine.ECS().Registry.on_construct<Transform>().connect<&Transform::OnTransformCreate>();
    Engine.ECS().Registry.on_destroy<Transform>().connect<&Transform::OnTransformDestroy>();
}

void bee::Transform::UnsubscribeToEvents()
{
    // Un-subscribe to the events of the ECS.
    Engine.ECS().Registry.on_construct<Transform>().disconnect<&Transform::OnTransformCreate>();
    Engine.ECS().Registry.on_destroy<Transform>().disconnect<&Transform::OnTransformDestroy>();
}

void Transform::RemoveChild(Entity child)
{
    assert(Engine.ECS().Registry.valid(child));

    if (m_first == entt::null)
    {
        // No children to remove
        return;
    }

    if (m_first == child)
    {
        // If the child to remove is the first child, update the m_first pointer
        m_first = Engine.ECS().Registry.get<Transform>(m_first).m_next;
    }
    else
    {
        // Traverse the list to find the child to remove
        auto itr = m_first;
        while (itr != entt::null)
        {
            auto& t = Engine.ECS().Registry.get<Transform>(itr);
            if (t.m_next == child)
            {
                // Bypass the child to remove it
                t.m_next = Engine.ECS().Registry.get<Transform>(child).m_next;
                break;
            }
            itr = t.m_next;
        }
    }
}
