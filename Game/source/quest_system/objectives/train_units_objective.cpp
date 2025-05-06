#include "quest_system/objectives/train_units_objective.hpp"

#include "actors/units/unit_manager_system.hpp"
#include "core/engine.hpp"
using namespace bee;

bee::TrainUnitsObjective::TrainUnitsObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle,Team a_team)
    : Objective(a_name, a_quantity, a_handle,a_team)
{
    objectiveName = a_name;
    quantity = a_quantity;
    handle = a_handle;
}

void bee::TrainUnitsObjective::Update()
{
    if (state == QuestState::Completed) return;

    currentObjectiveProgress = 0;

    auto unit_view = bee::Engine.ECS().Registry.view<AttributesComponent, AllyUnit>();
    for (auto [entity, attribute, ally] : unit_view.each())
    {
        if (attribute.GetEntityType() == handle) currentObjectiveProgress++;
    }

    Objective::Update();
}

void bee::TrainUnitsObjective::StartObjective()
{
    Objective::StartObjective();
    currentObjectiveProgress = 0;
}