#include "ai/grid_navigation_system.hpp"

#include <execution>

#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/navmesh_agent.hpp"
#include "core/transform.hpp"
#include "level_editor/level_editor_components.hpp"
#include "physics/physics_components.hpp"
#include <cfloat>
#include <cmath>

#include "animation/animation_state.hpp"
#include "tools/log.hpp"

void bee::ai::GridAgent::SetGoal(const glm::vec2& goalToSet, bool shouldRecomputePath)
{
    goal = goalToSet;
    recomputePath = shouldRecomputePath;
}

void bee::ai::GridAgent::ComputePath(bee::ai::NavigationGrid const& grid, const glm::vec2& currentPos)
{
    path = grid.ComputePath(currentPos, goal);
    recomputePath = false;
}

void bee::ai::GridAgent::CalculateVerticalPosition(const glm::vec3& currentPos, float dt)
{
    if (path.IsEmpty())
    {
        return;
    }

    const glm::vec2 agentPos2D = glm::vec2(currentPos.x, currentPos.y);
    const glm::vec3 referencePoint = glm::vec3(path.GetClosestPointOnPath(agentPos2D), 0);
    const float referencePointT = path.GetPercentageAlongPath(referencePoint);
    verticalPosition = path.FindPointOnPath(referencePointT).z;
}

void bee::ai::GridAgent::ComputePreferredVelocity(const glm::vec3& currentPos, float dt)
{
    const float normalTravelDist = speed * dt;

    if (path.IsEmpty())
    {
        preferredVelocity = {0.f, 0.f};
        return;
    }

    if (glm::distance2(static_cast<glm::vec2>(currentPos), static_cast<glm::vec2>(path.GetPoints().back())) < normalTravelDist)
    {
        path = NavigationPath();    
        preferredVelocity = {0.f, 0.f};
        return;
    }

    const glm::vec2 agentPos2D = glm::vec2(currentPos.x, currentPos.y);

    const glm::vec2 referencePoint = glm::vec3(path.GetClosestPointOnPath(agentPos2D), 0);
    const float referencePointT = path.GetPercentageAlongPath(referencePoint);
    const glm::vec2 attractionPoint = path.FindPointOnPathWithOffset(referencePointT+0.005f,1.0f);
    preferredVelocity = glm::normalize(glm::vec2(attractionPoint) - glm::vec2(currentPos)) * speed;
}

bee::ai::GridNavigationSystem::GridNavigationSystem(float fixedDeltaTime, const bee::ai::NavigationGrid& grid)
    : m_fixedDeltaTime(fixedDeltaTime), m_grid(grid)
{
}

void bee::ai::GridNavigationSystem::Update(float dt)
{
    System::Update(dt);
    m_timeSinceLastFrame += dt;

    const auto& view = bee::Engine.ECS().Registry.view<GridAgent, bee::physics::Body, bee::Transform>();

    if (m_timeSinceLastFrame >= m_fixedDeltaTime)
    {
        std::for_each(std::execution::par,view.begin(), view.end(),[view,this](const auto entity) 
            {
                  auto& body = view.get<bee::physics::Body>(entity);
                  auto& agent = view.get<GridAgent>(entity);
                  auto& transform = view.get<bee::Transform>(entity);
                  const glm::vec2& pos = glm::vec2(body.GetPosition());
                      if (agent.recomputePath) agent.ComputePath(m_grid, pos);

                      transform.Translation.z = agent.verticalPosition;
                      agent.ComputePreferredVelocity(glm::vec3(pos, 0), m_fixedDeltaTime);
                      transform.Translation.z = glm::mix(transform.Translation.z, agent.verticalPosition + 0.9f, 0.9f);
            });

        for(const auto&[entity1, agent1, body1, transform1] : view.each())
{
            for (const auto& [entity2, agent2, body2, transform2] : view.each())
            {
                if (entity1 == entity2) continue;

                Separation(agent1.m_detectionRadius, transform1.Translation, transform2.Translation, agent1.preferredVelocity);
            }
}
        m_timeSinceLastFrame -= m_fixedDeltaTime;
    }

    std::for_each(std::execution::par, view.begin(), view.end(),[view, this,dt](const auto entity) 
    {
          auto& body = view.get<bee::physics::Body>(entity);
          auto& agent = view.get<GridAgent>(entity);
          auto& transform = view.get<bee::Transform>(entity);
          const glm::vec2& pos = glm::vec2(body.GetPosition());
          agent.CalculateVerticalPosition(transform.Translation, dt);
          transform.Translation.z = agent.verticalPosition;
    });

    // link agents to physics
    for (const auto& [entity, agent, body, transform] : view.each())
    {
        body.SetLinearVelocity(agent.preferredVelocity);
        const auto animationAgent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(entity);
        if (animationAgent)
        {
            animationAgent->context.blackboard->SetData<float>("MoveSpeed", glm::length(body.GetLinearVelocity()));
        }
        if (glm::length2(body.GetLinearVelocity()) == 0.0f) continue;
        if (agent.path.IsEmpty()) continue;
        const glm::vec2 normalizedDir = glm::normalize(body.GetLinearVelocity());
        float angle = glm::atan(normalizedDir.y, normalizedDir.x);
        transform.Rotation = glm::slerp(transform.Rotation, glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f)), dt * 10.0f);
    }

    m_grid.DebugDraw(bee::Engine.DebugRenderer());
}

void bee::ai::GridNavigationSystem::Render()
{
    auto view = bee::Engine.ECS().Registry.view<GridAgent, bee::physics::Body>();
    for (auto entity : view)
    {
        auto& agent = view.get<GridAgent>(entity);
        auto& path = agent.path;
        auto& points = path.GetPoints();
        if (path.IsEmpty()) continue;
        for (size_t i = 0; i < points.size() - 1; i++)
        {
            bee::Engine.DebugRenderer().AddLine(DebugCategory::Enum::AIDecision, points[i], points[i + 1],
                                                glm::vec4(1, 0, 0, 1));
        }

        bee::Engine.DebugRenderer().AddCircle(DebugCategory::AIDecision, points.back(), 1.0f, glm::vec4(1, 0, 0, 1));
    }
}

void bee::ai::GridNavigationSystem::UpdateFromTerrain()
{
    auto view = Engine.ECS().Registry.view<lvle::TerrainDataComponent>();
    for (auto entity : view)
    {
        auto [data] = view.get(entity);
        for (int i = 0; i < data.m_tiles.size(); i++)
        {
            auto& tile = data.m_tiles[i];
            graph::VertexWithPosition v = graph::VertexWithPosition(tile.centralPos);
            v.traversable = !(tile.tileFlags & lvle::TileFlags::NoGroundTraverse);
            m_grid.SetVertexPosition(i, v);
        }
    }
}

void bee::ai::GridNavigationSystem::Separation(const float detectionRadius, 
                                               const glm::vec2& currentPos, 
                                               const glm::vec2& otherAgentPos, 
                                               glm::vec2& preferredVelocity) const
{
    //calculating how far we are to the other unit
    float distanceToAgent = glm::distance(currentPos, otherAgentPos);

    //if agent is outside of detection radius - no need to separate
    if(distanceToAgent >= detectionRadius)
    {
       return;
    }

    //if two agents are close to eachother, we separate them

    glm::vec2 rayToAgent = otherAgentPos - currentPos;
    if (rayToAgent.x == 0.0f && rayToAgent.y == 0.0f) return;

    //apply a stronger separation force the closer the agent is to another
    preferredVelocity += glm::mix(glm::vec2(0.0f), glm::normalize(-rayToAgent), (1.0f - (distanceToAgent / detectionRadius)));
}
