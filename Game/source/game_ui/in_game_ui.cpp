#include "game_ui/in_game_ui.hpp"

#include "Starcraft.hpp"
#include "actors/selection_system.hpp"
#include "ai_behaviors/enemy_ai_behaviors.hpp"
#include "ai_behaviors/wave_system.hpp"
#include "core/device.hpp"
#include "order/order_system.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_serializer.hpp"
#include "user_interface/user_interface_structs.hpp"
using namespace bee;
using namespace ui;
constexpr float SPACEBETWEENBUTTONS = 0.005f;
void AddUnitToPreview()
{
    // Add entity data to UI
    Engine.ECS().GetSystem<InGameUI>().SelectUnits();
}
void RemoveUnitFromPreview()
{
    // remove entity data to UI
    auto& ingameUI = Engine.ECS().GetSystem<InGameUI>();
    ingameUI.ClearPreview();
}
void ResourceCallbackFunc(const GameResourceType type, const int amount)
{
    Engine.ECS().GetSystem<InGameUI>().UpdateResources(
        type, Engine.ECS().GetSystem<ResourceSystem>().playerResourceData.resources.at(type));
}
UI_FUNCTION(SettingsButton, ToSettings, std::cout << "ToSettings"; );

UI_FUNCTION(PauseButton, ToPause, dynamic_cast<Starcraft&>(Engine.Game()).Pause(false););

UI_FUNCTION(CPopUp, ClosePopup, bee::Engine.ECS().GetSystem<InGameUI>().RemovePopup(); bee::Engine.Audio().PlaySoundW("audio/click.wav", 1.0f, false););

UI_FUNCTION(SetOrder, SetOrderFromUI, const auto orderType = bee::Engine.ECS().GetSystem<InGameUI>().GetOrderType(component);
            Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(orderType););

UI_FUNCTION(OrderBuildBarracks, DoOrderBuildBarracks,
            bee::Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(OrderType::BuildBarracks););

UI_FUNCTION(OrderBuildMageTower, DoOrderBuildMageTower,
            bee::Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(OrderType::BuildMageTower);)

UI_FUNCTION(OrderStartWall, DoOrderStartWall,
            bee::Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(OrderType::BuildWallStart););

UI_FUNCTION(OrderStartFence, DoOrderStartFence,
            bee::Engine.ECS().GetSystem<OrderSystem>().SetCurrentOrder(OrderType::BuildFenceStart););

UI_FUNCTION(HoveringOver, HoveringOverUI, Engine.ECS().GetSystem<SelectionSystem>().hoveringUI = true;);

UI_FUNCTION(HandleHoverBuild, ShowBuildHover, bee::Engine.ECS().GetSystem<InGameUI>().ShowHoverBuild(component););
UI_FUNCTION(HandleHoverOrder, ShowOrderHover, bee::Engine.ECS().GetSystem<InGameUI>().ShowHoverOrder(component););

UI_FUNCTION(
    ToSpeceficUnit, OpenSpeceficUnit,
    for (auto [entity, selected]
         : bee::Engine.ECS().Registry.view<Selected>().each()) {
        bee::Engine.ECS().GetSystem<InGameUI>().currentFocusedEntity = entity;
        bee::Engine.ECS().GetSystem<InGameUI>().currentComponent = component;
        break;
    } bee::Engine.ECS()
        .GetSystem<InGameUI>()
        .SetSelectedUnit(component););

glm::vec2 leftSideButtonSize = glm::vec2(0.0f);
float leftSideButtonTextSize = 0.0f;

