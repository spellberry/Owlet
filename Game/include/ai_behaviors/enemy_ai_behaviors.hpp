#pragma once
#include "actors/props/resource_system.hpp"
#include "actors/props/resource_type.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/Utils/editor_variables.hpp"
#include "ai_behaviors/unit_behaviors.hpp"
#include "core/ecs.hpp"
#include "structure_behaviors.hpp"
#include "tools/tools.hpp"

class TrainUnitEnemyAIState;
class PatrolOrderEnemyAI;
class ScoutOrderEnemyAI;
class BuildOrderEnemyAI;
class GatherOrderEnemyAI;
class AttackOrderEnemyAI;

class IdleEnemyAIState : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        if (!context.blackboard->HasKey<int>("ActionIndex"))
        {
            context.blackboard->SetData("ActionIndex", -1);
        }
    };
    void Update(bee::ai::StateMachineContext& context) override
    {
        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);
        agent.SetStateOfType<AttackOrderEnemyAI>();
    };
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(IdleEnemyAIState);

class GatherOrderEnemyAI : public bee::ai::State
{
public:
    bee::Entity GetClosestResourceOfType(GameResourceType minerals, glm::vec3 position) const
    {
        bee::Entity toReturn{};
        const auto view = bee::Engine.ECS().Registry.view<PropResourceComponent, bee::Transform>();
        float minDist = std::numeric_limits<float>::max();

        for (const auto entity : view)
        {
            auto& transform = view.get<bee::Transform>(entity);
            const float distance2 = glm::distance2(position, transform.Translation);
            if (minDist <= distance2) continue;
            minDist = distance2;
            toReturn = entity;
        }

        return toReturn;
    };

    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto view = bee::Engine.ECS().Registry.view<EnemyUnit, AttributesComponent, bee::Transform>();
        auto& aiAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);
        auto& unitSystem = bee::Engine.ECS().GetSystem<UnitManager>();

        if (bee::Engine.ECS().Registry.view<PropResourceComponent>().size() == 0)
        {
            aiAgent.SetStateOfType<IdleEnemyAIState>();
            return;
        }

        for (const auto element : view)
        {
            auto& attributes = view.get<AttributesComponent>(element);
            auto& orders = unitSystem.GetUnitTemplate(attributes.GetEntityType()).availableOrders;
            const auto& transform = view.get<bee::Transform>(element);
            if(/*std::find(orders.begin(),orders.end(),OrderType::Gather) != orders.end()*/false)
            {
                auto& stateMachineAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(element);

                //if (stateMachineAgent.IsInState<PlaceBuilding>()) continue;
                if (stateMachineAgent.IsInState<DeadState>()) continue;

                const auto targetEntity = GetClosestResourceOfType(GameResourceType::Wood,transform.Translation);


                stateMachineAgent.context.blackboard->SetData<bee::Entity>("TargetResourceEntity", targetEntity);
            }
            aiAgent.SetStateOfType<IdleEnemyAIState>();
        }
    };
    void Update(bee::ai::StateMachineContext& context) override
    {
        auto& aiAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);
        aiAgent.SetStateOfType<IdleEnemyAIState>();
    }
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(GatherOrderEnemyAI);

class AttackOrderEnemyAI : public bee::ai::State
{
public:
    bee::Entity GetClosestPlayerUnit(glm::vec3 position) const
    {
        bee::Entity toReturn{};
        const auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::Transform>();
        float minDist = std::numeric_limits<float>::max();

        for (const auto entity : view)
        {
            auto& transform = view.get<bee::Transform>(entity);
            const float distance2 = glm::distance2(position, transform.Translation);
            if (minDist <= distance2) continue;
            minDist = distance2;
            toReturn = entity;
        }

        return toReturn;
    }

    bee::Entity GetClosestPlayerBuilding(const glm::vec3& position) const
    {
        bee::Entity toReturn{};
        const auto view = bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform>();
        float minDist = std::numeric_limits<float>::max();

        for (const auto entity : view)
        {
            auto& transform = view.get<bee::Transform>(entity);
            const float distance2 = glm::distance2(position, transform.Translation);
            if (minDist <= distance2) continue;
            minDist = distance2;
            toReturn = entity;
        }

        return toReturn;
    }

    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto view = bee::Engine.ECS().Registry.view<EnemyUnit, AttributesComponent, bee::Transform>();

