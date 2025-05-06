#include <imgui/imgui.h>

#include <platform/dx12/animation_system.hpp>
#include <platform/dx12/skeletal_animation.hpp>
#include <platform/dx12/skeleton.hpp>
#include "platform/dx12/mesh_dx12.h"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "animation/animation_state.hpp"
#include "rendering/render_components.hpp"
#include "tools/log.hpp"
#include <iostream>
#include <execution>



bee::AnimationSystem::AnimationSystem() 
{ 
}

bee::AnimationSystem::~AnimationSystem() {}

void bee::AnimationSystem::AddAnimation(std::shared_ptr<SkeletalAnimation> animation) 
{
    m_animations.push_back(animation);
}

std::shared_ptr<bee::SkeletalAnimation> bee::AnimationSystem::GetAnimation(int index)
{
    if (index < 0 || index >= m_animations.size())
    {
        bee::Log::Warn("Animation index given is not good.");
        return nullptr;
    }


    return m_animations[index];
}

std::shared_ptr<bee::SkeletalAnimation> bee::AnimationSystem::GetAnimation(std::string name)
{
    for(int i=0;i<m_animations.size();i++)
    {
        if (name == m_animations[i]->GetName())
        {
            return m_animations[i];
        }
    }
  
    bee::Log::Warn("No animation matches this name.");
    return nullptr;
}


void bee::AnimationSystem::Update(float dt)
{
    const auto fsmEntities = bee::Engine.ECS().Registry.view<AnimationAgent>();

    std::for_each(std::execution::par_unseq, fsmEntities.begin(), fsmEntities.end(),
        [fsmEntities, this,dt](const auto entity) {
          if (!bee::Engine.ECS().Registry.valid(entity)) return;
            AnimationAgent& agent = fsmEntities.get<AnimationAgent>(entity);
          agent.context.deltaTime = dt;
            agent.fsm->Execute(agent.context);
    });
    
    std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>> drawables;
    for (const auto& [e, renderer, transform] : bee::Engine.ECS().Registry.view<bee::MeshRenderer, bee::Transform>().each())
        drawables.push_back({e, renderer, transform});

    for (const auto& [e, renderer, transform] : drawables)
    {
        if (renderer.Skeleton)
        {
            renderer.Skeleton->Update();
        }
    }
}
#ifdef BEE_INSPECTOR
void bee::AnimationSystem::Inspect(Entity e)
{

}
#endif