InGameUI::InGameUI()
{
    Title = "In game UI";
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    // load default stuff
    rtop = ui.serialiser->LoadElement(RIGHT_TOP_CORNER_NAME);
    rbot = ui.serialiser->LoadElement(RIGHT__BOTTOM_CORNER_NAME);
    rbotButtons = ui.serialiser->LoadElement("InGameBottomRightButtons");
    unitsPreview = ui.serialiser->LoadElement(LEFT__BOTTOM_CORNER_NAME);
    // load subwindows
    town = ui.serialiser->LoadElement("TownHall");
    Engine.ECS().GetSystem<ResourceSystem>().SetCallback(ResourceCallbackFunc);
    uiElementsTemplate = ui.serialiser->LoadElement(UI_ELEMENTS_TEMPLATE);

    ui.SetElementLayer(town, 5);
    m_buildMap.emplace(ui.GetComponentID(rbotButtons, "Barracksbutton"), OrderType::BuildBarracks);
    m_buildMap.emplace(ui.GetComponentID(rbotButtons, "MageTowerBuilding"), OrderType::BuildMageTower);
    m_buildMap.emplace(ui.GetComponentID(rbotButtons, "wallButton"), OrderType::BuildWallStart);
    m_buildMap.emplace(ui.GetComponentID(rbotButtons, "FenceButton"), OrderType::BuildWallStart);
    // m_buildMap.emplace(ui.GetComponentID(rbotButtons, "wallButton"), OrderType::BuildWallStart);

    // auto& textitem = ui.getComponentItem<Text>(unitsPreview, "unitsTitleText");
    // textitem.fullline = false;
    // textitem.Remake(textitem.element);

    buttonItem = ui.getComponentItem<sButton>(uiElementsTemplate, "buttonTemplate");
    textureItem = ui.getComponentItem<ui::Image>(uiElementsTemplate, "texTemplate");
    textItem = ui.getComponentItem<Text>(uiElementsTemplate, "textureTemplate");
    ui.DeleteComponent(buttonItem.ID);
    ui.DeleteComponent(textureItem.ID);
    ui.DeleteComponent(textItem.ID);

    auto& selectSystem = Engine.ECS().GetSystem<SelectionSystem>();
    selectSystem.SetSelectionCallback(AddUnitToPreview);
    selectSystem.SetDeselectionCallback(RemoveUnitFromPreview);

    auto& UI = bee::Engine.ECS().GetSystem<UserInterface>();
    m_healthBar.Img = UI.LoadTexture(Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/white.png").c_str());

    healthBars = UI.CreateUIElement(bee::ui::Alignment::top, bee::ui::Alignment::left);
    UI.SetElementLayer(healthBars, 10);
    m_initialCameraOrientation = glm::vec3(0.0f, -1.0f, 0.0f);

    auto& buildBarrackButton = ui.getComponentItem<sButton>(rbotButtons, "Barracksbutton");
    auto textcopyMelee = textItem;
    textcopyMelee.size = 0.2f;
    textcopyMelee.start = glm::vec2(buildBarrackButton.start.x, buildBarrackButton.start.y);
    textcopyMelee.name = std::string(magic_enum::enum_name(OrderType::BuildBarracks)) + "keybind";
    textcopyMelee.text = magic_enum::enum_name(
        bee::Engine.InputWrapper().GetDigitalActionKey(magic_enum::enum_name(OrderType::BuildBarracks).data()));
    textcopyMelee.Remake(rbotButtons);

    leftSideButtonTextSize = 0.2f;

    auto& buildMageTowerButton = ui.getComponentItem<sButton>(rbotButtons, "MageTowerBuilding");
    auto textcopyMage = textItem;
    textcopyMage.size = 0.2f;
    textcopyMage.start = glm::vec2(buildMageTowerButton.start.x, buildMageTowerButton.start.y);
    textcopyMage.name = std::string(magic_enum::enum_name(OrderType::BuildMageTower)) + "keybind";
    textcopyMage.text = magic_enum::enum_name(
        bee::Engine.InputWrapper().GetDigitalActionKey(magic_enum::enum_name(OrderType::BuildMageTower).data()));
    textcopyMage.Remake(rbotButtons);

    auto& buildWallButton = ui.getComponentItem<sButton>(rbotButtons, "wallButton");
    auto textcopyWall = textItem;
    textcopyWall.size = 0.2f;
    textcopyWall.start = glm::vec2(buildWallButton.start.x, buildWallButton.start.y);
    textcopyWall.name = std::string(magic_enum::enum_name(OrderType::BuildWallStart)) + "keybind";
    textcopyWall.text = magic_enum::enum_name(
        bee::Engine.InputWrapper().GetDigitalActionKey(magic_enum::enum_name(OrderType::BuildWallStart).data()));
    textcopyWall.Remake(rbotButtons);

    auto& buildFenceButton = ui.getComponentItem<sButton>(rbotButtons, "FenceButton");
    auto textcopyFence = textItem;
    textcopyFence.size = 0.2f;
    textcopyFence.start = glm::vec2(buildFenceButton.start.x, buildFenceButton.start.y);
    textcopyFence.name = std::string(magic_enum::enum_name(OrderType::BuildFenceStart)) + "keybind";
    textcopyFence.text = magic_enum::enum_name(
        bee::Engine.InputWrapper().GetDigitalActionKey(magic_enum::enum_name(OrderType::BuildFenceStart).data()));
    textcopyFence.Remake(rbotButtons);

    // presets
    m_UnitStarts.push_back(ui.getComponentItem<ui::Image>(unitsPreview, "Unit1").start);
    m_UnitStarts.push_back(ui.getComponentItem<ui::Image>(unitsPreview, "Unit2").start);
    m_orderStarts.push_back(ui.getComponentItem<ui::Image>(unitsPreview, "Order1").start);
    m_orderStarts.push_back(ui.getComponentItem<ui::Image>(unitsPreview, "Order2").start);
    m_orderStarts.push_back(ui.getComponentItem<ui::Image>(unitsPreview, "Order3").start);

    ui.DeleteComponent(ui.getComponentItem<ui::Image>(unitsPreview, "Unit1").ID);
    ui.DeleteComponent(ui.getComponentItem<ui::Image>(unitsPreview, "Unit2").ID);
    ui.DeleteComponent(ui.getComponentItem<ui::Image>(unitsPreview, "Order1").ID);
    ui.DeleteComponent(ui.getComponentItem<ui::Image>(unitsPreview, "Order2").ID);
    ui.DeleteComponent(ui.getComponentItem<ui::Image>(unitsPreview, "Order3").ID);

    {
        auto& buildMageTower = UI.getComponentItem<bee::ui::Image>(rbotButtons, "mageTowerTex");
        buildMageTower.imageFile = "assets/textures/UI/icons/MageTower_Button.png";
        buildMageTower.atlas = glm::vec4(0, 0, 1111, 1111);
        buildMageTower.Remake(rbotButtons);

        leftSideButtonSize = buildMageTower.size;

        auto& buildBarrack = UI.getComponentItem<bee::ui::Image>(rbotButtons, "swordsmenTex");
        buildBarrack.imageFile = "assets/textures/UI/icons/WarriorTower_Button.png";
        buildBarrack.atlas = glm::vec4(0, 0, 1111, 1111);
        buildBarrack.Remake(rbotButtons);

        auto& buildWall = UI.getComponentItem<bee::ui::Image>(rbotButtons, "wallTex");
        buildWall.imageFile = "assets/textures/UI/icons/StoneWall_Button.png";
        buildWall.atlas = glm::vec4(0, 0, 1111, 1111);
        buildWall.Remake(rbotButtons);

        auto& buildFence = UI.getComponentItem<bee::ui::Image>(rbotButtons, "FenceTex");
        buildFence.imageFile = "assets/textures/UI/icons/Fence_Button.png";
        buildFence.atlas = glm::vec4(0, 0, 1111, 1111);

        buildFence.Remake(rbotButtons);

        UI.SetDrawStateUIelement(rbotButtons, false);
        UI.SetDrawStateUIelement(rbot, false);
    }

    auto PorItem = ui.getComponentItem<ui::Image>(unitsPreview, "Portrait");
    PorItem.Delete();
    PorItem.Remake(uiElementsTemplate);
    ClearSelectedUnit();
    m_quedUnit = ui.serialiser->LoadElement("UnitQueued");
    ui.SetDrawStateUIelement(m_quedUnit, false);

    m_Hover = ui.serialiser->LoadElement("HoverTemplate");
    m_textLayer = ui.CreateUIElement(bee::ui::Alignment::bottom, bee::ui::Alignment::left);
    ui.SetElementLayer(m_textLayer, 3);

    m_arrowImage.Img = ui.LoadTexture(Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/Arrows.png").c_str());
    m_arrowsElement = UI.CreateUIElement(bee::ui::Alignment::top, bee::ui::Alignment::left);
    UI.SetElementLayer(m_arrowsElement, 3);
}

InGameUI::~InGameUI() {}

OrderType InGameUI::GetOrderType(bee::ui::UIComponentID id) const { return m_actionMap.find(id)->second; }

void InGameUI::CheckPopups()
{
    // popup checks
    if (!m_strings.find(Popups::FixRuins)->second.second)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<DebugMetricData>().each())
        {
            if (debugMetric.timeInsideGame > 5.0f && m_currentPopup == Popups::None)
            {
                ShowPopup(m_strings.find(Popups::FixRuins)->second.first);
                m_strings.find(Popups::FixRuins)->second.second = true;
                m_currentPopup = Popups::FixRuins;
            }
        }
    }

    if (!m_strings.find(Popups::ForestAttack)->second.second)
    {
        bool repairedRuin = true;
        for (auto [entity, allyStructure, transform] : bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform>().each())
        {
            if (transform.Name.find("TownHallLvl0") != std::string::npos)
            {
                repairedRuin = false;
                break;
            }
        }
        if (repairedRuin && m_currentPopup == Popups::FixRuins)
        {
            ShowPopup(m_strings.find(Popups::ForestAttack)->second.first);
            m_strings.find(Popups::ForestAttack)->second.second = true;
            m_currentPopup = Popups::ForestAttack;
            for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<DebugMetricData>().each())
                m_timer = debugMetric.timeInsideGame;
        }
    }

    if (!m_strings.find(Popups::FixBarAndMage)->second.second)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<DebugMetricData>().each())
        {
            if (m_currentPopup == Popups::ForestAttack)
            {
                if (debugMetric.timeInsideGame - m_timer > 2.0f)
                {
                    ShowPopup(m_strings.find(Popups::FixBarAndMage)->second.first);
                    m_strings.find(Popups::FixBarAndMage)->second.second = true;
                    m_currentPopup = Popups::FixBarAndMage;
                }
            }
        }
    }

    if (!m_strings.find(Popups::Train6Troops)->second.second)
    {
        bool fixedMage = true;
        bool fixedBar = true;
        for (auto [entity, allyStructure, transform] : bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform>().each())
        {
            fixedBar = transform.Name.find("SwordsmenTowerLvl0") != std::string::npos ? false : fixedBar;
            fixedMage = transform.Name.find("MageTowerLvl0") != std::string::npos ? false : fixedMage;
        }
        if (fixedBar && fixedMage && m_currentPopup == Popups::FixBarAndMage)
        {
            ShowPopup(m_strings.find(Popups::Train6Troops)->second.first);
            m_strings.find(Popups::Train6Troops)->second.second = true;
            m_currentPopup = Popups::Train6Troops;
        }
    }

    if (!m_strings.find(Popups::FirstWave)->second.second)
    {
        auto view = bee::Engine.ECS().Registry.view<AllyUnit>();
        if (view.size() > 5 && m_currentPopup == Popups::Train6Troops)
        {
            bee::Engine.ECS().GetSystem<WaveSystem>().Pause(false);
            ShowPopup(m_strings.find(Popups::FirstWave)->second.first);
            m_strings.find(Popups::FirstWave)->second.second = true;
            m_currentPopup = Popups::FirstWave;
        }
    }

    if (!m_strings.find(Popups::FirstEnemyDies)->second.second)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<DebugMetricData>().each())
        {
            if (debugMetric.enemyUnitsKilled.size() > 0 && m_currentPopup == Popups::FirstWave)
            {
                ShowPopup(m_strings.find(Popups::FirstEnemyDies)->second.first);
                m_strings.find(Popups::FirstEnemyDies)->second.second = true;
                m_currentPopup = Popups::FirstEnemyDies;
            }
        }
    }

    if (!m_strings.find(Popups::MoreWaves)->second.second)
    {
        if (bee::Engine.ECS().Registry.view<EnemyUnit>().size() == 0 && m_currentPopup == Popups::FirstEnemyDies)
        {
            auto& waveSystem = bee::Engine.ECS().GetSystem<WaveSystem>();
            waveSystem.Pause(true);
            waveSystem.ClearText();
            ShowPopup(m_strings.find(Popups::MoreWaves)->second.first);
            m_strings.find(Popups::MoreWaves)->second.second = true;
            m_currentPopup = Popups::MoreWaves;
            for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<DebugMetricData>().each())
                m_timer = debugMetric.timeInsideGame;
        }
    }

    if (!m_strings.find(Popups::BuildWall)->second.second)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<DebugMetricData>().each())
        {
            if (m_currentPopup == Popups::MoreWaves)
            {
                if (debugMetric.timeInsideGame - m_timer > 2.0f)
                {
                    auto& orderSystem = bee::Engine.ECS().GetSystem<OrderSystem>();
                    orderSystem.buttons.push_back(OrderType::BuildWallStart);

                    ShowPopup(m_strings.find(Popups::BuildWall)->second.first);
                    m_strings.find(Popups::BuildWall)->second.second = true;
                    m_currentPopup = Popups::BuildWall;

                    auto& UI = bee::Engine.ECS().GetSystem<UserInterface>();
                    UI.SetDrawStateUIelement(rbot, true);
                    UI.SetDrawStateUIelement(rbotButtons, true);
                    auto& buildMageTower = UI.getComponentItem<bee::ui::Image>(rbotButtons, "mageTowerTex");
                    buildMageTower.size = glm::vec2(0.0f);
                    buildMageTower.Remake(rbotButtons);

                    auto& buildBarrack = UI.getComponentItem<bee::ui::Image>(rbotButtons, "swordsmenTex");
                    buildBarrack.size = glm::vec2(0.0f);
                    buildBarrack.Remake(rbotButtons);

                    auto& buildFence = UI.getComponentItem<bee::ui::Image>(rbotButtons, "FenceTex");
                    buildFence.size = glm::vec2(0.0f);
                    buildFence.Remake(rbotButtons);

                    auto& buildMageTowerButton = UI.getComponentItem<sButton>(rbotButtons, "MageTowerBuilding");
                    buildMageTowerButton.size = glm::vec2(0.0f);
                    buildMageTowerButton.Remake(rbotButtons);

                    auto& buildBarrackTowerButton = UI.getComponentItem<sButton>(rbotButtons, "Barracksbutton");
                    buildBarrackTowerButton.size = glm::vec2(0.0f);
                    buildBarrackTowerButton.Remake(rbotButtons);

                    auto& buildFenceTowerButton = UI.getComponentItem<sButton>(rbotButtons, "FenceButton");
                    buildFenceTowerButton.size = glm::vec2(0.0f);
                    buildFenceTowerButton.Remake(rbotButtons);

                    auto& textcopyMelee = UI.getComponentItem<bee::ui::Text>(
                        rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildBarracks)) + "keybind");
                    textcopyMelee.size = 0.0;
                    textcopyMelee.Remake(rbotButtons);

                    auto& textcopyMage = UI.getComponentItem<bee::ui::Text>(
                        rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildMageTower)) + "keybind");
                    textcopyMage.size = 0.0;
                    textcopyMage.Remake(rbotButtons);

                     auto& textcopyFence = UI.getComponentItem<bee::ui::Text>(
                        rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildFenceStart)) + "keybind");
                    textcopyFence.size = 0.0f;
                    textcopyFence.Remake(rbotButtons);

                    m_buildMap.clear();
                    m_buildMap.emplace(UI.GetComponentID(rbotButtons, "Barracksbutton"), OrderType::BuildBarracks);
                    m_buildMap.emplace(UI.GetComponentID(rbotButtons, "MageTowerBuilding"), OrderType::BuildMageTower);
                    m_buildMap.emplace(UI.GetComponentID(rbotButtons, "wallButton"), OrderType::BuildWallStart);
                    m_buildMap.emplace(UI.GetComponentID(rbotButtons, "FenceButton"), OrderType::BuildFenceStart);
                }
            }
        }
    }

    if (!m_strings.find(Popups::Done)->second.second)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<DebugMetricData>().each())
        {
            if (debugMetric.buildingSpawned.find("Wall")->second>0 &&
                m_currentPopup == Popups::BuildWall)
            {
                auto& orderSystem = bee::Engine.ECS().GetSystem<OrderSystem>();
                orderSystem.buttons.push_back(OrderType::BuildMageTower);
                orderSystem.buttons.push_back(OrderType::BuildBarracks);
                orderSystem.buttons.push_back(OrderType::BuildFenceStart);
                RemovePopup();
                auto& waveSystem = bee::Engine.ECS().GetSystem<WaveSystem>();
                waveSystem.Pause(false);
                m_strings.find(Popups::Done)->second.second = true;
                m_currentPopup = Popups::Done;

                auto& UI = bee::Engine.ECS().GetSystem<UserInterface>();

                auto& buildMageTower = UI.getComponentItem<bee::ui::Image>(rbotButtons, "mageTowerTex");
                buildMageTower.size = leftSideButtonSize;
                buildMageTower.Remake(rbotButtons);

                auto& buildBarrack = UI.getComponentItem<bee::ui::Image>(rbotButtons, "swordsmenTex");
                buildBarrack.size = leftSideButtonSize;
                buildBarrack.Remake(rbotButtons);

                auto& buildFence = UI.getComponentItem<bee::ui::Image>(rbotButtons, "FenceTex");
                buildFence.size = leftSideButtonSize;
                buildFence.Remake(rbotButtons);

                auto& buildMageTowerButton = UI.getComponentItem<sButton>(rbotButtons, "MageTowerBuilding");
                buildMageTowerButton.size = leftSideButtonSize;
                buildMageTowerButton.Remake(rbotButtons);

                auto& buildBarrackTowerButton = UI.getComponentItem<sButton>(rbotButtons, "Barracksbutton");
                buildBarrackTowerButton.size = leftSideButtonSize;
                buildBarrackTowerButton.Remake(rbotButtons);

                auto& buildFenceTowerButton = UI.getComponentItem<sButton>(rbotButtons, "FenceButton");
                buildFenceTowerButton.size = leftSideButtonSize;
                buildFenceTowerButton.Remake(rbotButtons);

                auto& textcopyMelee = UI.getComponentItem<bee::ui::Text>(
                    rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildBarracks)) + "keybind");
                textcopyMelee.size = leftSideButtonTextSize;
                textcopyMelee.Remake(rbotButtons);

                auto& textcopyMage = UI.getComponentItem<bee::ui::Text>(
                    rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildMageTower)) + "keybind");
                textcopyMage.size = leftSideButtonTextSize;
                textcopyMage.Remake(rbotButtons);

                auto& textcopyFence = UI.getComponentItem<bee::ui::Text>(
                    rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildFenceStart)) + "keybind");
                textcopyFence.size = leftSideButtonTextSize;
                textcopyFence.Remake(rbotButtons);

                m_buildMap.clear();
                m_buildMap.emplace(UI.GetComponentID(rbotButtons, "Barracksbutton"), OrderType::BuildBarracks);
                m_buildMap.emplace(UI.GetComponentID(rbotButtons, "MageTowerBuilding"), OrderType::BuildMageTower);
                m_buildMap.emplace(UI.GetComponentID(rbotButtons, "wallButton"), OrderType::BuildWallStart);
                m_buildMap.emplace(UI.GetComponentID(rbotButtons, "FenceButton"), OrderType::BuildFenceStart);
            }
        }
    }
}
void const InGameUI::UpdateResources(GameResourceType type, int newAmount)
{
    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
    std::string name = "";
    switch (type)
    {
        case GameResourceType::Wood:
        {
            name = "WoodAmount";
            break;
        }
        case GameResourceType::Stone:
        {
            name = "StoneAmount";
            break;
        }
    }
    auto& TextItem = ui.getComponentItem<Text>(rtop, name);
    TextItem.text = std::to_string(newAmount);
    TextItem.Remake(TextItem.element);
}

