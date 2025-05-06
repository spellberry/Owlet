#pragma once

#include <stdexcept>
#include <vector>
#include <glm/geometric.hpp>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

/// <summary>
/// A namespace containing functions related to 2D geometry.
/// </summary>
namespace bee::geometry2d
{
// Input data types
using Polygon = std::vector<glm::vec2>;
using PolygonList = std::vector<Polygon>;

static Polygon ScalePolygon(Polygon& polygon,float scale)
{
    if (polygon.empty())
    {
        throw std::invalid_argument("Polygon cannot be empty");
    }

    size_t size = polygon.size();
    float centroidX = 0, centroidY = 0;

    for (const glm::vec2& p : polygon)
    {
        centroidX += p.x;
        centroidY += p.y;
    }

    centroidX /= static_cast<float>(size);
    centroidY /= static_cast<float>(size);

    Polygon scaledPolygon;
    scaledPolygon.reserve(size);

    for (const glm::vec2& p : polygon)
    {
        const float translatedX = p.x - centroidX;
        const float translatedY = p.y - centroidY;

        const float scaledX = translatedX * scale;
        const float scaledY = translatedY * scale;

        float finalX = scaledX + centroidX;
        float finalY = scaledY + centroidY;

        scaledPolygon.emplace_back(finalX, finalY);
    }

    return scaledPolygon;
}


static float PolygonPerimeter(const std::vector<glm::vec2>& polygon)
{
    float perimeter = 0.0;
    const size_t numVertices = polygon.size();
    for (size_t i = 0; i < numVertices; ++i)
    {
        perimeter += glm::distance(polygon[i], polygon[(i + 1) % numVertices]);
    }
    return perimeter;
}

 static glm::vec2 GetRandomPointOnEdge(const glm::vec2& p1, const glm::vec2& p2)
{
    const float t = static_cast<float>(rand()) / RAND_MAX;
    float x = p1.x + t * (p2.x - p1.x);
    float y = p1.y + t * (p2.y - p1.y);
    return {x, y};
}



 static glm::vec2 GetRandomPointOnPolygonEdge(const std::vector<glm::vec2>& polygon)
{
    if (polygon.size() < 2)
    {
        throw std::invalid_argument("Polygon must have at least 2 vertices");
    }

    const float totalPerimeter = PolygonPerimeter(polygon);
    const float randomDistance = static_cast<float>(rand()) / RAND_MAX * totalPerimeter;

    float currentDistance = 0.0;
    const size_t n = polygon.size();
    for (size_t i = 0; i < n; ++i)
    {
        glm::vec2 p1 = polygon[i];
        glm::vec2 p2 = polygon[(i + 1) % n];
        const float edgeLength = glm::distance(p1, p2);

        if (currentDistance + edgeLength >= randomDistance)
        {
            return GetRandomPointOnEdge(p1, p2);
        }

        currentDistance += edgeLength;
    }

    return {};
}

/// <summary>
/// Simple struct representing an axis-aligned bounding box.
/// </summary>
struct AABB
{
private:
    glm::vec2 m_min;
    glm::vec2 m_max;

public:
    AABB(const glm::vec2& minPos, const glm::vec2& maxPos) : m_min(minPos), m_max(maxPos) {}

    /// <summary>
    /// Computes and returns the four boundary vertices of this AABB.
    /// </summary>
    Polygon ComputeBoundary() const { return {m_min, glm::vec2(m_max.x, m_min.y), m_max, glm::vec2(m_min.x, m_max.y)}; }

    /// <summary>
    /// Computes and returns the center coordinate of this AABB.
    /// </summary>
    glm::vec2 ComputeCenter() const { return (m_min + m_max) / 2.f; }

    /// <summary>
    /// Computes and returns the size of this AABB, wrapped in a vec2 (x=width, y=height).
    /// </summary>
    /// <returns></returns>
    glm::vec2 ComputeSize() const { return m_max - m_min; }

    const glm::vec2& GetMin() const { return m_min; }
    const glm::vec2& GetMax() const { return m_max; }

