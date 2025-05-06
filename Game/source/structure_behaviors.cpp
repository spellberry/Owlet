#include "ai_behaviors/structure_behaviors.hpp"

#include "actors/buff_system.hpp"
#include "actors/projectile_system/projectile_system.hpp"
#include "actors/props/resource_system.hpp"
#include "actors/selection_system.hpp"
#include "actors/structures/structure_manager_system.hpp"
#include "core/audio.hpp"
#include "ai/Utils/editor_variables.hpp"
#include "ai/Utils/generic_factory.hpp"
#include "order/order_system.hpp"
#include "particle_system/particle_emitter.hpp"
#include "particle_system/particle_system.hpp"

void TrainUnitState::Initialize(bee::ai::StateMachineContext& context)
{
    const auto unitHandle = context.blackboard->GetData<std::string>("UnitToTrain");
    auto attributes = bee::Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(unitHandle).GetAttributes();
    auto timeReduction =
        bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::TimeReduction);
    context.blackboard->SetData("TrainTimer", attributes[BaseAttributes::CreationTime] - timeReduction);
    context.blackboard->SetData("isTraining", true);
}

void TrainUnitState::Update(bee::ai::StateMachineContext& context)
{
    const auto unitHandle = context.blackboard->GetData<std::string>("UnitToTrain");
    auto& timer = context.blackboard->GetData<double>("TrainTimer");
    const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
    auto timeReduction =
        attributes.GetValue(BaseAttributes::TimeReduction);
    auto myTeam = bee::Engine.ECS().Registry.try_get<AllyStructure>(context.entity) ? Team::Ally : Team::Enemy;
    timer -= context.deltaTime;

    if (timer <= 0 && context.blackboard->GetData<bool>("isTraining"))
    {
        auto& resourceSystem = bee::Engine.ECS().GetSystem<ResourceSystem>();
        if (myTeam == Team::Ally)
        {
            const auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
            const auto spawnedUnit = bee::Engine.ECS().GetSystem<UnitManager>().SpawnUnit(
                unitHandle, transform.Translation + glm::vec3(0, -2.0f, 0) * static_cast<float>(attributes.GetValue(BaseAttributes::DiskScale)), myTeam);
            if (!spawnedUnit.has_value()) return;
            const auto& spawnedUnitAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(spawnedUnit.value());
            const auto& spawnStructure = bee::Engine.ECS().Registry.get<SpawningStructure>(context.entity);
            spawnedUnitAgent.context.blackboard->SetData("IsMoving", true);
            spawnedUnitAgent.context.blackboard->SetData("PositionToMoveTo", static_cast<glm::vec2>(spawnStructure.rallyPoint));
        }

        auto attributes = bee::Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(unitHandle).GetAttributes();
        context.blackboard->SetData("TrainTimer", attributes[BaseAttributes::CreationTime] - timeReduction);
        auto& numUnits = context.blackboard->GetData<int>("NumUnits");

        //play spawn sound
        bee::Engine.Audio().PlaySoundW("audio/friend_spawn.wav", 1.5f, true);

        numUnits--;
    }
}

void BuildingDestroyedState::Initialize(bee::ai::StateMachineContext& context)
{
    auto structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
    auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::Transform, AttributesComponent>();

    const auto buffStructure = bee::Engine.ECS().Registry.try_get<BuffStructure>(context.entity);
    if (buffStructure)
    {
        for (const auto entity : buffStructure->buffedEntities)
        {
            if (!bee::Engine.ECS().Registry.valid(entity)) continue;
            auto& attributes = view.get<AttributesComponent>(entity);
            for (auto& modifier : attributes.GetModifiers(buffStructure->buffType))
            {
                const double value = modifier.GetValue();
                if (value != buffStructure->buffModifier.GetValue()) continue;
                if (modifier.GetModifierType() != buffStructure->buffModifier.GetModifierType()) continue;
                bee::Engine.ECS().GetSystem<BuffSystem>().RemoveBuff(attributes, *buffStructure, modifier);
            }
        }
    }
    bee::Engine.ECS().Registry.remove<Selected>(context.entity);

    bee::Engine.Audio().PlaySoundW("audio/rock_death2.wav",3.0f,true);
    const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
    auto& orderSystem = bee::Engine.ECS().GetSystem<OrderSystem>();

    if (attributes.HasAttribute(BaseAttributes::SwordsmenLimitIncrease))
    {
        orderSystem.swordsmenLimit -= attributes.GetValue(BaseAttributes::SwordsmenLimitIncrease);
    }

    if (attributes.HasAttribute(BaseAttributes::MageLimitIncrease))
    {
        orderSystem.mageLimit -= attributes.GetValue(BaseAttributes::MageLimitIncrease);
    }

    structureManager.RemoveStructure(context.entity);
}

void BeingBuilt::Initialize(bee::ai::StateMachineContext& context)
{
    context.blackboard->SetData("isBeingBuilt", true);
    context.blackboard->SetData("BuildingTimer", 2.0f);

    const auto particle = bee::Engine.ECS().CreateEntity();
    auto& particleTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(particle);
    auto& emitter = bee::Engine.ECS().CreateComponent<bee::ParticleEmitter>(particle);

    auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);

    emitter.AssignEntity(particle);
    const auto positionToPlaceBuilding = context.blackboard->GetData<glm::vec3>("PositionToPlaceBuilding");
    particleTransform.Translation = positionToPlaceBuilding;
    particleTransform.Scale*= attributes.GetValue(BaseAttributes::DiskScale);
    bee::Engine.ECS().GetSystem<bee::ParticleSystem>().LoadEmitterFromTemplate(emitter, "effects/DustParticles.pepitter");

    context.blackboard->SetData("ParticleEntity", particle);
    emitter.Emit();
}

void BeingBuilt::Build(const bee::ai::StateMachineContext& context)
{
    if (context.blackboard->GetData<bool>("isBeingBuilt"))
    {
        auto& timer = context.blackboard->GetData<float>("BuildingTimer");
        auto buildingEntity = context.entity;
        const auto buildingObject = buildingEntity;
        auto& buildingTransform = bee::Engine.ECS().Registry.get<bee::Transform>(buildingObject);
        const auto positionToPlaceBuilding = context.blackboard->GetData<glm::vec3>("PositionToPlaceBuilding");

        buildingTransform.Translation.z =
            glm::mix(positionToPlaceBuilding.z - 30.0f, positionToPlaceBuilding.z, (10.0f - timer) / 10.0f);

        bee::Engine.ECS().Registry.remove<bee::physics::DiskCollider>(buildingEntity);

        timer -= context.deltaTime;
        if (timer <= 0)
        {
            auto& stateMachineAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(buildingEntity);
            stateMachineAgent.active = true;
            if (bee::Engine.ECS().Registry.valid(buildingEntity))
            {
                bee::Engine.ECS().CreateComponent<bee::physics::DiskCollider>(
                    buildingEntity,
                    bee::Engine.ECS().Registry.get<AttributesComponent>(buildingEntity).GetValue(BaseAttributes::DiskScale));
            }

            bee::Engine.ECS().Registry.get<bee::ParticleEmitter>(context.blackboard->GetData<bee::Entity>("ParticleEntity")).SetLoop(false);
            bee::Engine.ECS().DeleteEntity(context.blackboard->GetData<bee::Entity>("ParticleEntity"));
            stateMachineAgent.SetStateOfType<BuildingIdle>();
            context.blackboard->SetData("isBeingBuilt", false);

        }
    }
}

void BeingBuilt::End(bee::ai::StateMachineContext& context)
{
    State::End(context);
}