void InGameUI::SetSelectedUnit(bee::ui::UIComponentID ID)
{
    //
    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
    if (ID != -1)
    {
        std::string name = "";
        std::string iconPath = "";
        float dam = 0.0f;
        float heal = 0.0f;
        glm::vec4 uvs = glm::vec4(1);
        if (m_mapUnitTypes.at(ID).second)
        {
            auto templ = bee::Engine.ECS().GetSystem<StructureManager>().GetStructureTemplate(m_mapUnitTypes.at(ID).first);
            auto attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(currentFocusedEntity);
            heal = attributes.GetValue(BaseAttributes::HitPoints);
            dam = 0.0f;
            uvs = glm::vec4(templ.iconTextureCoordinates.x, templ.iconTextureCoordinates.y,
                            templ.iconTextureCoordinates.x + templ.iconTextureCoordinates.z,
                            templ.iconTextureCoordinates.y + templ.iconTextureCoordinates.w);
            iconPath = templ.iconPath;
            name = templ.name;
        }
        else
        {
            auto templ = bee::Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(m_mapUnitTypes.at(ID).first);
            auto attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(currentFocusedEntity);
            heal = attributes.GetValue(BaseAttributes::HitPoints);
            dam = attributes.GetValue(BaseAttributes::Damage);
            uvs = glm::vec4(templ.iconTextureCoordinates.x, templ.iconTextureCoordinates.y,
                            templ.iconTextureCoordinates.x + templ.iconTextureCoordinates.z,
                            templ.iconTextureCoordinates.y + templ.iconTextureCoordinates.w);
            iconPath = templ.iconPath;
            name = templ.name;
        }

        auto& damItem = ui.getComponentItem<ui::Text>(unitsPreview, "DamageText");
        std::vector<std::string> strings1 = SplitString(std::to_string(dam), ".");
        damItem.text = strings1.at(0);
        damItem.Remake(damItem.element);

        auto& healItem = ui.getComponentItem<ui::Text>(unitsPreview, "HealthText");
        std::vector<std::string> strings2 = SplitString(std::to_string(heal), ".");
        healItem.text = strings2.at(0);
        healItem.Remake(healItem.element);

        auto& namItem = ui.getComponentItem<ui::Text>(unitsPreview, "NameText");
        namItem.text = name;
        namItem.Remake(namItem.element);

        auto& PorItem = ui.getComponentItem<ui::Image>(uiElementsTemplate, "Portrait");
        PorItem.atlas = uvs;
        PorItem.imageFile = iconPath;
        PorItem.Remake(PorItem.element);

        ui.ReplaceString(ui.GetComponentID(unitsPreview, "Health:"), "Health:");
        ui.ReplaceString(ui.GetComponentID(unitsPreview, "Name:"), "Name:");
        ui.ReplaceString(ui.GetComponentID(unitsPreview, "Damage:"), "Damage:");

        selectedUnitType = name;
    }
    else
    {
        auto& damItem = ui.getComponentItem<ui::Text>(unitsPreview, "DamageText");
        damItem.text = "";
        damItem.Remake(damItem.element);

        auto& healItem = ui.getComponentItem<ui::Text>(unitsPreview, "HealthText");
        healItem.text = "";
        healItem.Remake(healItem.element);

        auto& namItem = ui.getComponentItem<ui::Text>(unitsPreview, "NameText");
        namItem.text = "";
        namItem.Remake(namItem.element);

        ui.ReplaceString(ui.GetComponentID(unitsPreview, "Health:"), "");
        ui.ReplaceString(ui.GetComponentID(unitsPreview, "Name:"), "");
        ui.ReplaceString(ui.GetComponentID(unitsPreview, "Damage:"), "");

        auto& PorItem = ui.getComponentItem<ui::Image>(uiElementsTemplate, "Portrait");
        auto size = PorItem.size;
        PorItem.size = glm::vec2(0.0f);
        PorItem.atlas = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        PorItem.imageFile = "assets/textures/checkerboard.png";
        PorItem.Remake(PorItem.element);
        PorItem.size = size;
    }
}

