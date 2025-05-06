#include "particle_system/particle_system.hpp"

#include <imgui/imgui.h>

#include "particle_system/particle_emitter.hpp"

#include "core/engine.hpp"
#include "core/transform.hpp"
#include "imgui/imgui_curve.hpp"


#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/memory.hpp>

#include "tools/tools.hpp"
using namespace bee;



ParticleSystem::ParticleSystem()
{

    Title = "Particle System";
    m_props = new ParticleProps();
    m_props->colorBegin = {254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f};
    m_props->colorEnd = {254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f};
    
    m_props->lifeTime = 10.0f;
    m_props->velocity = {0.0f, 0.0f, 0.0f};
    m_props->velocityVariation = {5.0f, 1.0f, 1.0f};
    m_props->position = {0.0f, 0.0f, -4.0f};
}

void ParticleSystem::SpawnEmitter()
{
    const Entity entity = Engine.ECS().CreateEntity();
    Transform transform;
    transform.Name = "Particle Emitter ";
    Engine.ECS().CreateComponent<Transform>(entity,transform);
    Engine.ECS().CreateComponent<ParticleEmitter>(entity);
    Engine.ECS().Registry.get<ParticleEmitter>(entity).AssignEntity(entity);
}

void ParticleSystem::SpawnEmitter(const Transform& transform, const ParticleEmitter& emitterComponent)
{
    const Entity entity = Engine.ECS().CreateEntity();
    Engine.ECS().CreateComponent<Transform>(entity,transform);
    Engine.ECS().CreateComponent<ParticleEmitter>(entity,emitterComponent);
    Engine.ECS().Registry.get<ParticleEmitter>(entity).AssignEntity(entity);
}



void ParticleSystem::Update(float deltaTime)
{
    System::Update(deltaTime);

    const auto emitterView = Engine.ECS().Registry.view<ParticleEmitter>();

    for (const auto element : emitterView)
    {
        if(element != entt::null && Engine.ECS().Registry.all_of<ParticleEmitter>(element))
        {
            ParticleEmitter& emitter = emitterView.get<ParticleEmitter>(element);

            emitter.m_timeAccumulator += deltaTime;
            const float emitInterval = 1.0f / emitter.m_emitRate;

            while (emitter.m_timeAccumulator >= emitInterval)
            {
                if (emitter.m_isLoop) emitter.Emit(emitter.m_EmitAmount);
                emitter.m_timeAccumulator -= emitInterval;
            }
        }
    }
    

    const auto view = Engine.ECS().Registry.view<ParticleComponent, Transform>();

    for (const auto entity : view)
    {
        auto& particle = view.get<ParticleComponent>(entity);
        /*if(!Engine.ECS().Registry.valid(particle.spawnEmitter))
        {
            Engine.ECS().DeleteEntity(entity);
            continue;
        }*/
        
        if (particle.lifeRemaining <= 0.0f)
        {
            //Engine.ECS().Registry.destroy(entity);
            Engine.ECS().DeleteEntity(entity);
            continue;
        }

        particle.lifeRemaining -= deltaTime;

        if (particle.gravity != 0.0f) particle.velocity.z -= particle.gravity * deltaTime;

        auto& transform = view.get<Transform>(entity);
        transform.Translation += particle.velocity * deltaTime;
        particle.rotation += 0.01f * deltaTime;

        const float life = (particle.lifeTime - particle.lifeRemaining) / particle.lifeTime;
        
        //const auto& spawnerEmitter = Engine.ECS().Registry.get<ParticleEmitter>(particle.spawnEmitter);
        const float size = ImGui::CurveValue(life, 10,  particle.sizeCurvePoints); // calculate value at position 0.7
        transform.Scale= glm::vec3(size) * particle.emitterInitialSize;
        if(particle.sizeVariation.x > 1.0 || particle.sizeVariation.y > 1.0 || particle.sizeVariation.z > 1.0)
            transform.Scale*= particle.sizeVariation;
  
    }


    
    const auto rendererView = Engine.ECS().Registry.view<MeshRenderer, Transform>();
    for (const auto entity : rendererView)
    {
        auto& transform = rendererView.get<Transform>(entity);
        if (Engine.ECS().Registry.all_of<ParticleComponent>(transform.GetParent()))
        {
            auto& meshRenderer = rendererView.get<MeshRenderer>(entity);
            const auto& particle = Engine.ECS().Registry.get<ParticleComponent>(transform.GetParent());
            const float life = particle.lifeRemaining / particle.lifeTime;
            const glm::vec4 color = glm::mix(particle.colorEnd, particle.colorBegin, life);
             
            meshRenderer.constant_data.red = color.r;
            meshRenderer.constant_data.green = color.g;
            meshRenderer.constant_data.blue = color.b;
            meshRenderer.constant_data.opacity = color.a;
           
        }
    }

}

#ifdef BEE_INSPECTOR
void ParticleSystem::Inspect() {
    ImGui::Begin("Particle System");
    if(ImGui::Button("Add emitter"))
    {
        SpawnEmitter();
    }

    const auto view = Engine.ECS().Registry.view<ParticleComponent>();
    const std::string debugText = "Particle Count: " + std::to_string(view.size());
    ImGui::Text(debugText.c_str());
    ImGui::End();
}
#endif

void ParticleSystem::SaveEmitters(std::string& fileName)
{
    std::vector<std::pair<Transform, ParticleEmitter>> emitters;
    auto view = Engine.ECS().Registry.view<Transform, ParticleEmitter>();
    for (auto [entity, transform, emitter] : view.each())
    {
        emitters.push_back(std::pair<Transform,ParticleEmitter>(transform, emitter));
    }
    std::ofstream os(Engine.FileIO().GetPath(FileIO::Directory::Terrain, fileName + "_ParticleEmitters.json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(emitters));

}

void ParticleSystem::SaveEmitterAsTemplate(const ParticleEmitter& emitter, const std::string& templateName)
{
    std::ofstream os(Engine.FileIO().GetPath(FileIO::Directory::Asset, "/effects/" + templateName + ".pepitter"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(emitter));
}

void ParticleSystem::LoadEmitters(std::string& fileName)
{
    auto view = Engine.ECS().Registry.view</*bee::Transform, */ParticleEmitter>();
    for (auto [entity, emitter] : view.each())
    {
        Engine.ECS().DeleteEntity(entity);
    }

    // load new emitters
    if (fileExists(Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_ParticleEmitters.json")))
    {
        std::ifstream is(Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_ParticleEmitters.json"));
        cereal::JSONInputArchive archive(is);

        std::vector<std::pair<Transform, ParticleEmitter>> emitters;
        archive(CEREAL_NVP(emitters));
        for (auto& emitter : emitters)
        {   
            emitter.second.ReloadModels();
            SpawnEmitter(emitter.first,emitter.second);
        }
    }

    
}

void ParticleSystem::LoadEmitterFromTemplate(ParticleEmitter& emitter, const std::string& filename)
{
    if(!Engine.FileIO().Exists(FileIO::Directory::Asset, filename)) return;

    std::ifstream is(Engine.FileIO().GetPath(FileIO::Directory::Asset, filename));
    cereal::JSONInputArchive archive(is);

    archive(emitter);
    emitter.ReloadModels();
}
