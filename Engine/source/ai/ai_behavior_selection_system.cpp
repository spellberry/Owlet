#include "ai/ai_behavior_selection_system.hpp"
#include "core/engine.hpp"
#include <execution>

bee::ai::AIBehaviorSelectionSystem::AIBehaviorSelectionSystem(float fixedDeltaTime)
{
    Title = "AI Behavior Selection";
    m_fixedDeltaTime = fixedDeltaTime;
}

void bee::ai::AIBehaviorSelectionSystem::Update(float dt)
{
    m_hasExecutedFrame = false;
    m_timeSinceLastFrame += dt;

    while (m_timeSinceLastFrame >= m_fixedDeltaTime)
    {
        auto btEntities = bee::Engine.ECS().Registry.view<bee::ai::BTAgent>();

        std::for_each(std::execution::par, btEntities.begin(), btEntities.end(),
      [btEntities,this](auto&& entity) {
              auto& agent = btEntities.get<BTAgent>(entity);
              agent.context.deltaTime = m_fixedDeltaTime;
              agent.bt.Execute(agent.context);  
        });

        auto fsmEntities = bee::Engine.ECS().Registry.view<bee::ai::StateMachineAgent>();

        std::for_each(std::execution::seq, fsmEntities.begin(), fsmEntities.end(),
          [fsmEntities, this](auto&& entity)
          {
              if (!bee::Engine.ECS().Registry.valid(entity)) return;
              auto& agent = fsmEntities.get<StateMachineAgent>(entity);
              if (agent.active == false) return;
              agent.context.deltaTime = m_fixedDeltaTime;
              agent.fsm.Execute(agent.context);
          });

        m_timeSinceLastFrame -= m_fixedDeltaTime;
        m_hasExecutedFrame = true;
    }
}

void bee::ai::AIBehaviorSelectionSystem::Render()
{
    System::Render();
}

#ifdef BEE_INSPECTOR
void bee::ai::AIBehaviorSelectionSystem::Inspect()
{
    System::Inspect();
}

void bee::ai::AIBehaviorSelectionSystem::Inspect(bee::Entity e)
{
    System::Inspect(e);
}
#endif