#include "graph/graph_search.hpp"

#include "glm/glm.hpp"

// Heuristic function: Euclidean distance between two vertices
std::function<float(const bee::graph::VertexWithPosition&, const bee::graph::VertexWithPosition&)>
    bee::graph::AStarHeuristic_EuclideanDistance =
        [](const bee::graph::VertexWithPosition& v1, const bee::graph::VertexWithPosition& v2)
{ 
    return glm::distance(v1.position, v2.position); 
};

std::function<float(const bee::graph::VertexWithPosition&, const bee::graph::VertexWithPosition&)>
    bee::graph::AStarHeuristic_ManhattanDistance =
        [](const bee::graph::VertexWithPosition& v1, const bee::graph::VertexWithPosition& v2)
{ 
    const float deltaX = std::fabsf(v2.position.x - v1.position.x);
    const float deltaY = std::fabsf(v2.position.y - v1.position.y);
    const float deltaZ = std::fabsf(v2.position.z - v1.position.z);
    return (deltaX + deltaY + deltaZ);
};