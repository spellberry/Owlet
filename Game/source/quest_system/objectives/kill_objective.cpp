#include "quest_system/objectives/kill_objective.hpp"
#include "ai/FiniteStateMachines/finite_state_machine.hpp"
//#include "actors/actor_wrapper.hpp"
#include "actors/attributes.hpp"
#include "ai_behaviors/unit_behaviors.hpp"
#include "core/engine.hpp"

using namespace bee;

bee::KillObjective::KillObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle,
                                  const Team a_team)
    : Objective(a_name, a_quantity, a_handle, a_team)
{
    objectiveName = a_name;
    quantity = a_quantity;
    handle = a_handle;
    team = a_team;
}

void bee::KillObjective::Update()
{
    if (state == QuestState::Completed) return;

    // TODO: PLEASE REWORK THIS, VLAD! (by Vlad)
    // I want feedback on that. How can I make it so I don't copy-paste code?
    if (team == Team::Ally)
    {
        auto unit_view = Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent, AllyUnit>();
        int view_size = unit_view.size_hint();
        if (m_aliveTargetUnits > view_size)
        {
            int deadHandleUnitCnt = 0;
            for (auto [entity, attribute, agent, enemyTeam] : unit_view.each())
            {
                if (attribute.GetEntityType() == handle) deadHandleUnitCnt++;
            }
            if (m_handleUnitCnt != deadHandleUnitCnt)
                currentObjectiveProgress += m_aliveTargetUnits - view_size;
        }
        m_aliveTargetUnits = view_size;
        m_handleUnitCnt = 0;
        for (auto [entity, attribute, agent, enemyTeam] : unit_view.each())
            if (attribute.GetEntityType() == handle) m_handleUnitCnt++;
        // TODO: Same logic for structures
    }
    else if (team == Team::Enemy)
    {
        auto unit_view = Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent, EnemyUnit>();
        int view_size = unit_view.size_hint();
        if (m_aliveTargetUnits > view_size)
        {
            int deadHandleUnitCnt = 0;
            for (auto [entity, attribute, agent, enemyTeam] : unit_view.each())
            {
                if (attribute.GetEntityType() == handle) deadHandleUnitCnt++;
            }
            if (m_handleUnitCnt != deadHandleUnitCnt) currentObjectiveProgress += m_aliveTargetUnits - view_size;
        }
        m_aliveTargetUnits = view_size;
        m_handleUnitCnt = 0;
        for (auto [entity, attribute, agent, enemyTeam] : unit_view.each())
            if (attribute.GetEntityType() == handle) m_handleUnitCnt++;
        // TODO: Same logic for structures
    }
    else if (team == Team::Neutral)
    {
        auto unit_view = Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent, NeutralUnit>();
        int view_size = unit_view.size_hint();
        if (m_aliveTargetUnits > view_size)
        {
            int deadHandleUnitCnt = 0;
            for (auto [entity, attribute, agent, enemyTeam] : unit_view.each())
            {
                if (attribute.GetEntityType() == handle) deadHandleUnitCnt++;
            }
            if (m_handleUnitCnt != deadHandleUnitCnt) currentObjectiveProgress += m_aliveTargetUnits - view_size;
        }
        m_aliveTargetUnits = view_size;
        m_handleUnitCnt = 0;
        for (auto [entity, attribute, agent, enemyTeam] : unit_view.each())
            if (attribute.GetEntityType() == handle) m_handleUnitCnt++;
        // TODO: Same logic for structures
    }

    Objective::Update();
}

void bee::KillObjective::StartObjective()
{
    Objective::StartObjective();
    currentObjectiveProgress = 0;
}