        for (const auto element : view)
        {
            auto& stateMachineAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(element);
            const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(element);

            const auto closestPlayerUnit = GetClosestPlayerUnit(unitTransform.Translation);
            const auto closestPlayerBuilding = GetClosestPlayerBuilding(unitTransform.Translation);

            const auto& playerUnitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(closestPlayerUnit);
            const auto& playerBuildingTransform = bee::Engine.ECS().Registry.get<bee::Transform>(closestPlayerBuilding);
            const auto playerUnitPosition = playerUnitTransform.Translation;
            const auto playerBuildingPosition = playerBuildingTransform.Translation;

            glm::vec2 targetPosition{};

            if (glm::distance2(playerBuildingPosition, unitTransform.Translation) >
                glm::distance2(playerUnitPosition, unitTransform.Translation))
            {
                targetPosition = glm::vec2(playerUnitPosition.x, playerUnitPosition.y);
            }
            else
            {
                targetPosition = glm::vec2(playerBuildingPosition.x, playerBuildingPosition.y);
            }

            targetPosition =
                bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().GetGrid().SampleWalkablePoint(targetPosition);

            if (stateMachineAgent.IsInState<DeadState>()) continue;
            if (stateMachineAgent.IsInState<OffensiveMove>()) continue;
            if (stateMachineAgent.IsInState<RangedAttackState>()) continue;
            if (stateMachineAgent.IsInState<MeleeAttackState>()) continue;
            //if (stateMachineAgent.IsInState<PlaceBuilding>()) continue;

            stateMachineAgent.context.blackboard->SetData<glm::vec2>("PositionToMoveTo", targetPosition);
            stateMachineAgent.SetStateOfType<OffensiveMove>();
        }

        auto& aiAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);
        aiAgent.SetStateOfType<IdleEnemyAIState>();
    };
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(AttackOrderEnemyAI);

class BuildOrderEnemyAI : public bee::ai::State
{
public:
    SERIALIZE_FIELD(OrderType, buildOrderType);
    SERIALIZE_FIELD(float, coolDown);
    SERIALIZE_FIELD(float, chanceForBuild);
    SERIALIZE_FIELD(float, maxDistance);
    SERIALIZE_FIELD(std::string, buildingToBuild);

    void Initialize(bee::ai::StateMachineContext& context) override { context.blackboard->SetData("BuildTimer", coolDown); };
    void Update(bee::ai::StateMachineContext& context) override
    {
        auto& coolDownTimer = context.blackboard->GetData<float>("BuildTimer");
        coolDownTimer -= context.deltaTime;

        if (coolDownTimer <= 0.0f)
        {
            const float randomFloat = bee::GetRandomNumber(0.0f, 1.0f);
            auto& aiAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);

            if (randomFloat > chanceForBuild)
            {
                aiAgent.SetStateOfType<IdleEnemyAIState>();
                return;
            }

            const auto unitView = bee::Engine.ECS().Registry.view<EnemyUnit, AttributesComponent>();
            auto& unitManager = bee::Engine.ECS().GetSystem<UnitManager>();

            for (const auto unit : unitView)
            {
                auto& attributes = unitView.get<AttributesComponent>(unit);
                auto& orders = unitManager.GetUnitTemplate(attributes.GetEntityType()).availableOrders;
                auto& stateMachineAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(unit);
                if (std::find(orders.begin(), orders.end(), buildOrderType) != orders.end())
                {
                    const auto& workerTransform = bee::Engine.ECS().Registry.get<bee::Transform>(unit);
                    const float randomX = bee::GetRandomNumber(0.0f, 1.0f);
                    const float randomY = bee::GetRandomNumber(0.0f, 1.0f);
                    const auto randomPoint =
                        workerTransform.Translation +
                        glm::normalize(glm::vec3(randomX, randomY, 0) * bee::GetRandomNumber(0.5f, 1.0f) * maxDistance);

                    if (stateMachineAgent.IsInState<DeadState>()) continue;

                    stateMachineAgent.context.blackboard->SetData<std::string>("BuildingToPlace", buildingToBuild);
                    stateMachineAgent.context.blackboard->SetData<glm::vec3>("PositionToPlaceBuilding", randomPoint);
                    //stateMachineAgent.SetStateOfType<PlaceBuilding>();
                }
            }
            aiAgent.SetStateOfType<IdleEnemyAIState>();
        }
    };
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(BuildOrderEnemyAI);

class PatrolOrderEnemyAI : public bee::ai::State
{
public:
    SERIALIZE_FIELD(float, coolDown);
    SERIALIZE_FIELD(float, patrolChance);
    SERIALIZE_FIELD(float, maxDistance);
    void Initialize(bee::ai::StateMachineContext& context) override { context.blackboard->SetData("PatrolTimer", coolDown); };
    void Update(bee::ai::StateMachineContext& context) override
    {
        auto& coolDownTimer = context.blackboard->GetData<float>("PatrolTimer");
        coolDownTimer -= context.deltaTime;
        auto& aiAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);

