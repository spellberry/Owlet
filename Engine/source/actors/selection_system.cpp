#include "actors/selection_system.hpp"

#include "actors/units/unit_manager_system.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "level_editor/terrain_system.hpp"
#include "physics/physics_components.hpp"
#include "rendering/debug_render.hpp"
#include "tools/3d_utility_functions.hpp"
#ifdef STEAM_API_WINDOWS
#include "tools/steam_input_system.hpp"
#endif
#include "actors/structures/structure_manager_system.hpp"
#include "material_system/material_system.hpp"
#include "platform/pc/core/device_pc.hpp"
#include "tools/tools.hpp"

#include "tools/inspector.hpp"

#ifndef BEE_INSPECTOR
#include "user_interface/user_interface_serializer.hpp"
#include "user_interface/user_interface_editor_structs.hpp"
#endif

SelectionSystem::SelectionSystem()
{
    m_addToSelectedKey = bee::Input::KeyboardKey::LeftShift;
    m_selectionMouseButton = bee::Input::MouseButton::Left;
    m_giveOrderMouseButton = bee::Input::MouseButton::Right;
    m_selectionMaterial = bee::Engine.Resources().Load<bee::Material>("materials/SelectionCircle.pepimat");
    m_hoveredEnemyMaterial = bee::Engine.Resources().Load<bee::Material>("materials/EnemyCircle.pepimat");
    m_initialCameraOrientation = glm::vec3(1.0f, 0.0f, 0.0f);

#ifndef BEE_INSPECTOR
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    m_selection = UI.serialiser->LoadElement("Selection");
    m_selectionMiddle = UI.serialiser->LoadElement("SelectionMiddle");
    UI.SetElementOpacity(m_selectionMiddle, 0.5);
    UI.SetElementLayer(m_selection,11);
    UI.SetElementLayer(m_selectionMiddle, 12);
    UI.SetDrawStateUIelement(m_selection, false);
    UI.SetDrawStateUIelement(m_selectionMiddle, false);
#endif

}

void SelectionSystem::Update(float dt)
{
    if (m_isActive)
    {
        const auto screen = bee::Engine.ECS().Registry.view<bee::Transform, bee::Camera>();
        for (auto [entity, transform, camera] : screen.each())
        {
            glm::quat cameraWorldOrientation = transform.Rotation;
            m_currentCameraOrientation =glm::normalize(glm::rotate(cameraWorldOrientation, m_initialCameraOrientation));
        }


        const auto circlesView = bee::Engine.ECS().Registry.view<SelectionCircle, bee::Transform>();

        for (auto circle : circlesView)
        {
            auto& transform = circlesView.get<bee::Transform>(circle);

            float angle = m_circleRotationDelta * dt;
            glm::quat incrementalRotation = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::quat newOrientation = incrementalRotation * transform.Rotation;
            newOrientation = glm::normalize(newOrientation);
            transform.Rotation = newOrientation;
        }
        if (!hoveringUI)
        {
        
#ifdef STEAM_API_WINDOWS
        if (bee::Engine.SteamInputSystem().IsActive())
            SteamInputUpdateCycle(dt);
#else
         WindowsInputUpdateCycle(dt);
#endif
        }
        else
        {
            hoveringUI = false;
        }
    }
}

void SelectionSystem::Render()
{
    if (m_isActive)
    {
        glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        RenderSelectionShape(color);
    }
}

void SelectionSystem::RenderUnitsSelection(const glm::vec4& color)
{
    const auto viewSelected = bee::Engine.ECS().Registry.view<bee::physics::DiskCollider, Selected, bee::Transform>();

    // goes to render function
    for (auto [entity, collider, selection, transform] : viewSelected.each())
    {
        bee::Engine.DebugRenderer().AddCircle(bee::DebugCategory::General, transform.Translation, collider.radius, color);
    }

    const auto viewMidSelection = bee::Engine.ECS().Registry.view<bee::physics::DiskCollider, MidSelection, bee::Transform>();

    // goes to render function
    for (auto [entity, collider, selection, transform] : viewMidSelection.each())
    {
        bee::Engine.DebugRenderer().AddCircle(bee::DebugCategory::General, transform.Translation, collider.radius, color);
    }
}

