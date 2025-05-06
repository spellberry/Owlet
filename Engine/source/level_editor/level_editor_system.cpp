#define MAGIC_ENUM_RANGE_MIN -100
#define MAGIC_ENUM_RANGE_MAX 500
#include "level_editor/level_editor_system.hpp"

#include <imgui/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include <tinygltf/json.hpp>

#include "ai/grid_navigation_system.hpp"
#include "actors/actor_wrapper.hpp"
#include "actors/selection_system.hpp"
#include "light_system/light_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "imgui/imgui_stdlib.h"
#include "magic_enum/magic_enum_utility.hpp"
#include "material_system/material_system.hpp"
#include "rendering/debug_render.hpp"
#include "rendering/image.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/asset_explorer_system.hpp"
#include "tools/inspector.hpp"
#include "tools/tools.hpp"

#include "camera/camera_editor_system.hpp"
#include "particle_system/particle_system.hpp"


using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;



LevelEditor::LevelEditor()
{
    Priority = 3;
    Title = "Level Editor";
    Engine.DebugRenderer().SetCategoryFlags(DebugCategory::Editor | DebugCategory::General);

    // assign flags
    {
        m_debugFlags = EditorDebugCategory::Mouse | EditorDebugCategory::Pathing;
    }

    {  // Ground plane
        auto& terrain = Engine.ECS().CreateSystem<lvle::TerrainSystem>();
        terrain.CalculateAllTilesCentralPositions();
        terrain.UpdateTerrainDataComponent();
    }

    { // Grid navigation System (so it doesnt crash)
        // This loop should only execute ONCE if everything is done correctly.
        auto view = bee::Engine.ECS().Registry.view<lvle::TerrainDataComponent>();
        for (auto entity : view)
        {
            auto [data] = view.get(entity);
            bee::ai::NavigationGrid nav_grid(data.m_tiles[0].centralPos, data.m_step, data.m_width, data.m_height);
            auto& grid_nav_system = bee::Engine.ECS().CreateSystem<bee::ai::GridNavigationSystem>(0.1f, nav_grid);
            grid_nav_system.UpdateFromTerrain();
        }
    }

    {  // Selection
        auto& selectionSystem = bee::Engine.ECS().CreateSystem<SelectionSystem>();
        selectionSystem.Deactivate();
        selectionSystem.EnableAllSelect();
    }

    { // Brush
        auto& selectionSystem = Engine.ECS().GetSystem<SelectionSystem>();
        m_brush->Deactivate();
        m_brush->Disable();
        m_brush->RemovePreviewModel();
        m_selectedTemplate = "";
        selectionSystem.Activate();
        m_entitiesEditMode = true;
    }
    
    if (bee::Engine.FileIO().Exists(bee::FileIO::Directory::Asset, "Wave data.json"))
    {
        bee::Engine.Serializer().SerializeFrom("Wave data.json", m_waveData);
    }
    else
    {
        bee::Engine.Serializer().SerializeTo("Wave data.json", m_waveData);
    }

    actors::CreateActorSystems();
    //actors::LoadActorsTemplates(m_levelToLoad);

    bee::Engine.Inspector().UsingTheEditor(true);
    ImGuizmo::SetID(0);
    ResetOrderShortCuts();
}

void lvle::LevelEditor::Update(float dt)
{
    if (m_newLevel)
    {
        m_newLevel = false;
        return;
    }

    if (IsMouseInGameWindow())
    {
        ProcessInput();
        LockBrushOnAxis();
        //UpdateCamera(dt, m_cameraMoveStep, m_cameraWheelStep, m_cameraRotateStep);
        CalculateIntersectionPoint();
        m_brush->Update(m_intersectionPoint, m_selectedTemplate);
        if (m_brush->IsEnabled() && m_brush->IsActive() && Engine.Inspector().IsGameWindowFocused())
        {
            Edit(dt);
        }
    }
    if (m_editShortCut)
    {
        m_lastKeyPressed = bee::Engine.Input().lastPressedKey;        
    }


}

void lvle::LevelEditor::Render()
{
    if (m_newLevel)
    {
        m_newLevel = false;
        return;
    }

    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    if (m_debugFlags & EditorDebugCategory::Wireframe) terrain.DrawWireframe(vec4(1.0, 1.0, 1.0, 1.0));

    if (m_debugFlags & EditorDebugCategory::Normals) terrain.DrawNormals(vec4(1.0, 0.0, 1.0, 1.0));

    if (m_debugFlags & EditorDebugCategory::Cross)
    {
        Engine.DebugRenderer().AddLine(DebugCategory::Editor, {0.0, 0.0, 0.1}, {5.0, 0.0, 0.1}, {1.0, 0.0, 0.0, 1.0});
        Engine.DebugRenderer().AddLine(DebugCategory::Editor, {0.0, 0.0, 0.1}, {0.0, 5.0, 0.1}, {0.0, 1.0, 0.0, 1.0});
        Engine.DebugRenderer().AddLine(DebugCategory::Editor, {0.0, 0.0, 0.1}, {0.0, 0.0, 5.1}, {0.0, 0.0, 1.0, 1.0});
    }

    if (m_debugFlags & EditorDebugCategory::Mouse && m_mouseOnTerrain)
    {
        m_brush->Render(m_intersectionPoint);
    }

    if (m_debugFlags & EditorDebugCategory::Pathing)
    {
        terrain.DrawPathing();
    }

    if (m_debugFlags & EditorDebugCategory::Areas)
    {
        terrain.DrawAreas();
    }

    const auto actorView = bee::Engine.ECS().Registry.view<bee::physics::DiskCollider, bee::Transform>();
    for (auto [entity, collider, transform] : actorView.each())
    {
        if (transform.Name != "Preview Actor")
        {
            bee::Engine.DebugRenderer().AddCircle(bee::DebugCategory::Editor, transform.Translation + vec3(0.0f, 0.0f, 0.05f),
                                                  collider.radius, vec4(1.0, 0.0, 1.0, 1.0));
        }
    }
}

#ifdef BEE_INSPECTOR
void LevelEditor::Inspect()
{
    auto& selectionSystem = Engine.ECS().GetSystem<SelectionSystem>();
    if (m_entitiesEditMode)
    {
        if (ImGuizmo::IsUsing() || ImGuizmo::IsOver() || !IsMouseInGameWindow() || ImGui::IsAnyItemHovered())
        {
            selectionSystem.Deactivate();
        }
        else
        {
            selectionSystem.Activate();
        }
    }
    ImGui::Begin("Wave system");
    bee::Engine.Inspector().Inspect(m_waveData);
    ImGui::End();
    ImGui::Begin("Map Editor");
    const vector<string> palettes = {"Terrain Palette", "Unit Palette", "Structure Palette", "Prop Palette", "Foliage Palette"};
    if (ImGui::Button("Open Input Binding Window"))
    {
        m_openBindingWindow = true;
    }
    if (DisplayDropDown("Palettes", palettes, m_selectedPalette))
    {
        m_brush->RemovePreviewModel();
    }
    if (m_selectedPalette == palettes[0])
    {
        TerrainPalette();
    }
    else if (m_selectedPalette == palettes[1])
    {
        UnitPalette();
    }
    else if (m_selectedPalette == palettes[2])
    {
        StructurePalette();
    }
    else if (m_selectedPalette == palettes[3])
    {
        PropPalette();
    }
    else if (m_selectedPalette == palettes[4])
    {
        FoliagePalette();
    }
    DebugToggles();
    ImGui::End();

    if (m_openBindingWindow) EditBindings();
    Guizmo();
    MapDetails();
    
}
#endif

void lvle::LevelEditor::UpdateCamera(float dt, const float moveStep, const float wheelStep, const float rotateStep)
{
    static vec2 mousePosClicked;
    static float lastWheelValue;

    const auto view = Engine.ECS().Registry.view<Transform, Camera>();
    const auto& input = Engine.Input();
    for (auto& entity : view)
    {
        auto [transform, camera] = view.get(entity);

        const auto& viewMatrix = GetCameraViewMatrix(transform);
        const auto& projectionMatrix = camera.Projection;
        camera.View = viewMatrix;
        camera.VP = projectionMatrix * viewMatrix;

        // translate
        if (!input.GetKeyboardKey(Input::KeyboardKey::LeftControl))
        {
            if (input.GetMouseButton(Input::MouseButton::Right))
            {
                const vec2 mouseOffset = mousePosClicked - input.GetMousePosition();

                const float velocityX = mouseOffset.x * moveStep;
                const float velocityY = mouseOffset.y * moveStep;
                const float z = transform.Translation.z;
                transform.Translation += camera.Front * velocityX;
                transform.Translation += camera.Right * velocityY;
                transform.Translation.z = z;
            }
        }

        // rotate
        else
        {
            vec3 rotationEuler = glm::eulerAngles(transform.Rotation);
            if (input.GetMouseButton(Input::MouseButton::Right))
            {
                const vec2 mouseOffset = mousePosClicked - input.GetMousePosition();
                rotationEuler.x += mouseOffset.y * rotateStep;  // pitch
                rotationEuler.z += mouseOffset.x * rotateStep;  // yaw
            }
            transform.Rotation = glm::quat(rotationEuler);

            // calculate camera's Front vector
            glm::vec3 front;
            front.x = cos(rotationEuler.x) * cos(rotationEuler.z);
            front.y = sin(rotationEuler.z);
            front.z = sin(rotationEuler.x) * cos(rotationEuler.z);
            camera.Front = normalize(front);
            camera.Right = glm::normalize(glm::cross(camera.Front, vec3(0.0f, 0.0f, 1.0f)));
        }

        // zoom (translate z)
        if (!input.GetKeyboardKey(Input::KeyboardKey::LeftControl))
        {
            const auto offset = (lastWheelValue - input.GetMouseWheel());
            transform.Translation.z += (offset * wheelStep);
        }

        mousePosClicked = input.GetMousePosition();
        lastWheelValue = input.GetMouseWheel();
    }
}

