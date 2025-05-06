#pragma once

#include "core/geometry2d.hpp"
#include "graph/graph.hpp"

namespace bee::graph
{

/// <summary>
/// Represents a single vertex in a graph with a physical position.
/// </summary>
struct VertexWithPosition
{
    glm::vec3 position;
    VertexWithPosition(const glm::vec3& _position) : position(_position) {}
    bool traversable = true;
};

/// <summary>
/// Represents a graph with vertices in 2D Euclidean space, and with directed edges whose cost equal the Euclidean distance
/// between their endpoints.
/// </summary>
struct EuclideanGraph : public Graph<VertexWithPosition>
{
    void AddVertex(const glm::vec3& pos);
    void AddEdge(int vertex1, int vertex2, bool bidirectional = true);
    int GetClosestVertexToPosition(glm::vec3 pos) const;
    int GetClosestWalkableVertexToPosition(glm::vec3 pos) const;
    static EuclideanGraph CreateDualGraph(const geometry2d::PolygonList& polygons);
};

};  // namespace bee::graph