#include "ai/navigation_grid.hpp"
#include "core/engine.hpp"
#include "graph/graph_search.hpp"

bee::ai::NavigationPath bee::ai::NavigationGrid::ComputePath(glm::vec2 a, glm::vec2 b) const
{
    std::vector<glm::vec3> path;

    const size_t startIndex = m_graph.GetClosestVertexToPosition(glm::vec3(a, 0));
    const size_t goalIndex = m_graph.GetClosestVertexToPosition(glm::vec3(b,0));

    if (startIndex >= m_graph.GetNumberOfVertices()) 
        return {path};
    if (goalIndex >= m_graph.GetNumberOfVertices()) 
        return {path};

    const auto vertices = bee::graph::AStar(m_graph, static_cast<int>(startIndex),static_cast<int>(goalIndex), graph::AStarHeuristic_EuclideanDistance);

    for (const auto index : vertices)
    {
        path.push_back(m_graph.GetVertex(index).position);
    }

    return NavigationPath(path);
}

bee::ai::NavigationPath bee::ai::NavigationGrid::ComputePathManhattan(glm::vec2 a, glm::vec2 b) const
{
    std::vector<glm::vec3> path;

    const size_t startIndex = m_graph.GetClosestVertexToPosition(glm::vec3(a, 0));
    const size_t goalIndex = m_graph.GetClosestVertexToPosition(glm::vec3(b, 0));

    if (startIndex >= m_graph.GetNumberOfVertices()) return {path};
    if (goalIndex >= m_graph.GetNumberOfVertices()) return {path};

    const auto vertices = bee::graph::AStar(m_graph, static_cast<int>(startIndex), static_cast<int>(goalIndex), graph::AStarHeuristic_ManhattanDistance);

    for (const auto index : vertices)
    {
        path.push_back(m_graph.GetVertex(index).position);
    }

    return NavigationPath(path);
}

bee::ai::NavigationPath bee::ai::NavigationGrid::ComputePathStraightLine(glm::vec2 a, glm::vec2 b) const
{
    std::vector<glm::vec3> path;

    const size_t startIndex = m_graph.GetClosestVertexToPosition(glm::vec3(a, 0));
    const size_t goalIndex = m_graph.GetClosestVertexToPosition(glm::vec3(b, 0));

    if (startIndex >= m_graph.GetNumberOfVertices()) return {path};
    if (goalIndex >= m_graph.GetNumberOfVertices()) return {path};

    const auto vertices = bee::graph::AStar(m_graph, static_cast<int>(startIndex), static_cast<int>(goalIndex), graph::AStarHeuristic_ManhattanDistance);

    auto xDist = std::abs(a.x - b.x);
    auto yDist = std::abs(a.y - b.y);

    if (xDist > yDist)
    {
        for (const auto index : vertices)
        {
            const glm::vec3 newVertex = glm::vec3(m_graph.GetVertex(index).position.x, a.y, m_graph.GetVertex(index).position.z);

            path.push_back(newVertex);
        }
    }
    else // (xDist <= yDist)
    {
        for (const auto index : vertices)
        {
            const glm::vec3 newVertex = glm::vec3(a.x, m_graph.GetVertex(index).position.y, m_graph.GetVertex(index).position.z);
            
            path.push_back(newVertex);
        }

    }

    return NavigationPath(path);
}

bee::ai::NavigationGrid::NavigationGrid(const glm::vec2& position, const size_t tileSize, const size_t sizeX,const size_t sizeY): m_startPosition(position), m_tileSize(tileSize), m_sizeX(sizeX), m_sizeY(sizeY)
{
    m_graph.ClearGraph();
    m_tileSize = tileSize;
    m_sizeX = sizeX;
    m_sizeY = sizeY;

    bee::geometry2d::PolygonList positions;
    for (int y = 0; y < m_sizeY; y++)
    {
        for (int x = 0; x < m_sizeX; x++)
        {
            glm::vec3 temp = glm::vec3(x, y, 0) * static_cast<float>(m_tileSize);
            temp += glm::vec3(position, 0.0f);
            m_graph.AddVertex(temp);
        }
    }

    for (int y = 0; y < m_sizeY; y++)
    {
        for (int x = 0; x < m_sizeX; x++)
        {
            const int index1D = y * sizeX + x;
            if (y != 0) m_graph.AddEdge(index1D, index1D - sizeX);
            if (y != sizeY - 1) m_graph.AddEdge(index1D, index1D + sizeX);
            if (x != 0) m_graph.AddEdge(index1D, index1D - 1);
            if (x != sizeX - 1) m_graph.AddEdge(index1D, index1D + 1);
            if (y != 0 && x != 0) m_graph.AddEdge(index1D, index1D - sizeX - 1);
            if (y != 0 && x != sizeX - 1) m_graph.AddEdge(index1D, index1D - sizeX + 1);
            if (y != sizeY - 1 && x != 0) m_graph.AddEdge(index1D, index1D + sizeX - 1);
            if (y != sizeY - 1 && x != sizeX - 1) m_graph.AddEdge(index1D, index1D + sizeX + 1);
        }
    }
}

void bee::ai::NavigationGrid::DebugDraw(DebugRenderer& renderer) const
{
    glm::vec4 color_graph(0.7f, 0.0f, 0.1f, 1.0f);
    for (size_t v = 0; v < m_graph.GetNumberOfVertices(); ++v)
    {
        const auto& p1 = m_graph.GetVertex((int)v).position;

        if (m_graph.GetVertex(v).traversable)
        {
            Engine.DebugRenderer().AddCircle(DebugCategory::AINavigation, p1, 0.2f, glm::vec4(0,1,0,1));
        }
        else
        {
            Engine.DebugRenderer().AddCircle(DebugCategory::AINavigation, p1, 0.2f, glm::vec4(1, 0, 0, 1));
        }

        for (const auto& edge : m_graph.GetEdgesFromVertex((int)v))
        {
            const auto& p2 = m_graph.GetVertex(edge.m_targetVertex).position;
            //Engine.DebugRenderer().AddLine(DebugCategory::AINavigation, p1, p2, color_graph);
        }
    }
}

void bee::ai::NavigationGrid::SetVertexPosition(const int index, const bee::graph::VertexWithPosition& v)
{
    m_graph.SetVertex(index, v);
}

glm::vec2 bee::ai::NavigationGrid::SampleWalkablePoint(glm::vec2 pos) const
{
    return m_graph.GetVertex(m_graph.GetClosestWalkableVertexToPosition(glm::vec3(pos, 0))).position;
}
