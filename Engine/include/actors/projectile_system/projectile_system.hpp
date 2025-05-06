#pragma once
#include <mutex>
#include <glm/vec3.hpp>

#include "core/ecs.hpp"
#include "core/transform.hpp"

struct Projectile
{
    float speed = 35.0f;
    float size = 1.0f;
    bee::Entity targetEntity{};
    bee::Entity ownerEntity{};
    float damage = 1.0f;
    bool hasMesh = false;
    std::string model = "";
    std::string particlesOnDestroy = "";
};

class ProjectileSystem : public bee::System
{
public:
    ProjectileSystem();
    ~ProjectileSystem();
    bee::Entity CreateProjectile(const glm::vec3& origin, const bee::Entity targetEntity, const bee::Entity ownerEntity, const float size, const float speed, const
                                 float damage,std::string particlesOnDestroy);
    void Update(float dt) override;
private:
    bee::ThreadSafeEntityFactory m_factory;
};
