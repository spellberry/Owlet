#include "quest_system/objectives/build_objective.hpp"

#include "actors/structures/structure_manager_system.hpp"
#include "core/engine.hpp"

using namespace bee;

bee::BuildObjective::BuildObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle,
                                    const Team a_team)
    : Objective(a_name, a_quantity, a_handle, a_team)
{
    objectiveName = a_name;
    quantity = a_quantity;
    handle = a_handle;
    team = a_team;
}

void bee::BuildObjective::Update()
{
    if (state == QuestState::Completed) return;

    if (team == Team::Ally)
    {
        currentObjectiveProgress = 0;
        auto structure_view = Engine.ECS().Registry.view<AttributesComponent, AllyStructure>();
        for (auto [entity, attributes, allyTeam] : structure_view.each())
        {
            if (attributes.GetEntityType() == handle) currentObjectiveProgress++;
        }
    }
    else if (team == Team::Enemy)
    {
        currentObjectiveProgress = 0;
        auto structure_view = Engine.ECS().Registry.view<AttributesComponent, EnemyStructure>();
        for (auto [entity, attributes, allyTeam] : structure_view.each())
        {
            if (attributes.GetEntityType() == handle) currentObjectiveProgress++;
        }
    }
    else if (team == Team::Neutral)
    {
        currentObjectiveProgress = 0;
        auto structure_view = Engine.ECS().Registry.view<AttributesComponent, NeutralStructure>();
        for (auto [entity, attributes, allyTeam] : structure_view.each())
        {
            if (attributes.GetEntityType() == handle) currentObjectiveProgress++;
        }
    }

    Objective::Update();
}

void bee::BuildObjective::StartObjective()
{
    Objective::StartObjective();
    currentObjectiveProgress = 0;
}