void InGameUI::ClearSelectedUnit()
{
    //
    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();

    auto& damItem = ui.getComponentItem<ui::Text>(unitsPreview, "DamageText");
    damItem.text = "";
    damItem.Remake(damItem.element);

    auto& healItem = ui.getComponentItem<ui::Text>(unitsPreview, "HealthText");
    healItem.text = "";
    healItem.Remake(healItem.element);

    auto& namItem = ui.getComponentItem<ui::Text>(unitsPreview, "NameText");
    namItem.text = "";
    namItem.Remake(namItem.element);

    auto& PorItem = ui.getComponentItem<ui::Image>(uiElementsTemplate, "Portrait");
    PorItem.atlas = RESET_ATLAS;
    PorItem.imageFile = Engine.FileIO().GetPath(FileIO::Directory::Asset, RESETLOC);
    PorItem.Remake(PorItem.element);

    selectedUnitType = "";
}

void InGameUI::ShowHoverOrder(bee::ui::UIComponentID id)
{
    m_hoveringOrder = true;
    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
    auto& item = ui.getComponentItem<bee::ui::Image>(m_Hover, "backGround");
    for (auto& rem : hoverTextIds)
    {
        ui.DeleteComponent(rem);
    }
    hoverTextIds.clear();
    glm::vec2 newMousePos;
    glm::vec2 mousePos = Engine.Input().GetMousePosition();
    if (Engine.Inspector().GetVisible())
    {
        auto pos = Engine.Inspector().GetGamePos();
        auto size = Engine.Inspector().GetGameSize();
        glm::vec2 newMousePos2 = glm::vec2(mousePos.x - pos.x, mousePos.y - pos.y);
        newMousePos = glm::vec2((newMousePos2.x / size.x) * (size.x / size.y), newMousePos2.y / (size.y));
    }
    else
    {
        float height = static_cast<float>(Engine.Device().GetHeight());
        float width = static_cast<float>(Engine.Device().GetWidth());
        newMousePos = glm::vec2((mousePos.x / width) * (width / height), mousePos.y / (height));
    }

    item.start = glm::vec2(newMousePos.x, newMousePos.y - item.size.y);
    item.Remake(item.element);

    float fontsize = 0.1f;
    float enterSize = ui.PreCalculateStringEnds(m_Hover, 0, " ", glm::vec2(0, 0), fontsize, false).y;

    const float xoffset = item.start.x;
    const float yoffset = item.start.y + enterSize;
    float windowSize = item.size.x + item.start.x;

    glm::vec4 colour = glm::vec4(0.278f, 0.196f, 0.141f, 1.0f);
    auto Order = m_actionMap.at(id);
    std::string content = m_orderStrings.at(Order);
    auto cost = Engine.ECS().GetSystem<OrderSystem>().GetOrderCost(Order);
    bool showUpgradePrice = true;
    if (Order == OrderType::UpgradeBuilding)
    {
        auto viewStructures = Engine.ECS().Registry.view<Selected, AllyStructure>();
        if (viewStructures.size_hint() > 0)
        {
            const auto& attrib = Engine.ECS().Registry.get<AttributesComponent>(viewStructures.front());
            auto upgradeLvl = static_cast<int>(attrib.GetEntityType()[attrib.GetEntityType().size() - 1]) - 47;
            if (upgradeLvl > 1 && !IsTutorialDone())
            {
                content.append("Finish the tutorial before upgrading further");
                showUpgradePrice = false;
            }
            else
            {
                if (StringStartsWith(attrib.GetEntityType(), "TownHall"))
                {
                    content.append(
                        "Upgrading your cathedral to level X will unlock your barrack and mage tower level X upgrades");
                    auto firstPosition = content.find(" X ");
                    content[firstPosition + 1] = std::to_string(upgradeLvl)[0];
                    auto secondPosition = content.find(" X ");
                    content[secondPosition + 1] = std::to_string(upgradeLvl)[0];
                }
                else if (StringStartsWith(attrib.GetEntityType(), "SwordsmenTower") ||
                         StringStartsWith(attrib.GetEntityType(), "MageTower"))
                {
                    if (!cost.second) content.append("Requires a Town Hall of higher of equal level to the upgrade level.\n");
                    content.append("This upgrade to level X will reduce the training cost and decrease the training time");
                    auto firstPosition = content.find(" X ");
                    content[firstPosition + 1] = std::to_string(upgradeLvl)[0];
                }
            }
        }
    }

    if (cost.first.x == 0 && cost.first.y == 0)
    {
        // no cost
    }
    else
    {
        if (showUpgradePrice)
        {
            // there is cost
            content.append("\ncost:");
            bool previousAdded = false;
            if (cost.first.x != 0)
            {
                std::string coster = std::to_string(cost.first.x);
                std::vector<std::string> strings1 = SplitString(coster, ".");
                content.append(strings1.at(0) + " wood");
                previousAdded = true;
                auto woodResource =
                    Engine.ECS().GetSystem<ResourceSystem>().playerResourceData.resources.at(GameResourceType::Wood);
                if (woodResource < cost.first.x) cost.second = false;
            }

            if (cost.first.y != 0)
            {
                if (previousAdded)
                {
                    content.append(", ");
                }
                std::string coster = std::to_string(cost.first.y);
                std::vector<std::string> strings1 = SplitString(coster, ".");
                content.append(strings1.at(0) + " stone");
                auto stoneResource =
                    Engine.ECS().GetSystem<ResourceSystem>().playerResourceData.resources.at(GameResourceType::Stone);
                if (stoneResource < cost.first.x) cost.second = false;
            }
        }
    }
    hoverTextIds = MakeWrappedString(m_Hover, content, glm::vec2(xoffset, yoffset), fontsize, windowSize, colour, cost.second);
}

void InGameUI::ShowHoverBuild(bee::ui::UIComponentID id)
{
    m_hoveringOrder = true;
    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
    auto& item = ui.getComponentItem<bee::ui::Image>(m_Hover, "backGround");
    for (auto& rem : hoverTextIds)
    {
        ui.DeleteComponent(rem);
    }
    hoverTextIds.clear();

    glm::vec2 newMousePos;
    glm::vec2 mousePos = Engine.Input().GetMousePosition();
    if (Engine.Inspector().GetVisible())
    {
        auto pos = Engine.Inspector().GetGamePos();
        auto size = Engine.Inspector().GetGameSize();
        glm::vec2 newMousePos2 = glm::vec2(mousePos.x - pos.x, mousePos.y - pos.y);
        newMousePos = glm::vec2((newMousePos2.x / size.x) * (size.x / size.y), newMousePos2.y / (size.y));
    }
    else
    {
        float height = static_cast<float>(Engine.Device().GetHeight());
        float width = static_cast<float>(Engine.Device().GetWidth());
        newMousePos = glm::vec2((mousePos.x / width) * (width / height), mousePos.y / (height));
    }

    item.start = glm::vec2(newMousePos.x - item.size.x, newMousePos.y - item.size.y);
    item.Remake(item.element);

    float fontsize = 0.1f;
    float enterSize = ui.PreCalculateStringEnds(m_Hover, 0, " ", glm::vec2(0, 0), fontsize, false).y;

    const float xoffset = item.start.x;
    const float yoffset = item.start.y + enterSize;
    float windowSize = item.size.x + item.start.x;

    glm::vec4 colour = glm::vec4(0.278f, 0.196f, 0.141f, 1.0f);

    if (m_buildMap.find(id) == m_buildMap.end()) return;

    auto Order = m_buildMap.at(id);
    std::string content = m_orderStrings.at(Order);

    if (Order == OrderType::UpgradeBuilding)
    {
    }
    auto cost = Engine.ECS().GetSystem<OrderSystem>().GetOrderCost(Order);
    if (cost.first.x == 0 && cost.first.y == 0)
    {
        // no cost
    }
    else
    {
        // there is cost
        content.append("\ncost:");
        bool previousAdded = false;
        if (cost.first.x != 0)
        {
            std::string coster = std::to_string(cost.first.x);
            std::vector<std::string> strings1 = SplitString(coster, ".");
            content.append(strings1.at(0) + " wood ");
            previousAdded = true;
            auto woodResource =
                Engine.ECS().GetSystem<ResourceSystem>().playerResourceData.resources.at(GameResourceType::Wood);
            if (woodResource < cost.first.x) cost.second = false;
        }

        if (cost.first.y != 0)
        {
            if (previousAdded)
            {
                content.append(",");
            }
            std::string coster = std::to_string(cost.first.y);
            std::vector<std::string> strings1 = SplitString(coster, ".");
            content.append(strings1.at(0) + " stone ");
            auto stoneResource =
                Engine.ECS().GetSystem<ResourceSystem>().playerResourceData.resources.at(GameResourceType::Stone);
            if (stoneResource < cost.first.x) cost.second = false;
        }
    }

    hoverTextIds = MakeWrappedString(m_Hover, content, glm::vec2(xoffset, yoffset), fontsize, windowSize, colour, cost.second);
}

bool InGameUI::IsTutorialDone() { return m_currentPopup == Popups::Done; }

