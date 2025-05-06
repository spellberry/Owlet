#pragma once
#include "actors/projectile_system/projectile_system.hpp"
#include "actors/props/resource_system.hpp"
#include "actors/selection_system.hpp"
#include "actors/structures/structure_manager_system.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/Utils/editor_variables.hpp"
#include "ai/Utils/generic_factory.hpp"
#include "ai/grid_navigation_system.hpp"
#include "animation/animation_state.hpp"
#include "core/ecs.hpp"
#include "core/audio.hpp"
#include "core/transform.hpp"
#include "glm/vec3.hpp"
#include "level_editor/level_editor_system.hpp"
#include "particle_system/particle_system.hpp"
#include "physics/physics_components.hpp"
#include "rendering/model.hpp"
#include "structure_behaviors.hpp"
#include "tools/debug_metric.hpp"
#include "tools/tools.hpp"
#include <tools/3d_utility_functions.hpp>

class RangedAttackState : public bee::ai::State
{
public:
    std::shared_ptr<bee::Material> m_fireballMaterial;
    RangedAttackState() { m_fireballMaterial = bee::Engine.Resources().Load<bee::Material>("materials/Fireball.pepimat"); }

    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto attackSpeed =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::AttackCooldown);
        context.blackboard->SetData<float>("AttackTimer", attackSpeed);
        context.blackboard->SetData("HasTarget", true);

        if (!bee::Engine.ECS().Registry.valid(context.entity)) return;
        if (!bee::Engine.ECS().Registry.try_get<bee::ai::GridAgent>(context.entity)) return;

        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        agent.path = {};
    };

    void SpawnBullet(bee::Entity targetEntity, bee::Entity shooterEntity, const bee::Transform& shooterTransform)
    {
        const auto projectileSize =bee::Engine.ECS().Registry.get<AttributesComponent>(shooterEntity).GetValue(BaseAttributes::ProjectileSize);
        const auto projectileSpeed =bee::Engine.ECS().Registry.get<AttributesComponent>(shooterEntity).GetValue(BaseAttributes::ProjectileSpeed);
        const auto damage = bee::Engine.ECS().Registry.get<AttributesComponent>(shooterEntity).GetValue(BaseAttributes::Damage);
        auto spawnedBulletEntity = bee::Engine.ECS().GetSystem<ProjectileSystem>().CreateProjectile(shooterTransform.Translation, targetEntity, shooterEntity, projectileSize, projectileSpeed, damage, "Hit_Stone.pepitter");
        auto& bulletComponent = bee::Engine.ECS().Registry.get<Projectile>(spawnedBulletEntity);
        auto& bulletTransform = bee::Engine.ECS().Registry.get<bee::Transform>(spawnedBulletEntity);
        bulletTransform.Scale *= 4.0f;

        if (bee::Engine.ECS().Registry.try_get<AllyUnit>(shooterEntity))
        {
            bee::Engine.Resources().Load<bee::Model>("models/VFX_Mesh_Ball.glb")->Instantiate(spawnedBulletEntity);
            bee::GetComponentInChildren<bee::MeshRenderer>(spawnedBulletEntity).Material = m_fireballMaterial;
            bulletComponent.hasMesh = true;

            auto& emitter = bee::Engine.ECS().CreateComponent<bee::ParticleEmitter>(spawnedBulletEntity);
            emitter.AssignEntity(spawnedBulletEntity);
            bee::Engine.ECS().GetSystem<bee::ParticleSystem>().LoadEmitterFromTemplate(emitter,"effects/E_Projectile_trail.pepitter");

            emitter.SetLoop(true);
            bee::Engine.Audio().PlaySoundW("audio/mage_shoot.wav", 2.0f, true);
            emitter.Emit();
        }
        else
        {
            bee::Engine.Resources().Load<bee::Model>("models/rock.gltf")->Instantiate(spawnedBulletEntity);
            bulletComponent.hasMesh = true;
        }
    };

    void Shoot(const bee::ai::StateMachineContext& context, const bee::Transform& transform, const entt::entity targetEntity)
    {
        const auto animationAgent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(context.entity);
        if (animationAgent)
        {
            animationAgent->context.blackboard->SetData<bool>("isShooting", true);
        }

        const auto attackSpeed =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::AttackCooldown);

        auto& timer = context.blackboard->GetData<float>("AttackTimer");
        timer -= context.deltaTime;

        if (timer <= 0.0f)
        {
            timer = attackSpeed;
            SpawnBullet(targetEntity, context.entity, transform);
            context.blackboard->SetData("AttackTimer", timer);
        }
    }

    void GoToTarget(const bee::ai::StateMachineContext& context, const bee::Transform& unitTransform,
                    const bee::Transform& targetTransform, bee::Entity targetEntity) const
    {
        if (!bee::Engine.ECS().Registry.valid(context.entity)) return;
        if (!bee::Engine.ECS().Registry.try_get<bee::ai::GridAgent>(context.entity)) return;

        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        if (!agent.path.IsEmpty()) return;

        const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
        const double speed = attributes.GetValue(BaseAttributes::MovementSpeed);
        if (agent.speed != speed)
        {
            agent.speed = speed;
        }

        context.blackboard->SetData("IsMoving", true);

        auto& gridNavigationSystem = bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>();
        if (bee::Engine.ECS().Registry.try_get<AllyStructure>(targetEntity))
        {
            context.blackboard->SetData("PositionToMoveTo", static_cast<glm::vec2>(targetTransform.Translation));
            agent.SetGoal(targetTransform.Translation);
        }
        else
        {
            const auto sampledPosition = gridNavigationSystem.GetGrid().SampleWalkablePoint(targetTransform.Translation);
            context.blackboard->SetData("PositionToMoveTo", sampledPosition);
            agent.SetGoal(sampledPosition);
        }
    }

    void Update(bee::ai::StateMachineContext& context) override
    {
        if (!bee::Engine.ECS().Registry.valid(context.entity))
        {
            return;
        }

        const auto& targetEntity = context.blackboard->GetData<bee::Entity>("TargetEntity");

        if (!bee::Engine.ECS().Registry.valid(targetEntity))
        {
            context.blackboard->SetData("HasTarget", false);
            return;
        }

        context.blackboard->SetData("HasTarget", true);
        const auto range = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::Range);
        auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
        const auto& targetTransform = bee::Engine.ECS().Registry.get<bee::Transform>(targetEntity);

        float radius = 0.0f;
        const auto diskCollider = bee::Engine.ECS().Registry.try_get<bee::physics::DiskCollider>(targetEntity);
        if (diskCollider)
        {
            radius = diskCollider->radius;
        }

        const float distance = glm::distance(static_cast<glm::vec2>(targetTransform.Translation),
                                             static_cast<glm::vec2>(unitTransform.Translation)) -
                               radius;
        if (distance <= range)
        {
            if (!bee::Engine.ECS().Registry.valid(targetEntity))
            {
                context.blackboard->SetData("HasTarget", false);
                return;
            }

            if (!bee::Engine.ECS().Registry.valid(context.entity))
            {
                return;
            }

            const glm::vec2 normalizedDir = glm::normalize(targetTransform.Translation-unitTransform.Translation);

            if (glm::length(normalizedDir) != 0.0f)
            {
                const float angle = glm::atan(normalizedDir.y, normalizedDir.x);
                unitTransform.Rotation= glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
            }


            if (bee::Engine.ECS().Registry.get<AttributesComponent>(targetEntity).GetValue(BaseAttributes::HitPoints) > 0)
            {
                if (!bee::Engine.ECS().Registry.valid(context.entity)) return;
                if (!bee::Engine.ECS().Registry.try_get<bee::ai::GridAgent>(context.entity)) return;
                auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
                agent.path = {};

                context.blackboard->SetData("IsMoving", false);
                Shoot(context, unitTransform, targetEntity);
            }
        }
        else
        {
            GoToTarget(context, unitTransform, targetTransform, targetEntity);
        }
    };

    void End(bee::ai::StateMachineContext& context) override
    {
        const auto animationAgent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(context.entity);
        if (animationAgent)
        {
            animationAgent->context.blackboard->SetData<bool>("isShooting", false);
        }
        context.blackboard->SetData("HasTarget", false);
    };
};
REGISTER_STATE(RangedAttackState);

