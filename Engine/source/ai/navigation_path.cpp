#include "ai/navigation_path.hpp"

#include <algorithm>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.inl>
#include "core/geometry2d.hpp"

bee::ai::NavigationPath::NavigationPath(std::vector<graph::VertexWithPosition>& nodesToSet)
{
    for (const graph::VertexWithPosition node : nodesToSet)
    {
        points.push_back(node.position);
    }

    aiNodes = nodesToSet;
}

bee::ai::NavigationPath::NavigationPath(const std::vector<glm::vec3>& pointsToSet)
{
    points = pointsToSet;
}

/**
 * \brief Given a point on the path, calculate the float from 0-1 being a percentage
 * of how far along the path the point is
 * \param point - a point on the path
 * \return - a float in range 0-1 indicating how far along the path the point is
 */
float bee::ai::NavigationPath::GetPercentageAlongPath(const glm::vec2& point) const
{
    if (points.size() < 2) return 0.0f;

    float totalLength = 0.0f;
    std::vector<float> accumulatedLengths(points.size());

    for (size_t i = 0; i < points.size() - 1; i++)
    {
        glm::vec2 delta = points[i + 1] - points[i];
        const float segmentLength = glm::length(delta);
        totalLength += segmentLength;
        accumulatedLengths[i + 1] = accumulatedLengths[i] + segmentLength;
    }

    int segmentIndex = 0;
    float closestDistance = std::numeric_limits<float>::max();

    //find the closest point to the given point
    for (int i = 0; i < points.size() - 1; i++)
    {
        auto closestPoint = bee::geometry2d::GetNearestPointOnLineSegment(point, points[i + 1], points[i]);
        const float distance = glm::distance(point, closestPoint);

        if (distance < closestDistance)
        {
            closestDistance = distance;
            segmentIndex = i;
        }
    }

    // Calculate the percentage along the closest segment.
    const float t = glm::dot(point - static_cast<glm::vec2>(points[segmentIndex]),static_cast<glm::vec2>(points[segmentIndex + 1]) - static_cast<glm::vec2>(points[segmentIndex])) /glm::length2(points[segmentIndex + 1] - points[segmentIndex]);
    // Calculate the accumulated length up to the closest segment.
    const float accumulatedLengthUpToSegment = accumulatedLengths[segmentIndex] + t * (accumulatedLengths[segmentIndex + 1] - accumulatedLengths[segmentIndex]);
    // Calculate the total percentage along the entire path.
    const float totalT = accumulatedLengthUpToSegment / totalLength;
    return totalT;
}

/**
 * \brief Given a disconnected point from a path return the closest point
 * on the path to the given point
 * \param point - an arbitrary point
 * \return - the closest point on the path
 */
glm::vec2 bee::ai::NavigationPath::GetClosestPointOnPath(glm::vec2 point) const
{
    if (points.size() <= 1) return {};
    std::vector<glm::vec2> closestPoints = {};

    for (size_t i = 0 ; i < points.size()-1;i++)
    {
        closestPoints.push_back(bee::geometry2d::GetNearestPointOnLineSegment(point, points[i], points[i + 1]));
    }

    std::sort(closestPoints.begin(), closestPoints.end(),[point](const glm::vec2& a, const glm::vec2& b)
    {
        return glm::distance2(a, point) < glm::distance2(b, point);
    });

    return closestPoints[0];
}


/**
 * \brief Given a float t, return a point in the given percentage along the path
 * in a similar fasion to lerp functions: t being < 0 yields the start of the path,
 * t being > 1 yields the end of the path and every number between yields a point on the path
 * \param t - a float (preferably a percentage along the path)
 * \return - a point on the path
 */
glm::vec3 bee::ai::NavigationPath::FindPointOnPath(float t) const
{
    if (points.empty()) return {0, 0,0};
    if (t <= 0.0f)
    {
        return points.front();  // Return the first point for position 0 or less
    }
    if (t >= 1.0f)
    {
        return points.back();  // Return the last point for position 1 or more
    }

    float totalLength = 0.0f;
    std::vector<float> segmentLengths;

    for (int i = 0; i < points.size() - 1; i++)
    {
        float segmentLength = glm::distance(static_cast<glm::vec2>(points[i]), static_cast<glm::vec2>(points[i + 1]));
        totalLength += segmentLength;
        segmentLengths.push_back(segmentLength);
    }

    const float targetLength = t * totalLength;

    // Find the segment containing the target length
    size_t segmentIndex = 0;
    float accumulatedLength = 0.0f;
    for (size_t i = 0; i < segmentLengths.size(); i++)
    {
        accumulatedLength += segmentLengths[segmentIndex];
        if (accumulatedLength >= targetLength)
        {
            break;
        }
        segmentIndex++;
    }

    if (segmentIndex >= points.size() - 1) return points.back();
    const float segmentT = (targetLength - accumulatedLength + segmentLengths[segmentIndex]) / segmentLengths[segmentIndex];
    return glm::mix(points[segmentIndex], points[segmentIndex + 1], segmentT);
}

glm::vec2 bee::ai::NavigationPath::FindPointOnPathWithOffset(float t, float offset) const
{ 
    if (points.empty()) return {0, 0};
    if (t <= 0.0f) return points.front();  // Return the first point for position 0 or less
    if (t >= 1.0f) return points.back();   // Return the last point for position 1 or more

    float totalLength = 0.0f;
    std::vector<float> segmentLengths;

    // Calculate segment lengths and total length
    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        float segmentLength = glm::distance(points[i], points[i + 1]);
        totalLength += segmentLength;
        segmentLengths.push_back(segmentLength);
    }

    const float targetLength = t * totalLength;

    // Find the segment containing the target length
    size_t segmentIndex = 0;
    float accumulatedLength = 0.0f;
    for (size_t i = 0; i < segmentLengths.size(); ++i)
    {
        accumulatedLength += segmentLengths[i];
        if (accumulatedLength >= targetLength)
        {
            segmentIndex = i;
            break;
        }
    }

    // Interpolate within the segment
    if (segmentIndex >= points.size() - 1) return points.back();
    const float segmentT = (targetLength - (accumulatedLength - segmentLengths[segmentIndex])) / segmentLengths[segmentIndex];
    glm::vec2 pointOnPath = glm::mix(points[segmentIndex], points[segmentIndex + 1], segmentT);

     const glm::vec2 direction = glm::normalize(points[segmentIndex + 1] - points[segmentIndex]);

    // Offset the point along the perpendicular vector
    pointOnPath += direction * offset;

    return pointOnPath;
}