void InGameUI::Update(float dt)
{
    auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();

    ClearPreview();
    currentComponent = -1;
    SelectUnits();
    SetSelectedUnit(currentComponent);
    CheckPopups();

    ui.SetDrawStateUIelement(m_Hover, m_hoveringOrder);
    m_hoveringOrder = false;
    auto allyUnits = Engine.ECS().Registry.view<AllyUnit, AttributesComponent>();
    auto& orderSystem = bee::Engine.ECS().GetSystem<OrderSystem>();
    auto magesLimit = orderSystem.mageLimit;
    auto swordsmenLimit = orderSystem.swordsmenLimit;
    const int mages = orderSystem.GetNumberOfMages();
    const int warriors = orderSystem.GetNumberOfSwordsmen();

    auto& wTextItem = ui.getComponentItem<Text>(rtop, "WarriorAmount");
    wTextItem.text = std::to_string(warriors) + "/" + std::to_string(swordsmenLimit);
    wTextItem.Remake(wTextItem.element);
    auto& mTextItem = ui.getComponentItem<Text>(rtop, "MageAmount");
    mTextItem.text = std::to_string(mages) + "/" + std::to_string(magesLimit);
    mTextItem.Remake(wTextItem.element);

    const auto screen = bee::Engine.ECS().Registry.view<bee::Transform, bee::Camera>();
    for (auto [entity, transform, camera] : screen.each())
    {
        glm::quat cameraWorldOrientation = transform.Rotation;
        m_currentCameraOrientation = glm::normalize(glm::rotate(cameraWorldOrientation, m_initialCameraOrientation));
    }
    UpdateHeathBar();
    ShowEnemiesGeneralDirection();
    auto& UI = bee::Engine.ECS().GetSystem<UserInterface>();

    if (bee::Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::F1) && !IsTutorialDone())
    {
        auto& waveSystem = bee::Engine.ECS().GetSystem<WaveSystem>();
        RemovePopup();
        m_currentPopup = Popups::Done;
        waveSystem.Pause(false);
        auto& orderSystem = bee::Engine.ECS().GetSystem<OrderSystem>();
        orderSystem.buttons.clear();
        orderSystem.buttons.push_back(OrderType::BuildMageTower);
        orderSystem.buttons.push_back(OrderType::BuildBarracks);
        orderSystem.buttons.push_back(OrderType::BuildWallStart);
        orderSystem.buttons.push_back(OrderType::BuildFenceStart);
        auto& UI = bee::Engine.ECS().GetSystem<UserInterface>();
        UI.SetDrawStateUIelement(rbot, true);
        UI.SetDrawStateUIelement(rbotButtons, true);

        auto& buildMageTower = UI.getComponentItem<bee::ui::Image>(rbotButtons, "mageTowerTex");
        buildMageTower.size = leftSideButtonSize;
        buildMageTower.Remake(rbotButtons);

        auto& buildBarrack = UI.getComponentItem<bee::ui::Image>(rbotButtons, "swordsmenTex");
        buildBarrack.size = leftSideButtonSize;
        buildBarrack.Remake(rbotButtons);

        auto& buildFence = UI.getComponentItem<bee::ui::Image>(rbotButtons, "FenceTex");
        buildFence.size = leftSideButtonSize;
        buildFence.Remake(rbotButtons);

        auto& buildMageTowerButton = UI.getComponentItem<sButton>(rbotButtons, "MageTowerBuilding");
        buildMageTowerButton.size = leftSideButtonSize;
        buildMageTowerButton.Remake(rbotButtons);

        auto& buildBarrackTowerButton = UI.getComponentItem<sButton>(rbotButtons, "Barracksbutton");
        buildBarrackTowerButton.size = leftSideButtonSize;
        buildBarrackTowerButton.Remake(rbotButtons);

        auto& buildFenceTowerButton = UI.getComponentItem<sButton>(rbotButtons, "FenceButton");
        buildFenceTowerButton.size = leftSideButtonSize;
        buildFenceTowerButton.Remake(rbotButtons);

        auto& textcopyMelee = UI.getComponentItem<bee::ui::Text>(
            rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildBarracks)) + "keybind");
        textcopyMelee.size = leftSideButtonTextSize;
        textcopyMelee.Remake(rbotButtons);

        auto& textcopyMage = UI.getComponentItem<bee::ui::Text>(
            rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildMageTower)) + "keybind");
        textcopyMage.size = leftSideButtonTextSize;
        textcopyMage.Remake(rbotButtons);

        auto& textcopyFence = UI.getComponentItem<bee::ui::Text>(
            rbotButtons, std::string(magic_enum::enum_name(OrderType::BuildFenceStart)) + "keybind");
        textcopyFence.size = leftSideButtonTextSize;
        textcopyFence.Remake(rbotButtons);

        m_buildMap.clear();
        m_buildMap.emplace(UI.GetComponentID(rbotButtons, "Barracksbutton"), OrderType::BuildBarracks);
        m_buildMap.emplace(UI.GetComponentID(rbotButtons, "MageTowerBuilding"), OrderType::BuildMageTower);
        m_buildMap.emplace(UI.GetComponentID(rbotButtons, "wallButton"), OrderType::BuildWallStart);
        m_buildMap.emplace(UI.GetComponentID(rbotButtons, "FenceButton"), OrderType::BuildWallStart);
    }

    for (auto [entity, allyStructure, transform, attrib] :
         bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform, AttributesComponent>().each())
    {
        if (transform.Name.find("TownHall") != std::string::npos)
        {
            auto upgradeLvl = static_cast<int>(attrib.GetEntityType()[attrib.GetEntityType().size() - 1]) - 48;
            ui.ReplaceString(ui.GetComponentID(town, "townhallAmount"), std::to_string(upgradeLvl));
            break;
        }
    }
}

void InGameUI::ShowPopup(const std::string& content)
{
    if (m_popupID != entt::null)
    {
        RemovePopup();
    }

    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;

    float fontsize = 0.12f;
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    m_popupID = ui.serialiser->LoadElement("PopupTemplate");
    auto& item = ui.getComponentItem<bee::ui::Image>(m_popupID, "backGround");
    item.start = glm::vec2((norWidth / 2) - (item.size.x / 2), 1.0f - item.size.y);
    item.Remake(item.element);

    auto& butt = ui.getComponentItem<bee::ui::sButton>(m_popupID, "CloseButton");
    auto& closeButtTex = ui.getComponentItem<bee::ui::Image>(m_popupID, "CloseTex");

    butt.start += item.start;
    closeButtTex.start += item.start;
    closeButtTex.Remake(butt.element);
    butt.Remake(butt.element);

    float enterSize = ui.PreCalculateStringEnds(m_popupID, 0, " ", glm::vec2(0, 0), fontsize, false).y;

    const float xoffset = 0.04115f + item.start.x;
    const float yoffset = 0.0823f + item.start.y;
    float windowSize = item.size.x + item.start.x - 0.04115f;

    glm::vec4 colour = glm::vec4(0.278f, 0.196f, 0.141f, 1.0f);
    MakeWrappedString(m_popupID, content, glm::vec2(xoffset, yoffset), fontsize, windowSize, colour, false);
}

void InGameUI::RemovePopup()
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    if (bee::Engine.ECS().Registry.all_of<UIElementID>(m_popupID)) ui.DeleteUIElement(m_popupID);
    m_popupID = entt::null;
}

void InGameUI::ShowEnemiesGeneralDirection()
{
    const auto pos = Engine.Inspector().GetGamePos();
    const auto size = Engine.Inspector().GetGameSize();
    const auto resolution = (size.x / size.y);

    auto& UI = Engine.ECS().GetSystem<UserInterface>();
    int index = 0;
    auto& waveSystem = Engine.ECS().GetSystem<WaveSystem>();
    for (auto enemyPosition : waveSystem.GetEnemySpawnLocation())
    {
        // Calculates the point in screen space where the enemy transform would be
        glm::vec2 circleOrigin = bee::FromWorldToScreen(glm::vec3(enemyPosition, 0.5f));
        // Shows an arrow only if the enemy is outside the screen
        if (circleOrigin.x >= 0 && circleOrigin.x <= size.x && circleOrigin.y >= 0 && circleOrigin.y <= size.y) continue;

        // Checks to see if it could reuse already existing textureComponents
        if (index >= m_arrows.size())
            m_arrows.push_back(UI.CreateTextureComponentFromAtlas(m_arrowsElement, m_arrowImage.Img, m_arrowImage.GetAtlas(),
                                                                  glm::vec2(0.0f), glm::vec2(0.0f), 0,
                                                                  "arrow" + std::to_string(index)));

        auto& arrow = UI.getComponentItem<ui::Image>(m_arrowsElement, "arrow" + std::to_string(index));

        // Clamp the position to the screen boarders
        auto arrowDirection = CalculatePointOnEdge(circleOrigin, pos, size, resolution);
        auto uvs = m_arrowImage.GetAtlas();
        auto imgSize = glm::vec2(uvs.z / 4.0f, uvs.w / 2.0f);
        switch (arrowDirection)
        {
            case ArrowDirection::Bottom:
            {
                uvs = glm::vec4(2 * imgSize.x, imgSize.y, 3 * imgSize.x, 2 * imgSize.y);
                break;
            }
            case ArrowDirection::Top:
            {
                uvs = glm::vec4(imgSize.x, imgSize.y, 2 * imgSize.x, 2 * imgSize.y);
                break;
            }
            case ArrowDirection::Left:
            {
                uvs = glm::vec4(imgSize.x, 0.0f, 2 * imgSize.x, imgSize.y);
                break;
            }
            case ArrowDirection::Right:
            {
                uvs = glm::vec4(2 * imgSize.x, 0.0f, 3 * imgSize.x, imgSize.y);
                break;
            }
            case ArrowDirection::LeftTop:
            {
                uvs = glm::vec4(0.0f, 0.0f, imgSize.x, imgSize.y);
                break;
            }
            case ArrowDirection::LeftBottom:
            {
                uvs = glm::vec4(0.0f, imgSize.y, imgSize.x, 2 * imgSize.y);
                break;
            }
            case ArrowDirection::RightTop:
            {
                uvs = glm::vec4(3 * imgSize.x, 0.0f, 4 * imgSize.x, imgSize.y);
                break;
            }
            case ArrowDirection::RightBottom:
            {
                uvs = glm::vec4(3 * imgSize.x, imgSize.y, 4 * imgSize.x, 2 * imgSize.y);
                break;
            }
        }

        // Updates the arrow
        arrow.atlas = uvs;
        arrow.size = glm::vec2(arrowSize, arrowSize);
        arrow.start = circleOrigin;
        arrow.Remake(m_arrowsElement);

        index++;
    }

    for (; index < m_arrows.size(); index++)
    {
        auto& arrow = UI.getComponentItem<ui::Image>(m_arrowsElement, "arrow" + std::to_string(index));
        arrow.size = glm::vec2(0.0f, 0.0f);
        arrow.Remake(m_arrowsElement);
    }
}

