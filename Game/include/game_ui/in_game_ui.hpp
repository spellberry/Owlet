#pragma once
#include "actors/props/resource_type.hpp"
#include "actors/structures/structure_template.hpp"
#include "actors/units/unit_template.hpp"
#include "user_interface/user_interface_editor_structs.hpp"

struct entityInList;
constexpr char RIGHT_TOP_CORNER_NAME[] = "InGameTopRight";
constexpr char RIGHT__BOTTOM_CORNER_NAME[] = "InGameBottomRight";
constexpr char LEFT__BOTTOM_CORNER_NAME[] = "InGameBottomLeft";
constexpr char UI_ELEMENTS_TEMPLATE[] = "InGameTemplates";
constexpr float OrderSize = 0.0525f;
constexpr float UnitSize = 0.075f;
constexpr float MaxOrders = 3.0f;
constexpr float MaxUnits = 2.0f;

constexpr glm::vec4 RESET_ATLAS = glm::vec4(0, 0, 4, 4);
constexpr char RESETLOC[] = "textures/gray.png";
enum class Popups
{
    None = 0,
    FirstEnemyDies = 1,
    FixRuins = 2,
    ForestAttack = 3,
    FixBarAndMage = 4,
    Train6Troops = 5,
    BuildWall = 6,
    FirstWave = 7,
    MoreWaves = 8,
    Done = 0,

};

enum class ArrowDirection
{
    Left,
    Right,
    Top,
    Bottom,
    LeftTop,
    LeftBottom,
    RightTop,
    RightBottom

};

struct entityInList
{
    bee::ui::UIComponentID Button = -1;
    bee::ui::UIComponentID Tex = -1;
    bee::ui::UIComponentID Text = -1;
    std::string name = "";
};

class InGameUI : public bee::System
{
public:
    InGameUI();
    ~InGameUI();
    void Update(float dt) override;

    bee::ui::UIElementID rtop;
    bee::ui::UIElementID rbot;
    bee::ui::UIElementID rbotButtons;
    bee::ui::UIElementID town;

    void ShowPopup(const std::string& content);
    void RemovePopup();

    void ShowEnemiesGeneralDirection();
    ArrowDirection CalculatePointOnEdge(glm::vec2& point, const glm::vec2& pos, const glm::vec2& size, const float resolution);

    void UpdateHeathBar();
    int UpdateSelectedUnits(int index);
    int UpdateSelectedBuilding(int index);
    int UpdateEnemies(int index);
    void UpdateHealthBar(const bee::Transform& transform, float healthPercent, const glm::vec2& pos, const glm::vec2& size,
                         const float resolution, double range, bee::ui::sProgressBar& healthBar,bool enemy);

    bool IsWaveSystemPaused() const;
    void SetInputState(bool newState);
    // previews
    void SelectUnits();
    void ShowOrder(OrderType type);
    void ClearPreview();
    OrderType GetOrderType(bee::ui::UIComponentID id) const;
    bee::ui::UIElementID unitsPreview;
    bee::ui::UIElementID uiElementsTemplate;
    void CheckPopups();
    void const UpdateResources(GameResourceType type, int newAmount);
    void SetSelectedUnit(bee::ui::UIComponentID);
    void ClearSelectedUnit();
    bee::ui::UIElementID healthBars = entt::null;

    void ShowHoverOrder(bee::ui::UIComponentID id);
    void ShowHoverBuild(bee::ui::UIComponentID id);
    bool IsTutorialDone();

private:
    friend struct Price;
    friend struct OpenSpeceficUnit;
    std::unordered_map<bee::ui::UIComponentID, OrderType> m_actionMap;
    std::unordered_map<bee::ui::UIComponentID, OrderType> m_buildMap;

    bee::ui::UIElementID m_costView = entt::null;