void SelectionSystem::RenderSelectionShape(glm::vec4& color) const
{
    if (m_lastSelectionButton)
    {
#ifdef BEE_INSPECTOR
        if (bee::IsMouseInGameWindow())
        {
            color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

            if (!m_controllerMode)
            {
                const glm::vec2 a = m_rectangle.GetMin();
                const glm::vec2 c = m_rectangle.GetMax();
                const glm::vec2 b = glm::vec2(c.x, a.y);
                const glm::vec2 d = glm::vec2(a.x, c.y);

                bee::Engine.DebugRenderer().AddLine2D(bee::DebugCategory::General, a, b, color);
                bee::Engine.DebugRenderer().AddLine2D(bee::DebugCategory::General, b, c, color);
                bee::Engine.DebugRenderer().AddLine2D(bee::DebugCategory::General, c, d, color);
                bee::Engine.DebugRenderer().AddLine2D(bee::DebugCategory::General, a, d, color);
            }
            else
            {
                bee::Engine.DebugRenderer().AddCircle(bee::DebugCategory::General, glm::vec3(m_heldMouseStartWPosition, 1.0f),
                                                      m_radius, color);
            }
        }
#else

        auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
        glm::vec2 a = m_rectangle.GetMin();
        glm::vec2 c = m_rectangle.GetMax();

        const float height = static_cast<float>(bee::Engine.Device().GetHeight());
        const float width = static_cast<float>(bee::Engine.Device().GetWidth());
        const float norWidth = width / height;

        a = glm::vec2((a.x / width) * norWidth, a.y / height);
        c = glm::vec2((c.x / width) * norWidth, c.y / height);

        UI.SetDrawStateUIelement(m_selection, true);
        UI.SetDrawStateUIelement(m_selectionMiddle, true);
        const float size = 0.002f;
        auto& middle = UI.getComponentItem<bee::ui::Image>(m_selectionMiddle, "Middle");
        auto& up = UI.getComponentItem<bee::ui::Image>(m_selection, "Up");
        auto& down = UI.getComponentItem<bee::ui::Image>(m_selection, "Down");
        auto& left = UI.getComponentItem<bee::ui::Image>(m_selection, "Left");
        auto& right = UI.getComponentItem<bee::ui::Image>(m_selection, "Right");

        glm::vec2 sizeProportion = glm::vec2(c.x - a.x - size, c.y - a.y - size);

        up.start = a + glm::vec2(size, 0.0f);
        up.size = glm::vec2(sizeProportion.x, size);
        UI.DeleteComponent(up.ID);
        up.Remake(m_selection);

        down.start = glm::vec2(a.x, c.y - size);
        down.size = glm::vec2(sizeProportion.x, size);
        UI.DeleteComponent(down.ID);
        down.Remake(m_selection);

        left.start = a;
        left.size = glm::vec2(size, sizeProportion.y);
        UI.DeleteComponent(left.ID);
        left.Remake(m_selection);

        right.start = glm::vec2(c.x - size, a.y+size);
        right.size = glm::vec2(size, sizeProportion.y);
        UI.DeleteComponent(right.ID);
        right.Remake(m_selection);

        middle.start = a;
        middle.size = c - a;
        UI.DeleteComponent(middle.ID);
        middle.Remake(m_selectionMiddle);
#endif
    }
#ifndef BEE_INSPECTOR
    else
    {
        auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
        UI.SetDrawStateUIelement(m_selection, false);
        UI.SetDrawStateUIelement(m_selectionMiddle, false);
    }
#endif

    if (m_controllerMode)

    {
        const glm::vec2 screenCenter = bee::GetGameWindowPosition(
            glm::vec2(bee::Engine.Inspector().GetGameSize() / 2.0f + bee::Engine.Inspector().GetGamePos().x));
        const glm::vec2 a = screenCenter - glm::vec2(10.0f, 0.0f), b = screenCenter + glm::vec2(10.0f, 0.0f),
                        c = screenCenter - glm::vec2(0.0f, 10.0f), d = screenCenter + glm::vec2(0.0f, 10.0f);
        bee::Engine.DebugRenderer().AddLine2D(bee::DebugCategory::General, a, b, color);
        bee::Engine.DebugRenderer().AddLine2D(bee::DebugCategory::General, c, d, color);
    }
}

