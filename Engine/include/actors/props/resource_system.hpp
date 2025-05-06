#pragma once
#include "actors/actor_utils.hpp"
#include "actors/props/resource_type.hpp"
#include "core/ecs.hpp"
#include "magic_enum/magic_enum.hpp"
#include "magic_enum/magic_enum_utility.hpp"
#include "user_interface/user_interface_structs.hpp"

struct ResourceData
{
    ResourceData()
    {
        magic_enum::enum_for_each<GameResourceType>([this](auto val) { resources.insert({val, 100}); });
    }

    std::unordered_map<GameResourceType, int> resources;
};

struct PropResourceComponent
{
    GameResourceType type;
    int resourceGain = 50;
    float resourceAliveTimer = 20;
    float baseVerticalPosition = 0.0f;
};

class ResourceSystem : public bee::System
{
public:
    ResourceSystem();
    void AddResource(GameResourceType type, int amount);
    bool CanAfford(GameResourceType type, int amount);
    void Spend(GameResourceType type, int amount);
    bool SpendIfCanAfford(GameResourceType type, int amount);
    void HandleCollectingResourcesUnit();
    void HandleCollectingResourcesMouse();
    void HandleDisappearingOfResources(float dt) const;
    void HandleVisualsOfResources(float dt) const;
    void Update(float dt) override;
    void ChangeResourceCount(GameResourceType type, int amount);
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif
    void SetCallback(std::function<void(GameResourceType type, int amount)> func) { eventAddresourceCallback = func; }
    std::function<void(GameResourceType type, int amount)> eventAddresourceCallback;
    ResourceData playerResourceData{};
private:
    float m_rotationDelta = 1.0f;
    float m_upDownFrequency = 5.0f;
    float m_upDownAmplitude = 0.3f;
    float m_fadingTime = 3.0f;
    bee::ui::UIElementID m_plusResource = entt::null;
    bee::ui::UIElementID m_plusTransparency = entt::null;
    const float m_timerUntilTextDissapears = 0.75f;
    float m_woodTimer = 0.0f;
    float m_stoneTimer = 0.0f;
    float m_woodPrevAmount = 0.0f;
    float m_stonePrevAmount = 0.0f;
};