void lvle::LevelEditor::ProcessInput()
{
    const auto& input = Engine.Input();
    static float lastWheelValue;

    auto& selectionSystem = Engine.ECS().GetSystem<SelectionSystem>();
    if (input.GetKeyboardKeyOnce(Input::KeyboardKey::Escape))
    {
        if (!m_entitiesEditMode)
        {
            m_brush->Disable();
            m_brush->Deactivate();
            m_brush->RemovePreviewModel();
            m_selectedTemplate = "";
            selectionSystem.Activate();
        }else
        {
            m_brush->Enable();
            m_brush->Activate();
            selectionSystem.Deactivate();
        }

        m_entitiesEditMode = !m_entitiesEditMode;
        bee::Engine.Inspector().data.enabledGuizmo = m_entitiesEditMode;
    }

    if (input.GetKeyboardKeyOnce(Input::KeyboardKey::Delete))
    {
        DeleteSelectedActors();
    }

    if (!m_entitiesEditMode)
    {
        m_brush->Deactivate();
        if (input.GetMouseButton(Input::MouseButton::Left))
        {
            m_brush->Activate();
        }
    }

    if (input.GetKeyboardKey(Input::KeyboardKey::LeftControl))
    {
        m_brush->RotatePreviewModel((lastWheelValue - input.GetMouseWheel()) * 5);
    }
    lastWheelValue = input.GetMouseWheel();
}

void LevelEditor::Edit(const float dt)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    // If it's a terrain brush, PlaceObject should be empty.
    // If it's a unit/structure/prop brush, Terraform should be empty.
    // If it's some custom brush, both might have an implementation.
    m_brush->Terraform(dt);
    // spawn unit on releasing the button, at least until we write placement collision checks.
    if (Engine.Input().GetMouseButtonOnce(Input::MouseButton::Left)) m_brush->PlaceObject(m_selectedTemplate, m_selectedTeam);
    //

    // update the central positions of the tiles (this was moved so it's done only on save)
    // terrain.UpdateTilesCentralPositions();
    // update the mesh after you're doing terraforming.
    terrain.UpdatePlane();
    //// update the TerrainDataComponent (this is a bit backwards, but it is done for easy terrain access in the level editor).
    terrain.UpdateTerrainDataComponent();
}

void lvle::LevelEditor::CalculateIntersectionPoint()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    const auto view = Engine.ECS().Registry.view<Transform, Camera>();
    const auto& input = Engine.Input();
    for (auto& entity : view)
    {
        auto [transform, camera] = view.get(entity);
        vec3 camToMouseDir = GetRayFromScreenToWorld(input.GetMousePosition(), camera, transform);
        const auto tempIntersectionPoint = m_intersectionPoint;
        m_mouseOnTerrain = terrain.FindRayMeshIntersection(transform.Translation, camToMouseDir, m_intersectionPoint);
        switch (m_lockedAxis)
        {
            case Axis::X:
            {
                m_intersectionPoint.y = tempIntersectionPoint.y;
                break;
            }
            case Axis::Y:
            {
                m_intersectionPoint.x = tempIntersectionPoint.x;
                break;
            }
            default:
                break;
        }
    }
}

void lvle::LevelEditor::LockBrushOnAxis()
{
    const auto& input = Engine.Input();
    // const auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    Transform cameraTransform;
    Camera camera;
    auto view = Engine.ECS().Registry.view<Camera, Transform>();
    for (auto [entity, cameraComponent, transform] : view.each())
    {
        cameraTransform = transform;
        camera = cameraComponent;
    }
    static vec3 firstShiftMousePosClicked;
    static bool firstClick = false;
    if (!firstClick && input.GetKeyboardKey(Input::KeyboardKey::LeftControl) && input.GetMouseButton(Input::MouseButton::Left))
        firstShiftMousePosClicked = bee::GetRayFromScreenToWorld(input.GetMousePosition(), camera, cameraTransform);
    if (input.GetKeyboardKey(Input::KeyboardKey::LeftControl) && input.GetMouseButton(Input::MouseButton::Left))
    {
        firstClick = true;
        if (m_lockedAxis == Axis::None)
        {
            vec3 mouseOffset =
                firstShiftMousePosClicked - bee::GetRayFromScreenToWorld(input.GetMousePosition(), camera, cameraTransform);
            // mouseOffset = glm::rotate(cameraTransform.Rotation, mouseOffset);
            if (abs(mouseOffset.x) > abs(mouseOffset.y))
                m_lockedAxis = Axis::X;
            else
                m_lockedAxis = Axis::Y;
            if (mouseOffset.x == 0.0f && mouseOffset.y == 0.0f) m_lockedAxis = Axis::None;
        }
    }
    else
    {
        firstClick = false;
        m_lockedAxis = Axis::None;
    }
}

void lvle::LevelEditor::DeleteSelectedActors()
{
    auto view = Engine.ECS().Registry.view<Selected, AttributesComponent>();
    for (const auto entity : view)
    {
        auto [selected, attributes] = view.get(entity);
        if (actors::IsUnit(attributes.GetEntityType()))
        {
            auto unitManager = bee::Engine.ECS().GetSystem<UnitManager>();
            unitManager.RemoveUnit(entity);
        }
        else if (actors::IsStructure(attributes.GetEntityType()))
        {
            auto structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
            structureManager.RemoveStructure(entity);
        }
        else if (actors::IsProp(attributes.GetEntityType()))
        {
            auto propManager = bee::Engine.ECS().GetSystem<PropManager>();
            propManager.RemoveGameProp(entity);
        }
    }
}

void LevelEditor::DisplayAreaBrushProperties(Brush& brush)
{
    AreaBrush& areaBrush = static_cast<AreaBrush&>(brush);
    ImGui::InputInt("Area index", &areaBrush.areaIndex);
    const auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    if (ImGui::Button("Add area", ImVec2(100, 30)))
    {
        terrain.m_data->m_areaPresets.emplace_back();    
    }

    int index = 0;
    for (AreaPreset& areaPreset : terrain.m_data->m_areaPresets)
    {
        if( ImGui::CollapsingHeader((std::string("Area") + std::to_string(index)).c_str()))
        {
            ImGui::ColorPicker3((std::string("Display color") + std::to_string(index)).c_str(), areaPreset.color);
        }

        
        index++;
    }
}

void lvle::LevelEditor::TerrainPalette()
{
    if (ImGui::CollapsingHeader("Terrain Palette", ImGuiTreeNodeFlags_DefaultOpen))
    {
        m_brush->RemovePreviewModel();
        //
        DisplayBrushOptions(m_terraformBrush, BrushType::Terraform, "Terraform");
        DisplayBrushOptions(m_textureBrush, BrushType::Texturing, "Texture");
        //DisplayTextureOptions();
        DisplayBrushOptions(m_pathBrush, BrushType::Path, "Pathing");
        DisplayBrushOptions(m_areaBrush, BrushType::GameplayArea, "Area");
        //
        m_terraformBrush->SetMode(m_brushModes[static_cast<int>(BrushType::Terraform)]);
        m_textureBrush->SetMode(m_brushModes[static_cast<int>(BrushType::Texturing)]);
        m_pathBrush->SetMode(m_brushModes[static_cast<int>(BrushType::Path)]);
        m_areaBrush->SetMode(m_brushModes[static_cast<int>(BrushType::GameplayArea)]);
        //
        int radius = m_brush->GetRadius();
        ImGui::DragInt("Brush Size", &radius, 0.05f, 0, 16);
        m_brush->SetRadius(radius);

        if (ImGui::Button("Adjust actor vertical pos"))
        {
            m_terraformBrush->UpdateActorsVerticalPos();
            m_terraformBrush->UpdateFoliageVerticalPos();
        }

        // Additional brush-specific UI
        if (m_selectedBrush == BrushType::Terraform)
        {
            m_brush = m_terraformBrush.get();
            float intensity = m_terraformBrush->GetIntensity();
            ImGui::DragFloat("Brush Intensity", &intensity, 0.1f, 1.0f, m_terraformBrush->GetMaxIntensity());
            m_terraformBrush->SetIntensity(intensity);
            bool averaging = m_terraformBrush->GetDoAveraging();
            ImGui::Checkbox("Averaging (rougher features, only updates below average vertices)", &averaging);
            m_terraformBrush->SetDoAveraging(averaging);
        }
        //
        if (m_selectedBrush == BrushType::Texturing)
        {
            m_brush = m_textureBrush.get();
            ImGui::SliderFloat("Texture Weight", &m_textureBrush->weight, 0.0f, 1.0f);
        }
        //
        if (m_selectedBrush == BrushType::Path)
        {
            m_brush = m_pathBrush.get();
        }
        //
        if (m_selectedBrush == BrushType::GameplayArea)
        {
            m_brush = m_areaBrush.get();
            DisplayAreaBrushProperties(*m_brush);
        }
    }
}

