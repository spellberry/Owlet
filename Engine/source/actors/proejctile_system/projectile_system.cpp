#include "actors/projectile_system/projectile_system.hpp"

#include <fmt/format.h>

#include <platform/dx12/DeviceManager.hpp>
#include <platform/dx12/DeviceManager.hpp>
#include <platform/dx12/DeviceManager.hpp>
#include <platform/dx12/DeviceManager.hpp>
#include <platform/dx12/DeviceManager.hpp>
#include <platform/dx12/DeviceManager.hpp>
#include <platform/dx12/DeviceManager.hpp>
#include "core/engine.hpp"
#include "core/resources.hpp"
#include "rendering/model.hpp"
#include "actors/units/unit_template.hpp"
#include "particle_system/particle_system.hpp"


ProjectileSystem::ProjectileSystem()
    : m_factory((
          [this]() -> bee::Entity 
                    {
                          auto& ecs = bee::Engine.ECS();
                          const auto entity = ecs.CreateEntity();
                          auto& transform = ecs.CreateComponent<bee::Transform>(entity);
                          transform.Name = "Projectile"+std::to_string(static_cast<int>(entity));
                          auto& projectile = ecs.CreateComponent<Projectile>(entity);
                          return entity;
                    }))
{
}

ProjectileSystem::~ProjectileSystem()
{
    auto view = bee::Engine.ECS().Registry.view<Projectile>();
    for (auto [entity,projectile]:view.each())
    {
        bee::Engine.ECS().DeleteEntity(entity);
    }
}

bee::Entity ProjectileSystem::CreateProjectile(const glm::vec3& origin, const bee::Entity targetEntity,
                                               const bee::Entity ownerEntity, const float size, const float speed,
                                               const float damage, std::string particlesOnDestroy)
{
    auto entity =m_factory.CreateEntity();
    auto& registry = bee::Engine.ECS().Registry;
    auto& transform = registry.get<bee::Transform>(entity);
    auto& projectile = registry.get<Projectile>(entity);
    transform.Translation = origin;
    projectile.damage = damage;
    projectile.speed = speed;
    projectile.targetEntity = targetEntity;
    projectile.ownerEntity = ownerEntity;
    projectile.size = size;
    transform.Scale *= 0.25f;
    projectile.particlesOnDestroy = particlesOnDestroy;
    return entity;
}

void ProjectileSystem::Update(float dt)
{
    System::Update(dt);
    auto& registry = bee::Engine.ECS().Registry;
    auto view = registry.view<Projectile, bee::Transform>();

    for (const auto bulletEntity : view)
    {
        auto& transform = registry.get<bee::Transform>(bulletEntity);
        auto& bulletComponent = registry.get<Projectile>(bulletEntity);
        if (!registry.valid(bulletComponent.targetEntity))
        {
            bee::Engine.ECS().DeleteEntity(bulletEntity);
            continue;
        }

        auto& targetTransform = registry.get<bee::Transform>(bulletComponent.targetEntity);

        transform.Translation += glm::normalize(targetTransform.Translation - transform.Translation) * bulletComponent.speed * dt;

        if (glm::distance2(targetTransform.Translation,transform.Translation) <= bulletComponent.size*bulletComponent.size)
        {
            auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(bulletComponent.targetEntity);
            const auto targetArmor = attributes.GetValue(BaseAttributes::Armor);
            const auto damage = bulletComponent.damage;
            attributes.AddModifier(BaseAttributes::HitPoints, {ModifierType::Additive, (-1) *std::abs(damage - targetArmor)});

            if (!bulletComponent.particlesOnDestroy.empty())
            {
                const auto particle = bee::Engine.ECS().CreateEntity();
                auto& particleTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(particle);
                auto& emitter = bee::Engine.ECS().CreateComponent<bee::ParticleEmitter>(particle);

                emitter.AssignEntity(particle);
                particleTransform.Translation = transform.Translation;
                particleTransform.Translation.z += 0.5f;
                bee::Engine.ECS().GetSystem<bee::ParticleSystem>().LoadEmitterFromTemplate(emitter,bulletComponent.particlesOnDestroy);
                emitter.Emit();
            }

            const auto fsmAgent = bee::Engine.ECS().Registry.try_get<bee::ai::StateMachineAgent>(bulletComponent.targetEntity);
            if (fsmAgent)
            {
                fsmAgent->context.blackboard->SetData("ShouldRevenge", true);
                fsmAgent->context.blackboard->SetData("LastHitEnemy", bulletComponent.ownerEntity);
            }

            bee::Engine.ECS().DeleteEntity(bulletEntity);
            continue;
        }
    }
}

