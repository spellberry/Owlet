#include "tools/3d_utility_functions.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "actors/attributes.hpp"
#include "level_editor/terrain_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/transform.hpp"
#include "level_editor/terrain_system.hpp"
#include "physics/physics_components.hpp"
#include "rendering/render_components.hpp"
#include "tools/inspector.hpp"
#include <actors/selection_system.hpp>

using namespace bee;

glm::mat4 bee::GetCameraViewMatrix(const bee::Transform& transform)
{
    glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), transform.Scale);
    view *= glm::inverse(scaleMatrix);
    const glm::mat4 rotationMatrix = glm::mat4_cast(glm::inverse(transform.Rotation));
    view *= rotationMatrix;
    const glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -transform.Translation);
    view *= translationMatrix;

    return view;
}

glm::vec3 bee::GetRayFromScreenToWorld(const glm::vec2& screenPos, const bee::Camera& camera,
                                       const bee::Transform& camTransform)
{
    // convert to game window coordinates and sizes
    auto scrWidth = Engine.Inspector().GetGameSize().x;
    auto scrHeight = Engine.Inspector().GetGameSize().y;
    glm::vec2 gScreenPos = glm::vec2(0.0f, 0.0f);  // g for game
    gScreenPos.x = screenPos.x - Engine.Inspector().GetGamePos().x;
    gScreenPos.y = screenPos.y - Engine.Inspector().GetGamePos().y;

    // do ray casting
    glm::mat4 inverted = glm::inverse(camera.Projection * GetCameraViewMatrix(camTransform));
    glm::vec4 aNear = glm::vec4((gScreenPos.x - (scrWidth / 2)) / (scrWidth / 2),
                                 -1 * (gScreenPos.y - (scrHeight / 2)) / (scrHeight / 2), -1.0, 1.0);
    glm::vec4 aFar = glm::vec4((gScreenPos.x - (scrWidth / 2)) / (scrWidth / 2),
                                -1 * (gScreenPos.y - (scrHeight / 2)) / (scrHeight / 2), 1.0, 1.0);
    glm::vec4 nearResult = inverted * aNear;
    glm::vec4 farResult = inverted * aFar;
    nearResult /= nearResult.w;
    farResult /= farResult.w;
    glm::vec3 direction = glm::vec3(farResult - nearResult);
    glm::vec3 rayDirection = glm::normalize(direction);
    return rayDirection;
}

glm::vec2 bee::FromWorldToScreen(const glm::vec3& worldPos)
{
    auto screenSize = glm::vec2(Engine.Inspector().GetGameSize().x, Engine.Inspector().GetGameSize().y);

    auto view = bee::Engine.ECS().Registry.view<Camera, Transform>();
    for (auto [entity, camera, transform] : view.each())
    {

        // Transform world coordinates to clip space coordinates
        glm::vec4 clipSpacePos = camera.VP * glm::vec4(worldPos, 1.0);

        // Perform perspective divide to transform to normalized device coordinates (NDC)
        glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

        // Transform NDC to screen coordinates
        glm::vec2 screenPos = (glm::vec2(ndcSpacePos) + glm::vec2(1.0f)) * 0.5f * glm::vec2(screenSize);

        // Flip y coordinate (if necessary, depending on the coordinate system)
        screenPos.y = screenSize.y - screenPos.y;

        return screenPos;
    }

    return {};
}

bool bee::IsMouseInGameWindow()
{
    // convert to game window coordinates and sizes
    const auto scrWidth = Engine.Inspector().GetGameSize().x;
    const auto scrHeight = Engine.Inspector().GetGameSize().y;
    const auto mousePos = Engine.Input().GetMousePosition();
    glm::vec2 gScreenPos;  // g for game
    gScreenPos.x = mousePos.x - Engine.Inspector().GetGamePos().x;
    gScreenPos.y = mousePos.y - Engine.Inspector().GetGamePos().y;
    bool inWindow = false;
    if (gScreenPos.x > 0 && gScreenPos.x < scrWidth && gScreenPos.y > 0 && gScreenPos.y < scrHeight) inWindow = true;

    return inWindow;
}