        if (coolDownTimer <= 0.0f)
        {
            const auto unitView = bee::Engine.ECS().Registry.view<EnemyUnit, AttributesComponent>();
            for (const auto unit : unitView)
            {
                auto& attributes = unitView.get<AttributesComponent>(unit);
                auto& stateMachineAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(unit);
                const float randomFloat = bee::GetRandomNumber(0.0f, 1.0f);
                if (randomFloat >= patrolChance) continue;

                const auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(unit);
                const float randomX = bee::GetRandomNumber(0.0f, 1.0f);
                const float randomY = bee::GetRandomNumber(0.0f, 1.0f);
                const auto randomPoint = transform.Translation + glm::normalize(glm::vec3(randomX, randomY, 0) *
                                                                                bee::GetRandomNumber(0.5f, 1.0f) * maxDistance);

                if (stateMachineAgent.IsInState<DeadState>()) return;
                //if (stateMachineAgent.IsInState<PlaceBuilding>()) return;

                stateMachineAgent.context.blackboard->SetData<glm::vec3>("PatrolPoint1", transform.Translation);
                stateMachineAgent.context.blackboard->SetData<glm::vec3>("PatrolPoint2",
                                                                         glm::vec3(randomPoint.x, randomPoint.y, 0));
                stateMachineAgent.SetStateOfType<PatrolState>();
            }
        }
        aiAgent.SetStateOfType<IdleEnemyAIState>();
    };
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(PatrolOrderEnemyAI);

class ScoutOrderEnemyAI : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        auto& aiAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);
        int numPlayerBuildings = 0;
        glm::vec2 position = {};
        const auto playerStructureView = bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform>();
        for (const auto entity : playerStructureView)
        {
            const auto& transform = playerStructureView.get<bee::Transform>(entity);
            position += static_cast<glm::vec2>(transform.Translation);
            numPlayerBuildings++;
        }

        const auto unitsView = bee::Engine.ECS().Registry.view<EnemyUnit, bee::Transform, bee::ai::StateMachineAgent>();
        if (unitsView.size_hint() == 0)
        {
            aiAgent.SetStateOfType<IdleEnemyAIState>();
            return;
        }

        bee::Entity unitToScoutWith{};
        float minDist = std::numeric_limits<float>::max();

        for (const auto entity : unitsView)
        {
            auto& transform = unitsView.get<bee::Transform>(entity);
            const float distance2 = glm::distance2(position, static_cast<glm::vec2>(transform.Translation));
            if (minDist <= distance2) continue;
            minDist = distance2;
            unitToScoutWith = entity;
        }

        auto& stateMachineAgent = unitsView.get<bee::ai::StateMachineAgent>(unitToScoutWith);
        const auto& unitToScoutTransform = unitsView.get<bee::Transform>(unitToScoutWith);

        if (stateMachineAgent.IsInState<DeadState>()) return;
        //if (stateMachineAgent.IsInState<PlaceBuilding>()) return;

        stateMachineAgent.context.blackboard->SetData<glm::vec3>("PatrolPoint1", unitToScoutTransform.Translation);
        stateMachineAgent.context.blackboard->SetData<glm::vec3>("PatrolPoint2", glm::vec3(position.x, position.y, 0));
        stateMachineAgent.SetStateOfType<PatrolState>();

        aiAgent.SetStateOfType<IdleEnemyAIState>();
    };
    void Update(bee::ai::StateMachineContext& context) override{};
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(ScoutOrderEnemyAI);

class TrainUnitEnemyAIState : public bee::ai::State
{
public:
    SERIALIZE_FIELD(float, coolDown);
    SERIALIZE_FIELD(float, trainChance);
    SERIALIZE_FIELD(std::string, unitToTrain);
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto buildingview = bee::Engine.ECS().Registry.view<EnemyStructure, bee::ai::StateMachineAgent>();

        for (const auto entity : buildingview)
        {
            auto& agent = buildingview.get<bee::ai::StateMachineAgent>(entity);
            if (agent.IsInState<TrainUnitEnemyAIState>()) continue;
            agent.context.blackboard->SetData<std::string>("UnitToTrain", unitToTrain);
            agent.context.blackboard->SetData("isTraining", true);
        }

        auto& aiAgent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);
        aiAgent.SetStateOfType<IdleEnemyAIState>();
    };
    void Update(bee::ai::StateMachineContext& context) override{};
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(TrainUnitEnemyAIState);