ArrowDirection InGameUI::CalculatePointOnEdge(glm::vec2& point, const glm::vec2& pos, const glm::vec2& size,
                                              const float resolution)
{
    ArrowDirection arrow;

    if (point.x < pos.x) arrow = ArrowDirection::Left;
    if (point.x > pos.x + size.x) arrow = ArrowDirection::Right;
    if (point.y < pos.y) arrow = ArrowDirection::Top;
    if (point.y > pos.y + size.y) arrow = ArrowDirection::Bottom;
    if (point.x < pos.x && point.y < pos.y) arrow = ArrowDirection::LeftTop;
    if (point.x > pos.x + size.x && point.y < pos.y) arrow = ArrowDirection::RightTop;
    if (point.x < pos.x + size.x && point.y > pos.y + size.y) arrow = ArrowDirection::LeftBottom;
    if (point.x > pos.x && point.y > pos.y + size.y) arrow = ArrowDirection::RightBottom;

    glm::vec2 center = pos + size / 2.0f;
    // Calculate the direction vector from the center to the point
    float dirX = point.x - center.x;
    float dirY = point.y - center.y;

    // Variables to store the intersection point
    float tMin = 0, tMax = 1;
    float t;

    // Left edge (x = box.A.x)
    if (dirX != 0)
    {
        t = (pos.x - center.x) / dirX;
        if (t >= 0 && t <= 1)
        {
            double y = center.y + t * dirY;
            if (y >= pos.y && y <= pos.y + size.y)
            {
                tMin = std::max(tMin, t);
                tMax = std::min(tMax, t);
            }
        }
    }

    // Right edge (x = box.B.x)
    if (dirX != 0)
    {
        t = (pos.x + size.x - center.x) / dirX;
        if (t >= 0 && t <= 1)
        {
            double y = center.y + t * dirY;
            if (y >= pos.y && y <= pos.y + size.y)
            {
                tMin = std::max(tMin, t);
                tMax = std::min(tMax, t);
            }
        }
    }

    // Top edge (y = box.A.y)
    if (dirY != 0)
    {
        t = (pos.y - center.y) / dirY;
        if (t >= 0 && t <= 1)
        {
            double x = center.x + t * dirX;
            if (x >= pos.x && x <= pos.x + size.x)
            {
                tMin = std::max(tMin, t);
                tMax = std::min(tMax, t);
            }
        }
    }

    // Bottom edge (y = box.D.y)
    if (dirY != 0)
    {
        t = (pos.y + size.y - center.y) / dirY;
        if (t >= 0 && t <= 1)
        {
            double x = center.x + t * dirX;
            if (x >= pos.x && x <= pos.x + size.x)
            {
                tMin = std::max(tMin, t);
                tMax = std::min(tMax, t);
            }
        }
    }

    // Find the clamped point using the largest tMin within [0, 1]
    float xOffset = 0.0f;
    float yOffset = 0.0f;

    if (tMin * dirX > 0) xOffset = -arrowSize;
    if (tMin * dirY > 0) yOffset = -arrowSize;

    point = glm::vec2(center.x + tMin * dirX, center.y + tMin * dirY);
    point = glm::vec2((point.x / size.x) * resolution + xOffset, point.y / (size.y) + yOffset);

    return arrow;
}

void InGameUI::UpdateHeathBar()
{
    auto& UI = Engine.ECS().GetSystem<UserInterface>();
    int index = 0;

    index = UpdateSelectedUnits(index);
    index = UpdateSelectedBuilding(index);
    index = UpdateEnemies(index);

    for (; index < m_healthBars.size(); index++)
    {
        auto& healthBar = UI.getComponentItem<sProgressBar>(healthBars, "healthBar" + std::to_string(index));
        healthBar.size = glm::vec2(0.0f, 0.0f);
        healthBar.Remake(healthBars);
    }
}

int InGameUI::UpdateSelectedUnits(int index)
{
    const auto pos = Engine.Inspector().GetGamePos();
    const auto size = Engine.Inspector().GetGameSize();
    const auto resolution = (size.x / size.y);

    auto& unitManager = bee::Engine.ECS().GetSystem<UnitManager>();

    auto& UI = Engine.ECS().GetSystem<UserInterface>();

    auto viewSelectedUnits = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform, AllyUnit, Selected>();
    for (auto [entity, attributes, transform, unit, selected] : viewSelectedUnits.each())
    {
        // Checks to see if it could reuse already existing progress bars
        if (index >= m_healthBars.size())
            m_healthBars.push_back(UI.CreateProgressBar(healthBars, glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f),
                                                        m_healthBar.Img, m_healthBar.GetAtlas(), glm::vec4(1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 0.8f), 10, 100, "healthBar" + std::to_string(index)));

        // Calculates the health bar percentage
        auto& unitTemplate = unitManager.GetUnitTemplate(attributes.GetEntityType());
        const float hpPercent =
            attributes.GetValue(BaseAttributes::HitPoints) / unitTemplate.GetAttribute(BaseAttributes::HitPoints);
        auto& healthBar = UI.getComponentItem<sProgressBar>(healthBars, "healthBar" + std::to_string(index));

        UpdateHealthBar(transform, hpPercent, pos, size, resolution, attributes.GetValue(BaseAttributes::SelectionRange),
                        healthBar,false);
        index++;
    }
    return index;
}

int InGameUI::UpdateSelectedBuilding(int index)
{
    const auto pos = Engine.Inspector().GetGamePos();
    const auto size = Engine.Inspector().GetGameSize();
    const auto resolution = (size.x / size.y);

    auto& structureManager = bee::Engine.ECS().GetSystem<StructureManager>();

    auto& UI = Engine.ECS().GetSystem<UserInterface>();

    auto viewSelectedStructure =
        bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform, AllyStructure, Selected>();
    for (auto [entity, attributes, transform, structure, selected] : viewSelectedStructure.each())
    {
        // Checks to see if it could reuse already existing progress bars
        if (index >= m_healthBars.size())
            m_healthBars.push_back(UI.CreateProgressBar(healthBars, glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f),
                                                        m_healthBar.Img, m_healthBar.GetAtlas(), glm::vec4(1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 0.8f), 10, 100, "healthBar" + std::to_string(index)));

        // Calculates the health bar percentage
        auto& structureTemplate = structureManager.GetStructureTemplate(attributes.GetEntityType());
        const float hpPercent =
            attributes.GetValue(BaseAttributes::HitPoints) / structureTemplate.GetAttribute(BaseAttributes::HitPoints);
        auto& healthBar = UI.getComponentItem<sProgressBar>(healthBars, "healthBar" + std::to_string(index));

        UpdateHealthBar(transform, hpPercent, pos, size, resolution, attributes.GetValue(BaseAttributes::SelectionRange),
                        healthBar,false);
        index++;
    }

    return index;
}

int InGameUI::UpdateEnemies(int index)
{
    const auto pos = Engine.Inspector().GetGamePos();
    const auto size = Engine.Inspector().GetGameSize();
    const auto resolution = (size.x / size.y);
    auto& unitManager = bee::Engine.ECS().GetSystem<UnitManager>();

    auto& UI = Engine.ECS().GetSystem<UserInterface>();

    auto viewEnemies = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform, EnemyUnit>();
    for (auto [entity, attributes, transform, unit] : viewEnemies.each())
    {
        // Checks to see if it could reuse already existing progress bars
        if (index >= m_healthBars.size())
            m_healthBars.push_back(UI.CreateProgressBar(healthBars, glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f),
                                                        m_healthBar.Img, m_healthBar.GetAtlas(), glm::vec4(1.0f),
                                                        glm::vec4(0.5f,0.5f,0.5f,0.8f), 10, 100, "healthBar" + std::to_string(index)));

        // Calculates the health bar percentage
        auto& unitTemplate = unitManager.GetUnitTemplate(attributes.GetEntityType());
        const float hpPercent =
            attributes.GetValue(BaseAttributes::HitPoints) / unitTemplate.GetAttribute(BaseAttributes::HitPoints);
        auto& healthBar = UI.getComponentItem<sProgressBar>(healthBars, "healthBar" + std::to_string(index));
        UpdateHealthBar(transform, hpPercent, pos, size, resolution, attributes.GetValue(BaseAttributes::SelectionRange),
                        healthBar,true);
        index++;
    }

    return index;
}