bool bee::HitResponse(bee::Entity& hitResponse, const glm::vec3& rayOrigin, float length, const glm::vec3& rayDirection)
{
    const auto view = bee::Engine.ECS().Registry.view<bee::physics::DiskCollider, Transform, AttributesComponent>();

    bool hit = false;
    glm::vec3 direction1;
    glm::vec3 direction2;
    for (auto [entity, collider, transform, baseAttributes] : view.each())
    {
        const float distance = DistanceFromTheRay(transform.Translation, rayOrigin, rayDirection);
        if (distance < collider.radius)
        {
            if (auto intersection = SphereLineIntersection(rayOrigin, rayDirection, transform.Translation, collider.radius);
                intersection.has_value())
            {
                direction1 = intersection->first - rayOrigin;
                direction2 = intersection->second - rayOrigin;
                const float l1 = glm::length(direction1);
                const float l2 = glm::length(direction2);
                if (l1 < length)
                {
                    direction1 = glm::normalize(direction1) - glm::normalize(rayDirection);
                    if (glm::length(direction1) < 0.0001f)
                    {
                        length = l1;
                        hitResponse = entity;
                        hit = true;
                    }
                }
                if (l2 < length)
                {
                    direction2 = glm::normalize(direction2) - glm::normalize(rayDirection);
                    if (glm::length(direction2) < 0.0001f)
                    {
                        length = l2;
                        hitResponse = entity;
                        hit = true;
                    }
                }
            }
        }
    }
    return hit;
}

bool bee::SelectionHitResponse(bee::Entity& hitResponse, const glm::vec3& rayOrigin, float length, const glm::vec3& rayDirection)
{
    const auto view = bee::Engine.ECS().Registry.view<Transform, AttributesComponent,EnemyUnit>();

    bool hit = false;
    glm::vec3 direction1;
    glm::vec3 direction2;
    for (auto [entity, transform, baseAttributes,enemy] : view.each())
    {
        const float distance = DistanceFromTheRay(transform.Translation, rayOrigin, rayDirection);
        if (distance < baseAttributes.GetValue(BaseAttributes::SelectionRange))
        {
            if (auto intersection = SphereLineIntersection(rayOrigin, rayDirection, transform.Translation, baseAttributes.GetValue(BaseAttributes::SelectionRange));
                intersection.has_value())
            {
                direction1 = intersection->first - rayOrigin;
                direction2 = intersection->second - rayOrigin;
                const float l1 = glm::length(direction1);
                const float l2 = glm::length(direction2);
                if (l1 < length)
                {
                    direction1 = glm::normalize(direction1) - glm::normalize(rayDirection);
                    if (glm::length(direction1) < 0.0001f)
                    {
                        length = l1;
                        hitResponse = entity;
                        hit = true;
                    }
                }
                if (l2 < length)
                {
                    direction2 = glm::normalize(direction2) - glm::normalize(rayDirection);
                    if (glm::length(direction2) < 0.0001f)
                    {
                        length = l2;
                        hitResponse = entity;
                        hit = true;
                    }
                }
            }
        }
    }
    return hit;
}

bool bee::HitResponse2D(bee::Entity& hitResponse, const glm::vec2& rayOrigin, const glm::vec2& rayDirection)
{
    const auto view = bee::Engine.ECS().Registry.view<Selected, bee::physics::DiskCollider, Transform, AttributesComponent>();

    for (auto [entity, selected, disk, transform, attributes] : view.each())
    {
        auto f = rayOrigin - static_cast<glm::vec2>(transform.Translation);
        auto r = disk.radius;

        float a = glm::dot(rayDirection, rayDirection);
        float b = 2.0f * glm::dot(f, rayDirection);
        float c = glm::dot(f, f) - r * r;

        float discrimant = b * b - 4.0f * a * c;

        if(discrimant < 0)
        {
            continue;
            //return false;
        }

        float t1 = (-b - discrimant) / (2.0f * a);
        float t2 = (-b + discrimant) / (2.0f * a);

        if(t1 >= 0.0f && t1 <= 1.0f)
        {
            hitResponse = entity;
            return true;
        }
        
        if(t2 >= 0.0f && t2 <= 1.0f)
        {
            hitResponse = entity;
            return true;
        }

        //return false;
    }
    return false;
}

