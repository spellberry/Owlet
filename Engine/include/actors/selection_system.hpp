#pragma once
#include <glm/vec3.hpp>

#include "actors/units/unit_manager_system.hpp"
#include "core/ecs.hpp"
#include "core/geometry2d.hpp"
#include "core/input.hpp"
#ifndef BEE_INSPECTOR
#include "user_interface/user_interface_structs.hpp"
#endif


struct Selected
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};

struct MidSelection
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};

struct SelectionCircle
{
    entt::entity myEntity = entt::null;
};

struct EnemyHovered
{
    char dummy = 'd';
};

class SelectionSystem final : public bee::System
{
public:
    SelectionSystem();
    void Update(float dt) override;
    void Render() override;

    void SetSelectionCallback(const std::function<void()>& func);
    void SetDeselectionCallback(const std::function<void()>& func);
#ifdef STEAM_API_WINDOWS
    bool ControllerInput();
#endif
#ifndef BEE_INSPECTOR
    bee::ui::UIElementID m_selection = entt::null;
    bee::ui::UIElementID m_selectionMiddle = entt::null;
#endif

    void Activate() { m_isActive = true; }
    void Deactivate() { m_isActive = false; }
    bool IsActive() { return m_isActive; }

    void EnableAllSelect() { m_allSelect = true; }
    void DisableAllSelect() { m_allSelect = false; }

    void SelectUnits(bool preSelection = true) const;
    void SelectAllUnits(bool preSelection = true) const;
    void DeselectUnits(bool preSelection = true) const;

    void AddHoveringForEnemy() const;
    static void RemoveHoveringForEnemy();

    bool hoveringUI = false;
    float m_circleRotationDelta =1.0f;

private:
    static void RenderUnitsSelection(const glm::vec4& color);
    void RenderSelectionShape(glm::vec4& color) const;

#ifdef STEAM_API_WINDOWS
    void ControllerSelectionUnits(bool preSelection = true) const;
    void SteamInputUpdateCycle(float dt);
#endif
    
    void WindowsInputUpdateCycle(float dt);
    static bool CircleRectangleCollision(const glm::vec2& circleOrigin, const float& radius,
                                         const bee::geometry2d::AABB& rectangle);
    glm::vec3 MouseHitResponse() const;
    std::shared_ptr<bee::Material> m_selectionMaterial{};
    std::shared_ptr<bee::Material> m_hoveredEnemyMaterial{};

    bool m_allSelect = false; // Indicate that you should select all units, not only allies. (used for editor).
    bool m_isActive = true;   // A flag used to activate/deactivate the system's Update;
    bool m_lastSelectionButton = false;
    bool m_heldLeftClick = false;
    bool m_addToSelected = false;
    bee::geometry2d::AABB m_rectangle = bee::geometry2d::AABB(glm::vec2(0.0f), glm::vec2(0.0f));
    glm::vec2 m_heldMouseStartWPosition = {};
    glm::vec3 m_heldMouseInitialHitResponse = {};
    glm::vec3 m_initialCameraOrientation = {},m_currentCameraOrientation={};

    bee::Input::KeyboardKey m_addToSelectedKey{};
    bee::Input::MouseButton m_selectionMouseButton{};
    bee::Input::MouseButton m_giveOrderMouseButton{};

    std::function<void()> m_selFunc;
    std::function<void()> m_desFunc;
    float m_radius = 0.0f;
    float m_radiusMultiplier = 10.0f;
    bool m_switchMode = false;
    bool m_controllerMode = false;
};