#include "quest_system/objectives/location_objective.hpp"

#include "actors/attributes.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "rendering/debug_render.hpp"

#include "core/engine.hpp"

using namespace bee;

void bee::LocationObjective::Update()
{
    currentObjectiveProgress = 0;

    if (team == Team::Ally)
    {
        auto unit_view = Engine.ECS().Registry.view<Transform, AttributesComponent, AllyUnit>();
        for (auto [entity, transform, attributes, allyTeam] : unit_view.each())
        {
            if (attributes.GetEntityType() == handle)
            {
                float distanceSquared = glm::distance2(transform.Translation, m_goToPosition);
                if (distanceSquared <= m_locationRadius * m_locationRadius)
                {
                    currentObjectiveProgress++;
                }
            }
        }
    }
    else if (team == Team::Enemy)
    {
        auto unit_view = Engine.ECS().Registry.view<Transform, AttributesComponent, EnemyUnit>();
        for (auto [entity, transform, attributes, allyTeam] : unit_view.each())
        {
            if (attributes.GetEntityType() == handle)
            {
                float distanceSquared = glm::distance2(transform.Translation, m_goToPosition);
                if (distanceSquared <= m_locationRadius * m_locationRadius)
                {
                    currentObjectiveProgress++;
                }
            }
        }
    }
    else if (team == Team::Neutral)
    {
        auto unit_view = Engine.ECS().Registry.view<Transform, AttributesComponent, NeutralUnit>();
        for (auto [entity, transform, attributes, allyTeam] : unit_view.each())
        {
            if (attributes.GetEntityType() == handle)
            {
                float distanceSquared = glm::distance2(transform.Translation, m_goToPosition);
                if (distanceSquared <= m_locationRadius * m_locationRadius)
                {
                    currentObjectiveProgress++;
                }
            }
        }
    }

    Objective::Update();
}

void bee::LocationObjective::Render()
{
    Engine.DebugRenderer().AddCircle(bee::DebugCategory::Gameplay, m_goToPosition, m_locationRadius,
                                     glm::vec4(0.0, 1.0, 0.0, 1.0));
}

void bee::LocationObjective::StartObjective()
{
    Objective::StartObjective();
    currentObjectiveProgress = 0;
}