class MeleeAttackState : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto attackSpeed =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::AttackCooldown);
        context.blackboard->SetData<float>("AttackTimer", attackSpeed);
    };

    void Attack(const bee::ai::StateMachineContext& context, entt::entity targetEntity)
    {
        const auto attackSpeed =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::AttackCooldown);

        auto& timer = context.blackboard->GetData<float>("AttackTimer");
        timer -= context.deltaTime;

        if (timer <= 0.0f)
        {
            const auto animationAgent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(context.entity);
            if (animationAgent)
            {
                animationAgent->context.blackboard->SetData<bool>("isShooting", true);
            }

            timer = attackSpeed;
            context.blackboard->SetData("AttackTimer", timer);
            auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(targetEntity);

            const auto targetArmor = attributes.GetValue(BaseAttributes::Armor);
            const auto damage =
                bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::Damage);
            attributes.AddModifier(BaseAttributes::HitPoints, { ModifierType::Additive, (-1) * std::abs(damage - targetArmor) });
            bee::Engine.Audio().PlaySoundW("audio/melee_hit3.wav", 2.0f, true);

        }
    }

    void GoToTarget(const bee::ai::StateMachineContext& context, const bee::Transform& unitTransform,
                    const bee::Transform& targetTransform, bee::Entity targetEntity) const
    {
        const auto animationAgent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(context.entity);
        if (animationAgent)
        {
            animationAgent->context.blackboard->SetData<bool>("isShooting", false);
        }
        if (!bee::Engine.ECS().Registry.valid(context.entity)) return;
        if (!bee::Engine.ECS().Registry.try_get<bee::ai::GridAgent>(context.entity)) return;

        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        if (!agent.path.IsEmpty()) return;
        context.blackboard->SetData("IsMoving", true);

        const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
        const double speed = attributes.GetValue(BaseAttributes::MovementSpeed);
        if (agent.speed != speed)
        {
            agent.speed = speed;
        }

        auto& gridNavigationSystem = bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>();
        if (bee::Engine.ECS().Registry.try_get<AllyStructure>(targetEntity))
        {
            const auto sampledPosition = gridNavigationSystem.GetGrid().SampleWalkablePoint(targetTransform.Translation);
            context.blackboard->SetData("PositionToMoveTo", sampledPosition);
            agent.SetGoal(sampledPosition);
        }
        else
        {
            context.blackboard->SetData("PositionToMoveTo", static_cast<glm::vec2>(targetTransform.Translation));
            agent.SetGoal(targetTransform.Translation);

        }
    }

    void Update(bee::ai::StateMachineContext& context) override
    {
        const auto& targetEntity = context.blackboard->GetData<bee::Entity>("TargetEntity");
        context.blackboard->SetData("HasTarget", true);

        if (!bee::Engine.ECS().Registry.valid(targetEntity))
        {
            context.blackboard->SetData("HasTarget", false);
            return;
        }

        const auto range = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::Range);
        auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
        const auto& targetTransform = bee::Engine.ECS().Registry.get<bee::Transform>(targetEntity);

        float radius = 0.0f;
        const auto diskCollider = bee::Engine.ECS().Registry.try_get<bee::physics::DiskCollider>(targetEntity);
        if (diskCollider)
        {
            radius = diskCollider->radius;
        }

        const float distance = glm::distance(targetTransform.Translation, unitTransform.Translation) - radius;
        if (distance <= range)
        {
            if (bee::Engine.ECS().Registry.get<AttributesComponent>(targetEntity).GetValue(BaseAttributes::HitPoints) > 0)
            {
                const glm::vec2 normalizedDir = glm::normalize(targetTransform.Translation - unitTransform.Translation);

                if (glm::length(normalizedDir)!=0.0f)
                {
                    const float angle = glm::atan(normalizedDir.y, normalizedDir.x);
                    unitTransform.Rotation = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));

                }


                auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
                agent.path={};
                context.blackboard->SetData("IsMoving", false);
                Attack(context, targetEntity);
            }
        }
        else
        {
            GoToTarget(context, unitTransform, targetTransform, targetEntity);
        }
    };

    void End(bee::ai::StateMachineContext& context) override
    {
        const auto animationAgent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(context.entity);
        if (animationAgent)
        {
            animationAgent->context.blackboard->SetData<bool>("isShooting", false);
        }
        context.blackboard->SetData("HasTarget", false);
    }
};
REGISTER_STATE(MeleeAttackState);

