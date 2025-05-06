#pragma once
#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/Utils/editor_variables.hpp"
#include "ai/Utils/generic_factory.hpp"

class RangedAttackState;

class TrainUnitState : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override;

    void Update(bee::ai::StateMachineContext& context) override;
    void End(bee::ai::StateMachineContext& context) override {}
};
REGISTER_STATE(TrainUnitState);

class BuildingDestroyedState : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override;

    void Update(bee::ai::StateMachineContext& context) override{

    };
    void End(bee::ai::StateMachineContext& context) override{

    };
};
REGISTER_STATE(BuildingDestroyedState);

class BuildingIdle : public bee::ai::State
{
public:
    void Initialize(bee::ai::StateMachineContext& context) override{};
    void Update(bee::ai::StateMachineContext& context) override{};
    void End(bee::ai::StateMachineContext& context) override{};
};
REGISTER_STATE(BuildingIdle);


class BeingBuilt : public bee::ai::State
{
public:
    SERIALIZE_FIELD(float, distanceToBuilding);
    void Initialize(bee::ai::StateMachineContext& context) override;


    void Build(const bee::ai::StateMachineContext& context);

    void Update(bee::ai::StateMachineContext& context) override { Build(context); };
    void End(bee::ai::StateMachineContext& context) override;
};
REGISTER_STATE(BeingBuilt);