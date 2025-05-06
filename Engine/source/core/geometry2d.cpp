#include "core/geometry2d.hpp"

#pragma warning(push)
#pragma warning(disable : 4267)
#include <cdt/CDT.h>
#pragma warning(pop)

#include <predicates/predicates.h>

#include <glm/gtx/norm.hpp>

using namespace bee;
using namespace geometry2d;
using namespace glm;

glm::vec2 geometry2d::RotateCounterClockwise(const glm::vec2& v, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

bool geometry2d::IsPointLeftOfLine(const vec2& point, const vec2& line1, const vec2& line2)
{
    double p[2] = {point.x, point.y};
    double la[2] = {line1.x, line1.y};
    double lb[2] = {line2.x, line2.y};
    return RobustPredicates::orient2d(p, la, lb) > 0;
}

bool geometry2d::IsPointRightOfLine(const vec2& point, const vec2& line1, const vec2& line2)
{
    double p[2] = {point.x, point.y};
    double la[2] = {line1.x, line1.y};
    double lb[2] = {line2.x, line2.y};
    return RobustPredicates::orient2d(p, la, lb) < 0;
}

bool geometry2d::IsClockwise(const Polygon& polygon)
{
    size_t n = polygon.size();
    assert(n > 2);
    float signedArea = 0.f;

    for (size_t i = 0; i < n; ++i)
    {
        const auto& p0 = polygon[i];
        const auto& p1 = polygon[(i + 1) % n];

        signedArea += (p0.x * p1.y - p1.x * p0.y);
    }

    // Technically we now have 2 * the signed area.
    // But for the "is clockwise" check, we only care about the sign of this number,
    // so there is no need to divide by 2.
    return signedArea < 0.f;
}

bool geometry2d::IsPointInsidePolygon(const vec2& point, const Polygon& polygon)
{
    // Adapted from: https://wrfranklin.org/Research/Short_Notes/pnpoly.html

    size_t i, j;
    size_t n = polygon.size();
    bool inside = false;

    for (i = 0, j = n - 1; i < n; j = i++)
    {
        if ((polygon[i].y > point.y != polygon[j].y > point.y) &&
            (point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
            inside = !inside;
    }

    return inside;
}

vec2 geometry2d::GetNearestPointOnLineSegment(const vec2& p, const vec2& segmentA, const vec2& segmentB)
{
    float t = dot(p - segmentA, segmentB - segmentA) / distance2(segmentA, segmentB);
    if (t <= 0) return segmentA;
    if (t >= 1) return segmentB;
    return (1 - t) * segmentA + t * segmentB;
}

vec2 geometry2d::GetNearestPointOnPolygonBoundary(const vec2& point, const Polygon& polygon)
{
    float bestDist = std::numeric_limits<float>::max();
    vec2 bestNearest(0.f, 0.f);

    size_t n = polygon.size();
    for (size_t i = 0; i < n; ++i)
    {
        const vec2& nearest = GetNearestPointOnLineSegment(point, polygon[i], polygon[(i + 1) % n]);
        float dist = distance2(point, nearest);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestNearest = nearest;
        }
    }

    return bestNearest;
}

bool geometry2d::IsPointInsidePolygon(const glm::vec3& point, const Polygon& polygon)
{
    // Adapted from: https://wrfranklin.org/Research/Short_Notes/pnpoly.html

    size_t i, j;
    size_t n = polygon.size();
    bool inside = false;

    for (i = 0, j = n - 1; i < n; j = i++)
    {
        if ((polygon[i].y > point.y != polygon[j].y > point.y) &&
            (point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
            inside = !inside;
    }

    return inside;
}

vec2 geometry2d::ComputeCenterOfPolygon(const Polygon& polygon)
{
    vec2 total(0, 0);
    for (const vec2& p : polygon) total += p;
    return total / (float)polygon.size();
}

std::vector<size_t> geometry2d::TriangulatePolygon(const Polygon& polygon)
{
    // build up an overall list of CDT vertices and edges
    std::vector<CDT::V2d<double>> vertices;
    std::vector<CDT::Edge> edges;
    size_t nrVertices = polygon.size();

    // Convert the polygon data to CDT input
    for (size_t i = 0; i < nrVertices; ++i)
    {
        vertices.push_back({polygon[i].x, polygon[i].y});
        edges.push_back({(CDT::VertInd)i, (CDT::VertInd)((i + 1) % nrVertices)});
    }

    // Triangulate using the CDT library.
    // The algorithm is incremental, so inserting vertices/edges will update the triangulation.
    CDT::Triangulation<double> cdt;
    cdt.insertVertices(vertices);
    cdt.insertEdges(edges);

    // Clean up the triangulation
    cdt.eraseOuterTrianglesAndHoles();

    // Convert back to our own list of vertex indices
    std::vector<size_t> result;
    for (const auto& triangle : cdt.triangles)
    {
        result.push_back(static_cast<size_t>(triangle.vertices[0]));
        result.push_back(static_cast<size_t>(triangle.vertices[1]));
        result.push_back(static_cast<size_t>(triangle.vertices[2]));
    }

    return result;
}

PolygonList geometry2d::TriangulatePolygons(const PolygonList& boundaries)
{
    // build up an overall list of CDT vertices and edges
    std::vector<CDT::V2d<double>> vertices;
    std::vector<CDT::Edge> edges;
    size_t nrVertices = 0;

    for (const Polygon& poly : boundaries)
    {
        // Convert the polygon data to CDT input
        size_t polySize = poly.size();
        for (size_t i = 0; i < polySize; ++i)
        {
            vertices.push_back({poly[i].x, poly[i].y});
            edges.push_back({(CDT::VertInd)(nrVertices + i), (CDT::VertInd)(nrVertices + (i + 1) % polySize)});
        }
        nrVertices += polySize;
    }

    // Triangulate using the CDT library.
    // The algorithm is incremental, so inserting vertices/edges will update the triangulation.
    CDT::Triangulation<double> cdt;
    cdt.insertVertices(vertices);
    cdt.insertEdges(edges);

    // Clean up the triangulation
    cdt.eraseOuterTrianglesAndHoles();

    // Convert back to our own polygon format
    PolygonList result;
    for (const auto& triangle : cdt.triangles)
    {
        const auto& v0 = cdt.vertices[triangle.vertices[0]];
        const auto& v1 = cdt.vertices[triangle.vertices[1]];
        const auto& v2 = cdt.vertices[triangle.vertices[2]];
        result.push_back({{(float)v0.x, (float)v0.y}, {(float)v1.x, (float)v1.y}, {(float)v2.x, (float)v2.y}});
    }

    return result;
}

bool bee::geometry2d::CircleLineIntersect(glm::vec2& origin, glm::vec2& destination, glm::vec2& circleOrigin, float& radius)
{
    glm::vec2 l_d = destination - origin;
    glm::vec2 l_f = origin - circleOrigin;
    float l_a = dot(l_d, l_d);
    float l_b = 2 * dot(l_f, l_d);
    float l_c = dot(l_f, l_f) - radius * radius;
    float l_discriminant = l_b * l_b - 4 * l_a * l_c;
    if (l_discriminant < 0)
    {
        return false;
    }
    else
    {
        // ray didn't totally miss sphere,
        // so there is a solution to
        // the equation.

        l_discriminant = sqrt(l_discriminant);

        // either solution may be on or off the ray so need to test both
        // t1 is always the smaller value, because BOTH discriminant and
        // a are nonnegative.
        float l_t1 = (-l_b - l_discriminant) / (2 * l_a);
        float l_t2 = (-l_b + l_discriminant) / (2 * l_a);

        // 3x HIT cases:
        //          -o->             --|-->  |            |  --|->
        // Impale(t1 hit,t2 hit), Poke(t1 hit,t2>1), ExitWound(t1<0, t2 hit),

        // 3x MISS cases:
        //       ->  o                     o ->              | -> |
        // FallShort (t1>1,t2>1), Past (t1<0,t2<0), CompletelyInside(t1<0, t2>1)

        if (l_t1 >= 0 && l_t1 <= 1)
        {
            // t1 is the intersection, and it's closer than t2
            // (since t1 uses -b - discriminant)
            // Impale, Poke
            // cout << "wrong";
            return true;
        }

        // here t1 didn't intersect so we are either started
        // inside the sphere or completely past it
        if (l_t2 >= 0 && l_t2 <= 1)
        {
            // ExitWound
            // cout << "wrong";
            return true;
        }
        // no intn: FallShort, Past, CompletelyInside
        return false;
    }
}

bool bee::geometry2d::IsWithingCircle(glm::vec2& point, glm::vec2& circleOrigin, float& radius)
{
    return (glm::length2(circleOrigin - point) <= radius * radius);
}