void InGameUI::UpdateHealthBar(const bee::Transform& transform, float healthPercent, const glm::vec2& pos,
                               const glm::vec2& size, const float resolution, double range, bee::ui::sProgressBar& healthBar,
                               bool enemy)
{
    // Calculates the point in screen space where to draw the health bar
    const glm::vec2 circleOrigin = bee::FromWorldToScreen(transform.Translation);
    const glm::vec2 colliderPoint =
        bee::FromWorldToScreen(transform.Translation + m_currentCameraOrientation * static_cast<float>(range));

    // Calculates a unit point used for calculating the size of the size of the health bar
    const glm::vec2 colliderPointUnit = bee::FromWorldToScreen(transform.Translation + m_currentCameraOrientation);
    const auto UIRadius = (colliderPointUnit - circleOrigin).y / (size.y);

    // Calculates the health bar position in the UI coordinate system
    glm::vec2 position = glm::vec2((colliderPoint.x / size.x) * resolution, colliderPoint.y / (size.y));

    // Updates the health bar
    if (enemy)
        healthBar.fColor = glm::vec4(1.0f,0.0f, 0.0f, 1.0f);
    else
        healthBar.fColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    healthBar.value = healthPercent * 100;
    healthBar.size = glm::vec2(UIRadius * 1.0f * static_cast<float>(range), UIRadius * 0.25f);
    healthBar.start = position - glm::vec2(healthBar.size.x / 2.0, 0.0f);
    healthBar.Remake(healthBars);
}

bool InGameUI::IsWaveSystemPaused() const { return m_currentPopup == Popups::Done || m_currentPopup == Popups::FirstWave; }

void InGameUI::SetInputState(bool newState)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    if (m_popupID != entt::null)
    {
        ui.SetInputStateUIelement(m_popupID, newState);
    }
    ui.SetInputStateUIelement(rtop, newState);
    ui.SetInputStateUIelement(rbot, newState);
    ui.SetInputStateUIelement(rbotButtons, newState);
    ui.SetInputStateUIelement(town, newState);
    ui.SetInputStateUIelement(unitsPreview, newState);
    ui.SetInputStateUIelement(uiElementsTemplate, newState);
}

void InGameUI::SelectUnits()
{
    auto viewUnits = Engine.ECS().Registry.view<Selected, AllyUnit, AttributesComponent>();
    auto& UI = Engine.ECS().GetSystem<UserInterface>();
    bool unitSelected = false;
    bool clearSel = false;
    // There are more entities than that fit into the window. we need a new way to display these
    std::unordered_map<std::string, int> counter;
    std::unordered_map<std::string, entt::entity> firstEntity;
    if (!bee::Engine.ECS().Registry.all_of<Selected>(currentFocusedEntity))
    {
        currentFocusedEntity = entt::null;
        entityType = "";
    }
    for (auto [entity, sel, unit, attrib] : viewUnits.each())
    {
        if (currentFocusedEntity == entt::null)
        {
            currentFocusedEntity = entity;
            entityType = attrib.GetEntityType();
        }
        unitSelected = true;

        if (counter.count(attrib.GetEntityType()) == 0)
        {
            counter.emplace(attrib.GetEntityType(), 0);
        }
        if (firstEntity.count(attrib.GetEntityType()) == 0)
        {
            firstEntity.emplace(attrib.GetEntityType(), entity);
        }
        counter.find(attrib.GetEntityType())->second++;
    }

    for (auto& count : counter)
    {
        entityInList unit = entityInList();
        std::string str0 = std::to_string(count.second);
        glm::vec2 start = m_UnitStarts.at(m_CurUnits);
        unit.Text = UI.CreateString(m_textLayer, 0, str0, start, 0.2, glm::vec4(1));

        const auto& unitType = Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(count.first);
        AddUnitToPreviews(unitType, unit, firstEntity.find(count.first)->second);
        for (const auto& order : unitType.availableOrders)
        {
            bool found = false;
            for (const auto& action : m_actionMap)
            {
                if (action.second == order)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                ShowOrder(order);
            }
        }
    }

    if (unitSelected)
    {
        if (counter.count(selectedUnitType) == 0)
        {
            if (selectedUnitType != "") ClearSelectedUnit();
        }
        m_lastClicked.clear();
        return;
    }
    counter.clear();
    firstEntity.clear();

    auto viewStructures = Engine.ECS().Registry.view<Selected, AllyStructure, AttributesComponent>();
    for (auto [entity, sel, unit, attrib] : viewStructures.each())
    {
        // get a amount of each kind of entity there is and then add them to the preview with counters
        if (currentFocusedEntity == entt::null)
        {
            currentFocusedEntity = entity;
            entityType = attrib.GetEntityType();
        }

        if (counter.count(attrib.GetEntityType()) == 0)
        {
            counter.emplace(attrib.GetEntityType(), 0);
        }
        if (firstEntity.count(attrib.GetEntityType()) == 0)
        {
            firstEntity.emplace(attrib.GetEntityType(), entity);
        }
        counter.find(attrib.GetEntityType())->second++;
    }
    if (counter.count("WallCorner") > 0)
    {
        if (counter.count("Wall") > 0)
        {
            counter.find("Wall")->second++;
        }
        else
        {
            counter.emplace("Wall", 1);
            firstEntity.emplace("Wall", firstEntity.find("WallCorner")->second);
        }
        counter.erase("WallCorner");
        firstEntity.erase("WallCorner");
    }

    for (auto& count : counter)
    {
        entityInList unit = entityInList();
        std::string str0 = std::to_string(count.second);
        glm::vec2 start = m_UnitStarts.at(m_CurUnits);
        unit.Text = UI.CreateString(m_textLayer, 0, str0, start, 0.2, glm::vec4(1));

        const auto& structureType = Engine.ECS().GetSystem<StructureManager>().GetStructureTemplate(count.first);
        AddStructureToPreviews(structureType, unit, firstEntity.find(count.first)->second);
        for (const auto& order : structureType.availableOrders)
        {
            bool found = false;
            for (const auto& action : m_actionMap)
            {
                if (action.second == order)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                ShowOrder(order);
            }
        }
    }
    if (counter.count(selectedUnitType) == 0)
    {
        if (selectedUnitType != "") ClearSelectedUnit();
    }
    m_lastClicked.clear();
}

void InGameUI::ShowOrder(OrderType type)
{
    if (type == OrderType::None || type == OrderType::Move || type == OrderType::Attack) return;
    glm::vec2 start = m_orderStarts.at(m_CurOrder);
    m_CurOrder++;
    entityInList lister = entityInList();
    auto& UI = Engine.ECS().GetSystem<UserInterface>();
    auto& orderSystem = Engine.ECS().GetSystem<OrderSystem>();
    const auto& icon = orderSystem.GetOrderTemplate(type);
    const glm::vec4 uvs = glm::vec4(icon.iconTextureCoordinates.x, icon.iconTextureCoordinates.y,
                                    icon.iconTextureCoordinates.x + icon.iconTextureCoordinates.z,
                                    icon.iconTextureCoordinates.y + icon.iconTextureCoordinates.w);
    std::string path = icon.iconPath;
    auto texturecopy = textureItem;
    texturecopy.start = start;
    texturecopy.atlas = uvs;
    texturecopy.size = glm::vec2(OrderSize);
    texturecopy.name = std::string(magic_enum::enum_name(type)) + "previewUnit";
    texturecopy.imageFile = icon.iconPath;
    texturecopy.Remake(uiElementsTemplate);
    lister.Tex = texturecopy.ID;

    if (type == OrderType::TrainMage || type == OrderType::TrainWarrior)
    {
        bool draw = false;
        const auto spawnView =
            bee::Engine.ECS().Registry.view<Selected, AllyStructure, bee::ai::StateMachineAgent, SpawningStructure>();
        auto queNumber = UI.GetComponentID(m_quedUnit, "Queued");
        auto& queLoadingBar = UI.getComponentItem<sProgressBar>(m_quedUnit, "Progress");
        for (auto [entity, selected, ally, stateMachineAgent, spawningStructure] : spawnView.each())
        {
            if (stateMachineAgent.context.blackboard->HasKey<double>("TrainTimer"))
            {
                auto currentQue = stateMachineAgent.context.blackboard->GetData<int>("NumUnits");
                if (currentQue > 0) draw = true;
                auto timer = stateMachineAgent.context.blackboard->GetData<double>("TrainTimer");
                const auto unitHandle = stateMachineAgent.context.blackboard->GetData<std::string>("UnitToTrain");
                auto attributes = bee::Engine.ECS().GetSystem<UnitManager>().GetUnitTemplate(unitHandle).GetAttributes();
                auto maxTimer = attributes[BaseAttributes::CreationTime] -
                                bee::Engine.ECS()
                                    .Registry.get<AttributesComponent>(stateMachineAgent.context.entity)
                                    .GetValue(BaseAttributes::TimeReduction);
                float trainingPercet = (maxTimer - timer) / maxTimer;
                UI.ReplaceString(queNumber, std::to_string(currentQue));

                queLoadingBar.value = trainingPercet * 100.0f;
                queLoadingBar.Remake(m_quedUnit);
            }
        }
        auto& queTexture = UI.getComponentItem<bee::ui::Image>(m_quedUnit, "UnitTexture");
        queTexture.atlas = uvs;
        queTexture.imageFile = icon.iconPath;
        queTexture.Remake(m_quedUnit);
        UI.SetDrawStateUIelement(m_quedUnit, draw);
    }

    auto textcopy = textItem;
    textcopy.size = 0.15f;
    textcopy.start = start + glm::vec2(OrderSize / 2.0f, OrderSize / 10.0f);
    textcopy.name = std::string(magic_enum::enum_name(type)) + "keybind";
    textcopy.text = magic_enum::enum_name(bee::Engine.InputWrapper().GetDigitalActionKey(magic_enum::enum_name(type).data()));
    textcopy.Remake(uiElementsTemplate);
    lister.Text = textcopy.ID;

    auto buttoncopy = buttonItem;
    buttoncopy.start = start;
    buttoncopy.name = std::string(magic_enum::enum_name(type)) + "previewUnitButton";
    buttoncopy.linkedComponent = texturecopy.ID;
    buttoncopy.linkedComponentstr = std::string(magic_enum::enum_name(type)) + "previewUnit";
    buttoncopy.click = interaction(none, "SetOrderFromUI");
    buttoncopy.hover = interaction(none, "ShowOrderHover");
    buttoncopy.size = glm::vec2(OrderSize);
    buttoncopy.Remake(uiElementsTemplate);
    lister.Button = buttoncopy.ID;
    lister.name = buttoncopy.name;
    if (m_lastClicked.count(lister.name) > 0) UI.SetButtonLastClick(lister.Button, m_lastClicked.at(lister.name));

    m_actionsInList.push_back(lister);
    m_actionMap.emplace(buttoncopy.ID, type);
}

