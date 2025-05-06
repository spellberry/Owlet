#include "actors/buff_system.hpp"

#include "actors/selection_system.hpp"
#include "actors/structures/structure_manager_system.hpp"
#include "actors/structures/structure_template.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "ai/grid_navigation_system.hpp"
#include "core/engine.hpp"
#include "rendering/debug_render.hpp"
#include "tools/inspector.hpp"

BuffSystem::BuffSystem() { Title = "Buff System"; }

void BuffSystem::AddBuff(AttributesComponent& attributes, const BuffStructure& buffStructure)
{
    switch (buffStructure.buffType)
    {
        case BaseAttributes::Range:
            attributes.AddModifier(buffStructure.buffType, buffStructure.buffModifier.GetModifierType(),buffStructure.buffModifier.GetValue(), true);
            attributes.AddModifier(BaseAttributes::InterceptionRange, buffStructure.buffModifier.GetModifierType(),buffStructure.buffModifier.GetValue(), true);
            break;
        case BaseAttributes::HitPoints:
            attributes.AddModifier(buffStructure.buffType, buffStructure.buffModifier.GetModifierType(),
                                   buffStructure.buffModifier.GetValue(), true);
            break;
        case BaseAttributes::AttackCooldown:
            attributes.AddModifier(buffStructure.buffType, buffStructure.buffModifier.GetModifierType(),(-1)* buffStructure.buffModifier.GetValue(), true);
            break;
        default:
            attributes.AddModifier(buffStructure.buffType, buffStructure.buffModifier.GetModifierType(),buffStructure.buffModifier.GetValue(), true);
            break;
    }
}

void BuffSystem::RemoveBuff(AttributesComponent& attributes, const BuffStructure& buffStructure,const StatModifier& modifier)const
{
    switch (buffStructure.buffType)
    {
        case BaseAttributes::Range:
            attributes.RemoveModifier(buffStructure.buffType, modifier);
            attributes.RemoveModifier(BaseAttributes::InterceptionRange, ModifierType::Additive,modifier.GetValue(),true);
            break;
        case BaseAttributes::HitPoints:
            attributes.RemoveModifier(buffStructure.buffType, modifier);
            attributes.RemoveModifiersFromValue(BaseAttributes::HitPoints,buffStructure.buffModifier.GetValue() , false);
        break;
        case BaseAttributes::AttackCooldown:
            attributes.RemoveModifier(BaseAttributes::AttackCooldown, ModifierType::Additive,(-1) * buffStructure.buffModifier.GetValue(), true);
            break;
        default:
            attributes.RemoveModifier(buffStructure.buffType, modifier);
            break;
    }
}


void BuffSystem::HandleBuffAddition(entt::entity entity, BuffStructure& buffStructure)
{
    const auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::Transform, AttributesComponent>();
    auto& attributes = view.get<AttributesComponent>(entity);

    const auto& modifiers = attributes.GetModifiers(buffStructure.buffType);

    if (std::count_if(modifiers.begin(), modifiers.end(),[](const StatModifier& modifier)
    {
        return modifier.isBuff;
    }) > 0)
    {
        for (auto& modifier : modifiers)
        {
            const double value = modifier.GetValue();

            if (!modifier.isBuff) continue;
            if (value <= buffStructure.buffModifier.GetValue() &&buffStructure.buffModifier.GetModifierType() == modifier.GetModifierType())continue;

            AddBuff(attributes,buffStructure);
            RemoveBuff(attributes, buffStructure, modifier);
            attributes.RemoveModifier(buffStructure.buffType, modifier);

            buffStructure.buffedEntities.push_back(entity);
            return;
        }
    }
    else
    {
        AddBuff(attributes, buffStructure);
        buffStructure.buffedEntities.push_back(entity);
    }
}