    std::vector<bee::ui::UIComponentID> MakeWrappedString(bee::ui::UIElementID element, const std::string& str, glm::vec2 pos,
                                                          float fontSize, float WindowSize, glm::vec4 colour, bool canAfford);
    void AddUnitToPreviews(const UnitTemplate& unitType, entityInList& unit, const entt::entity entity);
    void AddStructureToPreviews(const StructureTemplate& structureType, entityInList& structure, const entt::entity entity);
    bee::ui::sButton buttonItem;
    bee::ui::Image textureItem;
    bee::ui::Text textItem;
    std::map<entt::entity, entityInList> m_unitsInList;
    std::vector<entityInList> m_actionsInList;

    bee::ui::UIImageElement m_healthBar = bee::ui::UIImageElement(glm::vec2(0.0f, 0.0f), glm::vec2(4.0f, 4.0f));
    std::vector<bee::ui::UIComponentID> m_healthBars;
    std::vector<bee::ui::UIComponentID> m_arrows;
    bee::ui::UIImageElement m_arrowImage = bee::ui::UIImageElement(glm::vec2(0.0f, 0.0f), glm::vec2(450.0f, 228.0f));
    bee::ui::UIElementID m_arrowsElement = entt::null;
    float arrowSize = 0.02f;

    glm::vec3 m_initialCameraOrientation = {}, m_currentCameraOrientation = {};

    std::unordered_map<std::string, bool> m_lastClicked;
    glm::vec2 m_cost = glm::vec2(0.0f);
    bool m_hovering = false;
    bool m_hoveringOrder = false;
    // popups
    std::unordered_map<Popups, std::pair<std::string, bool>> m_strings = {
        {Popups::FixRuins, {"Welcome to Owlet, you should start by restoring this ruined cathedral", false}},
        {Popups::ForestAttack, {"Oh no! You've angered the forest! You must prepare your defences", false}},
        {Popups::FixBarAndMage, {"Repair your barrack and mage tower to train an army", false}},
        {Popups::Train6Troops, {"Use your towers to train 6 troops to defend yourself", false}},
        {Popups::BuildWall, {"Slow down the enemies by building walls around your buildings", false}},
        {Popups::FirstWave, {"The forest approaches, be ready to defend", false}},
        {Popups::MoreWaves, {"Well done you survived the attack! But be careful, more attacks are coming.", false}},
        {Popups::FirstEnemyDies,
         {"Killing enemies grants you resources! Use resources to further increase your defences", false}},
        {Popups::Done, {"", false}}};

    std::unordered_map<OrderType, std::string> m_orderStrings = {
        {OrderType::BuildMageTower, "This building can train mages"},
        {OrderType::BuildBarracks, "This building can train warriors"},
        {OrderType::BuildWallStart, "This wall will wall off enemies"},
        {OrderType::BuildFenceStart, "This Fence will wall off enemies"},
        {OrderType::TrainMage, "You can train a mage"},
        {OrderType::TrainWarrior, "You can train a warrior"},
        {OrderType::Patrol, "Patrol This location"},
        {OrderType::UpgradeBuilding, ""},
        {OrderType::OffensiveMove, "Attack your enemies from this location"}};

    bee::ui::UIElementID m_popupID = entt::null;
    bee::ui::UIElementID m_Hover = entt::null;

    std::unordered_map<bee::ui::UIComponentID, std::pair<std::string, bool>> m_mapUnitTypes;
    std::vector<glm::vec2> m_orderStarts;
    int m_CurOrder = 0;
    std::vector<glm::vec2> m_UnitStarts;
    int m_CurUnits = 0;
    Popups m_currentPopup = Popups::None;

    bee::Entity currentFocusedEntity = entt::null;
    bee::ui::UIComponentID currentComponent = -1;
    std::string entityType = "";
    std::string selectedUnitType = "";
    bee::ui::UIElementID m_quedUnit = entt::null;
    bee::ui::UIElementID m_textLayer = entt::null;
    std::vector<bee::ui::UIComponentID> hoverTextIds;
    float m_timer = 0.0f;
};
