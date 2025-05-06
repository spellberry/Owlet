#include "graph/euclidean_graph.hpp"

#include <glm/glm.hpp>
#include <map>
#include <glm/gtx/norm.hpp>

using Segment = std::pair<glm::vec2, glm::vec2>;
using namespace bee::graph;
using namespace bee::geometry2d;

void EuclideanGraph::AddVertex(const glm::vec3& pos) { Graph<VertexWithPosition>::AddVertex(VertexWithPosition(pos)); }

void EuclideanGraph::AddEdge(int vertex1, int vertex2, bool bidirectional)
{
    float cost = glm::distance(m_vertices[vertex1].position, m_vertices[vertex2].position);
    Graph<VertexWithPosition>::AddEdge(vertex1, vertex2, cost, bidirectional);
}

int EuclideanGraph::GetClosestVertexToPosition(glm::vec3 pos) const
{
    float smallestDistance = std::numeric_limits<float>::max();
    int index = 0;
    int minIndex = -1;
    for (auto element : m_vertices)
    {
        const float dist = glm::distance2(static_cast<glm::vec2>(pos), static_cast<glm::vec2>(element.position));
        if (dist < smallestDistance)
        {
            minIndex = index;
            smallestDistance = dist;
        }

        index++;
    }

    return minIndex;
}

int EuclideanGraph::GetClosestWalkableVertexToPosition(glm::vec3 pos) const
{
    float smallestDistance = std::numeric_limits<float>::max();
    int index = 0;
    int minIndex = -1;
    for (auto element : m_vertices)
    {
        const float dist = glm::distance2(static_cast<glm::vec2>(pos), static_cast<glm::vec2>(element.position));
        if (dist < smallestDistance && element.traversable)
        {
            minIndex = index;
            smallestDistance = dist;
        }

        index++;
    }

    return minIndex;
}

/// <summary>
/// A comparator used for sorting Segment objects in an std::map.
/// </summary>
struct SegmentCompare
{
    bool operator()(const Segment& a, const Segment& b) const
    {
        if (a.first.x == b.first.x)
        {
            if (a.first.y == b.first.y)
            {
                if (a.second.x == b.second.x) return a.second.y < b.second.y;

                return a.second.x < b.second.x;
            }
            return a.first.y < b.first.y;
        }
        return a.first.x < b.first.x;
    }
};

EuclideanGraph EuclideanGraph::CreateDualGraph(const PolygonList& polygons)
{
    EuclideanGraph G;

    std::map<Segment, size_t, SegmentCompare> sides;

    // create all vertices, and prepare for the creation of edges
    for (size_t p = 0; p < polygons.size(); ++p)
    {
        const Polygon& poly = polygons[p];

        // compute the center of the cell
        glm::vec3 center(0.f);
        for (const glm::vec2& pt : poly) center += glm::vec3(pt,1.0f);
        center /= poly.size();

        // add a vertex there
        G.AddVertex(center);

        // register its sides
        size_t n = poly.size();
        for (size_t i = 0; i < n; ++i) sides.insert({{poly[i], poly[(i + 1) % n]}, p});
    }

    // check for edges
    for (const auto& side : sides)
    {
        const Segment& segment = side.first;
        const size_t cell = side.second;

        // skip half the edges
        if (segment.first.x < segment.second.x || (segment.first.x == segment.second.x && segment.first.y < segment.second.y))
            continue;

        // find the opposite segment
        const auto& it = sides.find({segment.second, segment.first});
        if (it != sides.end()) G.AddEdge((int)cell, (int)it->second);
    }

    return G;
}