#pragma once
#include "actors/attributes.hpp"
#include "actors/structures/structure_manager_system.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"


class BuffSystem : public bee::System
{
public:
    BuffSystem();
    void AddBuff(AttributesComponent& attributes, const BuffStructure& buffStructure);
    void RemoveBuff(AttributesComponent& attributes, const BuffStructure& buffStructure, const StatModifier& modifier)const;
    void HandleBuffAddition(entt::entity entity, BuffStructure& buffStructure);
    void RemoveBuff(entt::entity entity, BuffStructure& buffStructure) const;
    void HandleBuffs(BuffStructure& allyStructure, const AttributesComponent& attributes,
                     const bee::Transform& structureTransform);
    void Update(float dt) override;
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif
    bool m_seeAllRangeCircles = false;
};
