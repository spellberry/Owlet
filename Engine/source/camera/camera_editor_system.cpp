#include "camera/camera_editor_system.hpp"

// Core files
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "rendering/render_components.hpp"
#include "rendering/debug_render.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/inspector.hpp"

// Other
#include "actors/selection_system.hpp"

using namespace bee;

CameraSystemEditor::CameraSystemEditor() 
{
    // Creating camera
    const auto cameraEntity = bee::Engine.ECS().CreateEntity(); 
    auto& camera = bee::Engine.ECS().CreateComponent<bee::Camera>(cameraEntity);
        camera.Projection = glm::perspective(glm::radians(m_fov), 16.0f / 9.0f, 0.5f, m_farPlane);
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(cameraEntity);
        transform.Name = "Camera";  

    const glm::vec3 initialFront = glm::vec3(0.0f, 1.0f, 0.0f);
    const auto& view = glm::lookAt(transform.Translation, transform.Translation + initialFront, m_up);                        
    const auto& viewInv = glm::inverse(view);
    bee::Decompose(viewInv, transform.Translation, transform.Scale, transform.Rotation);

    transform.Translation += glm::vec3(0.0, -10.0f, 1.0f);
    camera.Front = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
    camera.Right = glm::normalize(glm::cross(camera.Front, m_up));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

    m_camera = cameraEntity;
    m_radius = glm::distance(transform.Translation, m_anchor);
}

void CameraSystemEditor::Update(float dt) 
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Updating VP
    const auto& viewMatrix = GetCameraViewMatrix(transform);
    camera.Projection = glm::perspective(glm::radians(m_fov), 16.0f / 9.0f, 0.5f, m_farPlane);
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

    // Polling input and moving camera
    if (!m_mode2) UpdateMode1(mouseCurrentPosition, dt);
    else UpdateMode2(mouseCurrentPosition, dt);

    // Checking if mode switch has occured
    static bool lastMode; 
    if (lastMode != m_mode2) Reset();
    lastMode = m_mode2;

    //AnchorDraw();
}

// ----------------------------------------
// Private Methods
// ----------------------------------------

// --- Important methods

void CameraSystemEditor::UpdateMode1(const glm::vec2& mouseCurrentPosition, const float dt) {
    if (!ImGui::IsAnyItemHovered() && IsMouseInGameWindow())
    {
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftAlt))
        {
            if (Engine.Input().GetMouseButton(Input::MouseButton::Left)) Rotate(mouseCurrentPosition, dt);
            
            if (Engine.Input().GetMouseButton(Input::MouseButton::Middle)) Pan(mouseCurrentPosition, dt);

            if (Engine.Input().GetMouseButton(Input::MouseButton::Right)) Raise(mouseCurrentPosition, dt);

            const auto offset = (m_mouseWheelLast - Engine.Input().GetMouseWheel());
            if (offset != 0.0f) Forward(mouseCurrentPosition, dt, offset);

            if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::F)) AnchorSet();

        }  // Holding Alt

        if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::F) && !Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftAlt))
            AnchorFocus();

        const auto offset = (m_mouseWheelLast - Engine.Input().GetMouseWheel());
        if (offset != 0.0f && !Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftAlt)) ChangeFOV(offset);

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftControl) && Engine.Input().GetMouseButton(Input::MouseButton::Middle))
            Orbit(mouseCurrentPosition, dt);

        m_mousePositionLast = mouseCurrentPosition;
        m_mouseWheelLast = Engine.Input().GetMouseWheel();
    }
}

