#pragma once
#include "BehaviorTrees/Editor/behavior_tree_editor.hpp"
#include "core/ecs.hpp"
#include "FiniteStateMachines/Editor/finite_state_machine_editor.hpp"
#include "Utils/blackboard_inspector.hpp"

namespace bee::ai
{

class BehaviorEditorSystem: public bee::System
    {
    public:
        BehaviorEditorSystem();
#ifdef BEE_INSPECTOR
        void Inspect() override;
        void Inspect(bee::Entity e) override;
#endif
private:
#if defined(BEE_PLATFORM_PC)
        std::unique_ptr<BehaviorTreeEditor> m_btEditor{};
        std::unique_ptr<FiniteStateMachineEditor> m_fsmEditor{};
        BlackboardInspector m_bbInspector{};
#endif
    };
}