void SelectionSystem::SelectUnits(const bool preSelection) const
{
   const auto alliedUnits = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform,AllyUnit>();
    const auto alliedStructures = bee::Engine.ECS().Registry.view<AttributesComponent, AllyStructure, bee::Transform>();
    auto selectionModel = bee::Engine.Resources().Load<bee::Model>("models/SelectionPlane.glb");
    bool selectOnlyStructures = false;
    for (auto [entity, attributes, transform,unit] : alliedUnits.each())
    {
        const glm::vec2 circleOrigin = bee::FromWorldToScreen(transform.Translation);
        const glm::vec2 selectionPoint =bee::FromWorldToScreen(transform.Translation + m_currentCameraOrientation * static_cast<float>(attributes.GetValue(BaseAttributes::SelectionRange)));
        if (CircleRectangleCollision(circleOrigin, glm::length(selectionPoint - circleOrigin), m_rectangle))
        {
            if (!preSelection)
            {
                const Selected* selected = bee::Engine.ECS().Registry.try_get<Selected>(entity);
                if (selected == nullptr) bee::Engine.ECS().CreateComponent<Selected>(entity);
                auto seletionEntity= bee::Engine.ECS().CreateEntity();
                bee::Engine.ECS().CreateComponent<SelectionCircle>(seletionEntity);
                auto& selectionTransform =bee::Engine.ECS().CreateComponent<bee::Transform>(seletionEntity);
                selectionTransform.SetParent(entity);
                selectionTransform.Translation.z = 0.255f;
                selectionTransform.Scale *=bee::Engine.ECS().Registry.get<AttributesComponent>(entity).GetValue(BaseAttributes::SelectionRange);
                selectionModel->Instantiate(seletionEntity);
                auto& mesh = bee::GetComponentInChildren<bee::MeshRenderer>(seletionEntity);
                mesh.Material = m_selectionMaterial;
                selectOnlyStructures = true;
            }
            else
            {
                const MidSelection* selected = bee::Engine.ECS().Registry.try_get<MidSelection>(entity);
                if (selected == nullptr) bee::Engine.ECS().CreateComponent<MidSelection>(entity);
                auto seletionEntity = bee::Engine.ECS().CreateEntity();
                bee::Engine.ECS().CreateComponent<SelectionCircle>(seletionEntity);
                auto& selectionTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(seletionEntity);
                selectionTransform.SetParent(entity);
                selectionTransform.Translation.z = 0.255f;
                selectionTransform.Scale *=
                    bee::Engine.ECS().Registry.get<AttributesComponent>(entity).GetValue(BaseAttributes::SelectionRange);
                selectionModel->Instantiate(seletionEntity);
                auto& mesh = bee::GetComponentInChildren<bee::MeshRenderer>(seletionEntity);
                mesh.Material = m_selectionMaterial;
            }
        }
    }
    if (m_selFunc != nullptr && !preSelection)
    {
        m_selFunc();
    }
    if (selectOnlyStructures)return;
    for (auto [entity,attributes, ally, transform] : alliedStructures.each())
    {
        const glm::vec2 circleOrigin = bee::FromWorldToScreen(transform.Translation);
        const glm::vec2 colliderPoint =
            bee::FromWorldToScreen(transform.Translation + m_currentCameraOrientation * static_cast<float>(attributes.GetValue(BaseAttributes::SelectionRange)));
        if (CircleRectangleCollision(circleOrigin, glm::length(colliderPoint - circleOrigin), m_rectangle))
        {
            const auto agent = bee::Engine.ECS().Registry.try_get<bee::ai::StateMachineAgent>(entity);
            if (agent)
            {
                if (agent->context.blackboard->HasKey<bool>("isBeingBuilt"))
                {
                    if (agent->context.blackboard->GetData<bool>("isBeingBuilt") == true) continue;
                }
            }

            if (!preSelection)
            {
                const Selected* selected = bee::Engine.ECS().Registry.try_get<Selected>(entity);
                if (selected == nullptr) bee::Engine.ECS().CreateComponent<Selected>(entity);
                auto seletionEntity = bee::Engine.ECS().CreateEntity();
                bee::Engine.ECS().CreateComponent<SelectionCircle>(seletionEntity);
                auto& selectionTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(seletionEntity);
                selectionTransform.SetParent(entity);
                selectionTransform.Translation.z = 0.255f;
                selectionTransform.Scale *=
                    bee::Engine.ECS().Registry.get<AttributesComponent>(entity).GetValue(BaseAttributes::SelectionRange);
                selectionModel->Instantiate(seletionEntity);
                auto& mesh = bee::GetComponentInChildren<bee::MeshRenderer>(seletionEntity);
                mesh.Material = m_selectionMaterial;
                break;
            }
            else
            {
                const MidSelection* selected = bee::Engine.ECS().Registry.try_get<MidSelection>(entity);
                if (selected == nullptr) bee::Engine.ECS().CreateComponent<MidSelection>(entity);
                auto seletionEntity = bee::Engine.ECS().CreateEntity();
                bee::Engine.ECS().CreateComponent<SelectionCircle>(seletionEntity);
                auto& selectionTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(seletionEntity);
                selectionTransform.SetParent(entity);
                selectionTransform.Translation.z = 0.255f;
                selectionTransform.Scale *=
                    bee::Engine.ECS().Registry.get<AttributesComponent>(entity).GetValue(BaseAttributes::SelectionRange);
                selectionModel->Instantiate(seletionEntity);
                auto& mesh = bee::GetComponentInChildren<bee::MeshRenderer>(seletionEntity);
                mesh.Material = m_selectionMaterial;
            }
        }
    }
}

