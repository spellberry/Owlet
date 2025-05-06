// #pragma once
// #include "actors/structures/structure_template.hpp"
// #include "actors/units/unit_order_type.hpp"
// #include "actors/units/unit_template.hpp"
// #include "core/ecs.hpp"
// #include "user_interface/user_interface_structs.hpp"
//
// constexpr float TEXT_SIZE = 0.2f;
// constexpr float SPACE_BETWEEN_BUTTONS = 0.005f;
//
// struct ShowableStats
//{
//     bee::ui::UIComponentID dynamicText;
//     bee::ui::UIComponentID staticText;
// };
//
// struct entityInList
//{
//     bee::ui::UIComponentID Button = -1;
//     bee::ui::UIComponentID Tex = -1;
//     bee::ui::UIComponentID Text = -1;
//     std::string name = "";
// };
//
// void AddUnit();
// void RemoveUnit();
//
// class UnitPreview : public bee::System
//{
// public:
//     UnitPreview();
//     ~UnitPreview();
//     void Update(float dt) override;
// #ifdef BEE_INSPECTOR
//     void Inspect() override;
// #endif
//
//     void AddUnitsToPreview();
//     void RemoveUnitsFromPreview();
//
//     [[nodiscard]] entt::entity GetComponentEntity(bee::ui::UIComponentID component) const;
//     void ShowSpecificUnit(entt::entity entity);
//     void ClearSpecificUnit();
//     [[nodiscard]] entt::entity GetSpecificUnitEnt() const { return m_specificSelectedUnit; };
//     [[nodiscard]] OrderType GetOrderType(bee::ui::UIComponentID id) const;
//
//     void ActivateSelectUnits() const;
//     void DeActivateSelectUnits();
//     void ActivateSelectOrder() const;
//     void DeActivateSelectOrder();
//
//     // This is a handler for the canvas that contains the left bottom UI
//     bee::ui::UIElementID element = entt::null;
//     // This is a handle for the canvas that contains the right bottom UI (the one with the orders)
//     bee::ui::UIElementID elementRight = entt::null;
//
// private:
//     friend struct Price;
//
//     entt::entity m_specificSelectedUnit = entt::null;
//
//     std::vector<ShowableStats> m_showableStats;
//
//     std::map<entt::entity, entityInList> m_unitsInList;
//     std::map<bee::ui::UIComponentID, entt::entity> m_componentToEntMap;
//
//     bee::ui::UIElementID m_costView = entt::null;
//
//     std::string m_healthText = "Health: ";
//     std::string m_armorText = "Armor: ";
//     std::string m_attackText = "Attack: ";
//
//     bee::ui::UIImageElement BackgroundEL3 = bee::ui::UIImageElement(glm::vec2(387, 2246), glm::vec2(1014, 424));
//     bee::ui::UIImageElement BackgroundEL4 = bee::ui::UIImageElement(glm::vec2(2384, 177), glm::vec2(1014, 424));
//     bee::ui::UIImageElement BackgroundEL5 = bee::ui::UIImageElement(glm::vec2(392, 2767), glm::vec2(1014, 424));
//
//     bee::ui::UIImageElement ActionHover = bee::ui::UIImageElement(glm::vec2(7191, 6554), glm::vec2(388, 388));
//     unsigned int actionHoverid = -1;
//
//     skye::AdvancedT<glm::vec2> m_nextUnitPos = skye::AdvancedT<glm::vec2>(glm::vec2(0.0f));
//     float m_buttonSize = 0.0f;
//     skye::AdvancedT<glm::vec2> m_nextActionPos = skye::AdvancedT<glm::vec2>(glm::vec2(0.0f));
//     float m_xDiv2 = 0.0f;
//
//     void ShowStat(glm::vec2& xy, glm::vec2& resetxy, const AttributesComponent& attrib, BaseAttributes attribute,
//                   const std::string& str);
//
//     void MakePreview(const UnitTemplate& unitType, entityInList& unit, entt::entity entity);
//     void MakePreview(const StructureTemplate& unitTemp, entityInList& unit, entt::entity entity);
//     void MakePreviewEnd(entityInList& unit, entt::entity entity);
//     bee::ui::UIComponentID m_specificPic = -1;
//     bee::ui::UIComponentID m_specificName = -1;
//     bee::ui::UIComponentID m_specificText = -1;
//     float m_minusValue = 0.0f;
//     bee::ui::UIComponentID m_repalceComp;
//
//     std::vector<bee::ui::UIComponentID> m_Actions;
//     std::unordered_map<bee::ui::UIComponentID, OrderType> m_actionMap;
//     void ShowOrder(OrderType type);
//     bee::ui::UIComponentID m_lastAction = -1;
//     bee::ui::UIComponentID m_lastUnit = -1;
//
//     bee::ui::UIComponentID m_lastSelectedOrder = -1;
//     bee::ui::UIComponentID m_lastSelectedUnit = -1;
//     int m_maxEntitiesInList = 0;
//
//     glm::vec2 m_cost = glm::vec2(0.0f);
//     bool m_hovering = false;
//
//     float m_elementSize = 0.6f;
// };