    /// <summary>
    /// Checks and returns whether this AABB overlaps with another AABB.
    /// </summary>
    /// <param name="other">The AABB to compare to.</param>
    /// <returns>true if the AABBs overlap, false otherwise.</returns>
    bool OverlapsWith(const AABB& other)
    {
        return m_max.x >= other.m_min.x && other.m_max.x >= m_min.x && m_max.y >= other.m_min.y && other.m_max.y >= m_max.y;
    }
};

glm::vec2 RotateCounterClockwise(const glm::vec2& v, float angle);

/// <summary>
/// Checks and returns whether a 2D point lies strictly to the left of an infinite directed line.
/// </summary>
/// <param name="point">A query point.</param>
/// <param name="line1">A first point on the query line.</param>
/// <param name="line2">A second point on the query line.</param>
/// <returns>true if "point" lies strictly to the left of the infinite directed line through line1 and line2;
/// false otherwise (i.e. if the point lies on or to the right of the line).</return>
bool IsPointLeftOfLine(const glm::vec2& point, const glm::vec2& line1, const glm::vec2& line2);

/// <summary>
/// Checks and returns whether a 2D point lies strictly to the right of an infinite directed line.
/// </summary>
/// <param name="point">A query point.</param>
/// <param name="line1">A first point on the query line.</param>
/// <param name="line2">A second point on the query line.</param>
/// <returns>true if "point" lies strictly to the right of the infinite directed line through line1 and line2;
/// false otherwise (i.e. if the point lies on or to the left of the line).</return>
bool IsPointRightOfLine(const glm::vec2& point, const glm::vec2& line1, const glm::vec2& line2);

/// <summary>
/// Checks and returns whether the points of a simple 2D polygon are given in clockwise order.
/// </summary>
/// <param name="polygon">A list of 2D points describing the boundary of a simple polygon (i.e. at least 3 points, nonzero area,
/// not self-intersecting).</param> <returns>true if the points are given in clockwise order, false otherwise.</returns>
bool IsClockwise(const Polygon& polygon);

/// <summary>
/// Checks and returns whether a given point lies inside a given 2D polygon.
/// </summary>
/// <param name="point">A query point.</param>
/// <param name="polygon">A simple 2D polygon.</param>
/// <returns>true if the point lies inside the polygon, false otherwise.</return>
bool IsPointInsidePolygon(const glm::vec2& point, const Polygon& polygon);

/// <summary>
/// Checks and returns whether a given point lies inside a given 2D polygon.
/// </summary>
/// <param name="point">A query point.</param>
/// <param name="polygon">A simple 2D polygon.</param>
/// <returns>true if the point lies inside the polygon, false otherwise.</return>
bool IsPointInsidePolygon(const glm::vec3& point, const Polygon& polygon);

/// <summary>
/// Computes and returns the centroid of a given polygon (= the average of its boundary points).
/// </summary>
glm::vec2 ComputeCenterOfPolygon(const Polygon& polygon);

/// <summary>
/// Computes and returns the nearest point on a line segment segmentA-segmentB to another point p.
/// </summary>
/// <param name="p">A query point.</param>
/// <param name="segmentA">The first endpoint of a line segment.</param>
/// <param name="segmentB">The second endpoint of a line segment.</param>
/// <returns>The point on the line segment segmentA-segmentB that is closest to p.</returns>
glm::vec2 GetNearestPointOnLineSegment(const glm::vec2& p, const glm::vec2& segmentA, const glm::vec2& segmentB);

/// <summary>
/// Computes and returns the nearest point on a polygon boundary to a given point.
/// </summary>
/// <param name="point">A query point.</param>
/// <param name="polygon">A polygon.</param>
/// <returns>The point on the boundary of 'polygon' that is closest to 'point'.</returns>
glm::vec2 GetNearestPointOnPolygonBoundary(const glm::vec2& point, const Polygon& polygon);

/// <summary>
/// Triangulates a simple polygon, and returns the triangulation as vertex indices.
/// </summary>
/// <param name="polygon">The input polygon.</param>
/// <returns>A list of vertex indices representing the triangulation of the input polygon.</returns>
std::vector<size_t> TriangulatePolygon(const Polygon& polygon);

/// <summary>
/// Triangulates a set of polygons, and returns the triangulation as a new set of polygons.
/// </summary>
/// <param name="polygon">A list of simple polygons. They are allowed to overlap.</param>
/// <returns>A list of polygons representing the triangulation of the union of all input polygons.</returns>
PolygonList TriangulatePolygons(const PolygonList& polygon);

/// <summary>
/// Returns a bool that indicates if a give segment intersect a given circle
/// </summary>
/// <param name="origin"></param>
/// <param name="destination"></param>
/// <param name="circleOrigin"></param>
/// <param name="radius"></param>
/// <returns>A bool that represents wether of not the line intersect the circle</returns>
bool CircleLineIntersect(glm::vec2& origin, glm::vec2& destination, glm::vec2& circleOrigin, float& radius);

/// <summary>
/// Returns wether of nor a give point is within a give circle
/// </summary>
/// <param name="point"></param>
/// <param name="circleOrigin"></param>
/// <param name="radius"></param>
/// <returns>A bool that represents wether or not the point is within the circle</returns>
bool IsWithingCircle(glm::vec2& point, glm::vec2& circleOrigin, float& radius);

static AABB GetPolygonBounds(bee::geometry2d::Polygon& polygon)
{
    glm::vec2 min = glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    glm::vec2 max = glm::vec2(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

    for (auto element : polygon)
    {
        min = glm::min(element, min);
        max = glm::max(element, max);
    }

    return AABB(min, max);
}
};  // namespace bee::geometry2d