void InGameUI::ClearPreview()
{
    auto& UI = Engine.ECS().GetSystem<UserInterface>();
    for (const auto& pair : m_unitsInList)
    {
        const auto& unit = pair.second;
        m_lastClicked.emplace(unit.name, UI.GetButtonLastClick(unit.Button));
        UI.DeleteComponent(unit.Button);
        UI.DeleteComponent(unit.Tex);
        UI.DeleteComponent(unit.Text);
    }
    for (const auto& pair : m_actionsInList)
    {
        m_lastClicked.emplace(pair.name, UI.GetButtonLastClick(pair.Button));
        UI.DeleteComponent(pair.Tex);
        UI.DeleteComponent(pair.Button);
        UI.DeleteComponent(pair.Text);
    }
    UI.SetDrawStateUIelement(m_quedUnit, false);
    m_actionsInList.clear();
    m_unitsInList.clear();
    m_actionMap.clear();
    m_CurUnits = 0;
    m_CurOrder = 0;
    m_mapUnitTypes.clear();
}

std::vector<UIComponentID> InGameUI::MakeWrappedString(UIElementID element, const std::string& str, glm::vec2 pos,
                                                       float fontSize, float windowSize, glm::vec4 colour, bool canAfford)
{
    std::vector<UIComponentID> ids;
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    int lastEnter = 0;
    float xposition = pos.x;
    float yposition = pos.y;
    float spaceSize = ui.PreCalculateStringEnds(element, 0, " ", glm::vec2(0, 0), fontSize, false).x;
    float enterSize = ui.PreCalculateStringEnds(element, 0, " ", glm::vec2(0, 0), fontSize, false).y;

    std::vector<std::string> enters = SplitString(str, "\n");

    for (auto& string : enters)
    {
        std::vector<std::string> words = SplitString(string, " ");

        for (int i = 0; i < words.size(); i++)
        {
            // tempposx is a temporary version of xposition that first needs to be checked
            float tempxpos =
                ui.PreCalculateStringEnds(element, 0, words.at(i), glm::vec2(xposition, yposition), fontSize, false).x;
            tempxpos += spaceSize;

            if (tempxpos >= windowSize)
            {
                // size is too big and thus needs to draw the previous text
                std::string str;
                for (int j = lastEnter; j < i; j++)
                {
                    str.append(words.at(j));
                    str.append(" ");
                }
                ids.push_back(ui.CreateString(
                    element, 0, str, glm::vec2(pos.x, yposition), fontSize,
                    (string.find("stone") != std::string::npos || string.find("wood") != std::string::npos) && !canAfford
                        ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
                        : colour));
                xposition = 0.0f;
                yposition += enterSize;
                lastEnter = i;

                tempxpos =
                    ui.PreCalculateStringEnds(element, 0, words.at(i), glm::vec2(pos.x + xposition, pos.y), fontSize, false).x;
                tempxpos += spaceSize;
            }
            xposition = tempxpos;
        }

        if (lastEnter != words.size())
        {
            std::string str0;
            for (int j = lastEnter; j < words.size(); j++)
            {
                str0.append(words.at(j));
                str0.append(" ");
            }
            ids.push_back(ui.CreateString(
                element, 0, str0, glm::vec2(pos.x, yposition), fontSize,
                (string.find("stone") != std::string::npos || string.find("wood") != std::string::npos) && !canAfford
                    ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
                    : colour));
        }
        lastEnter = 0;
        yposition += enterSize;
        xposition = pos.x;
    }
    return ids;
}

void InGameUI::AddUnitToPreviews(const UnitTemplate& unitType, entityInList& unit, const entt::entity entity)
{
    glm::vec2 start = m_UnitStarts.at(m_CurUnits);
    m_CurUnits++;
    auto& UI = Engine.ECS().GetSystem<UserInterface>();
    auto texturecopy = textureItem;
    const glm::vec4 uvs = glm::vec4(unitType.iconTextureCoordinates.x, unitType.iconTextureCoordinates.y,
                                    unitType.iconTextureCoordinates.x + unitType.iconTextureCoordinates.z,
                                    unitType.iconTextureCoordinates.y + unitType.iconTextureCoordinates.w);
    texturecopy.size = glm::vec2(UnitSize);
    texturecopy.start = start;
    texturecopy.atlas = uvs;
    texturecopy.name = unitType.name + "previewUnit";
    texturecopy.imageFile = unitType.iconPath;
    texturecopy.Remake(uiElementsTemplate);
    unit.Tex = texturecopy.ID;
    auto buttoncopy = buttonItem;
    buttoncopy.size = glm::vec2(UnitSize);
    buttoncopy.start = start;
    buttoncopy.name = unitType.name + "previewUnitButton";
    buttoncopy.linkedComponent = texturecopy.ID;
    buttoncopy.linkedComponentstr = unitType.name + "previewUnit";
    buttoncopy.click = interaction(SwitchType::none, "OpenSpeceficUnit");
    buttoncopy.Remake(uiElementsTemplate);
    m_mapUnitTypes.emplace(buttoncopy.ID, std::make_pair(unitType.name, false));
    unit.Button = buttoncopy.ID;
    unit.name = buttoncopy.name;

    if (entityType == unitType.name) currentComponent = buttoncopy.ID;

    if (m_lastClicked.count(unit.name) > 0) UI.SetButtonLastClick(unit.Button, m_lastClicked.at(unit.name));

    m_unitsInList.emplace(entity, unit);
}

void InGameUI::AddStructureToPreviews(const StructureTemplate& structureType, entityInList& structure,
                                      const entt::entity entity)
{
    glm::vec2 start = m_UnitStarts.at(m_CurUnits);
    m_CurUnits++;
    auto& UI = Engine.ECS().GetSystem<UserInterface>();
    auto texturecopy = textureItem;
    const glm::vec4 uvs = glm::vec4(structureType.iconTextureCoordinates.x, structureType.iconTextureCoordinates.y,
                                    structureType.iconTextureCoordinates.x + structureType.iconTextureCoordinates.z,
                                    structureType.iconTextureCoordinates.y + structureType.iconTextureCoordinates.w);
    texturecopy.size = glm::vec2(UnitSize);
    texturecopy.start = start;
    texturecopy.atlas = uvs;
    texturecopy.name = structureType.name + "previewUnit";
    texturecopy.imageFile = structureType.iconPath;
    texturecopy.Remake(uiElementsTemplate);

    structure.Tex = texturecopy.ID;
    auto buttoncopy = buttonItem;
    buttoncopy.size = glm::vec2(UnitSize);
    buttoncopy.start = start;
    buttoncopy.name = structureType.name + "previewUnitButton";
    buttoncopy.linkedComponent = texturecopy.ID;
    buttoncopy.linkedComponentstr = structureType.name + "previewUnit";
    buttoncopy.click = interaction(SwitchType::none, "OpenSpeceficUnit");
    buttoncopy.Remake(uiElementsTemplate);
    m_mapUnitTypes.emplace(buttoncopy.ID, std::make_pair(structureType.name, true));
    structure.Button = buttoncopy.ID;
    structure.name = buttoncopy.name;
    if (m_lastClicked.count(structure.name) > 0) UI.SetButtonLastClick(structure.Button, m_lastClicked.at(structure.name));

    if (entityType == structureType.name) currentComponent = buttoncopy.ID;

    m_unitsInList.emplace(entity, structure);
}