void SelectionSystem::SelectAllUnits(bool preSelection) const
{
    const auto alliedUnits = bee::Engine.ECS().Registry.view< bee::Transform, AttributesComponent>();
    for (auto [entity, transform, attributes] : alliedUnits.each())
    {
        const glm::vec2 circleOrigin = bee::FromWorldToScreen(transform.Translation);
        const glm::vec2 selectionPoint = bee::FromWorldToScreen(
            transform.Translation +
            m_currentCameraOrientation * static_cast<float>(attributes.GetValue(BaseAttributes::SelectionRange)));
        if (CircleRectangleCollision(circleOrigin, glm::length(selectionPoint - circleOrigin), m_rectangle))
        {
            if (!preSelection)
            {
                const Selected* selected = bee::Engine.ECS().Registry.try_get<Selected>(entity);
                if (selected == nullptr) bee::Engine.ECS().CreateComponent<Selected>(entity);
            }
            else
            {
                const MidSelection* selected = bee::Engine.ECS().Registry.try_get<MidSelection>(entity);
                if (selected == nullptr) bee::Engine.ECS().CreateComponent<MidSelection>(entity);
            }
        }
    }
    if (m_selFunc != nullptr && !preSelection)
    {
        m_selFunc();
    }
}
#ifdef STEAM_API_WINDOWS
void SelectionSystem::ControllerSelectionUnits(const bool preSelection) const
{
    const auto alliedUnits =
        bee::Engine.ECS().Registry.view<bee::physics::DiskCollider, bee::physics::Interactable, bee::Transform>();
    for (auto [entity, collider, interactable, transform] : alliedUnits.each())
    {
        glm::vec2 circleOrigin = transform.Translation;
        if (glm::length(circleOrigin - glm::vec2(m_heldMouseStartWPosition)) <= collider.radius + m_radius)
        {
            if (!preSelection)
            {
                if (const Selected* selected = bee::Engine.ECS().Registry.try_get<Selected>(entity); selected == nullptr)
                    bee::Engine.ECS().CreateComponent<Selected>(entity);

            }
            else
            {
                if (const MidSelection* selected = bee::Engine.ECS().Registry.try_get<MidSelection>(entity);
                    selected == nullptr)
                    bee::Engine.ECS().CreateComponent<MidSelection>(entity);
            }
        }
    }
    if (m_selFunc != nullptr && !preSelection)
    {
        m_selFunc();
    }

}
#endif

void SelectionSystem::DeselectUnits(const bool preSelection) const
{
    auto view = bee::Engine.ECS().Registry.view<SelectionCircle, bee::Transform>();
    if (!preSelection)
    {
        const auto alliedUnits = bee::Engine.ECS().Registry.view<Selected>();
        bee::Engine.ECS().Registry.remove<Selected>(alliedUnits.begin(), alliedUnits.end());
        for (const auto selectionCircle : view)
        {
            auto& transform = view.get<bee::Transform>(selectionCircle);
            bee::Engine.ECS().Registry.get<bee::Transform>(transform.GetParent()).RemoveChild(selectionCircle);
            bee::Engine.ECS().DeleteEntity(selectionCircle);
        }
    }
    else
    {
        const auto alliedUnits = bee::Engine.ECS().Registry.view<MidSelection>();
        bee::Engine.ECS().Registry.remove<MidSelection>(alliedUnits.begin(), alliedUnits.end());
        for (const auto selectionCircle : view)
        {
            auto& transform = view.get<bee::Transform>(selectionCircle);
            bee::Engine.ECS().Registry.get<bee::Transform>(transform.GetParent()).RemoveChild(selectionCircle);
            bee::Engine.ECS().DeleteEntity(selectionCircle);
        }
    }
    if (m_desFunc != nullptr && !preSelection)
    {
        m_desFunc();
    }
}