void CameraSystemEditor::UpdateMode2(const glm::vec2& mouseCurrentPosition, const float dt)
{
    if (!ImGui::IsAnyItemHovered() && IsMouseInGameWindow())
    {
        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::W)) M2Translate(m_Yaxis, dt);

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::A)) M2Translate(-m_Xaxis, dt);

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::S)) M2Translate(-m_Yaxis, dt);

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::D)) M2Translate(m_Xaxis, dt);

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::Space)) M2Translate(m_up, dt);

        if (Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftShift)) M2Translate(-m_up, dt);

        if (Engine.Input().GetMouseButton(Input::MouseButton::Middle)) M2Orbit(mouseCurrentPosition, dt);

        if (!Engine.Input().GetKeyboardKey(Input::KeyboardKey::LeftControl))
        {
            const auto offset = (m_mouseWheelLast - Engine.Input().GetMouseWheel());
            if (offset != 0.0f) ChangeFOV(offset);
        }

        m_mousePositionLast = mouseCurrentPosition;
        m_mouseWheelLast = Engine.Input().GetMouseWheel();
    }
}

void CameraSystemEditor::Reset()
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
        transform.Translation = glm::vec3(0.0, -10.0f, 1.0f);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);
        camera.Projection = glm::perspective(glm::radians(m_fov), 16.0f / 9.0f, 0.5f, 200.0f);

    const glm::vec3 newFront = glm::vec3(0.0f, 1.0f, 0.0f);
    const auto& view = glm::lookAt(transform.Translation, transform.Translation + newFront, m_up);
    const auto& viewInv = glm::inverse(view);
    bee::Decompose(viewInv, transform.Translation, transform.Scale, transform.Rotation);

    camera.Front = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
    camera.Right = glm::normalize(glm::cross(camera.Front, m_up));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

    m_anchor = glm::vec3(0.0f);
    glm::vec3 m_Yaxis = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_Xaxis = glm::vec3(1.0f, 0.0f, 0.0f);
    m_radius = glm::distance(transform.Translation, m_anchor);
}

// --- Transform methods

void CameraSystemEditor::Rotate(const glm::vec2& mouseCurrentPosition, const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Offset
    const glm::vec2 mouseOffset = m_mousePositionLast - mouseCurrentPosition;

    // Rotation
    glm::vec3 rotationEuler = glm::eulerAngles(transform.Rotation);
    rotationEuler.x += mouseOffset.y * m_stepRotate * dt;  // pitch
    rotationEuler.z += mouseOffset.x * m_stepRotate * dt;  // yaw
    transform.Rotation = glm::quat(rotationEuler);

    // Updating local camera vectors
    camera.Front = transform.Rotation * m_up;
    camera.Right = -glm::normalize(glm::cross(camera.Front, m_up));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
}

void CameraSystemEditor::Pan(const glm::vec2& mouseCurrentPosition, const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Offset
    const glm::vec2 mouseOffset = m_mousePositionLast - mouseCurrentPosition;

    // Translation
    const float velocityX = mouseOffset.x * m_stepTranslate * dt;
    const float velocityY = mouseOffset.y * m_stepTranslate * dt;
    transform.Translation += camera.Right * velocityX;
    transform.Translation += camera.Up * velocityY;

    m_radius = glm::distance(transform.Translation, m_anchor);
}

void CameraSystemEditor::Raise(const glm::vec2& mouseCurrentPosition, const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Offset
    const glm::vec2 mouseOffset = m_mousePositionLast - mouseCurrentPosition;

    // Translation
    const float velocityZ = mouseOffset.y * m_stepTranslate * dt;  // Only taking the Y delta
    transform.Translation += m_up * velocityZ;

    m_radius = glm::distance(transform.Translation, m_anchor);
}

void CameraSystemEditor::Forward(const glm::vec2& mouseCurrentPosition, const float dt, const float offset)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    transform.Translation += camera.Front * m_stepMouseWheel * dt * offset;
    m_radius = glm::distance(transform.Translation, m_anchor);
}

void CameraSystemEditor::Orbit(const glm::vec2& mouseCurrentPosition, const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Offset & current radius
    const glm::vec2 mouseOffset = m_mousePositionLast - mouseCurrentPosition;

    // Rotate
    glm::vec3 rotationEuler = glm::eulerAngles(transform.Rotation);
    rotationEuler.x += mouseOffset.y * m_stepOrbit * dt;  // pitch
    rotationEuler.z += mouseOffset.x * m_stepOrbit * dt;  // yaw
    transform.Rotation = glm::quat(rotationEuler);

    camera.Front = transform.Rotation * m_up;
    camera.Right = -glm::normalize(glm::cross(camera.Front, m_up));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));

    // Translate
    transform.Translation = m_anchor + (camera.Front * m_radius);
}