class PatrolState : public bee::ai::State
{
public:
    SERIALIZE_FIELD(float, stoppingDistance);
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
        if (!context.blackboard->HasKey<glm::vec3>("PatrolPoint1"))
        {
            context.blackboard->SetData("PatrolPoint1", unitTransform.Translation);
        }

        if (!context.blackboard->HasKey<glm::vec3>("PatrolPoint2"))
        {
            context.blackboard->SetData("PatrolPoint2", unitTransform.Translation);
        }

        context.blackboard->SetData("PatrolIndex", 1);
        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        const auto point2 = context.blackboard->GetData<glm::vec3>("PatrolPoint2");
        context.blackboard->SetData("IsMoving", true);
        agent.SetGoal(point2);
    }

    void Update(bee::ai::StateMachineContext& context) override
    {
        auto& patrolIndex = context.blackboard->GetData<int>("PatrolIndex");
        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);

        if (patrolIndex == 0)
        {
            auto point1 = context.blackboard->GetData<glm::vec3>("PatrolPoint1");
            point1.z = unitTransform.Translation.z;
            const float distance2 =
                glm::distance2(static_cast<glm::vec2>(unitTransform.Translation), static_cast<glm::vec2>(point1));
            if (distance2 <= glm::pow(stoppingDistance, 2))
            {
                const auto point2 = context.blackboard->GetData<glm::vec3>("PatrolPoint2");
                patrolIndex = 1;
                const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
                const double speed = attributes.GetValue(BaseAttributes::MovementSpeed);
                if (agent.speed != speed)
                {
                    agent.speed = speed;
                }
                agent.SetGoal(point2);
                context.blackboard->SetData("PatrolIndex", patrolIndex);
            }
        }
        else
        {
            auto point2 = context.blackboard->GetData<glm::vec3>("PatrolPoint2");
            point2.z = unitTransform.Translation.z;
            const float distance2 =
                glm::distance2(static_cast<glm::vec2>(unitTransform.Translation), static_cast<glm::vec2>(point2));
            if (distance2 <= glm::pow(stoppingDistance, 2))
            {
                const auto point1 = context.blackboard->GetData<glm::vec3>("PatrolPoint1");
                patrolIndex = 0;
                const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
                const double speed = attributes.GetValue(BaseAttributes::MovementSpeed);
                if (agent.speed != speed)
                {
                    agent.speed = speed;
                }
                agent.SetGoal(point1);
                context.blackboard->SetData("PatrolIndex", patrolIndex);
            }
        }
    }

    void End(bee::ai::StateMachineContext& context) override
    {
        const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        agent.SetGoal(unitTransform.Translation);
        context.blackboard->SetData("IsMoving", false);
    }
};
REGISTER_STATE(PatrolState);