void SelectionSystem::AddHoveringForEnemy() const
{
    bee::Entity enemyEntity = entt::null;
    const bool hitEntity = bee::MouseHitResponse(enemyEntity, false, false);
    const auto possibleEnemy = hitEntity ? bee::Engine.ECS().Registry.try_get<EnemyUnit>(enemyEntity) : nullptr;
    if (possibleEnemy != nullptr)
    {
        const auto selectionModel = bee::Engine.Resources().Load<bee::Model>("models/SelectionPlane.glb");
        const auto seletionEntity = bee::Engine.ECS().CreateEntity();
        bee::Engine.ECS().CreateComponent<SelectionCircle>(seletionEntity);
        bee::Engine.ECS().CreateComponent<EnemyUnit>(seletionEntity);
        auto& selectionTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(seletionEntity);
        selectionTransform.SetParent(enemyEntity);
        selectionTransform.Translation.z = 0.255f;
        selectionTransform.Scale *=
            bee::Engine.ECS().Registry.get<AttributesComponent>(enemyEntity).GetValue(BaseAttributes::SelectionRange);
        selectionModel->Instantiate(seletionEntity);
        auto& mesh = bee::GetComponentInChildren<bee::MeshRenderer>(seletionEntity);
        mesh.Material = m_hoveredEnemyMaterial;
    }
}

void SelectionSystem::RemoveHoveringForEnemy()
{
    for (auto [entity,transform,selection,enemy] : bee::Engine.ECS().Registry.view<bee::Transform, SelectionCircle, EnemyUnit>().each())
    {
        bee::Engine.ECS().Registry.get<bee::Transform>(transform.GetParent()).RemoveChild(entity);
        bee::Engine.ECS().DeleteEntity(entity);
    }
}


void SelectionSystem::SetSelectionCallback(const std::function<void()>& func) { m_selFunc = func; }
void SelectionSystem::SetDeselectionCallback(const std::function<void()>& func) { m_desFunc = func; }
#ifdef STEAM_API_WINDOWS
bool SelectionSystem::ControllerInput() { return m_controllerMode; }
void SelectionSystem::SteamInputUpdateCycle(float dt)
{
    // Checks for which selection mod to use
    if (bee::Engine.SteamInputSystem().GetDigitalData("Change_Mode").pressedOnce) m_controllerMode = !m_controllerMode;
    if (bee::Engine.SteamInputSystem().ControllerSelection())
    {
        m_switchMode = true;
        m_controllerMode = true;
    }
    // Handles what happens when you release the click
    if (m_lastSelectionButton && !bee::Engine.SteamInputSystem().GetDigitalData("Selection").rawInput.bState)
    {
        if (!bee::Engine.SteamInputSystem().GetDigitalData("All").rawInput.bState)
        {
            DeselectUnits(false);
        }
        if (!m_controllerMode)
            if (!m_allSelect)
                SelectUnits(false);
            else
                SelectAllUnits(false);
        else
        {
            ControllerSelectionUnits(false);
        }
        DeselectUnits(true);
    }

    m_heldLeftClick = bee::Engine.SteamInputSystem().GetDigitalData("Selection").rawInput.bState &&
                      m_lastSelectionButton != bee::Engine.SteamInputSystem().GetDigitalData("Selection").rawInput.bState;
    m_lastSelectionButton = bee::Engine.SteamInputSystem().GetDigitalData("Selection").rawInput.bState;
    // Handles what happens when you click again
    if (!m_heldLeftClick && m_lastSelectionButton)
    {
        DeselectUnits(true);
    }

    // If the click is held it changes the selection shape
    if (m_lastSelectionButton)
    {
        if (m_heldLeftClick && m_lastSelectionButton)
        {
            if (!m_controllerMode)
            {
                m_heldMouseStartWPosition = bee::GetGameWindowPosition(bee::Engine.Input().GetMousePosition());
            }else
            {
                m_heldMouseStartWPosition = MouseHitResponse();
            }
            m_radius = 0.0f;
        }
        if (!m_controllerMode)
        {
            const glm::vec2 l_mouseGameWindowPos = bee::GetGameWindowPosition(bee::Engine.Input().GetMousePosition());
            const glm::vec2 l_min = glm::vec2(std::min(m_heldMouseStartWPosition.x, l_mouseGameWindowPos.x),
                                              std::min(m_heldMouseStartWPosition.y, l_mouseGameWindowPos.y));
            const glm::vec2 l_max = glm::vec2(std::max(m_heldMouseStartWPosition.x, l_mouseGameWindowPos.x),
                                              std::max(m_heldMouseStartWPosition.y, l_mouseGameWindowPos.y));
            m_rectangle = bee::geometry2d::AABB(l_min, l_max);
            if (!m_allSelect)
                SelectUnits(true);
            else
                SelectAllUnits(true);
        }
        else
        {
            m_radius += m_radiusMultiplier * dt;
            ControllerSelectionUnits(true);
        }
    }
}
#endif

