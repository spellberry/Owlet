#include "ai/behavior_editor_system.hpp"

#include "ai/ai_behavior_selection_system.hpp"
#include "animation/animation_state.hpp"

bee::ai::BehaviorEditorSystem::BehaviorEditorSystem()
{
    Title = "Behavior structure editor";
#if defined(BEE_PLATFORM_PC)
    m_btEditor = std::make_unique<BehaviorTreeEditor>();
    m_fsmEditor = std::make_unique<FiniteStateMachineEditor>();
#endif
}

#ifdef BEE_INSPECTOR
void bee::ai::BehaviorEditorSystem::Inspect()
{
#if defined(BEE_PLATFORM_PC)
    System::Inspect();

    m_btEditor->Update();
    m_fsmEditor->Update();
#endif
}


void bee::ai::BehaviorEditorSystem::Inspect(bee::Entity e)
{
    System::Inspect(e);

    const auto agent = bee::Engine.ECS().Registry.try_get<StateMachineAgent>(e);
    if (agent)
    {
        m_fsmEditor->PreviewContext(agent->fsm, agent->context);
        m_bbInspector.Inspect(*agent->context.blackboard);
    }
}
#endif