class OffensiveMove : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        agent.path = {};
        context.blackboard->SetData("IsMoving", true);
        context.blackboard->SetData("OffensiveMove", true);
    };
    void Update(bee::ai::StateMachineContext& context) override
    {
        const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
        auto interceptionRange =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::InterceptionRange);
        bee::Entity closestEntity;
        float minDist = std::numeric_limits<float>::max();
        bool inRange = false;

        if (bee::Engine.ECS().Registry.try_get<AllyUnit>(context.entity))
        {
            const auto view = bee::Engine.ECS().Registry.view<EnemyUnit, bee::Transform>();
            for (const auto entity : view)
            {
                auto& transform = view.get<bee::Transform>(entity);
                const float distance2 = glm::distance2(transform.Translation, unitTransform.Translation);
                if (distance2 >= glm::pow(interceptionRange, 2)) continue;
                if (minDist <= distance2) continue;
                inRange = true;
                minDist = distance2;
                closestEntity = entity;
            }
        }
        else
        {
            const auto unitView = bee::Engine.ECS().Registry.view<AllyUnit, bee::Transform>();
            for (const auto entity : unitView)
            {
                auto& transform = unitView.get<bee::Transform>(entity);
                const float distance2 = glm::distance2(transform.Translation, unitTransform.Translation);
                if (distance2 > glm::pow(interceptionRange, 2)) continue;
                if (minDist <= distance2) continue;
                inRange = true;
                minDist = distance2;
                closestEntity = entity;
            }

            const auto buildingView = bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform>();
            for (const auto entity : buildingView)
            {
                auto& transform = buildingView.get<bee::Transform>(entity);
                const float distance2 = glm::distance2(transform.Translation, unitTransform.Translation);
                if (distance2 > glm::pow(interceptionRange, 2)) continue;
                if (minDist <= distance2) continue;
                inRange = true;
                minDist = distance2;
                closestEntity = entity;
            }
        }

        if (inRange)
        {
            context.blackboard->SetData("HasTarget", true);
            context.blackboard->SetData<bee::Entity>("TargetEntity", closestEntity);

            auto& agent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);

            if (agent.fsm.StateExists<RangedAttackState>())
            {
                agent.SetStateOfType<RangedAttackState>();
            }
            else if (agent.fsm.StateExists<MeleeAttackState>())
            {
                agent.SetStateOfType<MeleeAttackState>();
            }
        }
        else
        {
            auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
            if (!agent.path.IsEmpty()) return;
            if (!bee::Engine.ECS().Registry.valid(context.entity)) return;
            if (!bee::Engine.ECS().Registry.try_get<bee::physics::DiskCollider>(context.entity)) return;

            if (!agent.path.IsEmpty()) return;

            const auto& diskCollider = bee::Engine.ECS().Registry.get<bee::physics::DiskCollider>(context.entity);
            const auto& positionToMoveTo = context.blackboard->GetData<glm::vec2>("PositionToMoveTo");
            if (glm::distance(static_cast<glm::vec2>(unitTransform.Translation), positionToMoveTo) <=
                diskCollider.radius + 0.2f)
            {
                context.blackboard->SetData("IsMoving", false);
                context.blackboard->SetData("OffensiveMove", false);
            }

            const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
            const double speed = attributes.GetValue(BaseAttributes::MovementSpeed);
            if (agent.speed != speed)
            {
                agent.speed = speed;
            }
            agent.SetGoal(positionToMoveTo);
        }
    }
    void End(bee::ai::StateMachineContext& context) override { context.blackboard->SetData("IsMoving", false); };
};
REGISTER_STATE(OffensiveMove);

