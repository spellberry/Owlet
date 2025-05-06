#pragma once
#include <platform/dx12/skeletal_animation.hpp>

#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/Utils/editor_variables.hpp"
#include "ai/Utils/generic_factory.hpp"

class AnimationAgent
{
public:
    AnimationAgent(const std::shared_ptr<bee::ai::FiniteStateMachine>& fsm) : fsm(fsm) {}

    template <typename StateType>
    bool IsInState() const
    {
        if (!context.GetCurrentState().has_value()) return false;
        return fsm->GetStateType(context.GetCurrentState().value()) == typeid(StateType);
    }

    template <typename StateType>
    void SetStateOfType()
    {
        const std::vector<size_t> ids = fsm->GetStateIDsOfType<StateType>();

        if (ids.empty()) return;

        const auto state = ids[0];
        fsm->SetCurrentState(state, context);
    }

    std::shared_ptr<bee::ai::FiniteStateMachine> fsm{};
    bee::ai::StateMachineContext context{};
};

class AnimationState : public bee::ai::State
{
public:
    SERIALIZE_FIELD(float, speed)
    SERIALIZE_FIELD(bool, repeat)
    SERIALIZE_FIELD(std::filesystem::path, modelPath);
    SERIALIZE_FIELD(std::string, animationName);

    AnimationState();
    void LoadAnimations();
    void Initialize(bee::ai::StateMachineContext& context) override;
    void Update(bee::ai::StateMachineContext& context) override;
    void End(bee::ai::StateMachineContext& context) override;

private:
    std::shared_ptr<bee::Model> m_model;
    std::shared_ptr<bee::SkeletalAnimation> m_animation;
};
REGISTER_STATE(AnimationState);

