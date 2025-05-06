#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "core/fwd.hpp"

#if defined(PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 4267)
#include <entt/entity/registry.hpp>
#pragma warning(pop)
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <entt/entity/registry.hpp>
#pragma clang diagnostic pop
#endif

namespace bee
{
struct Delete
{
};  // Tag component for entities to be deleted
class System
{
public:
    virtual ~System() = default;
    virtual void Update(float dt) {}
    virtual void Render() {}
    int Priority = 0;
    std::string Title = {};

    void Pause(bool pause)
    {
        if (m_paused) printf(std::string(Title + "\n").c_str());
        m_paused = m_isPausable ? pause : false;
    }
    bool IsPause()
    {
        return m_paused;
    }
    void SetIsPausable(bool isPausable) { m_isPausable = isPausable; }
#ifdef BEE_INSPECTOR
    virtual void Inspect() {}
    virtual void Inspect(Entity e) {}
#endif
private:
    bool m_paused = false;
    bool m_isPausable = true;
};

class ThreadSafeEntityFactory
{
public:
    ThreadSafeEntityFactory(const std::function<bee::Entity()>& creator) : m_creatorFunction(creator){};
    bee::Entity CreateEntity()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_creatorFunction();
    }

private:
    std::function<bee::Entity()> m_creatorFunction;
    std::mutex m_mutex;
};

class EntityComponentSystem
{
public:
    entt::registry Registry;
    Entity CreateEntity() { return Registry.create(); }
    void DeleteEntity(Entity e);
    void UpdateSystems(float dt);
    void RenderSystems();
    void RemovedDeleted();
    template <typename T, typename... Args>
    decltype(auto) CreateComponent(Entity entity, Args&&... args);
    template <typename T, typename... Args>
    T& CreateSystem(Args&&... args);
    template <typename T>
    T& GetSystem();
    template <typename T>
    std::vector<T*> GetSystems();
    template <typename T>
    void RemoveSystem(T& system);
    template <typename T>
    void RemoveSystems();
    void RemoveSystems(std::vector<std::unique_ptr<System>>::iterator start,
                       std::vector<std::unique_ptr<System>>::iterator end);
    template <typename T>
    void RemoveAllSystemAfter();

private:
    friend class EngineClass;
    EntityComponentSystem();
    ~EntityComponentSystem();
    EntityComponentSystem(const EntityComponentSystem&) = delete;             // non construction-copyable
    EntityComponentSystem& operator=(const EntityComponentSystem&) = delete;  // non copyable

    std::vector<std::unique_ptr<System>> m_systems;
};

template <typename T, typename... Args>
decltype(auto) EntityComponentSystem::CreateComponent(Entity entity, Args&&... args)
{
    return Registry.emplace<T>(entity, args...);  // TODO: std::move this
}

template <typename T, typename... Args>
T& EntityComponentSystem::CreateSystem(Args&&... args)
{
    T* system = new T(std::forward<Args>(args)...);
    m_systems.push_back(std::unique_ptr<System>(system));
    std::sort(m_systems.begin(), m_systems.end(),
              [](const std::unique_ptr<System>& sl, const std::unique_ptr<System>& sr) { return sl->Priority > sr->Priority; });
    return *system;
}

template <typename T>
T& EntityComponentSystem::GetSystem()
{
    for (auto& s : m_systems)
    {
        T* found = dynamic_cast<T*>(s.get());
        if (found) return *found;
    }
    assert(false);
    return *dynamic_cast<T*>(m_systems[0].get());  // This line will always fail
}

template <typename T>
std::vector<T*> EntityComponentSystem::GetSystems()
{
    std::vector<T*> systems;
    for (auto& s : m_systems)
    {
        if (T* found = dynamic_cast<T*>(s.get())) systems.push_back(found);
    }
    return systems;
}

template <typename T>
void EntityComponentSystem::RemoveSystem(T& system)
{
    /*m_systems.erase(std::remove(m_systems.begin(), m_systems.end(), system), m_systems.end());*/
}

template <typename T>
void EntityComponentSystem::RemoveSystems()
{
    std::vector<std::unique_ptr<System>>::iterator it = m_systems.begin();

    // Deleting element from list while iterating
    while (it != m_systems.end())
    {
        if (T* found = dynamic_cast<T*>(it->get()))
        {
            it = m_systems.erase(it);
            continue;
        }
        it++;
    }
}

template <typename T>
inline void EntityComponentSystem::RemoveAllSystemAfter()
{
    std::vector<std::unique_ptr<System>>::iterator it = m_systems.begin();

    // Deleting element from list while iterating
    while (it != m_systems.end())
    {
        if (T* found = dynamic_cast<T*>(it->get()))
        {
            break;
        }
        it++;
    }
    if (it!=m_systems.end())
    {
        m_systems.erase(it, m_systems.end());
    }
}

}  // namespace bee