// --- Anchor methods

void CameraSystemEditor::AnchorSet()
{
    std::vector<glm::vec3> polygon;
    const auto view = bee::Engine.ECS().Registry.view<bee::Transform, Selected>();
    for (auto [entity, transform, selected] : view.each()) polygon.push_back(transform.Translation);

    glm::vec3 total(0.0f, 0.0f, 0.0f);
    for (const glm::vec3& position : polygon) total += position;

    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);

    if (static_cast<int>(polygon.size()) > 0)
    {
        m_anchor = total / (float)polygon.size();
        m_radius = glm::distance(transform.Translation, m_anchor);
    }
}

void CameraSystemEditor::AnchorLookAt(const glm::vec3& currentPosition)
{
    const auto& view = glm::lookAt(currentPosition, glm::normalize(m_anchor - currentPosition), m_up);
    const auto& viewInv = glm::inverse(view);

    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);

    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);
        camera.Front = glm::normalize(m_anchor - currentPosition);
        camera.Right = -glm::normalize(glm::cross(camera.Front, m_up));
        camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));   
    bee::Decompose(viewInv, transform.Translation, transform.Scale, transform.Rotation);
}

void CameraSystemEditor::AnchorFocus()
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    AnchorLookAt(transform.Translation);

    // Translating camera
    const float distance = glm::distance(transform.Translation, m_anchor);
    transform.Translation += camera.Front * (distance - m_anchorDistance);

    m_radius = glm::distance(transform.Translation, m_anchor);
}

void CameraSystemEditor::AnchorDraw()
{
    Engine.DebugRenderer().AddLine(bee::DebugCategory::General, m_anchor + glm::vec3(-1.0f, 1.0f, 0.0f),
                                   m_anchor + glm::vec3(1.0f, -1.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    Engine.DebugRenderer().AddLine(bee::DebugCategory::General, m_anchor + glm::vec3(-1.0f, -1.0f, 0.0f),
                                   m_anchor + glm::vec3(1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

// --- Other methods

void CameraSystemEditor::ChangeFOV(const float offset)
{
    m_fov += offset;
    if (m_fov < m_minFov) m_fov = m_minFov;
    if (m_fov > m_maxFov) m_fov = m_maxFov;
}

// --- Mode 2 methods

void CameraSystemEditor::M2Translate(const glm::vec3& axis, float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    transform.Translation += axis * (m_fov / m_maxFov) * m_stepTranslate * dt;
    m_anchor += axis * (m_fov / m_maxFov) * m_stepTranslate * dt;
}

void CameraSystemEditor::M2Orbit(const glm::vec2& mouseCurrentPosition, const float dt)
{
    auto& transform = Engine.ECS().Registry.get<bee::Transform>(m_camera);
    auto& camera = Engine.ECS().Registry.get<bee::Camera>(m_camera);

    // Offset & current radius
    const glm::vec2 mouseOffset = m_mousePositionLast - mouseCurrentPosition;

    // Rotate
    glm::vec3 rotationEuler = glm::eulerAngles(transform.Rotation);
    rotationEuler.x += mouseOffset.y * m_stepOrbit * dt;  // pitch
    rotationEuler.z += mouseOffset.x * m_stepOrbit * dt;  // yaw

    if (rotationEuler.x <= 0.1f)
        rotationEuler.x = 0.1f;
    else if (rotationEuler.x >= 3.13f)
        rotationEuler.x = 3.13f;

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

void CameraSystemEditor::M2ChangeFOV(const float offset) 
{
    m_fov += offset;
    if (m_fov < m_minFov) m_fov = m_minFov;
    if (m_fov > m_maxFov) m_fov = m_maxFov;
}