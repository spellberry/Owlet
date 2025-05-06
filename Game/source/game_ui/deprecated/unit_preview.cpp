// #include "game_ui/unit_preview.hpp"
//
// #include <actors/structures/structure_manager_system.hpp>
// #include <order/order_system.hpp>
//
// #include "actors/selection_system.hpp"
// #include "actors/units/unit_manager_system.hpp"
// #include "core/device.hpp"
// #include "core/engine.hpp"
// #include "tools/log.hpp"
// #include "user_interface/font_handler.hpp"
// #include "user_interface/user_interface.hpp"
// #include "user_interface/user_interface_editor_structs.hpp"
// #include "user_interface/user_interface_serializer.hpp"
//
// using namespace bee;
// using namespace ui;
//
////
//// function pointers
////
// void AddUnit()
//{
//     // Add entity data to UI
//     Engine.ECS().GetSystem<UnitPreview>().AddUnitsToPreview();
// }
// void RemoveUnit()
//{
//     // remove entity data to UI
//     Engine.ECS().GetSystem<UnitPreview>().RemoveUnitsFromPreview();
// }
// UI_FUNCTION(SelUnit, SelectUnit,
//             Engine.ECS().GetSystem<UnitPreview>().ShowSpecificUnit(
//                 Engine.ECS().GetSystem<UnitPreview>().GetComponentEntity(component)););
// UI_FUNCTION(HoverOver, HoverUI, Engine.ECS().GetSystem<SelectionSystem>().hoveringUI = true;);
// UI_FUNCTION(OrderExecute, HandleOrderButtonCallback,
//             const auto orderType = bee::Engine.ECS().GetSystem<UnitPreview>().GetOrderType(component);
//             Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(orderType););
//
//
//
// UnitPreview::UnitPreview()
//{
//     Title = "UnitPreview";
//     auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//     // create all default things like background etc...
//     // load all the texture too
//     const float height = static_cast<float>(Engine.Device().GetHeight());
//     const float width = static_cast<float>(Engine.Device().GetWidth());
//     const float norWidth = width / height;
//
//     auto& selectSystem = Engine.ECS().GetSystem<SelectionSystem>();
//     selectSystem.SetSelectionCallback(AddUnit);
//     selectSystem.SetDeselectionCallback(RemoveUnit);
//     element = UI.CreateUIElement(bee::ui::Alignment::bottom, bee::ui::Alignment::left);
//     elementRight = UI.CreateUIElement(bee::ui::Alignment::bottom, bee::ui::Alignment::right);
//
//     BackgroundEL5.Img = BackgroundEL4.Img = BackgroundEL3.Img = UI.LoadTexture(
//         Engine.FileIO()
//             .GetPath(bee::FileIO::Directory::Asset, "textures/UI/Game UI collection/png files/Borders blue-01.png")
//             .c_str());
//
//     ActionHover.Img = UI.LoadTexture(
//         Engine.FileIO()
//             .GetPath(bee::FileIO::Directory::Asset, "textures/UI/Game UI collection/png files/buttons blue-01.png")
//             .c_str());
//
//     const auto tex = UI.CreateTextureComponentFromAtlas(element, BackgroundEL3.Img, BackgroundEL3.GetAtlas(),
//                                                         glm::vec2(0.0f, 0.0f), BackgroundEL3.GetNormSize(m_elementSize));
//
//     m_xDiv2 = (norWidth / 2) - (BackgroundEL3.GetNormSize(m_elementSize).x / 2);
//     // float m_xDiv2 = (x - std::floor(x)) / 2;
//     UI.CreateTextureComponentFromAtlas(element, BackgroundEL4.Img, BackgroundEL4.GetAtlas(), glm::vec2(m_xDiv2, 0.0f),
//                                        BackgroundEL4.GetNormSize(m_elementSize));
//
//     m_buttonSize = (BackgroundEL4.GetNormSize(m_elementSize).y - 0.08f) / 2.0f;
//
//     m_nextUnitPos.SetReset(glm::vec2(m_xDiv2 + 0.01, BackgroundEL4.GetNormSize(m_elementSize).y - m_buttonSize - 0.01));
//
//     UI.CreateButton(element, glm::vec2(0.0f, 0.0f), glm::vec2(norWidth, BackgroundEL3.GetNormSize(m_elementSize).y),
//                     bee::ui::interaction(SwitchType::none), bee::ui::interaction(SwitchType::none, "HoverUI"),
//                     bee::ui::repeat, -1, false);
//     UI.CreateTextureComponentFromAtlas(elementRight, BackgroundEL5.Img, BackgroundEL5.GetAtlas(), glm::vec2(0, 0.0f),
//                                        BackgroundEL5.GetNormSize(m_elementSize));
//     const std::string str0 = "Orders:";
//     m_repalceComp = UI.CreateString(
//         elementRight, 0, str0,
//         glm::vec2(BackgroundEL3.GetNormSize(m_elementSize).x / 2 - 0.03, BackgroundEL3.GetNormSize(m_elementSize).y - 0.03),
//         m_elementSize * TEXT_SIZE, glm::vec4(1), 0);
//     m_nextActionPos.SetReset(glm::vec2(BackgroundEL3.GetNormSize(m_elementSize).x - m_buttonSize - 0.01,
//                                        BackgroundEL3.GetNormSize(m_elementSize).y - 0.12));
//
//     actionHoverid = UI.AddOverlayImage(ActionHover);
//
//     bool done = true;
//     while (done)
//     {
//         m_maxEntitiesInList++;
//         m_nextUnitPos.Set(glm::vec2(m_nextUnitPos.Get().x + m_buttonSize + SPACE_BETWEEN_BUTTONS, m_nextUnitPos.Get().y));
//
//         if (m_nextUnitPos.Get().x + m_buttonSize > 2 * BackgroundEL3.GetNormSize(m_elementSize).x)
//         {
//             if (m_nextUnitPos.Get().y - m_buttonSize - SPACE_BETWEEN_BUTTONS < 0)
//             {
//                 done = false;
//             }
//             m_nextUnitPos.Set(glm::vec2(m_xDiv2 + 0.01, m_nextUnitPos.Get().y - m_buttonSize - SPACE_BETWEEN_BUTTONS));
//         }
//     }
//     UI.SetElementLayer(element, 5);
//     UI.SetElementLayer(elementRight, 5);
//     m_costView = UI.serialiser->LoadElement("CostView");
//     UI.SetElementLayer(m_costView, 5);
//     UI.ReplaceString(UI.GetComponentID(m_costView, "Wood"), "");
//     UI.ReplaceString(UI.GetComponentID(m_costView, "Stone"), "");
//     // ShowActions();
// }
// UnitPreview::~UnitPreview() {}
//
// void UnitPreview::Update(float dt)
//{
//     auto& UI = bee::Engine.ECS().GetSystem<UserInterface>();
//     if (m_hovering)
//     {
//         if (m_cost.x!=0.0f || m_cost.y!=0.0f)
//         {
//
//             UI.ReplaceString(UI.GetComponentID(m_costView, "Wood"),"Wood: "+ std::to_string(static_cast<int>(m_cost.x)));
//             UI.ReplaceString(UI.GetComponentID(m_costView, "Stone"),"Stone: "+ std::to_string(static_cast<int>(m_cost.y)));
//         }
//         else
//         {
//             UI.ReplaceString(UI.GetComponentID(m_costView, "Wood"), "");
//             UI.ReplaceString(UI.GetComponentID(m_costView, "Stone"), "");
//         }
//     }else
//     {
//         UI.ReplaceString(UI.GetComponentID(m_costView, "Wood"), "");
//         UI.ReplaceString(UI.GetComponentID(m_costView, "Stone"), "");
//     }
//     m_hovering = false;
// }
//
// entt::entity UnitPreview::GetComponentEntity(bee::ui::UIComponentID component) const
//{
//     return m_componentToEntMap.find(component)->second;
// }
//
// void UnitPreview::AddUnitsToPreview()
//{
//     auto view = Engine.ECS().Registry.view<Selected>();
//     auto& UI = Engine.ECS().GetSystem<UserInterface>();
//     if (view.size() > m_maxEntitiesInList)
//     {
//         // There are more entities than that fit into the window. we need a new way to display these
//         std::unordered_map<std::string, int> counter;
//         std::unordered_map<std::string, entt::entity> firstEntity;
//         for (auto [entity, sel] : view.each())
//         {
//             // get a amount of each kind of entity there is and then add them to the preview with counters
//
//             const auto& attrib = Engine.ECS().Registry.get<AttributesComponent>(entity);
//
//             if (counter.count(attrib.GetEntityType()) == 0)
//             {
//                 counter.emplace(attrib.GetEntityType(), 0);
//             }
//             if (firstEntity.count(attrib.GetEntityType()) == 0)
//             {
//                 firstEntity.emplace(attrib.GetEntityType(), entity);
//             }
//             counter.find(attrib.GetEntityType())->second++;
//         }
//         for (auto& count : counter)
//         {
//             entityInList unit = entityInList();
//
//             std::string str0 = std::to_string(count.second);
//             unit.Text = UI.CreateString(element, 0, str0, m_nextUnitPos.Get(), 0.3 * m_elementSize, glm::vec4(1.0f));
//             auto element = firstEntity.find(count.first);
//             if (bee::Engine.ECS().Registry.try_get<AllyUnit>(element->second) != nullptr)
//             {
//                 const auto& unitType = Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(count.first);
//                 MakePreview(unitType, unit, firstEntity.find(count.first)->second);
//             }
//             else
//             {
//                 const auto& structureType = Engine.ECS().GetSystem<StructureManager>().GetStructureTemplate(count.first);
//                 MakePreview(structureType, unit, firstEntity.find(count.first)->second);
//             }
//         }
//         return;
//     }
//     for (auto [entity, sel] : view.each())
//     {
//         // safeguard
//         if (m_unitsInList.count(entity) > 0)
//         {
//             return;
//         }
//         entityInList unit = entityInList();
//
//         if (bee::Engine.ECS().Registry.try_get<AllyUnit>(entity))
//         {
//             // entity is unit
//             const auto& attrib = Engine.ECS().Registry.get<AttributesComponent>(entity);
//             const auto& unitType = Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(attrib.GetEntityType());
//             MakePreview(unitType, unit, entity);
//         }
//         else if (bee::Engine.ECS().Registry.try_get<AllyStructure>(entity))
//         {
//             // entity is structure
//             const auto& attrib = Engine.ECS().Registry.get<AttributesComponent>(entity);
//             const auto& unitTemp = Engine.ECS().GetSystem<StructureManager>().GetStructureTemplate(attrib.GetEntityType());
//             MakePreview(unitTemp, unit, entity);
//         }
//         else
//         {
//             Log::Error("tried to add a entity to unit preview but entity was not a unit or a structure");
//         }
//     }
// }
//
// void UnitPreview::MakePreview(const UnitTemplate& unitType, entityInList& unit, const entt::entity entity)
//{
//     auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//     const glm::vec4 uvs = glm::vec4(unitType.iconTextureCoordinates.x, unitType.iconTextureCoordinates.y,
//                                     unitType.iconTextureCoordinates.x + unitType.iconTextureCoordinates.z,
//                                     unitType.iconTextureCoordinates.y + unitType.iconTextureCoordinates.w);
//
//     const int tex = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, unitType.iconPath).c_str());
//     unit.Tex = UI.CreateTextureComponentFromAtlas(element, tex, uvs, m_nextUnitPos.Get(), glm::vec2(m_buttonSize));
//     for (auto& order : unitType.availableOrders)
//     {
//         bool found = false;
//         for (const auto& action : m_actionMap)
//         {
//             if (action.second == order)
//             {
//                 found = true;
//                 break;
//             }
//         }
//         if (!found) ShowOrder(order);
//     }
//
//     MakePreviewEnd(unit, entity);
// }
// void UnitPreview::MakePreview(const StructureTemplate& unitTemp, entityInList& unit, const entt::entity entity)
//{
//     auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//     const glm::vec4 uvs = glm::vec4(unitTemp.iconTextureCoordinates.x, unitTemp.iconTextureCoordinates.y,
//                                     unitTemp.iconTextureCoordinates.x + unitTemp.iconTextureCoordinates.z,
//                                     unitTemp.iconTextureCoordinates.y + unitTemp.iconTextureCoordinates.w);
//     const int tex = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, unitTemp.iconPath).c_str());
//     unit.Tex = UI.CreateTextureComponentFromAtlas(element, tex, uvs, m_nextUnitPos.Get(), glm::vec2(m_buttonSize));
//     for (const auto& order : unitTemp.availableOrders)
//     {
//         bool found = false;
//         for (const auto& action : m_actionMap)
//         {
//             if (action.second == order)
//             {
//                 found = true;
//                 break;
//             }
//         }
//         if (!found) ShowOrder(order);
//     }
//     MakePreviewEnd(unit, entity);
// }
//
// void UnitPreview::MakePreviewEnd(entityInList& unit, const entt::entity entity)
//{
//     auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//     unit.Button = UI.CreateButton(element, m_nextUnitPos.Get(), glm::vec2(m_buttonSize, m_buttonSize),
//                                   bee::ui::interaction(bee::ui::SwitchType::none, "SelectUnit"),
//                                   bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single,
//                                   actionHoverid);
//     m_nextUnitPos.Set(glm::vec2(m_nextUnitPos.Get().x + m_buttonSize + SPACE_BETWEEN_BUTTONS, m_nextUnitPos.Get().y));
//
//     if (m_nextUnitPos.Get().x + m_buttonSize > 2 * BackgroundEL3.GetNormSize(m_elementSize).x)
//     {
//         m_nextUnitPos.Set(glm::vec2(m_xDiv2 + 0.01, m_nextUnitPos.Get().y - m_buttonSize - SPACE_BETWEEN_BUTTONS));
//     }
//     if (m_lastUnit != -1)
//     {
//         UI.SetComponentNeighbourInDirection(unit.Button, m_lastUnit, Alignment::left);
//         UI.SetComponentNeighbourInDirection(m_lastUnit, unit.Button, Alignment::right);
//     }
//     m_unitsInList.emplace(entity, unit);
//     m_componentToEntMap.emplace(unit.Button, entity);
//     m_lastUnit = unit.Button;
// }
// void UnitPreview::RemoveUnitsFromPreview()
//{
//     auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//     // if this is currently selected unit also clearSpecificUnit
//
//     ClearSpecificUnit();
//
//     for (const auto& pair : m_unitsInList)
//     {
//         const auto& unit = pair.second;
//         UI.DeleteComponent(unit.Button);
//         UI.DeleteComponent(unit.Tex);
//         if (unit.Text != -1) UI.DeleteComponent(unit.Text);
//     }
//     m_componentToEntMap.clear();
//     m_unitsInList.clear();
//
//     m_nextUnitPos.Reset();
//     m_lastAction = -1;
//     m_lastUnit = -1;
//     m_nextActionPos.Reset();
//     m_actionMap.clear();
//     for (const auto action : m_Actions)
//     {
//         UI.DeleteComponent(action);
//     }
//     m_Actions.clear();
// }
////
//// SpecificUnit
////
//
// void UnitPreview::ShowSpecificUnit(entt::entity entity)
//{
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//    // Load all Specific preview of all unit data
//    if (m_specificSelectedUnit != entt::null)
//    {
//        ClearSpecificUnit();
//    }
//    else if (entity == m_specificSelectedUnit)
//    {
//        return;
//    }
//
//    const float newX = BackgroundEL3.GetNormSize(m_elementSize).x - 0.2f;
//    const float newY = BackgroundEL3.GetNormSize(m_elementSize).y - 0.1f;
//    const float newYSave = newY;
//    const float newXSave = newX;
//    glm::vec2 resetxy = glm::vec2(newXSave, newYSave);
//    glm::vec2 xy = glm::vec2(newX, newY);
//    m_specificSelectedUnit = entity;
//
//    if (bee::Engine.ECS().Registry.try_get<AllyUnit>(entity))
//    {
//        const auto& attrib = Engine.ECS().Registry.get<AttributesComponent>(entity);
//        const auto& unitTemp = Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(attrib.GetEntityType());
//        ShowStat(xy, resetxy, attrib, BaseAttributes::HitPoints, m_healthText);
//        ShowStat(xy, resetxy, attrib, BaseAttributes::Armor, m_armorText);
//        ShowStat(xy, resetxy, attrib, BaseAttributes::Damage, m_attackText);
//
//        const glm::vec4 uvs = glm::vec4(unitTemp.iconTextureCoordinates.x, unitTemp.iconTextureCoordinates.y,
//                                        unitTemp.iconTextureCoordinates.x + unitTemp.iconTextureCoordinates.z,
//                                        unitTemp.iconTextureCoordinates.y + unitTemp.iconTextureCoordinates.w);
//        const int tex = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset,
//        unitTemp.iconPath).c_str()); m_specificPic = UI.CreateTextureComponentFromAtlas(
//            element, tex, uvs, glm::vec2(0.1f, 0.1f),
//            glm::normalize(glm::vec2(unitTemp.iconTextureCoordinates.z, unitTemp.iconTextureCoordinates.w)) * 0.2f *
//                m_elementSize);  // size
//
//        m_specificName = UI.CreateString(element, 0, attrib.GetEntityType(), glm::vec2(0.2f, 0.2f), m_elementSize * TEXT_SIZE,
//                                         glm::vec4(1), 0);
//        const std::string str0 = std::to_string(static_cast<unsigned int>(entity));
//        m_specificText = UI.CreateString(element, 0, str0, glm::vec2(0.2f, 0.14f), m_elementSize * TEXT_SIZE, glm::vec4(1),
//        0);
//    }
//    else if (bee::Engine.ECS().Registry.try_get<AllyStructure>(entity))
//    {
//        const auto& attrib = Engine.ECS().Registry.get<AttributesComponent>(entity);
//        const auto& unitTemp = Engine.ECS().GetSystem<StructureManager>().GetStructureTemplate(attrib.GetEntityType());
//        ShowStat(xy, resetxy, attrib, BaseAttributes::HitPoints, m_healthText);
//        ShowStat(xy, resetxy, attrib, BaseAttributes::Armor, m_armorText);
//        ShowStat(xy, resetxy, attrib, BaseAttributes::Damage, m_attackText);
//        const glm::vec4 uvs = glm::vec4(unitTemp.iconTextureCoordinates.x, unitTemp.iconTextureCoordinates.y,
//                                        unitTemp.iconTextureCoordinates.x + unitTemp.iconTextureCoordinates.z,
//                                        unitTemp.iconTextureCoordinates.y + unitTemp.iconTextureCoordinates.w);
//        const int tex = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset,
//        unitTemp.iconPath).c_str()); m_specificPic = UI.CreateTextureComponentFromAtlas(
//            element, tex, uvs, glm::vec2(0.1f, 0.1f),
//            glm::normalize(glm::vec2(unitTemp.iconTextureCoordinates.z, unitTemp.iconTextureCoordinates.w)) * 0.2f *
//                m_elementSize);  // size
//        m_specificName = UI.CreateString(element, 0, attrib.GetEntityType(), glm::vec2(0.2f, 0.2f), m_elementSize * TEXT_SIZE,
//                                         glm::vec4(1), 0);
//        const std::string str0 = std::to_string(static_cast<unsigned int>(entity));
//        m_specificText = UI.CreateString(element, 0, str0, glm::vec2(0.2f, 0.14f), m_elementSize * TEXT_SIZE, glm::vec4(1),
//        0);
//    }
//}
//
// void UnitPreview::ClearSpecificUnit()
//{
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//    if (m_specificSelectedUnit == entt::null)
//    {
//        return;
//    }
//    // remove all Specific preview of all unit data
//    m_specificSelectedUnit = entt::null;
//
//    const unsigned int size = static_cast<unsigned int>(m_showableStats.size());
//    for (unsigned int i = 0; i < size; i++)
//    {
//        const auto& stat = m_showableStats.at(i);
//        UI.DeleteComponent(stat.dynamicText);
//        UI.DeleteComponent(stat.staticText);
//    }
//    m_showableStats.clear();
//    UI.DeleteComponent(m_specificPic);
//    UI.DeleteComponent(m_specificName);
//    UI.DeleteComponent(m_specificText);
//}
//
// void UnitPreview::ShowStat(glm::vec2& xy, glm::vec2& resetxy, const AttributesComponent& attrib, const BaseAttributes
// attribute,
//                           const std::string& str)
//
//{
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//    ShowableStats stats = ShowableStats();
//
//    stats.staticText = UI.CreateString(element, 0, str, glm::vec2(xy.x, xy.y), m_elementSize * TEXT_SIZE, glm::vec4(1.0)
//    ,xy.x, xy.y, false, 0);
//
//    const std::string str0 = (attrib.HasAttribute(attribute) ? std::to_string(attrib.GetValue(attribute)) : "");
//
//    stats.dynamicText =
//        UI.CreateString(element, 0, str0, glm::vec2(xy.x, resetxy.y), m_elementSize * TEXT_SIZE, glm::vec4(1), 0);
//    resetxy.y = xy.y;
//    xy.x = resetxy.x;
//    m_showableStats.push_back(stats);
//}
//
// OrderType UnitPreview::GetOrderType(bee::ui::UIComponentID id) const { return m_actionMap.find(id)->second; }
//
// void UnitPreview::ShowOrder(OrderType type)
//{
//    if (type == OrderType::None || type == OrderType::Move || type == OrderType::Attack) return;
//
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//    auto& orderSystem = Engine.ECS().GetSystem<OrderSystem>();
//    const auto& icon = orderSystem.GetOrderTemplate(type);
//    const glm::vec4 uvs = glm::vec4(icon.iconTextureCoordinates.x, icon.iconTextureCoordinates.y,
//                                    icon.iconTextureCoordinates.x + icon.iconTextureCoordinates.z,
//                                    icon.iconTextureCoordinates.y + icon.iconTextureCoordinates.w);
//    const int tex = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, icon.iconPath).c_str());
//    m_Actions.push_back(
//        UI.CreateTextureComponentFromAtlas(elementRight, tex, uvs, m_nextActionPos.Get(), glm::vec2(m_buttonSize)));
//
//    m_Actions.push_back(UI.CreateButton(elementRight, m_nextActionPos.Get(), glm::vec2(m_buttonSize),
//                                        bee::ui::interaction(SwitchType::none, "HandleOrderButtonCallback"),
//                                        bee::ui::interaction(SwitchType::none, "Price"), bee::ui::ButtonType::single,
//                                        actionHoverid));
//
//    m_actionMap.emplace(m_Actions.back(), type);
//    m_nextActionPos.Set(glm::vec2(m_nextActionPos.Get().x - m_buttonSize - SPACE_BETWEEN_BUTTONS, m_nextActionPos.Get().y));
//    if (m_nextActionPos.Get().x < 0)
//    {
//        m_nextActionPos.Set(glm::vec2(BackgroundEL3.GetNormSize(m_elementSize).x - m_buttonSize - 0.01,
//                                      m_nextActionPos.Get().y - m_buttonSize - SPACE_BETWEEN_BUTTONS));
//    }
//    if (m_lastAction != -1)
//    {
//        UI.SetComponentNeighbourInDirection(m_Actions.back(), m_lastAction, Alignment::left);
//        UI.SetComponentNeighbourInDirection(m_lastAction, m_Actions.back(), Alignment::right);
//    }
//    m_lastAction = m_Actions.back();
//}
//
// void UnitPreview::ActivateSelectUnits() const
//{
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//    if (m_unitsInList.size() > 0)
//    {
//        for (auto& unitsInList : m_unitsInList)
//        {
//            if (m_lastSelectedUnit == unitsInList.second.Button)
//            {
//                UI.SetSelectedInteractable(unitsInList.second.Button);
//                return;
//            }
//        }
//        UI.SetSelectedInteractable(m_unitsInList.begin()->second.Button);
//    }
//}
// void UnitPreview::DeActivateSelectUnits()
//{
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//    m_lastSelectedUnit = UI.GetSelectedInteractable();
//}
//
// void UnitPreview::ActivateSelectOrder() const
//{
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//
//    if (m_Actions.size() > 0)
//    {
//        for (auto& unitsInList : m_Actions)
//        {
//            if (m_lastSelectedUnit == unitsInList)
//            {
//                UI.SetSelectedInteractable(unitsInList);
//                return;
//            }
//        }
//        UI.SetSelectedInteractable(m_Actions.at(1));
//    }
//}
// void UnitPreview::DeActivateSelectOrder()
//{
//    auto& UI = Engine.ECS().GetSystem<UserInterface>();
//    m_lastSelectedOrder = UI.GetSelectedInteractable();
//}
//
// #ifdef BEE_INSPECTOR
// void UnitPreview::Inspect()
//{
//    ImGui::Begin("UnitPreview Inspector");
//    ImGui::End();
//}
// #endif