bool bee::MouseHitResponse(bee::Entity& hitResponse, bool rayFromTheCenterOfTheScreen, bool selection)
{
    glm::vec3 rayOrigin = glm::vec3(0.0f), rayDirection = glm::vec3(0.0f);
    GetMousePositionDirection(rayOrigin, rayDirection,rayFromTheCenterOfTheScreen);
    return selection ? HitResponse(hitResponse, rayOrigin, 10000, rayDirection) : SelectionHitResponse(hitResponse, rayOrigin, 10000, rayDirection);
}

bool bee::RayHitResponse(const glm::vec3& pointA, const glm::vec3& pointB, bee::Entity& hitResponse)
{
    const float length = glm::length(pointB - pointA) + 0.001f;
    const glm::vec3 direction = glm::normalize(pointB - pointA);
    return HitResponse(hitResponse, pointA, length, direction);
}

void bee::GetMousePositionDirection(glm::vec3& cameraWorldPosition, glm::vec3& mouseWorldDirection,bool rayFromTheCenterOfTheScreen)
{
    const auto screen = bee::Engine.ECS().Registry.view<bee::Transform, bee::Camera>();
    const auto& input = bee::Engine.Input();
    for (auto [entity, transform, camera] : screen.each())
    {
        if (!rayFromTheCenterOfTheScreen)
        mouseWorldDirection = bee::GetRayFromScreenToWorld(input.GetMousePosition(), camera, transform);
        else
        {
            const auto scrWidth = bee::Engine.Inspector().GetGameSize().x;
            const auto scrHeight = bee::Engine.Inspector().GetGameSize().y;
            glm::vec2 mousePos = {scrWidth * 0.5f + bee::Engine.Inspector().GetGamePos().x,
                                  scrHeight * 0.5f + bee::Engine.Inspector().GetGamePos().y};
            mouseWorldDirection = bee::GetRayFromScreenToWorld(mousePos, camera, transform);
        }
        cameraWorldPosition = transform.Translation;
    }
}

float bee::DistanceFromTheRay(const glm::vec3& objectPoint, const glm::vec3& mouseWorldPosition, const glm::vec3& rayDirection)
{
    return glm::length(glm::cross(objectPoint - mouseWorldPosition, rayDirection)) / glm::length(rayDirection);
}

std::optional<std::pair<glm::vec3, glm::vec3>> bee::SphereLineIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                                           const glm::vec3& spherePosition,
                                                                           const float& sphereRadius)
{
    const glm::vec3 m = origin - spherePosition;
    const float b = glm::dot(m, direction);
    const float c = glm::dot(m, m) - sphereRadius * sphereRadius;

    // Calculate discriminant
    const float discriminant = b * b - c;

    if (discriminant < 0)
    {
        // No intersection
        return {};
    }

    const float sqrtDiscriminant = std::sqrt(discriminant);
    const float t1 = -b - sqrtDiscriminant;
    const float t2 = -b + sqrtDiscriminant;

    // Calculate intersection points
    glm::vec3 intersection1 = origin + direction * t1;
    glm::vec3 intersection2 = origin + direction * t2;

    return std::make_pair(intersection1, intersection2);
}

glm::vec2 bee::GetGameWindowPosition(const glm::vec2& screenPos)
{  // convert to game window coordinates and sizes
    auto scrWidth = Engine.Inspector().GetGameSize().x;
    auto scrHeight = Engine.Inspector().GetGameSize().y;
    glm::vec2 gScreenPos = glm::vec2(0.0f, 0.0f);  // g for game
    gScreenPos.x = screenPos.x - Engine.Inspector().GetGamePos().x;
    gScreenPos.y = screenPos.y - Engine.Inspector().GetGamePos().y;
    return gScreenPos;
}