void lvle::LevelEditor::UnitPalette()
{
    if (ImGui::CollapsingHeader("Unit Palette", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& unitManager = Engine.ECS().GetSystem<UnitManager>();
        auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();

        vector<int> teams = {0, 1, 2};
        if (ImGui::BeginCombo("Teams", magic_enum::enum_name(m_selectedTeam).data()))
        {
            for (int i = 0; i < teams.size(); i++)
            {
                Team team = static_cast<Team>(i);
                bool isSelected = m_selectedTeam == team;
                if (ImGui::Selectable(magic_enum::enum_name(team).data(), isSelected))
                {
                    m_selectedTeam = team;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();
        m_brush = m_unitBrush.get();
        static bool openUnitPopup = false;
        static std::string templateToEdit = "";
        auto pairs = unitManager.GetUnits();
        for (auto& pair : pairs)
        {
            auto unitType = pair.first;
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf;
            if (m_selectedTemplate == unitType) nodeFlags |= ImGuiTreeNodeFlags_Selected;
            if (ImGui::TreeNodeEx(unitType.c_str(), nodeFlags))
            {
                if (ImGui::IsItemClicked())
                {
                    m_selectedTemplate = unitType;
                    m_brush->SetPreviewModel(m_selectedTemplate);
                    Engine.ECS().GetSystem<SelectionSystem>().Deactivate();
                    Engine.ECS().GetSystem<SelectionSystem>().DeselectUnits(false);
                    m_brush->Activate();
                    m_entitiesEditMode = false;
                    bee::Engine.Inspector().data.enabledGuizmo = false;
                }
                if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
                {
                    openUnitPopup = true;
                    m_selectedTemplate = unitType;
                    templateToEdit = unitType;
                }

                ImGui::TreePop();
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::Button("Edit"))
                {
                    openUnitPopup = true;
                    templateToEdit = unitType;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button("Delete"))
                {
                    unitManager.RemoveUnitTemplate(unitType);
                    unitManager.RemoveUnitsOfTemplate(unitType);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::Separator();
        if (ImGui::Button("New Template##AddNewUnitTemplate"))
        {
            ImGui::OpenPopup("New Unit Template");
        }
        if (ImGui::BeginPopupModal("New Unit Template"))
        {
            static string name = "NewUnit";
            static UnitTemplatePresets preset = UnitTemplatePresets::None;
            if (ImGui::InputText("Template Name", &name,
                                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                UnitTemplate unitTemplate(preset);
                unitTemplate.name = name;
                unitManager.AddNewUnitTemplate(unitTemplate);
                templateToEdit = unitTemplate.name;
                openUnitPopup = true;
                ImGui::CloseCurrentPopup();
            }
            if (DisplayEnumDropDown(preset)) {}
            ImGui::SameLine();
            ImGui::Text("Template Presets");
            if (ImGui::Button("Create"))
            {
                UnitTemplate unitTemplate(preset);
                unitTemplate.name = name;
                unitManager.AddNewUnitTemplate(unitTemplate);
                templateToEdit = unitTemplate.name;
                openUnitPopup = true;
                ImGui::CloseCurrentPopup();
            }
            if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::Escape)) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (openUnitPopup)
        {
            if (ImGui::Begin("Edit Unit Template"))
            {
                ImGui::SetWindowFontScale(1.5f);
                ImGui::Text(templateToEdit.c_str());
                ImGui::SetWindowFontScale(1.0f);
                auto& editedUnit = unitManager.GetUnitTemplate(templateToEdit);
                

                // Model
                ImGui::InputText("Model", &editedUnit.modelPath);
                std::filesystem::path path;
                if (assetSystem.SetDragDropTarget(path, {".gltf", ".glb"}))
                {
                    editedUnit.modelPath = path.string();
                    bee::RemoveSubstring(editedUnit.modelPath, "assets/");
                    editedUnit.model = Engine.Resources().Load<Model>(editedUnit.modelPath);
                    PrepareActorMaterialPaths(editedUnit);
                }
                // Corpse Model
                ImGui::InputText("Corpse Model", &editedUnit.corpsePath);
                if (assetSystem.SetDragDropTarget(path, {".gltf", ".glb"}))
                {
                    editedUnit.corpsePath = path.string();
                    bee::RemoveSubstring(editedUnit.corpsePath, "assets/");
                    editedUnit.corpseModel = Engine.Resources().Load<Model>(editedUnit.corpsePath);
                }

                // Material
                for (int i = 0; i < editedUnit.materialPaths.size(); i++)
                {
                    std::string name = "Material " + std::to_string(i);
                    ImGui::InputText(name.c_str(), &editedUnit.materialPaths[i]);
                    if (assetSystem.SetDragDropTarget(path, {".pepimat"}))
                    {
                        editedUnit.materialPaths[i] = path.string();
                        bee::RemoveSubstring(editedUnit.materialPaths[i], "assets/");
                        editedUnit.materials[i] = Engine.Resources().Load<Material>(editedUnit.materialPaths[i]);
                    }
                }

                // Model Transform fine-tuning
                // model rotation
                ImGui::Text("Model Rotation: ");
                glm::vec3 modelRotation = glm::degrees(glm::eulerAngles(editedUnit.modelOffset.Rotation));
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("X", &modelRotation.x, 5.0f, -180.0f, 180.0f))
                {
                    editedUnit.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Y", &modelRotation.y, 5.0f, -180.0f, 180.0f))
                {
                    editedUnit.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Z", &modelRotation.z, 5.0f, -180.0f, 180.0f))
                {
                    editedUnit.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                
                ImGui::SameLine(500);
                if (ImGui::Button("Rotate 90 deg. on X"))
                {
                    modelRotation.x += 90.0f;
                    editedUnit.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Click this if the model had Y up in Maya/Houdini");
                    ImGui::EndTooltip();
                }
                // model rooting
                float zOffset = editedUnit.modelOffset.Translation.z;
                ImGui::Text("Model Translation: ");
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Vertical Offset", &zOffset, 0.1f, -10.0f, 10.0f))
                {
                    editedUnit.modelOffset.Translation.z = zOffset;
                }
                // FSM
                ImGui::InputText("Finite State Machine", &editedUnit.fsmPath);
                // std::filesystem::path path; // defined above
                if (assetSystem.SetDragDropTarget(path, {".json"}))  // TODO: change extension to .fsm
                {
                    editedUnit.fsmPath = path.string();
                    bee::RemoveSubstring(editedUnit.fsmPath, "assets/");
                }
                // Animation Controller
                ImGui::InputText("AnimationController", &editedUnit.animationControllerPath);
                if (assetSystem.SetDragDropTarget(path, {".anim"}))
                {
                    editedUnit.animationControllerPath = path.string();
                    bee::RemoveSubstring(editedUnit.animationControllerPath, "assets/");
                }

                int xDimensions = editedUnit.tileDimensions.x;
                int yDimensions = editedUnit.tileDimensions.y;

                ImGui::SetNextItemWidth(60);
                if (ImGui::DragInt("Tile Width", &xDimensions, 0.02f, 1, 8))
                {
                    editedUnit.tileDimensions.x = xDimensions;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60);
                if (ImGui::DragInt("Tile Height", &yDimensions, 0.02f, 1, 8))
                {
                    editedUnit.tileDimensions.y = yDimensions;
                }
                ImGui::Separator();

                if (ImGui::Button("Add attribute"))
                {
                    ImGui::OpenPopup("AttributeSelectionPopup");
                }

                bool selected = false;
                BaseAttributes selectedAttribute;
                if (ImGui::BeginPopup("AttributeSelectionPopup"))
                {
                    for (int i = 0; i < magic_enum::enum_count<BaseAttributes>(); i++)
                    {
                        BaseAttributes current = static_cast<BaseAttributes>(i);
                        if (editedUnit.HasAttribute(current)) continue;
                        if (ImGui::Selectable(std::string(magic_enum::enum_name(current)).c_str(), selected))
                        {
                            selectedAttribute = current;
                            selected = true;
                        }
                    }

                    ImGui::EndPopup();
                }

                if (selected)
                {
                    editedUnit.SetAttribute(selectedAttribute, 1.0, false);
                }

                // AttributesComponent
                for (int i = 0; i < magic_enum::enum_count<BaseAttributes>(); i++)
                {
                    auto attributeName = magic_enum::enum_name(static_cast<BaseAttributes>(i));
                    if (editedUnit.HasAttribute(static_cast<BaseAttributes>(i)))
                    {
                        auto attributeValue = editedUnit.GetAttribute(static_cast<BaseAttributes>(i));
                        if (ImGui::InputDouble(attributeName.data(), &attributeValue))
                        {
                            editedUnit.SetAttribute(static_cast<BaseAttributes>(i), attributeValue);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(std::string(std::string(u8"\uf1f8") + std::string("##") +
                                                      std::string(magic_enum::enum_name(static_cast<BaseAttributes>(i))))
                                              .c_str()))
                        {
                            editedUnit.RemoveAttribute(static_cast<BaseAttributes>(i));
                        }
                    }
                }

                ImGui::Text("Orders available");
                std::vector<OrderType> ordersForUnit = editedUnit.availableOrders;

                for (const auto& item : magic_enum::enum_values<OrderType>())
                {
                    bool checked = (std::find(editedUnit.availableOrders.begin(), editedUnit.availableOrders.end(),
                                              item) != editedUnit.availableOrders.end());

                    if (ImGui::Checkbox(
                            std::string(std::string(magic_enum::enum_name(item)) + std::string("##Order"))
                                .c_str(),
                            &checked))
                    {
                        if (checked)
                        {
                            ordersForUnit.push_back(item);
                        }
                        else
                        {
                            ordersForUnit.erase(
                                std::remove(ordersForUnit.begin(), ordersForUnit.end(), item));
                        }
                    }
                }
                editedUnit.availableOrders = ordersForUnit;

                // Audio
                ImGui::NewLine();

                if (ImGui::Button("Add Sound"))
                {
                    ImGui::OpenPopup("SoundSelectionPopup");
                }

                bool selectedSound = false;
                UnitSoundTypes sound = UnitSoundTypes::None;
                if (ImGui::BeginPopup("SoundSelectionPopup"))
                {
                    for (int i = 1; i < magic_enum::enum_count<UnitSoundTypes>(); i++)
                    {
                        UnitSoundTypes current = static_cast<UnitSoundTypes>(i);
                        if (editedUnit.unitSounds.find(current) != editedUnit.unitSounds.end()) continue;
                        if (ImGui::Selectable(std::string(magic_enum::enum_name(current)).c_str(), selected))
                        {
                            sound = current;
                            selectedSound = true;
                        }
                    }

                    ImGui::EndPopup();
                }

                if (selectedSound)
                {
                    editedUnit.unitSounds[sound] = {};
                }

                for (auto& pair : editedUnit.unitSounds)
                {
                    ImGui::Text(std::string(std::string(magic_enum::enum_name(pair.first)) + " Sound").c_str());
                    ImGui::SameLine();
                    ImGui::InputText(pair.second.c_str(), &editedUnit.iconPath);
                    if (assetSystem.SetDragDropTarget(path, {".mp3", ".wav", ".ogg"}))
                    {
                        pair.second = path.string();
                        bee::RemoveSubstring(editedUnit.iconPath, "assets/");
                    }
                }

                ImGui::NewLine();
                ImGui::Text("Icon");
                ImGui::InputText("Icon", &editedUnit.iconPath);
                if (assetSystem.SetDragDropTarget(path, {".png"}))
                {
                    editedUnit.iconPath = path.string();
                    bee::RemoveSubstring(editedUnit.iconPath, "assets/");
                    bee::GetPngImageDimensions(Engine.FileIO().GetPath(FileIO::Directory::Asset, editedUnit.iconPath),
                                               editedUnit.iconTextureCoordinates.z, editedUnit.iconTextureCoordinates.w);
                }
                ImGui::InputFloat2("coordinates (in pixels)", &editedUnit.iconTextureCoordinates.x);
                ImGui::InputFloat2("size (in pixels)", &editedUnit.iconTextureCoordinates.z);

                ImGui::NewLine();
                if (ImGui::Button("Close"))
                {
                    openUnitPopup = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
                if (ImGui::Button("Apply"))
                {
                    unitManager.ReloadUnitsFromTemplate(templateToEdit);
                    m_brush->SetPreviewModel(templateToEdit);
                    openUnitPopup = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::End();
        }
    }
}

void lvle::LevelEditor::StructurePalette()
{
    if (ImGui::CollapsingHeader("Structure Palette", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& structureManager = Engine.ECS().GetSystem<StructureManager>();
        auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();

        vector<int> teams = {0, 1, 2};
        if (ImGui::BeginCombo("Teams", magic_enum::enum_name(m_selectedTeam).data()))
        {
            for (int i = 0; i < teams.size(); i++)
            {
                Team team = static_cast<Team>(i);
                bool isSelected = m_selectedTeam == team;
                if (ImGui::Selectable(magic_enum::enum_name(team).data(), isSelected))
                {
                    m_selectedTeam = team;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();

        static std::string templateToEdit = "";
        static bool openStructurePopUp = false;

        m_brush = m_structureBrush.get();
        m_brush->SetRadius(0);
        auto pairs = structureManager.GetStructures();
        for (auto pair : pairs)
        {
            auto structureType = pair.first;
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf;
            if (m_selectedTemplate == structureType) nodeFlags |= ImGuiTreeNodeFlags_Selected;
            if (ImGui::TreeNodeEx(structureType.c_str(), nodeFlags))
            {
                if (ImGui::IsItemClicked())
                {
                    m_selectedTemplate = structureType;
                    m_brush->SetPreviewModel(m_selectedTemplate);
                    m_brush->SetObjectDimensions(structureManager.GetStructureTemplate(structureType).tileDimensions);
                    Engine.ECS().GetSystem<SelectionSystem>().Deactivate();
                    Engine.ECS().GetSystem<SelectionSystem>().DeselectUnits(false);
                    m_brush->Activate();
                    m_entitiesEditMode = false;
                    bee::Engine.Inspector().data.enabledGuizmo = false;
                }
                if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
                {
                    m_selectedTemplate = structureType;
                    m_brush->SetPreviewModel(m_selectedTemplate);
                    openStructurePopUp = true;
                    m_selectedTemplate = structureType;
                    templateToEdit = structureType;
                }

                ImGui::TreePop();
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::Button("Edit"))
                {
                    openStructurePopUp = true;
                    templateToEdit = structureType;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button("Delete"))
                {
                    structureManager.RemoveStructureTemplate(structureType);
                    structureManager.RemoveStructuresOfTemplate(structureType);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::Separator();

        if (ImGui::Button("New Template##AddNewStructureTemplate"))
        {
            ImGui::OpenPopup("New Structure Template");
        }
        if (ImGui::BeginPopupModal("New Structure Template"))
        {
            static string name = "NewStructure";
            static StructureTemplatePresets preset = StructureTemplatePresets::None;
            if (ImGui::InputText("Template Name", &name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                StructureTemplate structureTemplate(preset);
                structureTemplate.name = name;
                structureManager.AddNewStructureTemplate(structureTemplate);
                templateToEdit = structureTemplate.name;
                openStructurePopUp = true;
                ImGui::CloseCurrentPopup();
            }
            if (DisplayEnumDropDown(preset))
            {
            }
            ImGui::SameLine();
            ImGui::Text("Template Presets");
            if (ImGui::Button("Create"))
            {
                StructureTemplate structureTemplate(preset);
                structureTemplate.name = name;
                structureManager.AddNewStructureTemplate(structureTemplate);
                templateToEdit = structureTemplate.name;
                openStructurePopUp = true;
                ImGui::CloseCurrentPopup();
            }
            if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::Escape)) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (openStructurePopUp)
        {
            if (ImGui::Begin("Edit Structure Template"))
            {
                ImGui::SetWindowFontScale(1.5f);
                ImGui::Text(templateToEdit.c_str());
                ImGui::SetWindowFontScale(1.0f);
                auto& editedStructure = structureManager.GetStructureTemplate(templateToEdit);
                //Structure Type
                static std::vector<int> types{ 0,1,2,3,4 };
                m_selectedStructureTypes = editedStructure.structureType;
                if (ImGui::BeginCombo("Structure Type", magic_enum::enum_name(m_selectedStructureTypes).data()))
                {
                    for (int i = 0; i < types.size(); i++)
                    {
                        StructureTypes type = static_cast<StructureTypes>(i);
                        bool isSelected = m_selectedStructureTypes == type;
                        if (ImGui::Selectable(magic_enum::enum_name(type).data(), isSelected))
                        {
                            m_selectedStructureTypes = type;
                            editedStructure.structureType = type;
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Model
                ImGui::InputText("Model", &editedStructure.modelPath);
                std::filesystem::path path;
                if (assetSystem.SetDragDropTarget(path, {".gltf", ".glb"}))
                {
                    editedStructure.modelPath = path.string();
                    bee::RemoveSubstring(editedStructure.modelPath, "assets/");
                    editedStructure.model = Engine.Resources().Load<Model>(editedStructure.modelPath);
                    PrepareActorMaterialPaths(editedStructure);
                }
                // Corpse Model
                ImGui::InputText("Corpse Model", &editedStructure.corpsePath);
                if (assetSystem.SetDragDropTarget(path, {".gltf", ".glb"}))
                {
                    editedStructure.corpsePath = path.string();
                    bee::RemoveSubstring(editedStructure.corpsePath, "assets/");
                    editedStructure.corpseModel = Engine.Resources().Load<Model>(editedStructure.corpsePath);
                }
                // Materials
                for (int i = 0; i < editedStructure.materialPaths.size(); i++)
                {
                    std::string name = "Material " + std::to_string(i);
                    ImGui::InputText(name.c_str(), &editedStructure.materialPaths[i]);
                    if (assetSystem.SetDragDropTarget(path, {".pepimat"}))
                    {
                        editedStructure.materialPaths[i] = path.string();
                        bee::RemoveSubstring(editedStructure.materialPaths[i], "assets/");
                        editedStructure.materials[i] = Engine.Resources().Load<Material>(editedStructure.materialPaths[i]);
                    }
                }
                // Model Transform fine-tuning
                // model rotation
                ImGui::Text("Model Rotation: ");
                glm::vec3 modelRotation = glm::degrees(glm::eulerAngles(editedStructure.modelOffset.Rotation));
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("X", &modelRotation.x, 5.0f, -180.0f, 180.0f))
                {
                    editedStructure.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Y", &modelRotation.y, 5.0f, -180.0f, 180.0f))
                {
                    editedStructure.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Z", &modelRotation.z, 5.0f, -180.0f, 180.0f))
                {
                    editedStructure.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }

                ImGui::SameLine(500);
                if (ImGui::Button("Rotate 90 deg. on X"))
                {
                    modelRotation.x += 90.0f;
                    editedStructure.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Click this if the model had Y up in Maya/Houdini");
                    ImGui::EndTooltip();
                }
                // model rooting
                float zOffset = editedStructure.modelOffset.Translation.z;
                ImGui::Text("Model Translation: ");
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Vertical Offset", &zOffset, 0.1f, -10.0f, 10.0f))
                {
                    editedStructure.modelOffset.Translation.z = zOffset;
                }
                //Handle of upgrade structure
                ImGui::InputText("Upgrade Structure", &editedStructure.buildingUpgradeHandle);
                // FSM
                ImGui::InputText("Finite State Machine", &editedStructure.fsmPath);
                if (assetSystem.SetDragDropTarget(path, {".json"}))  // TODO: change extension to .fsm
                {
                    editedStructure.fsmPath = path.string();
                    bee::RemoveSubstring(editedStructure.fsmPath, "assets/");
                }

                int xDimensions = editedStructure.tileDimensions.x;
                int yDimensions = editedStructure.tileDimensions.y;

                ImGui::SetNextItemWidth(60);
                if (ImGui::DragInt("Tile Width", &xDimensions, 0.02f, 1, 8))
                {
                    editedStructure.tileDimensions.x = xDimensions;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60);
                if (ImGui::DragInt("Tile Height", &yDimensions, 0.02f, 1, 8))
                {
                    editedStructure.tileDimensions.y = yDimensions;
                }

                ImGui::Separator();

                if (ImGui::Button("Add attribute"))
                {
                    ImGui::OpenPopup("AttributeSelectionPopup");
                }

                bool selected = false;
                BaseAttributes selectedAttribute;
                if (ImGui::BeginPopup("AttributeSelectionPopup"))
                {
                    for (int i = 0; i < magic_enum::enum_count<BaseAttributes>(); i++)
                    {
                        BaseAttributes current = static_cast<BaseAttributes>(i);
                        if (editedStructure.HasAttribute(current)) continue;
                        if (ImGui::Selectable(std::string(magic_enum::enum_name(current)).c_str(), selected))
                        {
                            selectedAttribute = current;
                            selected = true;
                        }
                    }

                    ImGui::EndPopup();
                }

                if (selected)
                {
                    editedStructure.SetAttribute(selectedAttribute, 1.0, false);
                }

                // AttributesComponent
                for (int i = 0; i < magic_enum::enum_count<BaseAttributes>(); i++)
                {
                    auto attributeName = magic_enum::enum_name(static_cast<BaseAttributes>(i));
                    if (editedStructure.HasAttribute(static_cast<BaseAttributes>(i)))
                    {
                        auto attributeValue = editedStructure.GetAttribute(static_cast<BaseAttributes>(i));
                        if (ImGui::InputDouble(attributeName.data(), &attributeValue))
                        {
                            editedStructure.SetAttribute(static_cast<BaseAttributes>(i), attributeValue);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(std::string(std::string(u8"\uf1f8") + std::string("##") +
                                                      std::string(magic_enum::enum_name(static_cast<BaseAttributes>(i))))
                                              .c_str()))
                        {
                            editedStructure.RemoveAttribute(static_cast<BaseAttributes>(i));
                        }
                    }
                }

                bool checked = editedStructure.HasAttribute(BaseAttributes::BuffRange) &&
                               editedStructure.HasAttribute(BaseAttributes::BuffValue);
                if (checked)
                {
                    ImGui::Text("Buffs");
                    DisplayEnumDropDown(editedStructure.buffedAttribute);
                }

                // Orders
                ImGui::Text("Orders available");
                std::vector<OrderType> ordersForUnit = editedStructure.availableOrders;

                for (const auto& item : magic_enum::enum_values<OrderType>())
                {
                    bool checked = (std::find(editedStructure.availableOrders.begin(), editedStructure.availableOrders.end(),
                                              item) != editedStructure.availableOrders.end());

                    if (ImGui::Checkbox(

                            std::string(std::string(magic_enum::enum_name(item)) + std::string("##OrderCheckBox"))
                                .c_str(),
                            &checked))
                    {
                        if (checked)
                        {
                            ordersForUnit.push_back(item);
                        }
                        else
                        {
                            ordersForUnit.erase(
                                std::remove(ordersForUnit.begin(), ordersForUnit.end(), item));
                        }
                    }
                }

                editedStructure.availableOrders = ordersForUnit;

                ImGui::NewLine();
                ImGui::Text("Icon");
                ImGui::InputText("Icon", &editedStructure.iconPath);
                if (assetSystem.SetDragDropTarget(path, {".png"}))
                {
                    editedStructure.iconPath = path.string();
                    bee::RemoveSubstring(editedStructure.iconPath, "assets/");
                }
                ImGui::InputFloat2("coordinates (in pixels)", &editedStructure.iconTextureCoordinates.x);
                ImGui::InputFloat2("size (in pixels)", &editedStructure.iconTextureCoordinates.z);

                ImGui::NewLine();
                if (ImGui::Button("Close"))
                {
                    openStructurePopUp = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Apply"))
                {
                    structureManager.ReloadStructuresFromTemplate(templateToEdit);
                    m_brush->SetPreviewModel(templateToEdit);
                    m_brush->SetObjectDimensions(editedStructure.tileDimensions);
                    openStructurePopUp = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::End();
        }
    }
}

void lvle::LevelEditor::PropPalette()
{
    if (ImGui::CollapsingHeader("Prop Palette", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& propManager = Engine.ECS().GetSystem<PropManager>();
        auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();

        static std::string templateToEdit = "";
        static bool openPropPopup = false;
        auto pairs = propManager.GetProps();

        m_brush = m_propBrush.get();
        m_brush->SetRadius(0);
        for (auto pair : pairs)
        {
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf;
            auto propType = pair.first;
            if (m_selectedTemplate == propType) nodeFlags |= ImGuiTreeNodeFlags_Selected;
            if (ImGui::TreeNodeEx(propType.c_str(), nodeFlags))
            {
                if (ImGui::IsItemClicked())
                {
                    m_selectedTemplate = propType;
                    m_brush->SetPreviewModel(m_selectedTemplate);
                    m_brush->SetObjectDimensions(propManager.GetPropTemplate(propType).tileDimensions);
                    Engine.ECS().GetSystem<SelectionSystem>().Deactivate();
                    Engine.ECS().GetSystem<SelectionSystem>().DeselectUnits(false);
                    m_brush->Activate();
                    m_entitiesEditMode = false;
                    bee::Engine.Inspector().data.enabledGuizmo = false;
                }
                if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
                {
                    m_selectedTemplate = propType;
                    m_brush->SetPreviewModel(m_selectedTemplate);

                    openPropPopup = true;
                    m_selectedTemplate = propType;
                    templateToEdit = propType;
                }

                ImGui::TreePop();
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::Button("Edit"))
                {
                    openPropPopup = true;
                    templateToEdit = propType;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button("Delete"))
                {
                    propManager.RemovePropTemplate(propType);
                    propManager.RemovePropsOfTemplate(propType);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        ImGui::Separator();

        if (ImGui::Button("New Template##AddNewPropTemplate"))
        {
            ImGui::OpenPopup("New Prop Template");
        }
        if (ImGui::BeginPopupModal("New Prop Template"))
        {
            static string name = "New Prop";
            if (ImGui::InputText("Template Name", &name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                PropTemplate propTemplate;
                propTemplate.name = name;
                propManager.AddNewPropTemplate(propTemplate);
                templateToEdit = propTemplate.name;
                openPropPopup = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Create"))
            {
                PropTemplate propTemplate;
                propTemplate.name = name;
                propManager.AddNewPropTemplate(propTemplate);
                templateToEdit = propTemplate.name;
                openPropPopup = true;
                ImGui::CloseCurrentPopup();
            }
            if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::Escape)) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (openPropPopup)
        {
            if (ImGui::Begin("Edit Prop Template"))
            {
                ImGui::SetWindowFontScale(1.5f);
                ImGui::Text(templateToEdit.c_str());
                ImGui::SetWindowFontScale(1.0f);
                auto& editedTemplate = propManager.GetPropTemplate(templateToEdit);
                // Model
                // that's a placeholder for now
                ImGui::InputText("Model", &editedTemplate.modelPath);
                std::filesystem::path path;
                if (assetSystem.SetDragDropTarget(path, {".gltf", ".glb"}))
                {
                    editedTemplate.modelPath = path.string();
                    bee::RemoveSubstring(editedTemplate.modelPath, "assets/");
                    editedTemplate.model = Engine.Resources().Load<Model>(editedTemplate.modelPath);
                    PrepareActorMaterialPaths(editedTemplate);
                }
                // Materials
                for (int i = 0; i < editedTemplate.materialPaths.size(); i++)
                {
                    std::string name = "Material " + std::to_string(i);
                    ImGui::InputText(name.c_str(), &editedTemplate.materialPaths[i]);
                    if (assetSystem.SetDragDropTarget(path, {".pepimat"}))
                    {
                        editedTemplate.materialPaths[i] = path.string();
                        bee::RemoveSubstring(editedTemplate.materialPaths[i], "assets/");
                        editedTemplate.materials[i] = Engine.Resources().Load<Material>(editedTemplate.materialPaths[i]);
                    }
                }
                // Model Transform fine-tuning
                // model rotation
                ImGui::Text("Model Rotation: ");
                glm::vec3 modelRotation = glm::degrees(glm::eulerAngles(editedTemplate.modelOffset.Rotation));
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("X", &modelRotation.x, 5.0f, -180.0f, 180.0f))
                {
                    editedTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Y", &modelRotation.y, 5.0f, -180.0f, 180.0f))
                {
                    editedTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Z", &modelRotation.z, 5.0f, -180.0f, 180.0f))
                {
                    editedTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }

                ImGui::SameLine(500);
                if (ImGui::Button("Rotate 90 deg. on X"))
                {
                    modelRotation.x += 90.0f;
                    editedTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Click this if the model had Y up in Maya/Houdini");
                    ImGui::EndTooltip();
                }
                // model rooting
                float zOffset = editedTemplate.modelOffset.Translation.z;
                ImGui::Text("Model Translation: ");
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Vertical Offset", &zOffset, 0.1f, -10.0f, 10.0f))
                {
                    editedTemplate.modelOffset.Translation.z = zOffset;
                }
                // Prop dimensions
                int xDimensions = editedTemplate.tileDimensions.x;
                int yDimensions = editedTemplate.tileDimensions.y;
                ImGui::SetNextItemWidth(60);
                if (ImGui::DragInt("Tile Width", &xDimensions, 0.02f, 1, 8))
                {
                    editedTemplate.tileDimensions.x = xDimensions;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60);
                if (ImGui::DragInt("Tile Height", &yDimensions, 0.02f, 1, 8))
                {
                    editedTemplate.tileDimensions.y = yDimensions;
                }
                ImGui::Separator();
                // AttributesComponent
                if (ImGui::Button("Add attribute"))
                {
                    ImGui::OpenPopup("AttributeSelectionPopup");
                }

                bool selected = false;
                BaseAttributes selectedAttribute;
                if (ImGui::BeginPopup("AttributeSelectionPopup"))
                {
                    for (int i = 0; i < magic_enum::enum_count<BaseAttributes>(); i++)
                    {
                        BaseAttributes current = static_cast<BaseAttributes>(i);
                        if (editedTemplate.HasAttribute(current)) continue;
                        if (ImGui::Selectable(std::string(magic_enum::enum_name(current)).c_str(), selected))
                        {
                            selectedAttribute = current;
                            selected = true;
                        }
                    }

                    ImGui::EndPopup();
                }

                if (selected)
                {
                    editedTemplate.SetAttribute(selectedAttribute, 1.0, false);
                }

                for (int i = 0; i < magic_enum::enum_count<BaseAttributes>(); i++)
                {
                    auto attributeName = magic_enum::enum_name(static_cast<BaseAttributes>(i));
                    if (editedTemplate.HasAttribute(static_cast<BaseAttributes>(i)))
                    {
                        auto attributeValue = editedTemplate.GetAttribute(static_cast<BaseAttributes>(i));
                        if (ImGui::InputDouble(attributeName.data(), &attributeValue))
                        {
                            editedTemplate.SetAttribute(static_cast<BaseAttributes>(i), attributeValue);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(std::string(std::string(u8"\uf1f8") + std::string("##") +
                                                      std::string(magic_enum::enum_name(static_cast<BaseAttributes>(i))))
                                              .c_str()))
                        {
                            editedTemplate.RemoveAttribute(static_cast<BaseAttributes>(i));
                        }
                    }
                }

                if (ImGui::Checkbox("Create Polygon Collider", &editedTemplate.createCollider)){};

                bool isResource = editedTemplate.resourceType != GameResourceType::None;
                if (ImGui::Checkbox(std::string("Is resource:").c_str(), &isResource))
                {
                    if (!isResource)
                    {
                        editedTemplate.resourceType = GameResourceType::None;
                    }
                    else
                    {
                        editedTemplate.resourceType = GameResourceType::Wood;
                    }
                }

                if (isResource)
                {
                    std::unordered_map<size_t, std::string> resourceTypes;
                    int index = 0;
                    magic_enum::enum_for_each<GameResourceType>(
                        [&resourceTypes, &index](auto val)
                        {
                            constexpr GameResourceType resourceType = val;
                            resourceTypes.insert(
                                std::make_pair(static_cast<size_t>(index), std::string(magic_enum::enum_name(resourceType))));
                            index++;
                        });

                    size_t selection = static_cast<size_t>(editedTemplate.resourceType);
                    if (DisplayDropDown("Resource type", resourceTypes, selection))
                    {
                        editedTemplate.resourceType = magic_enum::enum_cast<GameResourceType>(resourceTypes[selection]).value();
                    }
                }

                ImGui::NewLine();
                if (ImGui::Button("Close"))
                {
                    openPropPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Apply"))
                {
                    propManager.ReloadPropsFromTemplate(templateToEdit);
                    m_brush->SetPreviewModel(templateToEdit);
                    m_brush->SetObjectDimensions(editedTemplate.tileDimensions);
                    openPropPopup = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::End();
        }
    }
}

void lvle::LevelEditor::FoliagePalette()
{
    if (ImGui::CollapsingHeader("Foliage Palette", ImGuiTreeNodeFlags_DefaultOpen))
    {
        m_brush = m_foliageBrush.get();
        FoliageBrush& foliageBrush = static_cast<FoliageBrush&>(*m_brush);
        auto& pairs = foliageBrush.m_foliageTypes;
        auto& selectedFoliage = foliageBrush.m_selectedFoliage;

        auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();

        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("Global Brush Options");
        ImGui::SetWindowFontScale(1.0f);

        int radius = foliageBrush.GetRadius();
        ImGui::DragInt("Brush Size", &radius, 0.05f, 0, 16);
        foliageBrush.SetRadius(radius);

        ImGui::TextWrapped(
            "Note: The minus doesn't work very well. Just scale it down to the minimum and go up from there to be sure.");
        if (ImGui::InputInt("Noise Mask Tile Size", &foliageBrush.noiseMaskSize, foliageBrush.noiseMaskSize, foliageBrush.noiseMaskSize))
        {
            if (foliageBrush.noiseMaskSize < 4) foliageBrush.noiseMaskSize = 4;
        }

        if (ImGui::RadioButton("Apply", !foliageBrush.m_erase)) foliageBrush.m_erase = !foliageBrush.m_erase;
        ImGui::SameLine();
        if (ImGui::RadioButton("Erase", foliageBrush.m_erase)) foliageBrush.m_erase = !foliageBrush.m_erase;

        auto view = Engine.ECS().Registry.view<FoliageComponent>();
        int size = static_cast<int>(view.size());
        std::string sizeS = "Amount: " + to_string(size);
        ImGui::Text(sizeS.c_str());

        if (ImGui::Button("Add Foliage Type"))
        {
            ImGui::OpenPopup("New Foliage Template");
        }
        if (ImGui::BeginPopupModal("New Foliage Template"))
        {
            static string name = "New Foliage";
            if (ImGui::InputText("Template Name", &name,
                                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                FoliageTemplate foliageTemplate;
                foliageTemplate.name = name;
                foliageBrush.AddFoliageType(foliageTemplate);
                selectedFoliage = foliageTemplate.name;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Create"))
            {
                FoliageTemplate foliageTemplate;
                foliageTemplate.name = name;
                foliageBrush.AddFoliageType(foliageTemplate);
                selectedFoliage = foliageTemplate.name;
                ImGui::CloseCurrentPopup();
            }
            if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::Escape)) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button(std::string(u8"\uf1f8").c_str()))
        {
            foliageBrush.RemoveFoliageType(selectedFoliage);
            foliageBrush.RemoveAllFoliageFromType(selectedFoliage);
            selectedFoliage = "";
        }

        ImGui::Separator();

        for (auto& pair : pairs)
        {
            auto foliageName = pair.first;
            ImGuiTreeNodeFlags nodeFlags =
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;
            if (selectedFoliage == foliageName) nodeFlags |= ImGuiTreeNodeFlags_Selected;
            if (!pair.second.active)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            }
            if (ImGui::TreeNodeEx(foliageName.c_str(), nodeFlags))
            {
                if (ImGui::IsItemClicked())
                {
                    selectedFoliage = foliageName;
                    Engine.ECS().GetSystem<SelectionSystem>().Deactivate();
                    Engine.ECS().GetSystem<SelectionSystem>().DeselectUnits(false);
                    m_brush->Activate();
                    m_entitiesEditMode = false;
                    bee::Engine.Inspector().data.enabledGuizmo = false;
                }
                if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
                {
                    selectedFoliage = foliageName;
                }

                ImGui::TreePop();
            }
            if (!pair.second.active)
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::BeginPopupContextItem())
            {
                auto& foliageTemplate = foliageBrush.GetFoliageTemplate(foliageName);
                std::string activateDeactivate = "";
                if (foliageTemplate.active)
                    activateDeactivate = "Deactivate";
                else
                    activateDeactivate = "Activate";
                if (ImGui::Button(activateDeactivate.c_str()))
                {
                    foliageTemplate.active = !foliageTemplate.active;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button("Delete"))
                {
                    foliageBrush.RemoveFoliageType(foliageName);
                    foliageBrush.RemoveAllFoliageFromType(foliageName);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::Separator();

        // Display all options and parameters
        auto& foliageTemplate = foliageBrush.GetFoliageTemplate(selectedFoliage);
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text(selectedFoliage.c_str());
        ImGui::SetWindowFontScale(1.0f);
        if (selectedFoliage == "") return;
        if (ImGui::BeginTabBar("Foliage Type Options"))
        {
            // Setup the initial model
            if (ImGui::BeginTabItem("Model"))
            {
                // Model
                // that's a placeholder for now
                ImGui::InputText("Model", &foliageTemplate.modelPath);
                std::filesystem::path path;
                if (assetSystem.SetDragDropTarget(path, {".gltf", ".glb"}))
                {
                    foliageTemplate.modelPath = path.string();
                    PrepareActorMaterialPaths(foliageTemplate);
                    /*bee::RemoveSubstring(foliageTemplate.modelPath, "assets/");
                    foliageTemplate.model = Engine.Resources().Load<Model>(foliageTemplate.modelPath);
                    foliageTemplate.materials.clear();
                    foliageTemplate.materialPaths.clear();
                    foliageTemplate.materialPaths.resize(foliageTemplate.model->GetMeshes().size());
                    foliageTemplate.materials.resize(foliageTemplate.model->GetMeshes().size());

                    for(int i=0; i<foliageTemplate.materialPaths.size();i++)
                    {
                        foliageTemplate.materialPaths[i] = "materials/Empty.pepimat";
                        foliageTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(foliageTemplate.materialPaths[i]);
                    }*/
                }
                // Material
                for(int i=0;i<foliageTemplate.materialPaths.size();i++)
                {
                    std::string name = "Material " + std::to_string(i);
                    ImGui::InputText(name.c_str(), &foliageTemplate.materialPaths[i]);
                    if (assetSystem.SetDragDropTarget(path, {".pepimat"}))
                    {
                        foliageTemplate.materialPaths[i] = path.string();
                        bee::RemoveSubstring(foliageTemplate.materialPaths[i], "assets/");
                        foliageTemplate.materials[i] = Engine.Resources().Load<Material>(foliageTemplate.materialPaths[i]);
                    }
                }
                
                // Model Transform fine-tuning
                // model rotation
                ImGui::Text("Model Rotation: ");
                glm::vec3 modelRotation = glm::degrees(glm::eulerAngles(foliageTemplate.modelOffset.Rotation));
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("X", &modelRotation.x, 5.0f, -180.0f, 180.0f))
                {
                    foliageTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Y", &modelRotation.y, 5.0f, -180.0f, 180.0f))
                {
                    foliageTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Z", &modelRotation.z, 5.0f, -180.0f, 180.0f))
                {
                    foliageTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }

                if (ImGui::Button("Rotate 90 deg. on X"))
                {
                    modelRotation.x += 90.0f;
                    foliageTemplate.modelOffset.Rotation = glm::quat(glm::radians(modelRotation));
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Click this if the model had Y up in Maya/Houdini");
                    ImGui::EndTooltip();
                }
                // model rooting
                float zOffset = foliageTemplate.modelOffset.Translation.z;
                ImGui::Text("Model Translation: ");
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Vertical Offset", &zOffset, 0.1f, -10.0f, 10.0f))
                {
                    foliageTemplate.modelOffset.Translation.z = zOffset;
                }
                // uniform scale
                ImGui::Text("Model Uniform Scale: ");
                float scale = foliageTemplate.modelOffset.Scale.x;
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Scale", &scale, 5.0f, 0.0f, 1000.0f))
                {
                    foliageTemplate.modelOffset.Scale = glm::vec3(scale);
                }

                ImGui::EndTabItem();
            }

            // Tweak parameters
            if (ImGui::BeginTabItem("Placement Params"))
            {
                // wavy foliage?
                ImGui::DragFloat("Wind multiplier", &foliageTemplate.windMultiplier, 0.01f, 0.0f, 2.0f, "%.2f");

                // density
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragInt("Density", &foliageTemplate.density, 5.0f, 0.0f, 1000.0f))
                {
                }
                // distance radius
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Distance Radius", &foliageTemplate.distanceRadius, 5.0f, 0.0f, 1000.0f))
                {
                }
                // z offset range
                ImGui::Text("Height offset");
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("##HeightMin", &foliageTemplate.zOffsetMin, 5.0f, -10000.0f, 10000.0f))
                {
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("##HeightMax", &foliageTemplate.zOffsetMax, 5.0f, 10000.0f, 10000.0f))
                {
                }
                // scale range
                ImGui::Text("Scale Offset");
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("##ScaleMin", &foliageTemplate.scaleRangeMin, 0.1f, 0.00001f, 10000.0f, "%.5f"))
                {
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("##ScaleMax", &foliageTemplate.scaleRangeMax, 0.1f, 0.00001f, 10000.0f, "%.5f"))
                {
                }
                // rotation range
                ImGui::Text("Rotation Offset: ");
                glm::vec3 rotationOffsetInDegrees = glm::degrees(foliageTemplate.rotationRange);
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("X", &rotationOffsetInDegrees.x, 5.0f, 0.0f, 180.0f))
                {
                    foliageTemplate.rotationRange = glm::radians(rotationOffsetInDegrees);
                }
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Y", &rotationOffsetInDegrees.y, 5.0f, 0.0f, 180.0f))
                {
                    foliageTemplate.rotationRange = glm::radians(rotationOffsetInDegrees);
                }
                ImGui::SetNextItemWidth(120);
                if (ImGui::DragFloat("Z", &rotationOffsetInDegrees.z, 5.0f, 0.0f, 180.0f))
                {
                    foliageTemplate.rotationRange = glm::radians(rotationOffsetInDegrees);
                }

                // color variation
                ImGui::Text("Color Variations:");
                if (ImGui::Button("Add Color"))
                {
                    foliageTemplate.colorVariations.push_back(glm::vec3(1.0f));
                }
                ImGui::NewLine();
                for (int i = 0; i < foliageTemplate.colorVariations.size(); i++)
                {
                    std::string label = "##" + to_string(i);
                    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs;
                    ImGui::SameLine();
                    ImGui::ColorEdit3(label.c_str(), (float*)&foliageTemplate.colorVariations[i], flags);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    {
                        foliageTemplate.colorVariations.erase(foliageTemplate.colorVariations.begin() + i);
                    }
                }
                ImGui::SliderFloat("Color Intensity", &foliageTemplate.colorOverlayIntensity, 0.0f, 1.0f);

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
}

void lvle::LevelEditor::DebugToggles()
{
    // flags
    if (ImGui::CollapsingHeader("Debug Toggles"))
    {
        auto flags = m_debugFlags;
        bool changed = false;
        changed |= ImGui::CheckboxFlags("Big Wireframe", &flags, EditorDebugCategory::Wireframe);
        changed |= ImGui::CheckboxFlags("Mouse Position", &flags, EditorDebugCategory::Mouse);
        changed |= ImGui::CheckboxFlags("XYZ Cross", &flags, EditorDebugCategory::Cross);
        changed |= ImGui::CheckboxFlags("Normals", &flags, EditorDebugCategory::Normals);
        changed |= ImGui::CheckboxFlags("Pathing", &flags, EditorDebugCategory::Pathing);
        changed |= ImGui::CheckboxFlags("Areas", &flags, EditorDebugCategory::Areas);
        if (changed)
        {
            m_debugFlags = flags;
        }
    }
}

void lvle::LevelEditor::MapDetails()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();

    // Map props
    ImGui::Begin("Map Properties");
    const string widthDisplay = "Width: " + to_string(terrain.m_data->m_width);
    const string heightDisplay = "Height" + to_string(terrain.m_data->m_height);
    const string stepDisplay = "Square Size: " + to_string(terrain.m_data->m_step);
    ImGui::Text(widthDisplay.c_str());
    ImGui::SameLine(100);
    ImGui::Text(stepDisplay.c_str());
    ImGui::SameLine(300);
    // Creating New Level
    static bool openConfigPopup = false;
    if (ImGui::Button("New Level"))
    {
        openConfigPopup = true;
    }
    ImGui::Text(heightDisplay.c_str());
    ImGui::SameLine(100);
    // Save/Load Levels
    ImGui::SetNextItemWidth(200);
    if (ImGui::Button("Save Level"))
    {
        ImGui::OpenPopup("Add file name");
    }
    if (ImGui::BeginPopupModal("Add file name"))
    {
        static std::string fileName;
        ImGui::InputText("File name", &fileName);
        if (ImGui::Button("Save"))
        {
            std::string s = fileName + ".json";
            terrain.SaveLevel(s);
            actors::SaveActorsTemplates(fileName);
            actors::SaveActorsData(fileName);
            Engine.ECS().GetSystem<LightSystem>().SaveLights(fileName);
            Engine.ECS().GetSystem<ParticleSystem>().SaveEmitters(fileName);
            m_foliageBrush->SaveFoliageTemplates(fileName);
            m_foliageBrush->SaveFoliageInstances(fileName);
            fileName = "";
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine(300);
    ImGui::SetNextItemWidth(200);
    static bool openPopUpLoad = false;
    if (ImGui::Button("Load Level"))
    {
        openPopUpLoad = true;
    }
    static std::filesystem::path path;
    if (openPopUpLoad)
    {
        if (ImGui::Begin("Drag and drop a level file"))
        {
            static std::string text = "Drop File Here";
            static std::string inputText = "";
            ImGui::InputText(text.c_str(), &inputText);
            if (assetSystem.SetDragDropTarget(path, {".json"}))
            {
                inputText = path.string();
            }
            if (ImGui::Button("Load"))
            {
                // formatting the string
                std::string pathString = path.string();
                bee::RemoveSubstring(pathString, "assets/levels\\");
                bee::RemoveSubstring(pathString, ".json");

                // reset brush
                m_intersectionPoint = vec3(0.0f, 0.0f, 0.1f);
                m_brush->SetHoveredVertexIndex(0);
                m_brush->SetHoveredSmallPointIndex(0);
                m_brush->Clear();

                // loading the level
                terrain.LoadLevel(pathString);
                terrain.UpdateTerrainDataComponent();
                terrain.CalculateAllTilesCentralPositions();
                bee::ai::NavigationGrid nav_grid(terrain.m_data->m_tiles[0].centralPos, terrain.m_data->m_step,
                                                 terrain.m_data->m_width, terrain.m_data->m_height);
                bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().GetGrid() = nav_grid;
                bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();
                actors::LoadActorsTemplates(pathString);
                actors::LoadActorsData(pathString);
                Engine.ECS().GetSystem<LightSystem>().LoadLights(pathString);
                Engine.ECS().GetSystem<ParticleSystem>().LoadEmitters(pathString);
                m_foliageBrush->LoadFoliageTemplates(pathString);
                m_foliageBrush->LoadFoliageInstances(pathString);

                // reseting the text and closing the pop up
                inputText = "";
                openPopUpLoad = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                openPopUpLoad = false;
            }
            ImGui::End();
        }
    }
    glm::ivec2 brushPos = terrain.IndexToCoords(m_brush->GetHoveredVertexIndex(), terrain.m_data->m_width + 1);
    brushPos -= glm::ivec2(terrain.m_data->m_width / 2, terrain.m_data->m_height / 2);
    std::string brushPosStr = "Coordinates: " + to_string(brushPos.x) + " " + to_string(brushPos.y);
    ImGui::Text(brushPosStr.c_str());

    auto& cs = Engine.ECS().GetSystem<CameraSystemEditor>();
    ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::DragFloat("Camera Speed", &cs.GetTranslate(), 5.0f, 50.0f, 1600.0f); ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::DragFloat("View Distance", &cs.GetFarPlane(), 1.0f, 200.0f, 1000.0f); ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::DragFloat("Camera Front Speed", &cs.GetMouseWheel(), 1.0f, 10.0f, 1600.0f); ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::DragFloat("Camera Rotate", &cs.GetRotate(), 1.0f, 1.0f, 345.0f); ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::DragFloat("Camera FOV", &cs.GetFOV(), 1.0f, 15.0f, 70.0f); ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::DragFloat("Camera Orbit", &cs.GetOrbit(), 1.0f, 90.0f, 720.0f); ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::DragFloat("Focus Distance", &cs.GetFocusDistance(), 0.1f, 1.0f, 20.0f); ImGui::SetNextItemWidth(300);
    ImGui::NewLine();
    ImGui::Checkbox(":Enable In-Game Controls", &cs.GetMode());
    ImGui::End();
    // New map
    if (openConfigPopup)
    {
        // ImGui::SetNextWindowPos(ImVec2(600, 900));
        ImGui::SetNextWindowSize(ImVec2(600, 150));
        ImGui::Begin("New Level Config");
        static int width = 16, height = 16;
        static float step = 1.0f;

        ImGui::SetNextItemWidth(60);

        // Width button
        ImGui::InputInt("Width", &width, 0, 0);

        // Width combo
        ImGui::SameLine(120);
        ImGui::SetNextItemWidth(150);
        static size_t selectedWidth = 2;
        if (DisplayDropDown("Preset Map Width", m_presetMapSize, selectedWidth))
        {
            width = selectedWidth;
        }
        ImGui::SameLine(500);

        // Cancel button
        if (ImGui::Button("Cancel"))
        {
            openConfigPopup = false;
        }
        ImGui::SetNextItemWidth(60);

        // Height button
        ImGui::InputInt("Height", &height, 0, 0);

        // Height combo

        ImGui::SameLine(120);
        ImGui::SetNextItemWidth(150);
        static size_t selectedHeight = 2;
        if (DisplayDropDown("Preset Map Height", m_presetMapSize, selectedHeight))
        {
            height = selectedHeight;
        }
        ImGui::SameLine(330);
        ImGui::Text("All progress will be lost.");
        ImGui::SameLine(500);
        if (ImGui::Button("Confirm"))
        {
            m_intersectionPoint = vec3(0.0f, 0.0f, 0.1f);
            m_brush->SetHoveredVertexIndex(0);
            m_brush->SetHoveredSmallPointIndex(0);
            m_brush->Clear();
            actors::DeleteActorEntities();

            openConfigPopup = false;
            m_newLevel = true;
            terrain.CreatePlane(width, height, step);
            terrain.CalculateAllTilesCentralPositions();
            //// update the TerrainDataComponent (this is a bit backwards but it is done for easy terrain access in the level
            //// editor).
            terrain.UpdateTerrainDataComponent();
            bee::ai::NavigationGrid nav_grid(terrain.m_data->m_tiles[0].centralPos, terrain.m_data->m_step,
                                             terrain.m_data->m_width, terrain.m_data->m_height);
            bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().GetGrid() = nav_grid;
            bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();
        }

        ImGui::SetNextItemWidth(80);
        ImGui::InputFloat("Square size", &step);

        ImGui::End();
    }
}

void lvle::LevelEditor::Guizmo()
{
    // Camera projection
    static bool isPerspective = true;
    static float fov = 27.f;
    float viewWidth = 10.f;  // for orthographic

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::BeginFrame();

    ImGui::Begin("Transform Editor");

    ImGui::Text("Camera");
    ImGui::Checkbox("Free transform movement", &bee::Engine.Inspector().data.freeMovement);
    if (ImGui::RadioButton("Perspective", isPerspective)) isPerspective = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("Orthographic", !isPerspective)) isPerspective = false;
    if (isPerspective)
    {
        ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);
    }
    else
    {
        ImGui::SliderFloat("Ortho width", &viewWidth, 1, 20);
    }

    ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
    if (ImGuizmo::IsUsing())
    {
        ImGui::Text("Using gizmo");
    }
    else
    {
        ImGui::Text(ImGuizmo::IsOver() ? "Over gizmo" : "");
        ImGui::SameLine();
        ImGui::Text(ImGuizmo::IsOver(ImGuizmo::TRANSLATE) ? "Over translate gizmo" : "");
        ImGui::SameLine();
        ImGui::Text(ImGuizmo::IsOver(ImGuizmo::ROTATE) ? "Over rotate gizmo" : "");
        ImGui::SameLine();
        ImGui::Text(ImGuizmo::IsOver(ImGuizmo::SCALE) ? "Over scale gizmo" : "");
    }
    ImGui::Separator();
    auto selectedView = Engine.ECS().Registry.view<Selected, Transform>();
    for (auto [entity, selectedObject, transform] : selectedView.each())
    {
        const glm::mat4 transformM = transform.WorldMatrix;
            float* floatProjection;
            float* floatViewM;
            float* floattransformM = (float*)glm::value_ptr(transformM);
            auto view = Engine.ECS().Registry.view<Camera>();
            for (auto [entity, cameraComponent] : view.each())
            {
                floatProjection = (float*)glm::value_ptr(cameraComponent.Projection);
                floatViewM = (float*)glm::value_ptr(cameraComponent.View);

                bee::Engine.Inspector().data.EditTransform(floatViewM, floatProjection, floattransformM);
                transform.Translation = bee::Engine.Inspector().data.transformTranslation;
                transform.Scale = bee::Engine.Inspector().data.transformScale;
                transform.Rotation = bee::Engine.Inspector().data.transformRotation;
            }
        break;
    }
        ImGui::End();
}

void lvle::LevelEditor::EditBindings()
{
    ImGui::Begin("Input Binding Editor");
    if (ImGui::CollapsingHeader("Order Input Binding", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int count = 0;
        for (const auto& item : magic_enum::enum_values<OrderType>())
        {
            if (item == OrderType::None) continue;
            ImGui::Text(std::string(std::string(magic_enum::enum_name(item))).c_str());

            bool focus = m_editedOrder == item;
            if (focus) ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 100, 255));
            ImGui::SameLine(300, -1);
            auto magicEnumReturn = magic_enum::enum_name(m_orderShortCuts[item]);
            std::string button = !magicEnumReturn.empty() ? magicEnumReturn.data() : "Error";

            button += "##" + to_string(static_cast<int>(item));
            if (ImGui::Button(button.c_str()))
            {
                printf("test\n");
                m_editShortCut = true;
                m_editedOrder = item;
                m_lastKeyPressed = Input::KeyboardKey::None;
                Engine.Input().lastPressedKey = Input::KeyboardKey::None;
                m_orderShortCuts[m_editedOrder] = Input::KeyboardKey::None;
            }
            if (focus) ImGui::PopStyleColor();
            count++;
        }
        printf("%i\n", count);
        if (!ImGui::IsWindowFocused())
        {
            m_editShortCut = false;
            m_editedOrder = OrderType::None;
            m_lastKeyPressed = Input::KeyboardKey::None;
            Engine.Input().lastPressedKey = Input::KeyboardKey::None;
        }
        if (m_editedOrder != OrderType::None && m_editShortCut && m_lastKeyPressed != Input::KeyboardKey::None)
        {
            m_orderShortCuts[m_editedOrder] = m_lastKeyPressed;
            m_editShortCut = false;
            m_editedOrder = OrderType::None;
            m_lastKeyPressed = Input::KeyboardKey::None;
            Engine.Input().lastPressedKey = Input::KeyboardKey::None;
        }
        if (ImGui::Button("Save Order Key Binding"))
        {
            SaveOrderShortCuts();
        }
    }
    if (ImGui::Button("Load Key Binding"))
    {
        Engine.InputWrapper().LoadActionBinds(Engine.InputWrapper().pathToActionBinds);
        ResetOrderShortCuts();
    }
    ImGui::SameLine(150);
    if (ImGui::Button("Close"))
    {
        m_openBindingWindow = false;
    }
    ImGui::End();
}

void lvle::LevelEditor::ResetOrderShortCuts()
{
    for (const auto& item : magic_enum::enum_values<OrderType>())
    {
        if (m_orderShortCuts.find(item) == m_orderShortCuts.end())
            m_orderShortCuts.insert(std::pair<OrderType, bee::Input::KeyboardKey>(
                item, Engine.InputWrapper().GetDigitalActionKey(std::string(magic_enum::enum_name(item)))));
        else
            m_orderShortCuts[item] = Engine.InputWrapper().GetDigitalActionKey(std::string(magic_enum::enum_name(item)));
    }
}

void lvle::LevelEditor::SaveOrderShortCuts()
{
    for (const auto& item : magic_enum::enum_values<OrderType>())
    {
        Engine.InputWrapper().RemapeDigitalAction(std::string(magic_enum::enum_name(item)), static_cast<int>(m_orderShortCuts[item]));
    }
    Engine.InputWrapper().SaveActionBinds(Engine.InputWrapper().pathToActionBinds);
}

bool lvle::LevelEditor::DisplayDropDown(const std::string& name, const unordered_map<size_t, string>& list,
                                        size_t& selectedItem)
{
    bool newSelect = false;
    const string tag = "##" + name;
    if (ImGui::BeginCombo(tag.c_str(), name.c_str()))
    {
        for (auto& item : list)
        {
            const bool isSelected = (selectedItem == item.first);
            if (ImGui::Selectable(item.second.c_str(), isSelected))
            {
                newSelect = true;
                selectedItem = item.first;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    return newSelect;
}

bool lvle::LevelEditor::DisplayDropDown(const std::string& name, const std::vector<std::string>& list,
                                        std::string& selectedItem)
{
    bool newSelect = false;
    const string tag = "##" + name;
    if (ImGui::BeginCombo(tag.c_str(), name.c_str()))
    {
        for (auto& item : list)
        {
            const bool isSelected = item == selectedItem;
            if (ImGui::Selectable(item.c_str(), isSelected))
            {
                newSelect = true;
                selectedItem = item;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    return newSelect;
}

bool lvle::LevelEditor::DisplayDropDown(const std::string& name, const std::vector<int>& list, size_t& selectedItem)
{
    bool newSelect = false;
    const string tag = "##" + name;
    if (ImGui::BeginCombo(tag.c_str(), name.c_str()))
    {
        for (auto& item : list)
        {
            const bool isSelected = (item == selectedItem);
            if (ImGui::Selectable(to_string(item).c_str(), isSelected))
            {
                newSelect = true;
                selectedItem = item;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    return newSelect;
}

template <typename ActorTemplate>
void lvle::LevelEditor::PrepareActorMaterialPaths(ActorTemplate& actorTemplate)
{
    bee::RemoveSubstring(actorTemplate.modelPath, "assets/");
    actorTemplate.model = Engine.Resources().Load<Model>(actorTemplate.modelPath);
    actorTemplate.materials.clear();
    actorTemplate.materialPaths.clear();
    actorTemplate.materialPaths.resize(actorTemplate.model->GetMeshes().size());
    actorTemplate.materials.resize(actorTemplate.model->GetMeshes().size());

    for (int i = 0; i < actorTemplate.materialPaths.size(); i++)
    {
        actorTemplate.materialPaths[i] = "materials/Empty.pepimat";
        actorTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(actorTemplate.materialPaths[i]);
    }
}

template <typename EnumClass>
bool lvle::LevelEditor::DisplayEnumDropDown(EnumClass& selectedItem)
{
    bool newSelect = false;
    const string name = magic_enum::enum_name(selectedItem).data();
    const string tag = "##" + name;
    if (ImGui::BeginCombo(tag.c_str(), name.c_str()))
    {
        for (int i = 0; i < magic_enum::enum_count<EnumClass>(); i++)
        {
            EnumClass item = static_cast<EnumClass>(i);
            const bool isSelected = (item == selectedItem);
            const string itemName = magic_enum::enum_name(item).data();
            if (ImGui::Selectable(itemName.c_str(), isSelected))
            {
                newSelect = true;
                selectedItem = item;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    return newSelect;
}

template <typename T>
void lvle::LevelEditor::DisplayBrushOptions(std::unique_ptr<T>& brush, BrushType brushType, std::string typeName)
{
    // brush options
    static_assert(std::is_base_of<Brush, T>::value, "T must be derived from Brush");

    const string name = "Apply " + typeName;
    ImGui::SetWindowFontScale(1.1f);
    ImGui::Text(name.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SetWindowFontScale(0.5f);
    ImGui::Text("");
    ImGui::SetWindowFontScale(1.0f);
    for (int i = 0; i < brush->GetAllModes().size(); i++)
    {
        if (ImGui::RadioButton(brush->GetAllModes()[i].c_str(), &m_brushModes[static_cast<int>(brushType)], i))
        {
            for (auto& item : m_brushModes) item = -1;
            m_brushModes[static_cast<int>(brushType)] = i;
            m_selectedBrush = brushType;

            auto& selectionSystem = Engine.ECS().GetSystem<SelectionSystem>();
            selectionSystem.Deactivate();
        }
        // ImGui::SameLine();
    }
    // ImGui::NewLine();
    //
    ImGui::Separator();
    ImGui::SetWindowFontScale(0.6f);
    ImGui::Text("");
    ImGui::SetWindowFontScale(1.0f);
}