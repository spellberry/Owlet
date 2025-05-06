#pragma once
#include "BehaviorTrees/behavior_tree.hpp"
#include "BehaviorTrees/Editor/behavior_tree_editor.hpp"
#include "core/ecs.hpp"
#include "core/transform.hpp"
#include "FiniteStateMachines/finite_state_machine.hpp"
#include "FiniteStateMachines/Editor/finite_state_machine_editor.hpp"
#include "Utils/blackboard_inspector.hpp"

namespace bee::ai
{
    class AIBehaviorSelectionSystem : public bee::System
    {
    public:
        AIBehaviorSelectionSystem(float fixedDeltaTime);
        void Update(float dt) override;
        void Render() override;
    #ifdef BEE_INSPECTOR
        void Inspect() override;
        void Inspect(bee::Entity e) override;
    #endif
    private:
       float m_fixedDeltaTime = 0.1f;
       float m_timeSinceLastFrame = 0.0f;
       bool m_hasExecutedFrame = false;
    };

    class StateMachineAgent
    {
    public:
        bool active = true;
        StateMachineAgent(ai::FiniteStateMachine& fsm) : fsm(fsm) {}

        template <typename StateType>
        bool IsInState() const
        {
            if (!context.GetCurrentState().has_value()) return false;
            return fsm.GetStateType(context.GetCurrentState().value()) == typeid(StateType);
        }

        template<typename StateType>
        void SetStateOfType()
        {
            const std::vector<size_t> ids = fsm.GetStateIDsOfType<StateType>();

            if (ids.empty()) return;

            const auto state = ids[0];
            fsm.SetCurrentState(state,context);
        }

        ai::FiniteStateMachine& fsm;
        ai::StateMachineContext context;
    };

    class BTAgent
    {
    public:
        BTAgent(ai::BehaviorTree& btToSet) : bt(btToSet) {}

        ai::BehaviorTree& bt;
        ai::BehaviorTreeContext context;
    };

}