class IdleState : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
    };
    void Update(bee::ai::StateMachineContext& context) override{

    };

    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(IdleState);

class OffensiveIdleState : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
    };
    void Update(bee::ai::StateMachineContext& context) override
    {
        if (!bee::Engine.ECS().Registry.valid(context.entity)) return;
        const auto offensiveVisionRange =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::InterceptionRange);
        const auto& unitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
        bee::Entity closestEntity;
        float minDist = std::numeric_limits<float>::max();
        bool inRange = false;

        if (bee::Engine.ECS().Registry.try_get<AllyUnit>(context.entity))
        {
            const auto view = bee::Engine.ECS().Registry.view<EnemyUnit, bee::Transform>();
            for (const auto entity : view)
            {
                auto& transform = view.get<bee::Transform>(entity);
                const float distance2 = glm::distance2(transform.Translation, unitTransform.Translation);
                if (distance2 >= glm::pow(offensiveVisionRange, 2)) continue;
                if (minDist <= distance2) continue;
                inRange = true;
                minDist = distance2;
                closestEntity = entity;
            }
        }
        else
        {
            auto view = bee::Engine.ECS().Registry.view<AllyUnit, bee::Transform>();
            for (const auto entity : view)
            {
                auto& transform = view.get<bee::Transform>(entity);
                const float distance2 = glm::distance2(transform.Translation, unitTransform.Translation);
                if (distance2 > glm::pow(offensiveVisionRange, 2)) continue;
                if (minDist <= distance2) continue;
                inRange = true;
                minDist = distance2;
                closestEntity = entity;
            }
        }

        if (inRange)
        {
            context.blackboard->SetData("HasTarget", true);
            context.blackboard->SetData<bee::Entity>("TargetEntity", closestEntity);

            auto& agent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);
            // set the state to either melee or ranged attack, depending on what state the unit's FSM has
            if (agent.fsm.StateExists<RangedAttackState>())
                agent.SetStateOfType<RangedAttackState>();

            else if (agent.fsm.StateExists<MeleeAttackState>())
                agent.SetStateOfType<MeleeAttackState>();
        }
    };

    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(OffensiveIdleState);

