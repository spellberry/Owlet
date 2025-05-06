#include "quest_system/objectives/collect_objective.hpp"

#include "actors/props/resource_system.hpp"
#include "actors/units/unit_template.hpp"
#include "ai/ai_behavior_selection_system.hpp"
#include "core/engine.hpp"
#include "magic_enum/magic_enum.hpp"

using namespace bee;

bee::CollectObjective::CollectObjective(const std::string& a_name, const int a_quantity, const std::string& a_handle,const Team a_team)
    : Objective(a_name, a_quantity, a_handle,a_team)
{
    objectiveName = a_name;
    quantity = a_quantity;
    handle = a_handle;
}

void bee::CollectObjective::Update()
{
    if (state == QuestState::Completed) return;

    currentObjectiveProgress = 0;


    auto maybeResourceType = magic_enum::enum_cast<GameResourceType>(handle);
    if (!maybeResourceType.has_value())
    {
        // Handle the case where the string does not correspond to any enum value
        return;
    }
    GameResourceType targetType = maybeResourceType.value();
    currentObjectiveProgress = bee::Engine.ECS().GetSystem<ResourceSystem>().playerResourceData.resources[targetType];

    Objective::Update();
}

void bee::CollectObjective::StartObjective()
{
    Objective::StartObjective();
    currentObjectiveProgress = 0;
}

