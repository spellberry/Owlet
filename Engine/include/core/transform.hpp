#pragma once

#include <cassert>
#include <cereal/cereal.hpp>
#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

#include "core/fwd.hpp"

namespace bee
{
/// <summary>
/// Transform component. Contains the position, rotation and scale of the entity.
/// Implemented on top of the entity-component-system (entt).
/// </summary>
struct Transform
{
    /// <summary>
    /// Translation in local space.
    /// </summary>
    glm::vec3 Translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 TranslationWorld = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4 WorldMatrix = glm::mat4(1.0f);

    /// <summary>
    /// Scale in local space.
    /// </summary>
    glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

    /// <summary>
    /// Rotation in local space.
    /// </summary>
    glm::quat Rotation = glm::identity<glm::quat>();

    /// <summary>
    /// The name of the entity. Used for debugging and editor purposes.
    /// If small, it can benefit from the small string optimization.
    /// </summary>
    std::string Name = {};

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(Translation), CEREAL_NVP(Scale), CEREAL_NVP(Rotation), CEREAL_NVP(Name));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(Translation), CEREAL_NVP(Scale), CEREAL_NVP(Rotation), CEREAL_NVP(Name));
    }

    entt::entity GetParent() const { return m_parent; }

    /// <summary>
    /// Creates a Transform with default translation (0,0,0), scale (1,1,1), and rotation (identity quaternion).
    /// </summary>
    Transform() {}

    /// <summary>
    /// Creates a Transform with the given translation, scale, and rotation.
    /// </summary>
    Transform(const glm::vec3& translation, const glm::vec3& scale, const glm::quat& rotation)
        : Translation(translation), Scale(scale), Rotation(rotation)
    {
    }

    entt::entity GetNextChild() const { return m_next; }

    /// <summary>
    /// Sets the parent of the entity. Will automatically add the entity to
    /// the parent's children.
    /// </summary>
    /// <param name="parent">The parent entity.</param>
    void SetParent(Entity parent);

    /// <summary>
    /// The matrix that transforms from local space to world space.
    /// </summary>
    [[nodiscard]] glm::mat4 World() const;


    /// <summary>
    /// Position in world space.
    /// </summary>
    [[nodiscard]] glm::vec3 WorldTranslation() const;

    /// <summary>True if the entity has a children.</summary>
    [[nodiscard]] bool HasChildern() const { return m_first != entt::null; }

    /// <summary>True if the entity has a parent.</summary>
    [[nodiscard]] bool HasParent() const { return m_parent != entt::null; }

    // <summary> The first child of the entity. </summary>
    [[nodiscard]] Entity FirstChild() const { return m_first; }

    /// <summary>
    /// Tells the entity it has no children. This can potentially cause you to lose access to the children.
    /// The intended use case is when the children of an entity are deleted, but it still thinks it has them.
    /// </summary>
    void NoChildren() { m_first = entt::null; }

    /// <summary>Subscribe to the events from the ECS. Call this from the engine init.</summary>
    static void SubscribeToEvents();

    /// <summary>Un-subscribe to the events from the ECS. Call this from the engine shutdown.</summary>
    static void UnsubscribeToEvents();

    void RemoveChild(Entity child);

private:
    // The hierarchy is implemented as a linked list.
    entt::entity m_parent{entt::null};
    entt::entity m_first{entt::null};
    entt::entity m_next{entt::null};

    // Add a child to the entity. Called by SetParent.
    void AddChild(Entity child);

    static void OnTransformCreate(entt::registry& registry, entt::entity entity);
    static void OnTransformDestroy(entt::registry& registry, entt::entity entity);

public:
    /// <summary>
    /// Iterator for the children of the entity.
    /// </summary>
    class Iterator
    {
    public:
        Iterator() = default;
        Iterator(entt::entity ent);
        Iterator& operator++();
        bool operator!=(const Iterator& iterator);
        Entity operator*();

    private:
        entt::entity m_current{entt::null};
    };

    /// <summary>
    /// The begin iterator for the children of the entity.
    /// </summary>
    Iterator begin() { return Iterator(m_first); }

    /// <summary>
    /// The end iterator for the children of the entity.
    /// </summary>
    Iterator end() { return Iterator(); }
};

/// Decomposes a transform matrix into its translation, scale and rotation components
void Decompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& scale, glm::quat& rotation);

}  // namespace bee
