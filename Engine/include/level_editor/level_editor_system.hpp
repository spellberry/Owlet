#pragma once

#include <actors/units/unit_order_type.hpp>

#include "ai/wave_data.hpp"
#include "brushes/brush.hpp"
#include "brushes/path_brush.hpp"
#include "brushes/prop_brush.hpp"
#include "brushes/structure_brush.hpp"
#include "brushes/terraform_brush.hpp"
#include "brushes/texture_brush.hpp"
#include "brushes/unit_brush.hpp"
#include "brushes/area_brush.hpp"
#include "brushes/foliage_brush.hpp"
#include "core/ecs.hpp"

namespace lvle
{
enum class Axis
{
    None,
    X,
    Y
};

struct GameplayArea
{
    int index = 0;
    float color[3];
};

/// <summary>
/// Upon creating the system, a TerrainSystem will also be created. There is no need to create it separately.
/// </summary>
class LevelEditor : public bee::System
{
public:
    LevelEditor();
    ~LevelEditor() override = default;
    void Update(float dt) override;
    void Render() override;
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif

private:  // functions
    /// <summary>
    /// Function that hijacks the Camera entity (if there is one) and enables camera movement for the level editor.
    /// </summary>
    /// <param name="dt">Delta time</param>
    /// <param name="moveStep">How fast the camera moves.</param>
    /// <param name="wheelStep">How fast the camera "zooms".</param>
    /// <param name="rotateStep">How fast the camera looks around.</param>
    void UpdateCamera(float dt, float moveStep, float wheelStep, float rotateStep);

    /// <summary>
    /// Processes all input NOT related to camera transformation, e.g. mouse picking
    /// </summary>
    void ProcessInput();

    /// <summary>
    /// Give the green light to the correct brush and update the mesh.
    /// </summary>
    void Edit(float dt);

    /// <summary>
    /// Updates the intersection point between the terrain and the raycast from the camera to the mouse cursor.
    /// </summary>
    void CalculateIntersectionPoint();

    /// <summary>
    /// Locks the brush on either the X or Y axis in order to make straight lines.
    /// </summary>
    void LockBrushOnAxis();

    /// <summary>
    /// Deletes selected actors.
    /// </summary>
    void DeleteSelectedActors();

    template <typename ActorTemplate>
    void PrepareActorMaterialPaths(ActorTemplate& actorTemplate);

    /// <summary>
    /// A function that displays an ImGui ComboBox/Dropdown menu and can select items based on id.
    /// </summary>
    /// <typeparam name="Container">Any data structure that has a key system (example: unordered_map, vector of pairs,
    /// etc.)</typeparam> <param name="name">Name of the dropdown menu</param> <param name="list">The data structure which
    /// should be displayed</param> <param name="selectedItem">Return argument which will give you the id of the selected menu
    /// item.</param> <returns>Returns true if a new item was selected.</returns>
    bool DisplayDropDown(const std::string& name, const std::unordered_map<size_t, std::string>& list, size_t& selectedItem);
    bool DisplayDropDown(const std::string& name, const std::vector<std::string>& list, std::string& selectedItem);
    bool DisplayDropDown(const std::string& name, const std::vector<int>& list, size_t& selectedItem);

    template <typename EnumClass>
    bool DisplayEnumDropDown(EnumClass& selectedItem);

    /// <summary>
    /// Displays radio buttons for every brush mode of the given brush.
    /// </summary>
    /// <typeparam name="T"> Must be a class derived from Brush.</typeparam>
    /// <param name="brush">The brush from the member parameters in the level editor class.</param>
    /// <param name="brushType">An enum value from BrushType</param>
    /// <param name="typeName">The name which will be displayed.</param>
    template <typename T>
    void DisplayBrushOptions(std::unique_ptr<T>& brush, BrushType brushType, std::string typeName);
    
    void DisplayAreaBrushProperties(Brush& brush);

    // ----------------------------------
    // helper functions for the inspector
    // ----------------------------------
    void TerrainPalette();

    void UnitPalette();
    void StructurePalette();
    void PropPalette();
    void FoliagePalette();

    void DebugToggles();
    void MapDetails();

    void Guizmo();
    void EditBindings();
    void ResetOrderShortCuts();
    void SaveOrderShortCuts();

private:  // params
    unsigned int m_debugFlags;
    bool m_newLevel = false;
    WaveData m_waveData{};
    // We use the preview entity of the terraform brush just because it is the first one.
    std::unique_ptr<TerraformBrush> m_terraformBrush = std::make_unique<TerraformBrush>();
    std::unique_ptr<TextureBrush> m_textureBrush = std::make_unique<TextureBrush>();
    std::unique_ptr<PathBrush> m_pathBrush = std::make_unique<PathBrush>();
    std::unique_ptr<UnitBrush> m_unitBrush = std::make_unique<UnitBrush>();
    std::unique_ptr<PropBrush> m_propBrush = std::make_unique<PropBrush>();
    std::unique_ptr<AreaBrush> m_areaBrush = std::make_unique<AreaBrush>();
    std::unique_ptr<StructureBrush> m_structureBrush = std::make_unique<StructureBrush>();
    std::unique_ptr<FoliageBrush> m_foliageBrush = std::make_unique<FoliageBrush>();
    Brush* m_brush = m_textureBrush.get();
    // Used for terrain editing
    BrushType m_selectedBrush = BrushType::Terraform;
    int m_brushModes[6] = {-1, -1, -1, -1, -1, -1};

    Axis m_lockedAxis = Axis::None;

    std::string m_selectedTemplate = "";
    Team m_selectedTeam = Team::Ally;

    std::string m_selectedPalette = "";

    bool m_mouseOnTerrain = false;
    bool m_entitiesEditMode = false;
    glm::vec3 m_intersectionPoint = glm::vec3(0.0f, 0.0f, 0.0f);

    // currently doesn't work
    std::vector<std::string> m_modelExtensions{".gltf", ".glb"};
    std::vector<std::string> m_fsmExtensions{".json"};
    std::vector<std::string> m_imageExtensions{".png"};

    float m_cameraMoveStep = 0.05f;
    float m_cameraWheelStep = 1.0f;
    float m_cameraRotateStep = 0.002f;

    std::vector<int> m_presetMapSize{16, 32, 64, 128, 256, 384};
    StructureTypes m_selectedStructureTypes = StructureTypes::None;
    std::string m_levelToLoad = "test";

    std::unordered_map<OrderType, bee::Input::KeyboardKey> m_orderShortCuts;
    bee::Input::KeyboardKey m_lastKeyPressed = bee::Input::KeyboardKey::None;
    OrderType m_editedOrder = OrderType::None;
    bool m_editShortCut = false;
    bool m_openBindingWindow = false;
};

}  // namespace lvle
