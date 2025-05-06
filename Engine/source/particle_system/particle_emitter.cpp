#include "particle_system/particle_emitter.hpp"

#include <memory>

#include "core/engine.hpp"
#include "core/transform.hpp"
#include "imgui/imgui_curve.hpp"
#include "material_system/material_system.hpp"
#include "tools/tools.hpp"
#include "particle_system/particle_system.hpp"
#include "platform/dx12/mesh_dx12.h"
#include "rendering/render_components.hpp"

using namespace bee;


ParticleEmitter::ParticleEmitter()
{
    //some default values
    particleProps = std::make_shared<ParticleProps>();
    particleProps->colorBegin = {254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f};
    particleProps->colorEnd = {254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f};
    particleProps->lifeTime = 10.0f;
    particleProps->velocity = {0.0f, 0.0f, 0.0f};
    particleProps->velocityVariation = {5.0f, 1.0f, 1.0f};
    particleProps->position = {0.0f, 0.0f, -4.0f};
    particleProps->sizeCurvePoints[0].x = ImGui::CurveTerminator; // init data so editor knows to take it from here

    //this will load models and materials
    ReloadModels();
}

ParticleEmitter::~ParticleEmitter()
{
   
}

void ParticleEmitter::Emit() const
{
    
    if(particleProps->type == ParticleType::Mesh && m_particleModelPath == "") return;
    
    Entity entity = Engine.ECS().CreateEntity();
    ParticleComponent p;
    
    p.type = particleProps->type;
    p.rotation = glm::vec3(0.0f);
    // Velocity
    if (particleProps->hasGravity)
        p.gravity = particleProps->gravity;
    else
        p.gravity = 0.0f;

    p.velocity = particleProps->velocity;
    p.velocity.x += particleProps->velocityVariation.x * (GetRandomNumber(0.0,1.0) - 0.5f);
    p.velocity.y += particleProps->velocityVariation.y * (GetRandomNumber(0.0,1.0) - 0.5f);
    p.velocity.z += particleProps->velocityVariation.z * (GetRandomNumber(0.0,1.0) - 0.5f);

    // Color
    p.colorBegin = particleProps->colorBegin;
    p.colorEnd = particleProps->colorEnd;

    p.lifeTime = particleProps->lifeTime;
    p.lifeRemaining = particleProps->lifeTime;
    p.spawnEmitter = m_Entity;
    //p.sizeVariation = particleProps->sizeVariation * (GetRandomNumber(0.0,1.0) - 0.5f);
    p.sizeVariation.x = particleProps->sizeVariation.x * (GetRandomNumber(0.0,1.0) - 0.5f);
    p.sizeVariation.y = particleProps->sizeVariation.y * (GetRandomNumber(0.0,1.0) - 0.5f);
    p.sizeVariation.z = particleProps->sizeVariation.z * (GetRandomNumber(0.0,1.0) - 0.5f);
    
    for(int i =0;i<10;i++)
    {
        p.sizeCurvePoints[i] = particleProps->sizeCurvePoints[i];
    }
    p.emitterInitialSize = particleProps->sizeBegin;
    Engine.ECS().CreateComponent<ParticleComponent>(entity,p);

    Transform transform;
    transform.Translation = Engine.ECS().Registry.get<Transform>(m_Entity).Translation;
    Engine.ECS().CreateComponent<Transform>(entity,transform);
    
    m_particleModel->Instantiate(entity);
    
    auto& t = Engine.ECS().Registry.get<Transform>(entity);
    auto& meshRenderer = Engine.ECS().Registry.get<MeshRenderer>(t.FirstChild());
    meshRenderer.Material = m_particleMaterial;

    
}

void ParticleEmitter::Emit(const int amount) const
{
    for (int i = 0; i < amount; i++)
    {
        Emit();
    }
}



void ParticleEmitter::AssignEntity(Entity newEntity)
{
    m_Entity = newEntity;
}

void ParticleEmitter::ReloadModels()
{
    if(m_materialPath != "")
    {
        m_particleMaterial = Engine.Resources().Load<Material>(m_materialPath);
    }
    else 
    {
        m_particleMaterial = Engine.Resources().Load<Material>("materials/pop.pepimat");
        m_materialPath = "materials/pop.pepimat";
    }

    if(particleProps->type == ParticleType::Mesh)
    {
        if(m_particleModelPath != "")
        {
            m_particleModel = Engine.Resources().Load<Model>(m_particleModelPath);
        }
        else
        {
            m_particleModel = Engine.Resources().Load<Model>("models/rock.gltf");
            m_particleModelPath = "models/rock.gltf";
        }
    }
    else if(particleProps->type == ParticleType::Billboard)
    {
        m_particleModel = Engine.Resources().Load<Model>("models/PlaneWithT.glb");
        m_particleModelPath = "models/PlaneWithT.glb";
    }
        
        
}


