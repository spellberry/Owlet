#include "actors/props/resource_system.hpp"

#include "actors/units/unit_manager_system.hpp"
#include "core/engine.hpp"
#include "physics/world.hpp"
#include "physics/physics_components.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_serializer.hpp"

ResourceSystem::ResourceSystem()
{
    Title = "ResourceSystem";
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    m_plusResource = UI.serialiser->LoadElement("CollectedResources");
    m_plusTransparency = UI.serialiser->LoadElement("CollectResourceTransparency");
    UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusWood"), "");
    UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusStone"), "");
}

void ResourceSystem::ChangeResourceCount(GameResourceType type, int amount)
{
    if (amount > 0.0f)
    {
        auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
        if (type == GameResourceType::Wood)
        {
            if (m_woodTimer > 0.0f){
                UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusWood"), "+" + std::to_string(static_cast<int>(m_woodPrevAmount+amount)));
                m_woodPrevAmount += amount;
            }
            else
            {
                UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusWood"), "+" + std::to_string(static_cast<int>(amount)));
                m_woodPrevAmount = amount;
            }
            m_woodTimer = m_timerUntilTextDissapears;
        }
        else
        {
            if (m_stoneTimer > 0.0f)
            {
                UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusStone"),
                                 "+" + std::to_string(static_cast<int>(m_stonePrevAmount + amount)));
                m_stonePrevAmount += amount;
            }
            else
            {
                UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusStone"), "+" + std::to_string(static_cast<int>(amount)));
                m_woodPrevAmount = amount;
            }
            m_stoneTimer = m_timerUntilTextDissapears;
        }
    }
    eventAddresourceCallback(type, amount);
}

void ResourceSystem::AddResource(GameResourceType type, int amount) 
{
    playerResourceData.resources[type] =
        std::clamp(playerResourceData.resources[type] + amount, 0, std::numeric_limits<int>::max());
    ChangeResourceCount(type, amount);
}

bool ResourceSystem::CanAfford(GameResourceType type, int amount)
{
    return playerResourceData.resources[type] >= amount;
}

void ResourceSystem::Spend(GameResourceType type, int amount)
{
    playerResourceData.resources[type] -= amount;
    ChangeResourceCount(type, -amount);
}

bool ResourceSystem::SpendIfCanAfford(GameResourceType type, int amount)
{
    if (CanAfford(type, amount))
    {
        Spend(type, amount);
        return true;
    }
    return false;
}

void ResourceSystem::HandleCollectingResourcesUnit()
{
    const auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::physics::Body>();
    const auto resourceView = bee::Engine.ECS().Registry.view<PropResourceComponent>();

    for (const auto entity : view)
    {
        auto& body = view.get<bee::physics::Body>(entity);
        if (!bee::Engine.ECS().GetSystem<bee::physics::World>().HasExecutedFrame()) break;
        for (auto& collisionData : body.GetCollisionData())
        {
            const auto entity1 = collisionData.entity1;
            const auto entity2 = collisionData.entity2;

            if (!bee::Engine.ECS().Registry.valid(entity1) || !bee::Engine.ECS().Registry.valid(entity2)) continue;
            if (resourceView.contains(entity1))
            {
                const auto& resource = resourceView.get<PropResourceComponent>(entity1);
                AddResource(resource.type, resource.resourceGain);
                bee::Engine.ECS().DeleteEntity(entity1);
            }

            if (resourceView.contains(entity2))
            {
                const auto& resource = resourceView.get<PropResourceComponent>(entity2);
                AddResource(resource.type, resource.resourceGain);
                bee::Engine.ECS().DeleteEntity(entity2);
            }
        }
    }
}

void ResourceSystem::HandleCollectingResourcesMouse()
{
    bee::Entity targetEntity{};
    if (!bee::MouseHitResponse(targetEntity)) return;
    PropResourceComponent* resource = bee::Engine.ECS().Registry.try_get<PropResourceComponent>(targetEntity);
    if (!resource) return;
    AddResource(resource->type, resource->resourceGain);
    bee::Engine.ECS().DeleteEntity(targetEntity);

}

void ResourceSystem::HandleDisappearingOfResources(float dt) const
{
    const auto resourceView = bee::Engine.ECS().Registry.view<PropResourceComponent>();

    for (const auto entity : resourceView)
    {
        auto& resource = resourceView.get<PropResourceComponent>(entity);
        resource.resourceAliveTimer -= dt;

        if (resource.resourceAliveTimer <= m_fadingTime)
        {
            auto& renderer = bee::GetComponentInChildren<bee::MeshRenderer>(entity);
            renderer.constant_data.opacity = resource.resourceAliveTimer / m_fadingTime;
        }

        if (resource.resourceAliveTimer <= 0.0f)
        {
            bee::Engine.ECS().DeleteEntity(entity);
        }
    }
}

void ResourceSystem::HandleVisualsOfResources(float dt) const
{
    const auto resourceView = bee::Engine.ECS().Registry.view<PropResourceComponent, bee::Transform>();

    for (const auto entity : resourceView)
    {
        auto& transform = resourceView.get<bee::Transform>(entity);
        auto& resource = resourceView.get<PropResourceComponent>(entity);

        float angle = m_rotationDelta * dt;
        glm::quat incrementalRotation = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat newOrientation = incrementalRotation * transform.Rotation;
        newOrientation = glm::normalize(newOrientation);
        transform.Rotation = newOrientation;

        const float verticalOffset =
            m_upDownAmplitude * sin(m_upDownFrequency * resource.resourceAliveTimer) + m_upDownAmplitude;

        transform.Translation.z = resource.baseVerticalPosition + verticalOffset;
    }
}

void ResourceSystem::Update(float dt)
{
    HandleVisualsOfResources(dt);
    HandleDisappearingOfResources(dt);

    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    if (m_woodTimer <= 0.0f)
    {
        UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusWood"), "");
    }
    if (m_stoneTimer <= 0.0f)
    {
        UI.ReplaceString(UI.GetComponentID(m_plusResource, "PlusStone"), "");
    }

    m_woodTimer -= dt;
    m_stoneTimer -= dt;
}

#ifdef BEE_INSPECTOR
void ResourceSystem::Inspect()
{
    ImGui::Begin("ResourceSystem");

    ImGui::Separator();
    ImGui::Text("ally resources:");

    auto playerResourceDataLoc = playerResourceData;
    magic_enum::enum_for_each<GameResourceType>(
        [&playerResourceDataLoc](auto val)
        {
            constexpr GameResourceType resource = val;
            if (resource != GameResourceType::None)
            {
                const std::string name = std::string(magic_enum::enum_name(val()));
                ImGui::Text(name.c_str());
                const std::string intname = "##int" + name;
                int newval = playerResourceDataLoc.resources.at(resource);
                ImGui::InputInt(intname.c_str(), &newval);
                const int change = newval - playerResourceDataLoc.resources.at(resource);
                if (change != 0) bee::Engine.ECS().GetSystem<ResourceSystem>().AddResource(resource, change);
            }
        });

    ImGui::End();
}
#endif