#include "camera/camera_rts_system.hpp"

#include "core/audio.hpp"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/transform.hpp"
#include "rendering/debug_render.hpp"
#include "rendering/render_components.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/inspector.hpp"

#ifdef STEAM_API_WINDOWS
#include "tools/steam_input_system.hpp"
#endif

using namespace bee;
using namespace std;

CameraSystemRTS::CameraSystemRTS()
{
    // Creating camera
    const auto cameraEntity = bee::Engine.ECS().CreateEntity();
    auto& camera = bee::Engine.ECS().CreateComponent<bee::Camera>(cameraEntity);
        camera.Projection = glm::perspective(glm::radians(m_fov), 16.0f / 9.0f, 0.5f, 600.0f);
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(cameraEntity);
        transform.Name = "Camera";
        transform.Translation += glm::vec3(-115.0f, -97.0f, 89.0f);
        transform.Rotation = glm::quat(0.769f, 0.474f, -0.188f, -0.385f);

    camera.Front = transform.Rotation * m_up;
    camera.Right = -glm::normalize(glm::cross(camera.Front, m_up));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

    m_camera = cameraEntity;
    m_radius = glm::distance(transform.Translation, m_anchor);
    m_anchor = glm::vec3(0.0f, 0.0f, 18.0f);

    // Updating world axis
    m_Xaxis = camera.Right;
    m_Yaxis = -glm::normalize(glm::cross(camera.Right, m_up));
}

void CameraSystemRTS::ResetCamera()
{
    // Creating camera
    auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(m_camera);
        transform.Translation = glm::vec3(-115.0f, -97.0f, 89.0f);
        transform.Rotation = glm::quat(0.769f, 0.474f, -0.188f, -0.385f);

    auto& camera = bee::Engine.ECS().Registry.get<bee::Camera>(m_camera);
        camera.Front = transform.Rotation * m_up;
        camera.Right = -glm::normalize(glm::cross(camera.Front, m_up));
        camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

    // Updating world axis
    m_Xaxis = camera.Right;
    m_Yaxis = -glm::normalize(glm::cross(camera.Right, m_up));
}

void CameraSystemRTS::Update(const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Updating VP
    const auto& viewMatrix = GetCameraViewMatrix(transform);
    camera.Projection = glm::perspective(glm::radians(m_fov), 16.0f / 9.0f, 0.5f, 600.0f);
    camera.VP = camera.Projection * viewMatrix;

    // Updating screen width
    const auto scrWidth = Engine.Inspector().GetGameSize().x;
    const auto scrHeight = Engine.Inspector().GetGameSize().y;

    // Getting mouse position
    glm::vec2 mouseCurrentPosition = Engine.Input().GetMousePosition();
    GetGameWindowPosition(mouseCurrentPosition);
    mouseCurrentPosition = (mouseCurrentPosition - 0.5f * glm::vec2(scrWidth, scrHeight));
    mouseCurrentPosition.x /= scrWidth;
    mouseCurrentPosition.y /= scrHeight;

    bool slideCurrent = false;
    bee::Engine.Audio().UpdateListenerPosition(transform.Translation, {0, 0, 0}, camera.Front, camera.Up);

#ifdef BEE_INSPECTOR
    if (!ImGui::IsAnyItemHovered() && IsMouseInGameWindow() && !m_LERP)
#else
    if (!m_LERP)
#endif
    {
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::W)) Translate(m_Yaxis, dt);
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::A)) Translate(-m_Xaxis, dt);
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::S)) Translate(-m_Yaxis, dt);
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::D)) Translate(m_Xaxis, dt);
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftShift)) Translate(-m_up, dt);
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::Space)) Translate(m_up, dt);

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::Q)) Orbit(-0.01f, dt);
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::E)) Orbit(0.01f, dt);
        if (Engine.Input().GetMouseButton(Input::MouseButton::Middle)) Orbit(mouseCurrentPosition, dt);

        //if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::C)) Return(glm::vec3(-115.0f, -97.0f, 89.0f));

        const auto offset = (m_mouseWheelLast - Engine.Input().GetMouseWheel());
        if (offset != 0.0f) ChangeFOV(offset);

        m_mousePositionLast = mouseCurrentPosition;
        m_mouseWheelLast = Engine.Input().GetMouseWheel();
        m_slideLast = slideCurrent;

        ApplyMomentum(dt);
    }
    else if (m_LERP)
        LERP(dt);

    //AnchorDraw();
}

void CameraSystemRTS::Translate(const glm::vec3& axis, float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);

    m_velocity += axis * (m_fov / m_maxFov) * m_stepTranslate * dt;
}

