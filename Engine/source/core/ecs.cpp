#include "core/ecs.hpp"

#include "core/transform.hpp"

using namespace bee;
using namespace std;
using namespace entt;

constexpr float kMaxDeltaTime = 1.0f / 30.0f;

void bee::EntityComponentSystem::RemoveSystems(const std::vector<std::unique_ptr<System>>::iterator start,
                                               const std::vector<std::unique_ptr<System>>::iterator end)
{
    m_systems.erase(start, end);
}

EntityComponentSystem::EntityComponentSystem() = default;

bee::EntityComponentSystem::~EntityComponentSystem() = default;

void EntityComponentSystem::DeleteEntity(Entity e)
{
    // mark entity for deletion
    assert(Registry.valid(e));
    Registry.emplace_or_replace<Delete>(e);

    // mark all of its children for deletion
    auto* transform = Registry.try_get<Transform>(e);
    if (transform != nullptr)
    {
        for (auto child : *transform)
        {
            DeleteEntity(child);
        }
    }
}

void EntityComponentSystem::UpdateSystems(float dt)
{
    dt = min(dt, kMaxDeltaTime);
    for (int i = 0; i < m_systems.size(); i++)
    {
        if (!m_systems.at(i).get()->IsPause())
        m_systems.at(i).get()->Update(dt);
    }
}

void EntityComponentSystem::RenderSystems()
{
    for (auto& s : m_systems) s->Render();
}

void EntityComponentSystem::RemovedDeleted()
{
    bool isDeleteQueueEmpty;
    do
    {
        // Deleting entities can cause other entities to be deleted,
        // so we need to do this in a loop
        const auto del = Registry.view<Delete>();
        Registry.destroy(del.begin(), del.end());
        isDeleteQueueEmpty = del.empty();
    } while (!isDeleteQueueEmpty);
}