void BuffSystem::RemoveBuff(entt::entity entity, BuffStructure& buffStructure) const
{
    const auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::Transform, AttributesComponent>();
    auto& attributes = view.get<AttributesComponent>(entity);

    for (auto& modifier : attributes.GetModifiers(buffStructure.buffType))
    {
        const double value = modifier.GetValue();
        if (value != buffStructure.buffModifier.GetValue()) continue;
        if (modifier.GetModifierType() != buffStructure.buffModifier.GetModifierType()) continue;

        RemoveBuff(attributes, buffStructure, modifier);
        return;
    }
}

void BuffSystem::HandleBuffs(BuffStructure& allyStructure, const AttributesComponent& attributes,
                                  const bee::Transform& structureTransform)
{
    float range = attributes.GetValue(BaseAttributes::BuffRange);
    const auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::Transform, AttributesComponent>();

    for (const auto entity : view)
    {
        auto& transform = view.get<bee::Transform>(entity);
        if (glm::distance2(transform.Translation, structureTransform.Translation) <= glm::pow(range, 2))
        {
            HandleBuffAddition(entity, allyStructure);
        }
    }

    for (const auto entity : allyStructure.buffedEntities)
    {
        if (!bee::Engine.ECS().Registry.valid(entity))
        {
            allyStructure.buffedEntities.erase(
                std::remove(allyStructure.buffedEntities.begin(), allyStructure.buffedEntities.end(), entity));
            continue;
        }

        auto& transform = view.get<bee::Transform>(entity);
        if (glm::distance2(transform.Translation, structureTransform.Translation) > glm::pow(range, 2))
        {
            RemoveBuff(entity, allyStructure);
        }
    }
}

void BuffSystem::Update(float dt)
{
    const auto alliedStructuresView = bee::Engine.ECS().Registry.view<BuffStructure, bee::Transform, AttributesComponent>();
    for (auto [structureEntity, allyStructure, structureTransform, attributes] : alliedStructuresView.each())
    {
        if (!attributes.HasAttribute(BaseAttributes::BuffRange) || !attributes.HasAttribute(BaseAttributes::BuffValue))
            continue;
        HandleBuffs(allyStructure, attributes, structureTransform);
    }
    //Debug code for HP
   /* if (bee::Engine.Input().GetKeyboardKeyOnce(bee::Input::KeyboardKey::Space))
    {
        auto view = bee::Engine.ECS().Registry.view<AllyUnit, AttributesComponent,Selected>();
        for (auto [entity,ally,attribute,selected]:view.each())
        {
            attribute.AddModifier(BaseAttributes::HitPoints, ModifierType::Additive, -1, false);
            break;
        }
    }*/
}
#ifdef BEE_INSPECTOR
void BuffSystem::Inspect()
{
    if (!m_seeAllRangeCircles)
    {
        // Draws a circle on the selected towers in order to show the buff range
        glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        const auto buffStructureView =
            bee::Engine.ECS().Registry.view<Selected, BuffStructure, AttributesComponent, bee::Transform>();
        for (auto [structureEntity, selected, buffStructure, structureAttributes, structureTransform] :
             buffStructureView.each())
        {
            bee::Engine.DebugRenderer().AddCircle(bee::DebugCategory::General, structureTransform.Translation,
                                                  structureAttributes.GetValue(BaseAttributes::BuffRange), color);
        }
    }
    ImGui::Begin("Buff System Debug Information");
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, AllyUnit, Selected>();
    for (auto [entity, attributes, ally, selected] : view.each())
    {
        std::string entityText = attributes.GetEntityType() + " :" + std::to_string(static_cast<int>(entity));
        ImGui::Text(entityText.c_str());
        for (auto attribute : attributes.GetAttributes())
        {
            entityText = magic_enum::enum_name(attribute.first);
            entityText = entityText + " " + std::to_string(attribute.second.GetValue());
            ImGui::Text(entityText.c_str());
        }
        ImGui::Separator();
    }
    ImGui::End();
}
#endif