void CameraSystemRTS::Orbit(const glm::vec2& mouseCurrentPosition, const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Offset & current radius
    const glm::vec2 mouseOffset = m_mousePositionLast - mouseCurrentPosition;

    // Rotate
    glm::vec3 rotationEuler = glm::eulerAngles(transform.Rotation);
    rotationEuler.x += mouseOffset.y * (m_fov / m_maxFov) * m_stepOrbit * dt;  // pitch
    rotationEuler.z += mouseOffset.x * (m_fov / m_maxFov) * m_stepOrbit * dt;  // yaw

    if (rotationEuler.x <= 0.1f)
        rotationEuler.x = 0.1f;
    else if (rotationEuler.x >= 1.3f)
        rotationEuler.x = 1.3f;

    transform.Rotation = glm::quat(rotationEuler);

    camera.Front = transform.Rotation * m_up;
    camera.Right = -glm::normalize(glm::cross(camera.Front, m_up));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

    // Translate
    transform.Translation = m_anchor + (camera.Front * m_radius);

    // Updating world axis
    m_Xaxis = camera.Right;
    m_Yaxis = -glm::normalize(glm::cross(camera.Right, m_up));
}

void CameraSystemRTS::Orbit(const float angle, const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Rotate
    glm::vec3 rotationEuler = glm::eulerAngles(transform.Rotation);
    rotationEuler.z += angle * m_stepOrbit * dt;  // yaw

    transform.Rotation = glm::quat(rotationEuler);

    camera.Front = transform.Rotation * m_up;
    camera.Right = -glm::normalize(glm::cross(camera.Front, m_up));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

    // Translate
    transform.Translation = m_anchor + (camera.Front * m_radius);

    // Updating world axis
    m_Xaxis = camera.Right;
    m_Yaxis = -glm::normalize(glm::cross(camera.Right, m_up));
}

void CameraSystemRTS::Return(const glm::vec3& returnTo)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
        transform.Rotation = glm::quat(0.769f, 0.474f, -0.188f, -0.385f);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    m_fov = 5.0f;

    // Setting LERP formula values
    m_start = transform.Translation;
    m_end = returnTo + (camera.Front * m_radius);
    m_LERP = true;
}

void CameraSystemRTS::LERP(const float dt)
{
    // Using higher pericision formula
    // a * (1.0 - f) + (b * f)
    // a = initial camera position
    // b = target camera position
    // f = lerpAmount

    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);

    // Updating positions
    const glm::vec3 value = (m_start * (1.0f - m_lerpAmount) + (m_end * m_lerpAmount));
    transform.Translation -= value;

    // Updating coefficant
    m_lerpAmount += dt;
    if (m_lerpAmount >= 1.0f)
    {
        m_anchor = glm::vec3(0.0f, 0.0f, 18.0f);
        m_radius = glm::distance(transform.Translation, m_anchor);
        m_lerpAmount = 0.0f;
        m_LERP = false;
    }
}

void CameraSystemRTS::ChangeFOV(const float offset)
{
    m_fov += offset;
    if (m_fov < m_minFov) m_fov = m_minFov;
    if (m_fov > m_maxFov) m_fov = m_maxFov;
}

void CameraSystemRTS::ApplyMomentum(const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);

    // Applying velocity to make camera accelerate / decelerate
    transform.Translation += m_velocity * dt;
    m_anchor += m_velocity * dt;

    // slow down
    m_velocity -= m_velocity * (m_drag * dt);

    // Capping maximum velocity
    if (m_velocity.x > m_maxSpeed) m_velocity.x = m_maxSpeed;
    if (m_velocity.y > m_maxSpeed) m_velocity.y = m_maxSpeed;
    if (m_velocity.z > m_maxSpeed) m_velocity.z = m_maxSpeed;

    if (m_velocity.x < -m_maxSpeed) m_velocity.x = -m_maxSpeed;
    if (m_velocity.y < -m_maxSpeed) m_velocity.y = -m_maxSpeed;
    if (m_velocity.z < -m_maxSpeed) m_velocity.z = -m_maxSpeed;

    // Height caps
    if (transform.Translation.z < 40.0f)
    {
        transform.Translation.z = 40.0f;
        m_velocity.z = 0.0f;
    }
    if (transform.Translation.z > 200.0f)
    {
        transform.Translation.z = 200.0f;
        m_velocity.z = 0.0f;
    }

}

void CameraSystemRTS::AnchorDraw()
{
    Engine.DebugRenderer().AddLine(bee::DebugCategory::General, m_anchor + glm::vec3(-1.0f, 1.0f, 0.0f),
                                   m_anchor + glm::vec3(1.0f, -1.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    Engine.DebugRenderer().AddLine(bee::DebugCategory::General, m_anchor + glm::vec3(-1.0f, -1.0f, 0.0f),
                                   m_anchor + glm::vec3(1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}