void SelectionSystem::WindowsInputUpdateCycle(float dt)
{
    // Handles what happens when you release the click
    if (m_lastSelectionButton && !bee::Engine.InputWrapper().GetDigitalData("Selection").pressed)
    {
            DeselectUnits(false);
        DeselectUnits(true);
            SelectUnits(false);
    }

    m_heldLeftClick = bee::Engine.InputWrapper().GetDigitalData("Selection").pressedOnce;
    m_lastSelectionButton = bee::Engine.InputWrapper().GetDigitalData("Selection").pressed;

    // Handles what happens when you click again
    if (!m_heldLeftClick && m_lastSelectionButton)
    {
        DeselectUnits(true);
    }


    RemoveHoveringForEnemy();
    AddHoveringForEnemy();

    const glm::vec3 hitResponse = MouseHitResponse();
    const glm::vec2 mouseGameWindowPos =bee::FromWorldToScreen(hitResponse);

    // If the click is held it changes the selection rectangle
    if (m_lastSelectionButton)
    {
        if (m_heldLeftClick && m_lastSelectionButton)
        {
            m_heldMouseInitialHitResponse = hitResponse;
        }
        m_heldMouseStartWPosition = bee::FromWorldToScreen(m_heldMouseInitialHitResponse);
         /*= bee::GetGameWindowPosition(bee::Engine.Input().GetMousePosition());*/
        const glm::vec2 l_min = glm::vec2(std::min(m_heldMouseStartWPosition.x, mouseGameWindowPos.x),
                                          std::min(m_heldMouseStartWPosition.y, mouseGameWindowPos.y));
        const glm::vec2 l_max = glm::vec2(std::max(m_heldMouseStartWPosition.x, mouseGameWindowPos.x),
                                          std::max(m_heldMouseStartWPosition.y, mouseGameWindowPos.y));
        m_rectangle = bee::geometry2d::AABB(l_min, l_max);
            SelectUnits(true);
    }
}

bool SelectionSystem::CircleRectangleCollision(const glm::vec2& circleOrigin, const float& radius,
                                               const bee::geometry2d::AABB& rectangle)
{
    // Reinterpreted code from https://www.jeffreythompson.org/collision-detection/circle-rect.php
    //  temporary variables to set edges for testing
    glm::vec2 test = circleOrigin;
    const glm::vec2 topLeft = rectangle.GetMin();
    const glm::vec2 bottomRight = rectangle.GetMax();

    // which edge is closest?
    if (circleOrigin.x < topLeft.x)
        test.x = topLeft.x;  // test left edge
    else if (circleOrigin.x > bottomRight.x)
        test.x = bottomRight.x;  // right edge
    if (circleOrigin.y < topLeft.y)
        test.y = topLeft.y;  // top edge
    else if (circleOrigin.y > bottomRight.y)
        test.y = bottomRight.y;  // bottom edge

    // get distance from closest edges
    const float distance = glm::length(circleOrigin - test);

    // if the distance is less than the radius, collision!
    return distance <= radius;
}

glm::vec3 SelectionSystem::MouseHitResponse() const
{
    glm::vec3 mouseWorldDirection, cameraWorldPosition;
    const auto screen = bee::Engine.ECS().Registry.view<bee::Transform, bee::Camera>();
    const auto& input = bee::Engine.Input();
    for (auto [entity, transform, camera] : screen.each())
    {
        if (!m_controllerMode)
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

    glm::vec3 hit = glm::vec3(0.0f);
    auto& terrainSystem = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();

    if (const bool result = terrainSystem.FindRayMeshIntersection(cameraWorldPosition, mouseWorldDirection, hit); !result)
    {
        const float multiplier = -cameraWorldPosition.z / mouseWorldDirection.z;

        hit = cameraWorldPosition + mouseWorldDirection * multiplier;
    }
    return hit;
}
