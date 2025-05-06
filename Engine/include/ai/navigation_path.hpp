#pragma once
#include <list>
#include <vector>
#include <glm/vec2.hpp>

#include "graph/euclidean_graph.hpp"

namespace bee::ai
{
class NavigationPath
{
public:
    NavigationPath() = default;
    NavigationPath(std::vector<graph::VertexWithPosition>& nodesToSet);
    NavigationPath(const std::vector<glm::vec3>& pointsToSet);

    //Given a point return a t value of how far along the path the given point is
    float GetPercentageAlongPath(const glm::vec2& point) const;
    //get an arbitrary closest point on the path to a given point 
    glm::vec2 GetClosestPointOnPath(glm::vec2 point) const;
    //Given a t value- return a point in this percentage along the path
    glm::vec3 FindPointOnPath(float t) const;
    glm::vec2 FindPointOnPathWithOffset(float t, float offset) const;

    const std::vector<glm::vec3>& GetPoints() const{ return points; }
    std::vector<graph::VertexWithPosition>& GetGraphNodes() { return aiNodes; }
    bool IsEmpty() const { return points.empty(); }
    void EmptyPath() { points.clear(); }

private:
    std::vector<glm::vec3> points;
    std::vector<graph::VertexWithPosition> aiNodes;
};
}

