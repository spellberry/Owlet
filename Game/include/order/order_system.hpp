#pragma once
#include <actors/units/unit_order_type.hpp>
#include <functional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "core/fwd.hpp"
#include "level_editor/brushes/structure_brush.hpp"
#include "user_interface/user_interface_structs.hpp"

namespace bee
{
    namespace ui { struct Icon; }
}  // namespace bee

struct Order
{
    std::function<void()> onOrderSet = []() {};
    std::function<void()> onOrderHover = []() {};
    std::function<void()> orderBody = []() {};
};

class OrderSystem : public bee::System
{
public:
    /// <summary>
    /// Called Initialize Order Systems
    /// </summary>
    OrderSystem();
    ~OrderSystem() override;

    void Update(float dt) override;
    void HandleLeftMouseClick();
    void HandleRightMouseClick();
    void PollWrapperInput();
    void InputUpdate(float dt);
    static glm::vec3 GetDirectionToMouse();
    static glm::vec3 GetCameraPosition();
    void SetCurrentOrder(OrderType type);
    void Render() override;

    bee::ui::Icon& GetOrderTemplate(OrderType type);
    std::pair<glm::vec2,bool> GetOrderCost(OrderType orderType);
    void SaveOrderTemplates();
    void LoadOrderTemplates();

    void ActivateBrush() { m_brushActive = true; }
    void DeactivateBrush() { m_brushActive = false; }
    OrderType GetCurrentOrder() const { return m_currentOrder; }

#ifdef BEE_INSPECTOR
    void Inspect() override;
    void Inspect(bee::Entity e) override;
#endif

    int mageLimit = 0;
    int swordsmenLimit = 0;
    int GetNumberOfSwordsmen();
    int GetNumberOfMages();
    std::vector<OrderType> buttons{};

private:
    bee::ui::UIElementID m_supplyUI = entt::null;
    friend struct BuildBarracks;
    friend struct BuildMageTower;

    bool GetTerrainRaycastPosition(const glm::vec3 mouseWorldDirection, const glm::vec3 cameraWorldPosition, glm::vec3& point);

    // Unit Orders
    void InitializeAttackOrder();
    void InitializeMoveOrder();
    void InitializeOffensiveMoveOrder();
    void InitializePatrolOrder();

    // 'Varied' Orders
    void InitializeBuildOrder(const std::string& buildingHandle, OrderType orderType);
    void InitializeTrainOrder(const std::string& unitHandle, OrderType orderType);
    void InitializeUpgradeOrder();
    bool CanSpawnUnit(const std::string& unitHandle);

    // Player Actions
    void InitializeBuildWall(const std::string& buildingHandle);
    void InitializeBuildFence(const std::string& buildingHandle);

    void DragInitialize();  // Creates the semi-transparent entities trail that are visible when dragging the wall.
    void DragUpdate();      // Updates A* path inside the update when the player is dragging cursor.
    void DragReset();       // Moves back all transparent walls back to origin position.
    const glm::vec3 DragDetermineDirection(const glm::vec3& hit);  // Returns only the world positon of the largest component.
    void DragDetermineValues();                                    // Updates the transparent entity values.

    void InitializeOrders();
    void InitializeGoalEntity();
    void InitializeAttackGoalEntity();

    // Brush variables
    lvle::StructureBrush m_structureBrush{};
    bool m_brushActive = true;
    glm::vec2 m_mousePositionLast = glm::vec2(0.0f);  // Stores the position of the mouse in the last tick

    // Build amount
    const size_t m_buildMax = 64;
    size_t m_buildCurrent = 0;

    // Drag variables
    bool m_drag = false;
    glm::vec3 m_dragStart = glm::vec3(0.0f);  // The terrain world position that starts on the drag
    std::vector<glm::vec3> m_dragPoints = {};
    std::vector<bee::Entity> m_dragWalls = {};
    std::vector<bee::Entity> m_dragFences = {};
    bool m_wallType = false; // false wall, true fence

    // Angles
    const glm::quat m_vertical = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    const glm::quat m_horizontal = glm::normalize(glm::rotate(m_vertical, glm::radians(-89.9f), glm::vec3(0.0f, 0.0f, 1.0f)));  // Quaternions don't like 90 degrees
    glm::quat m_currentOrientation = m_vertical;

    // Order data
    OrderType m_currentOrder = OrderType::Move;
    std::unordered_map<OrderType, Order> m_orders{};
    std::unordered_map<OrderType, bee::ui::Icon> m_OrderIcons;
    std::unordered_map<OrderType, std::pair<glm::vec2,bool>> m_orderCost = {};

    bee::Entity m_goalEntity = entt::null;
    bee::Entity m_attackGoalEntity = entt::null;
};