class RevengeAttack : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        const auto& targetEntity = context.blackboard->GetData<bee::Entity>("LastHitEnemy");
        context.blackboard->SetData("TargetEntity", targetEntity);

        context.blackboard->SetData("HasTarget", true);

        if (!bee::Engine.ECS().Registry.valid(targetEntity))
        {
            context.blackboard->SetData("HasTarget", false);
            return;
        }

        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::StateMachineAgent>(context.entity);

        agent.SetStateOfType<RangedAttackState>();
    };
    void Update(bee::ai::StateMachineContext& context) override{

    };
    void End(bee::ai::StateMachineContext& context) override { context.blackboard->SetData("ShouldRevenge", false); };
};
REGISTER_STATE(RevengeAttack);

class MoveToPointState : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override
    {
        if (!context.blackboard->HasKey<glm::vec2>("PositionToMoveTo"))
        {
            context.blackboard->SetData(
                "PositionToMoveTo",
                glm::vec2(
                    15, 2)); 
        }

        context.blackboard->SetData("OffensiveMove", false);
        context.blackboard->SetData("IsMoving", true);

        //extracting the position the agent needs to move to 
        const auto& positionToMoveTo = context.blackboard->GetData<glm::vec2>("PositionToMoveTo");
        bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity).SetGoal(positionToMoveTo);
    }
    void Update(bee::ai::StateMachineContext& context) override
    {
        //how close to the goal we want the unit to stop
        float stoppingDistanceSqr = 16.0f;

        //getting all necessary components
        const auto& positionToMoveTo = context.blackboard->GetData<glm::vec2>("PositionToMoveTo");
        const auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
        auto& agent = bee::Engine.ECS().Registry.get<bee::ai::GridAgent>(context.entity);
        const auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity);
        const double speed = attributes.GetValue(BaseAttributes::MovementSpeed);

        //casting a ray to the goal
        bee::Entity hitEntity = entt::null;
        glm::vec2 rayDirection = positionToMoveTo - glm::vec2(transform.Translation.x, transform.Translation.y);
        bool hasHit = bee::HitResponse2D(hitEntity, transform.Translation, glm::normalize(rayDirection));

        //setting the agent's speed
        if (agent.speed != speed)
        {
            agent.speed = speed;
        }

        if (hitEntity != entt::null && hitEntity != context.entity) //if we have hit a valid entity which is not the current agent, we check the distance to it.
        {
            auto hitGridAgent = bee::Engine.ECS().Registry.try_get < bee::ai::GridAgent>(hitEntity);
            auto& hitUnitTransform = bee::Engine.ECS().Registry.get<bee::Transform>(hitEntity);


            auto distToUnit = glm::distance2(hitUnitTransform.Translation, transform.Translation);
            if (hitGridAgent == nullptr) return;

            //if the other agent in our path has stopped and is close enough, this agent also stops
            if (distToUnit < stoppingDistanceSqr && hitGridAgent->path.IsEmpty())
            {
                agent.path.EmptyPath();
                context.blackboard->SetData("IsMoving", false); 
            }
        }
        else //if we didn't hit another agent/entity, we simply calculate the distance to the goal
        {
            //calculating the distance to the goal
            const float dist = glm::distance2(static_cast<glm::vec2>(transform.Translation), positionToMoveTo);

            //stop the agent if it's close enough to the goal to avoid crowding
            if (dist < stoppingDistanceSqr)
            {
                agent.path.EmptyPath();
                context.blackboard->SetData("IsMoving", false);
            }
        }


    }
    void End(bee::ai::StateMachineContext& context) override {}
};
REGISTER_STATE(MoveToPointState);

