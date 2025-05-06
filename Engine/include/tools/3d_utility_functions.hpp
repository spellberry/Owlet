#pragma once
#include <optional>
#include <glm/fwd.hpp>

#include "core/fwd.hpp"

namespace bee
{
struct Camera;
struct Transform;
/// <summary>
    /// Constructs the View Matrix of the camera and returns it.
    /// </summary>
    /// <param name="transform">Camera's Transform component.</param>
    /// <returns>A copy of a newly constructed view matrix.</returns>
    glm::mat4 GetCameraViewMatrix(const bee::Transform& transform);

    /// <summary>
    /// Get a direction from the Camera position in world space to the Cursor in world space.
    /// </summary>
    /// <param name="screenPos">Cursor position in screen space.</param>
    /// <param name="camera">Camera's Camera component</param>
    /// <param name="camTransform">Camera's Transform Component</param>
    /// <returns>A unit vector representing the direction from the Camera position in world space to the Cursor in world
    /// space.</returns>
    glm::vec3 GetRayFromScreenToWorld(const glm::vec2& screenPos, const bee::Camera& camera,
                                      const bee::Transform& camTransform);

    /// <summary>
    /// Convert a point from 3D world space to 2D screen space.
    /// </summary>
    /// <param name="worldPos">The point in the 3D world you want to convert.</param>
    /// <returns>Screen space coordinates of the point.</returns>
    glm::vec2 FromWorldToScreen(const glm::vec3& worldPos);

    /// <summary>
    /// Checks if the mouse is in the Game Window.
    /// </summary>
    /// <returns>True if mouse is inside the Game Window. Will also return true if the Game window isn't focused but the mouse is still in its borders. </returns>
    bool IsMouseInGameWindow();

    /// \brief
    /// \param hitResponse
    /// \param rayOrigin
    /// \param length
    /// \param rayDirection
    /// \return It returns a bool representing the collision response with a ray given
    bool HitResponse(bee::Entity & hitResponse, const glm::vec3& rayOrigin, float length, const glm::vec3& rayDirection);
    
    /// \brief
    /// \param hitResponse
    /// \param rayOrigin
    /// \param length
    /// \param rayDirection
    /// \return It returns a bool representing the collision response with a ray given
    bool SelectionHitResponse(bee::Entity & hitResponse, const glm::vec3& rayOrigin, float length, const glm::vec3& rayDirection);
    
    /// \brief
    /// \param hitResponse
    /// \param rayOrigin
    /// \param length
    /// \param rayDirection
    /// \return It returns a bool representing the collision response with a ray given
    bool HitResponse2D(bee::Entity & hitResponse, const glm::vec2& rayOrigin, const glm::vec2& rayDirection);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="hitResponse"></param>
    /// <param name="rayFromTheCenterOfTheScreen"></param>
    /// <param name="selection"></param>
    /// <returns></returns>
    bool MouseHitResponse(bee::Entity& hitResponse, bool rayFromTheCenterOfTheScreen = false, bool selection = false);

    /// \brief
    /// \param pointA
    /// \param pointB
    /// \param hitResponse
    /// \return Returns a bool response for the collision between the entity collider and the ray given
    bool RayHitResponse(const glm::vec3& pointA, const glm::vec3& pointB, bee::Entity& hitResponse);

    /// \brief this will give the mouse world position and directional ray
    /// \param cameraWorldPosition
    /// \param mouseWorldDirection
    /// \param rayFromTheCenterOfTheScreen
    static void GetMousePositionDirection(glm::vec3 & cameraWorldPosition, glm::vec3 & mouseWorldDirection,bool rayFromTheCenterOfTheScreen=false);

    /// \brief
    /// \param objectPoint
    /// \param mouseWorldPosition
    /// \param rayDirection
    /// \return This returns the length of the projection of a given point on the ray
    float DistanceFromTheRay(const glm::vec3& objectPoint, const glm::vec3& mouseWorldPosition, const glm::vec3& rayDirection);

    /// \brief
    /// \param origin
    /// \param direction
    /// \param spherePosition
    /// \param sphereRadius
    /// \return This will give the exit/entrance points for the ray if the ray hit a sphere
    std::optional<std::pair<glm::vec3, glm::vec3>> SphereLineIntersection(
        const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& spherePosition, const float& sphereRadius);

    /// \brief 
    /// \return this returns the mouse position related to the game window
    glm::vec2 GetGameWindowPosition(const glm::vec2& screenPos);

    }