class DeadState : public bee::ai::State
{
public:
    SERIALIZE_FIELD(float, skeletonBaseHeight);

    void EnemyDropResource(bee::ai::StateMachineContext& context, const bee::Entity corpseEntity)
    {
        auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(corpseEntity);
        auto& resource = bee::Engine.ECS().CreateComponent<PropResourceComponent>(corpseEntity);
        resource.type = GameResourceType::None;

        auto& body =
            bee::Engine.ECS().CreateComponent<bee::physics::Body>(corpseEntity, bee::physics::Body::Type::Dynamic, 100000.0f);
        auto& collider = bee::Engine.ECS().CreateComponent<bee::physics::DiskCollider>(corpseEntity, 1.0f);
        bee::Engine.ECS().Registry.remove<bee::physics::DiskCollider>(context.entity);

        bee::Engine.Resources().Load<bee::Model>(actualResourcePath)->Instantiate(corpseEntity);

        const auto woodBounty =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::WoodBounty);
        const auto stoneBounty =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::StoneBounty);
        if (woodBounty != 0.0)
        {
            resource.type = GameResourceType::Wood;
            const auto woodBountyDeviation = bee::Engine.ECS()
                                                 .Registry.get<AttributesComponent>(context.entity)
                                                 .GetValue(BaseAttributes::WoodBountyDeviation);
            resource.resourceGain = bee::GetRandomNumberInt(static_cast<int>(woodBounty - woodBountyDeviation),
                                                            static_cast<int>(woodBounty + woodBountyDeviation));
            bee::Engine.ECS().CreateComponent<AttributesComponent>(corpseEntity);
            transform.Translation = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity).Translation;
            transform.Translation.z += skeletonBaseHeight;
            transform.Name = "DroppedWood";
            
        }
        if (stoneBounty != 0.0)
        {
            resource.type = GameResourceType::Stone;
            const auto stoneBountyDeviation = bee::Engine.ECS()
                                                  .Registry.get<AttributesComponent>(context.entity)
                                                  .GetValue(BaseAttributes::StoneBountyDeviation);
            resource.resourceGain = bee::GetRandomNumberInt(static_cast<int>(stoneBounty - stoneBountyDeviation),
                                                            static_cast<int>(stoneBounty + stoneBountyDeviation));
            transform.Translation = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity).Translation;
            bee::Engine.ECS().CreateComponent<AttributesComponent>(corpseEntity);
            transform.Translation.z += skeletonBaseHeight;
            transform.Name = "DroppedStone";
            
        }
        resource.baseVerticalPosition = transform.Translation.z;
        body.SetPosition(transform.Translation);
    }

    void EnemyAddResource(const bee::ai::StateMachineContext& context)
    {
        const auto woodBounty =bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::WoodBounty);
        const auto stoneBounty =bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::StoneBounty);

        if (woodBounty != 0.0)
        {
            bee::Engine.ECS().GetSystem<ResourceSystem>().AddResource(GameResourceType::Wood, woodBounty);
        }
        if (stoneBounty != 0.0)
        {
            bee::Engine.ECS().GetSystem<ResourceSystem>().AddResource(GameResourceType::Stone, stoneBounty);
        }
    }

    void Initialize(bee::ai::StateMachineContext& context) override
    {
        /*if (actualResourcePath.empty())
        {*/
        auto& unitManager = bee::Engine.ECS().GetSystem<UnitManager>();
        const auto& templateHandle = bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetEntityType();
        auto& unitTemplate = unitManager.GetUnitTemplate(templateHandle);
        actualResourcePath = unitTemplate.corpsePath;
        //}

        if (bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetTeam() == static_cast<int>(Team::Enemy))
        {
            auto corpseEntity = bee::Engine.ECS().CreateEntity();

            EnemyAddResource(context);
            context.blackboard->SetData("SkeletonEntity", corpseEntity);
        }

        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
            {
                if (bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetTeam() ==
                    static_cast<int>(Team::Ally))
                {
                    if (debugMetric.allyUnitsKilled.find(templateHandle) == debugMetric.allyUnitsKilled.end())
                        debugMetric.allyUnitsKilled.insert(std::pair<std::string, int>(templateHandle, 1));
                    else
                        debugMetric.allyUnitsKilled[templateHandle] += 1;
                }
                else if (bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetTeam() ==
                         static_cast<int>(Team::Enemy))
                {
                    if (debugMetric.enemyUnitsKilled.find(templateHandle) == debugMetric.enemyUnitsKilled.end())
                        debugMetric.enemyUnitsKilled.insert(std::pair<std::string, int>(templateHandle, 1));
                    else
                        debugMetric.enemyUnitsKilled[templateHandle] += 1;
                }
            }
        const auto animationAgent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(context.entity);
        if (animationAgent)
        {
            animationAgent->context.blackboard->SetData<bool>("isDead", true);
        }

        bee::Engine.ECS().Registry.remove<bee::MeshRenderer>(context.entity);

        const auto deathTimer =
            bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::DeathCooldown);
        context.blackboard->SetData<float>("DestructionTimer", deathTimer);
        bee::Engine.ECS().Registry.remove<Selected>(context.entity);
    }

    void Update(bee::ai::StateMachineContext& context) override
    {
        const auto woodBounty =bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::WoodBounty);
        const auto stoneBounty =bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetValue(BaseAttributes::StoneBounty);
        auto& timer = context.blackboard->GetData<float>("DestructionTimer");
        timer -= context.deltaTime;

        if (timer <= 0.0f)
        {
            // Deleting the prop if it exists before deleting the unit
            auto& tempTransform = bee::Engine.ECS().Registry.get<bee::Transform>(context.entity);
            auto* propTagR = bee::Engine.ECS().Registry.try_get<UnitPropTagR>(tempTransform.FirstChild());
            if (propTagR != nullptr) bee::Engine.ECS().DeleteEntity(propTagR->propRight);
            auto* propTagL = bee::Engine.ECS().Registry.try_get<UnitPropTagL>(tempTransform.FirstChild());
            if (propTagL != nullptr) bee::Engine.ECS().DeleteEntity(propTagL->propLeft);
            if (woodBounty > 0.0f)
            {
                bee::Engine.Audio().PlaySoundW("audio/wood_death.wav", 2.0f, true);
            }
            else if (stoneBounty > 0.0f)
            {
                bee::Engine.Audio().PlaySoundW("audio/rock_death_variation.wav", 2.0f, true);
            }
            
            else if (bee::Engine.ECS().Registry.get<AttributesComponent>(context.entity).GetTeam() == static_cast<int>(Team::Ally))
            {
                bee::Engine.Audio().PlaySoundW("audio/owl_death.wav", 2.0f, true);
            }
            bee::Engine.ECS().DeleteEntity(context.entity);
            return;
        }
        context.blackboard->SetData<float>("DestructionTimer", timer);
    }
    void End(bee::ai::StateMachineContext& context) override {}

private:
    std::string actualResourcePath{};
};
REGISTER_STATE(DeadState);
