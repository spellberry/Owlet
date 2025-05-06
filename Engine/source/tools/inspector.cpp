#include "actors/selection_system.hpp"
#include "level_editor/terrain_system.hpp"
#include "material_system/material_system.hpp"
#include "particle_system/particle_emitter.hpp"
#include "particle_system/particle_system.hpp"
#include "platform/pc/core/device_pc.hpp"
#include "tools/asset_explorer_system.hpp"

#if defined(BEE_INSPECTOR) && defined(BEE_PLATFORM_PC)
#include <imgui/IconsFontAwesome.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/implot.h>

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui_curve.hpp>

#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/input.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "imgui/imgui_stdlib.h"
#include "platform/dx12/DeviceManager.hpp"
#include "platform/dx12/RenderPipeline.hpp"
#include "rendering/debug_render.hpp"
#include "rendering/render_components.hpp"
#include "tools/3d_utility_functions.hpp"
#include "tools/inspector.hpp"
#include "tools/log.hpp"
#include "tools/profiler.hpp"
#include "tools/tools.hpp"

#define SHOW_IMGUI TRUE

using namespace bee;
using namespace std;

void SetStyleDark();
void SetStyleLight();

Inspector::Inspector()
{
    if (!Engine.IsHeadless())
    {
        ImGui::CreateContext();
        ImPlot::CreateContext();

        //  ImGui_Impl_Init();
        ImGui_ImplGlfw_InitForOther(Engine.Device().GetWindow(), true);

        ImGuiIO& io = ImGui::GetIO();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        const std::string iniPath = Engine.FileIO().GetPath(FileIO::Directory::Save, "imgui.ini");
        const char* constStr = iniPath.c_str();
        char* str = new char[iniPath.size() + 1];
        strcpy_s(str, iniPath.size() + 1, constStr);
        io.IniFilename = str;

        const float UIScale = Engine.Device().GetMonitorUIScale();
        const float fontSize = 14.0f;
        const float iconSize = 12.0f;

        ImFontConfig config;
        config.OversampleH = 8;
        config.OversampleV = 8;
        io.Fonts->AddFontFromFileTTF(Engine.FileIO().GetPath(FileIO::Directory::Asset, "/fonts/DroidSans.ttf").c_str(),
                                     fontSize * UIScale, &config);
        static const ImWchar icons_ranges[] = {0xf000, 0xf3ff, 0};  // will not be copied by AddFont* so keep in scope.
        config.MergeMode = true;
        config.OversampleH = 8;
        config.OversampleV = 8;

        string fontpath = Engine.FileIO().GetPath(FileIO::Directory::Asset, "/fonts/FontAwesome5FreeSolid900.otf");
        io.Fonts->AddFontFromFileTTF(fontpath.c_str(), iconSize * UIScale, &config, icons_ranges);

        // SetStyleLight();
        SetStyleDark();
        m_openWindows["Configuration"] = false;

        InitFromFile();
    }
}

Inspector::~Inspector()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SaveToFile();
}

void bee::Inspector::SaveToFile()
{
    nlohmann::json j;
    j["Inspector"] = m_openWindows;

    std::ofstream ofs;
    auto filename = Engine.FileIO().GetPath(FileIO::Directory::Save, "inspector.json");
    ofs.open(filename);
    if (ofs.is_open())
    {
        auto str = j.dump(4);
        ofs << str;
        ofs.close();
    }
}

void bee::Inspector::InitFromFile()
{
    std::ifstream ifs;
    auto filename = Engine.FileIO().GetPath(FileIO::Directory::Save, "inspector.json");
    ifs.open(filename);
    if (ifs.is_open())
    {
        nlohmann::json j = nlohmann::json::parse(ifs);
        m_openWindows = j["Inspector"];
    }
}

glm::vec2 WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                        const glm::ivec2& screenSize)
{
    // Transform world coordinates to clip space coordinates
    glm::vec4 clipSpacePos = projectionMatrix * viewMatrix * glm::vec4(worldPos, 1.0);

    // Perform perspective divide to transform to normalized device coordinates (NDC)
    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

    // Transform NDC to screen coordinates
    glm::vec2 screenPos = (glm::vec2(ndcSpacePos) + glm::vec2(1.0f)) * 0.5f * glm::vec2(screenSize);

    // Flip y coordinate (if necessary, depending on the coordinate system)
    screenPos.y = screenSize.y - screenPos.y;

    return screenPos;
}

void Inspector::Inspect(float dt)
{
#ifdef BEE_INSPECTOR

#if defined(BEE_PLATFORM_PC)
    if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::RightShift) &&
            Engine.Input().GetKeyboardKey(Input::KeyboardKey::I) ||
        Engine.Input().GetKeyboardKey(Input::KeyboardKey::RightShift) &&
            Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::I))
        m_visible = !m_visible;
#elif defined(BEE_PLATFORM_SWITCH)
    m_visible = Engine.Input().GetMouse().IsValid;
#endif

    if (!m_visible) return;

    // ImGui_Impl_NewFrame();

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    m_inInspectCall = true;

    const auto systems = Engine.ECS().GetSystems<System>();

    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |=
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole,
    // so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    static bool truethat = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", &truethat, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen) ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiID dockspace_id = ImGui::GetID("DockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {
                                                             8.0f,
                                                             7.0f,
                                                         });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {
                                                           8.0f,
                                                           7.0f,
                                                       });
        ImGui::BeginMainMenuBar();

        if (ImGui::BeginMenu("File"))
        {
            /*
            if (ImGui::MenuItem("Quit", nullptr, nullptr))
            {
                // TODO: Engine.Device().CloseApplication();
            }

            ImGui::MenuItem("Configuration", nullptr, &m_openWindows["Configuration"]);
            */

#if SHOW_IMGUI
#endif
            ImGui::MenuItem("ImGui Test", nullptr, &m_showImguiTest);

            if (ImGui::MenuItem("Close Inspector", nullptr, nullptr))
            {
                m_visible = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Managers"))
        {
            // ImGui::MenuItem("Device", nullptr, &m_openWindows["Device"]);
            // ImGui::MenuItem("Account", nullptr, &m_openWindows["Account"]);
            // ImGui::MenuItem("Resource Manager", nullptr, &m_openWindows["Resource Manager"]);

            ImGui::MenuItem("Profiler", nullptr, &m_openWindows["Profiler"]);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Systems"))
        {
            for (const auto s : systems)
            {
                auto title = s->Title;
                if (!title.empty())
                {
                    ImGui::MenuItem(title.c_str(), nullptr, &m_openWindows[s->Title]);
                }
                else
                {
                    title = typeid(*s).name();
                    title = StringReplace(title, "class ", "");
                    title = "[" + title + "]";
                    ImGui::MenuItem(title.c_str(), nullptr, nullptr);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug Render"))
        {
            auto debugRenderFlags = Engine.DebugRenderer().GetCategoryFlags();
            bool changed = false;
            changed |= ImGui::CheckboxFlags("General", &debugRenderFlags, DebugCategory::General);
            changed |= ImGui::CheckboxFlags("Gameplay", &debugRenderFlags, DebugCategory::Gameplay);
            changed |= ImGui::CheckboxFlags("Physics", &debugRenderFlags, DebugCategory::Physics);
            changed |= ImGui::CheckboxFlags("AI Navigation", &debugRenderFlags, DebugCategory::AINavigation);
            changed |= ImGui::CheckboxFlags("AI Decision Making", &debugRenderFlags, DebugCategory::AIDecision);
            changed |= ImGui::CheckboxFlags("Sound", &debugRenderFlags, DebugCategory::Sound);
            changed |= ImGui::CheckboxFlags("Rendering", &debugRenderFlags, DebugCategory::Rendering);
            changed |= ImGui::CheckboxFlags("Editor", &debugRenderFlags, DebugCategory::Editor);
            changed |= ImGui::CheckboxFlags("Acceleration Struct", &debugRenderFlags, DebugCategory::AccelStructs);
            if (changed)
            {
                Engine.DebugRenderer().SetCategoryFlags(debugRenderFlags);
                // SaveToFile();
            }
            ImGui::EndMenu();
        }

        ImGui::SameLine();
        const auto frameTime = dt * 1000.0f;
        ImGui::Text("        Frame Time: %2.2fms", frameTime);
        // ImGui::Text("        Entities: %d", Engine.ECS().Registry.size());
        ImGui::EndMainMenuBar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    }

    // if (m_openWindows["Profiler"]) Engine.Profiler().Inspect(); TODO: Make profiler

    // Actual game
    // if (InspectorColorbuffer != 0)
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        ImGui::Begin(u8"\U0000f11b Game", &truethat, ImGuiWindowFlags_NoScrollbar);

        float width = ImGui::GetWindowWidth();
        float height = ImGui::GetWindowHeight();

        m_gameSize = glm::vec2(width, height);

        m_gamePos = glm::vec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);

        auto screenAspectRatio = (float)9 / 16;
        //      const auto lm = static_cast<unsigned long long>(InspectorColorbuffer);
        //   const ImTextureID id = reinterpret_cast<void*>(lm);
        m_gameFocused = false;
        if (ImGui::IsWindowFocused()) m_gameFocused = true;
        //    auto screenAspectRatio = (float)Engine.Device().GetHeight() / (float)Engine.Device().GetWidth();
        const auto lm = static_cast<unsigned long long>(InspectorColorbuffer);
        const ImTextureID id = reinterpret_cast<void*>(lm);
        if (height / width < screenAspectRatio)
            width = height * 1.0f / screenAspectRatio;
        else
            height = width * screenAspectRatio;
        m_gameSize = glm::vec2(width, height);
        ImGui::SetCursorPos(ImVec2(0.0f, ImGui::GetFrameHeight()));
        m_gamePos = glm::vec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + ImGui::GetFrameHeight());
        // ImGui::Image(id, ImVec2(width, height), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        DeviceManager* device_manager = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager();

        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStartHandle(
            device_manager->m_ImGui_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        auto rtvDescriptorSize =
            device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        gpuStartHandle.Offset(rtvDescriptorSize * 1);

        ImGui::Image((ImTextureID)gpuStartHandle.ptr, ImVec2(width, height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        auto view = Engine.ECS().Registry.view<Transform, Camera>();
        auto& input = Engine.Input();
        glm::mat4 proj;
        glm::mat4 viewM;

        auto cameras = Engine.ECS().Registry.view<Transform, Camera>();
        for (auto e : cameras)
        {
            const auto& camera = cameras.get<Camera>(e);
            const auto& cameraTransform = cameras.get<Transform>(e);

            viewM = inverse(cameraTransform.WorldMatrix);
            proj = camera.Projection;
        }

        for (int i = 0; i < Engine.DebugRenderer().m_line_array.size(); i++)
        {
            glm::vec2 p0 = WorldToScreen(Engine.DebugRenderer().m_line_array[i].p1, viewM, proj, glm::ivec2(width, height));

            glm::vec2 p1 = WorldToScreen(Engine.DebugRenderer().m_line_array[i].p2, viewM, proj, glm::ivec2(width, height));
            if (!Engine.DebugRenderer().m_line_array[i].is2D)
            {
                p0 = WorldToScreen(Engine.DebugRenderer().m_line_array[i].p1, viewM, proj, glm::ivec2(width, height));

                p1 = WorldToScreen(Engine.DebugRenderer().m_line_array[i].p2, viewM, proj, glm::ivec2(width, height));
            }
            else
            {
                p0 = Engine.DebugRenderer().m_line_array[i].p1;

                p1 = Engine.DebugRenderer().m_line_array[i].p2;
            }

            glm::vec4 color = Engine.DebugRenderer().m_line_array[i].color;
            draw_list->AddLine(ImVec2(p0.x + m_gamePos.x, p0.y + m_gamePos.y), ImVec2(p1.x + m_gamePos.x, p1.y + m_gamePos.y),
                               IM_COL32(color.r * 255, color.g * 255, color.b * 255, color.a * 255), 1.0f);
        }
        Engine.DebugRenderer().m_line_array.clear();

        auto btnColor = colors[ImGuiCol_Button];
        btnColor.w *= 0.4f;
        const float UIScale = 1.0f;  // Game.Device().GetMonitorUIScale();
        const auto s = ImGui::GetIO().FontGlobalScale * UIScale;
        const ImVec2 btnSize(24.0f * s, 24.0f * s);
        ImGui::SetCursorPos(ImVec2(6.0f, 6.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, btnColor);

        ImGui::PopStyleColor();

        if (m_insideEditor && data.enabledGuizmo)
        {
            ImGuiIO& io = ImGui::GetIO();
            static ImGuiWindowFlags gizmoWindowFlags = 0;
            {
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, width, height);
                gizmoWindowFlags =
                    ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(
                                                    /*window->InnerRect.Min, window->InnerRect.Max*/ ImVec2(
                                                        ImGui::GetWindowPos().x, ImGui::GetWindowPos().y),
                                                    ImVec2(width + ImGui::GetWindowPos().x, height + ImGui::GetWindowPos().y))
                        ? ImGuiWindowFlags_NoMove
                        : 0;
            }

            /*ImGuizmo::DrawGrid(bee::Engine.Inspector().data.cameraView, bee::Engine.Inspector().data.cameraProjection,
                               bee::Engine.Inspector().data.identityMatrix, 100.f);*/
            /*ImGuizmo::DrawCubes(bee::Engine.Inspector().data.cameraView, bee::Engine.Inspector().data. cameraProjection,
                                &bee::Engine.Inspector().data.objectMatrix[0][0], bee::Engine.Inspector().data.gizmoCount);*/
            auto view = Engine.ECS().Registry.view<Selected, Transform>();
            static Entity lastEntity = entt::null;
            for (auto [entity, selectedObject, transform] : view.each())
            {
                if (lastEntity != entity) data.freeMovement = true;
                data.floatProjection = (float*)glm::value_ptr(proj);
                data.floatViewM = (float*)glm::value_ptr(viewM);
                data.floatTransformM = (float*)glm::value_ptr(transform.WorldMatrix);

                auto guizmoSnap = data.useSnap ? &data.snap[0] : NULL;
                auto guizmoBounds = data.boundSizing ? data.bounds : NULL;
                auto guizmoSnapingBounds = data.boundSizingSnap ? data.boundsSnap : NULL;

                ImGuizmo::Manipulate(data.floatViewM, data.floatProjection, bee::Engine.Inspector().data.mCurrentGizmoOperation,
                                     bee::Engine.Inspector().data.mCurrentGizmoMode, data.floatTransformM, NULL, guizmoSnap,
                                     guizmoBounds, guizmoSnapingBounds);

                float scale[3], translate[3], rotate[3];
                ImGuizmo::DecomposeMatrixToComponents(data.floatTransformM, translate, rotate, scale);

                if (!data.freeMovement) lvle::GetTerrainHeightAtPoint(translate[0], translate[1], translate[2]);

                transform.Translation = glm::vec3(translate[0], translate[1], translate[2]);
                transform.Scale = glm::vec3(scale[0], scale[1], scale[2]);
                transform.Rotation = glm::quat(glm::vec3(rotate[0], rotate[1], rotate[2]) * glm::pi<float>() / 180.0f);

                lastEntity = entity;

                break;
            }
            /*ImGuizmo::ViewManipulate(bee::Engine.Inspector().data.cameraView, bee::Engine.Inspector().data.camDistance,
                                     ImVec2(viewManipulateRight - 128, viewManipulateTop),
                                     ImVec2(128, 128), 0x10101010);*/
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
    }

    if (m_showImguiTest)
    {
        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();
    }

    ImGui::Begin("Scene");
    // static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    std::set<Entity> inspected;

    Engine.ECS().Registry.view<Transform>().each(
        [this, &inspected](auto entity, Transform& transform)
        {
            if (!transform.HasParent() && !Engine.ECS().Registry.all_of<ParticleComponent>(entity))
                Inspect(entity, transform, inspected);
        });
    ImGui::End();

    ImGui::Begin("Details");
    if (Engine.ECS().Registry.valid(m_selectedEntity))
    {
        ImGui::LabelText("ID:", "%s", to_string(static_cast<int>(m_selectedEntity)).c_str());

        if (ImGui::Button("Delete")) Engine.ECS().DeleteEntity(m_selectedEntity);

        ImGui::PushID((void*)m_selectedEntity);

        if (Engine.ECS().Registry.try_get<Transform>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Transform& t = Engine.ECS().Registry.get<Transform>(m_selectedEntity);
                ImGui::DragFloat3("Position", value_ptr(t.Translation), 0.01f);
                ImGui::DragFloat3("Scale", value_ptr(t.Scale), 0.01f);
                ImGui::DragFloat4("Rotation", value_ptr(t.Rotation), 0.01f);
            }
        }

        if (Engine.ECS().Registry.try_get<Light>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Light& light = Engine.ECS().Registry.get<Light>(m_selectedEntity);
                ImGui::ColorEdit3("Color", &light.Color.x);
                ImGui::DragFloat("Intensity", &light.Intensity, 0.1f);
                ImGui::DragFloat("Range", &light.Range, 0.1f);

                ImGui::Checkbox("Has Shadows", &light.CastShadows);
                if (light.CastShadows) ImGui::DragFloat("Shadow Size", &light.ShadowExtent, 0.1f, 0.0f);

                auto selectedLight = Engine.ECS().Registry.try_get<Selected>(m_selectedEntity);
                if (selectedLight == nullptr && m_insideEditor)
                {
                    Engine.ECS().CreateComponent<Selected>(m_selectedEntity);
                }
            }
        }

        if (Engine.ECS().Registry.try_get<MeshRenderer>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                MeshRenderer& meshRenderer = Engine.ECS().Registry.get<MeshRenderer>(m_selectedEntity);

                static std::string materialPath;
                const std::string indexText = std::to_string(meshRenderer.Material->material_index);
                ImGui::Text(indexText.c_str());
                ImGui::InputText("Material", &materialPath);
                std::filesystem::path path;
                if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".pepimat"}))
                {
                    materialPath = path.string();
                    RemoveSubstring(materialPath, "assets/");
                    // Engine.ECS().GetSystem<MaterialSystem>().LoadMaterial(*meshRenderer.Material,materialPath);
                    meshRenderer.Material = Engine.Resources().Load<Material>(materialPath);
                }
            }
        }

        if (Engine.ECS().Registry.try_get<ParticleEmitter>(m_selectedEntity))
        {
            static bool popUpFlag = false;
            if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ParticleEmitter& emitter = Engine.ECS().Registry.get<ParticleEmitter>(m_selectedEntity);
                if (ImGui::Button("Save as template"))
                {
                    ImGui::OpenPopup("Template name");
                }
                if (ImGui::BeginPopupModal("Template name"))
                {
                    static std::string templateName = "New Emitter";
                    ImGui::InputText("Template name", &templateName);
                    if (ImGui::Button("Save"))
                    {
                        Engine.ECS().GetSystem<ParticleSystem>().SaveEmitterAsTemplate(emitter, templateName);
                        templateName = "New Emitter";
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::Button("Load from template")) popUpFlag = true;

                if (popUpFlag)
                {
                    ImGui::Begin("Emitter template", &popUpFlag);
                    // Draw popup contents.
                    static std::string templatePath = "";
                    ImGui::InputText("Template", &templatePath);
                    std::filesystem::path path;
                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".pepitter"}))
                    {
                        templatePath = path.string();
                        bee::RemoveSubstring(templatePath, "assets/");
                    }
                    if (ImGui::Button("Load"))
                    {
                        Engine.ECS().GetSystem<ParticleSystem>().LoadEmitterFromTemplate(emitter, templatePath);
                        templatePath = "";
                        popUpFlag = false;
                    }
                    ImGui::End();
                }

                ImGui::Separator();
                ImGui::Text("Rendering");

                const char* items[] = {"Billboard", "Mesh"};
                int currentItem = static_cast<int>(emitter.particleProps->type);
                if (ImGui::Combo("Particle Type", &currentItem, items, IM_ARRAYSIZE(items)))
                {
                    emitter.particleProps->type = static_cast<ParticleType>(currentItem);

                    if (emitter.particleProps->type == ParticleType::Billboard)
                    {
                        emitter.m_particleModel = Engine.Resources().Load<Model>("models/PlaneWithT.glb");
                        emitter.m_particleModelPath = "models/PlaneWithT.glb";
                        // emitter.m_particleModel->SetAllMaterials(emitter.m_particleMaterial);
                    }

                    if (emitter.particleProps->type == ParticleType::Mesh)
                    {
                        emitter.m_particleModel = Engine.Resources().Load<Model>("models/rock.gltf");
                        emitter.m_particleModelPath = "models/rock.gltf";
                        // emitter.m_particleModel->SetAllMaterials(emitter.m_particleMaterial);
                    }
                }

                if (emitter.particleProps->type == ParticleType::Mesh)
                {
                    ImGui::InputText("Model", &emitter.m_particleModelPath);
                    std::filesystem::path path;
                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".gltf", ".glb"}))
                    {
                        emitter.m_particleModelPath = path.string();
                        bee::RemoveSubstring(emitter.m_particleModelPath, "assets/");
                        emitter.m_particleModel = Engine.Resources().Load<Model>(emitter.m_particleModelPath);
                        // emitter.m_particleModel->SetAllMaterials(emitter.m_particleMaterial);
                    }
                }

                {
                    const std::string indexText = std::to_string(emitter.m_particleMaterial->material_index);
                    ImGui::Text(indexText.c_str());
                    ImGui::InputText("Material", &emitter.m_materialPath);
                    std::filesystem::path path;
                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".pepimat"}))
                    {
                        emitter.m_materialPath = path.string();
                        RemoveSubstring(emitter.m_materialPath, "assets/");
                        // Engine.ECS().GetSystem<MaterialSystem>().LoadMaterial(*emitter.m_particleMaterial,emitter.m_materialPath);
                        emitter.m_particleMaterial = Engine.Resources().Load<Material>(emitter.m_materialPath);
                        // emitter.m_particleModel->SetAllMaterials(emitter.m_particleMaterial);
                    }
                }

                ImGui::Separator();
                ImGui::Text("Color");
                ImGui::ColorEdit4("Color begin", &emitter.particleProps->colorBegin.x);
                ImGui::ColorEdit4("Color end", &emitter.particleProps->colorEnd.x);

                static int selectionIdx = -1;
                static ImVec2 rangemin(0, 0);
                static ImVec2 rangemax(1, 1);
                ImGui::Separator();
                ImGui::Text("Size");
                ImVec2 graphSize = ImGui::GetContentRegionAvail() * 0.6725;
                graphSize.y += 100;
                ImGui::Curve("Size graph", graphSize, 10, emitter.particleProps->sizeCurvePoints, &selectionIdx, rangemin,
                             rangemax);
                ImGui::DragFloat3("Size", &emitter.particleProps->sizeBegin.x, 0.5f);
                ImGui::DragFloat3("Size variation", &emitter.particleProps->sizeVariation.x, 0.5f);
                ImGui::Separator();
                ImGui::Text("Simulation");
                ImGui::DragFloat("Lifetime", &emitter.particleProps->lifeTime, 0.5f);
                ImGui::DragFloat3("Velocity", &emitter.particleProps->velocity.x, 0.5f);
                ImGui::DragFloat3("Velocity variation", &emitter.particleProps->velocityVariation.x, 0.5f);
                ImGui::Checkbox("Use gravity", &emitter.particleProps->hasGravity);
                if (emitter.particleProps->hasGravity)
                {
                    ImGui::SameLine();
                    ImGui::DragFloat("Gravity", &emitter.particleProps->gravity, 0.25f);
                }
                ImGui::Separator();
                ImGui::Text("Emitter control");
                ImGui::Checkbox("Loop", &emitter.m_isLoop);
                ImGui::DragInt("EmitAmount", &emitter.m_EmitAmount, 1);
                ImGui::DragFloat("Emit Rate", &emitter.m_emitRate, 0.1f);
                if (ImGui::Button("Emit"))
                {
                    emitter.Emit(emitter.m_EmitAmount);
                }
            }
        }

        for (const auto s : systems)
        {
            auto m = s;
            m->Inspect(m_selectedEntity);
        }

        ImGui::PopID();
    }

    ImGui::End();

    // ImGui::PushItemWidth(50.0f);
    for (const auto s : systems)
    {
        auto title = s->Title;
        if (!title.empty() && m_openWindows[title]) s->Inspect();
    }
    // ImGui::PopItemWidth();

    if (m_openWindows["Profiler"]) Engine.Profiler().Inspect();

    ImGui::End();  // there is no start for this

    // ImGui::Render();
    // ImGui_Impl_RenderDrawData(ImGui::GetDrawData());

    DeviceManager* device_manager = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager();

    // CD3DX12_GPU_DESCRIPTOR_HANDLE
    // gpuStartHandle(device_manager->m_ImGui_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    // auto rtvDescriptorSize =
    //     device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // gpuStartHandle.Offset(rtvDescriptorSize);

    // ImGui::Begin("Viewport");

    ImGuiWindowFlags window_flags1 = 0;  // Start with no flags

    //  ImGui::Begin("Viewport", nullptr, window_flags1);

    //   m_viewport_pos = ImGui::GetWindowPos();
    //  ImGui::GetWindowSize();
    // m_viewport_size = ImGui::GetWindowSize();

    //   ImGui::Image((ImTextureID)gpuStartHandle.ptr, ImVec2(m_device_manager->m_window->GetWidth(),
    //   m_device_manager->m_window->GetHeight()));
    //    ImGui::Image((ImTextureID)gpuStartHandle.ptr, ImVec2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y));
    //  ImGui::End();

    ID3D12DescriptorHeap* imguiDescriptorHeaps[] = {device_manager->m_ImGui_DescriptorHeap.Get()};
    device_manager->GetCommandList()->SetDescriptorHeaps(_countof(imguiDescriptorHeaps), imguiDescriptorHeaps);

    m_inInspectCall = false;

    ImGui::Render();

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), device_manager->GetCommandList().Get());

    device_manager->EndFrame();

#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////
////										Helper stuff
////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
void AddToInspected(Entity entity, std::set<Entity>& inspected)
{
    inspected.insert(entity);
    if (auto transform = Engine.ECS().Registry.try_get<Transform>(entity))
    {
        for (auto child : *transform)
        {
            AddToInspected(child, inspected);
        }
    }
}

}  // namespace

void Inspector::Inspect(Entity entity, Transform& transform, std::set<Entity>& inspected)
{
    if (inspected.find(entity) != inspected.end()) return;
    inspected.insert(entity);

    string name = transform.Name.empty() ? "Entity-" + std::to_string(static_cast<std::uint32_t>(entity)) : transform.Name;

    static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (transform.HasChildern())
    {
        const bool nodeOpen =
            ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<long long>(entity)),  // NOLINT(performance-no-int-to-ptr)
                              nodeFlags, "%s", name.c_str());
        if (ImGui::IsItemClicked())
        {
            if (m_insideEditor && !transform.HasParent())
            {
                auto view = bee::Engine.ECS().Registry.view<Selected>();
                bee::Engine.ECS().Registry.remove<Selected>(view.begin(), view.end());
                bee::Engine.ECS().Registry.emplace<Selected>(entity);
            }
            m_selectedEntity = entity;
        }

        if (nodeOpen)
        {
            for (auto child : transform)
            {
                if (Engine.ECS().Registry.valid(child))
                {
                    auto& childTransform = Engine.ECS().Registry.get<Transform>(child);
                    Inspect(child, childTransform, inspected);
                }
            }
            ImGui::TreePop();
        }
        else
        {
            for (auto child : transform)
            {
                AddToInspected(child, inspected);
            }
        }
    }
    else
    {
        ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<long long>(entity)),  // NOLINT(performance-no-int-to-ptr)
                          nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s", name.c_str());
        if (ImGui::IsItemClicked()) m_selectedEntity = entity;
    }
}

void Inspector::Inspect(const char* name, float& f) { ImGui::DragFloat(name, &f, 0.01f); }

void Inspector::Inspect(const char* name, int& i) { ImGui::DragInt(name, &i); }

void Inspector::Inspect(const char* name, bool& b) { ImGui::Checkbox(name, &b); }

void Inspector::Inspect(const char* name, glm::vec2& v) { ImGui::DragFloat2(name, glm::value_ptr(v)); }

void Inspector::Inspect(const char* name, glm::vec3& v)
{
    if (StringEndsWith(string(name), "Color") || StringEndsWith(string(name), "color"))
    {
        ImGui::ColorEdit3(name, glm::value_ptr(v), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
    }
    else
    {
        ImGui::DragFloat3(name, glm::value_ptr(v));
    }
}

void Inspector::Inspect(const char* name, glm::vec4& v)
{
    if (StringEndsWith(string(name), "Color") || StringEndsWith(string(name), "color"))
    {
        ImGui::ColorEdit4(name, glm::value_ptr(v), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
    }
    else
    {
        ImGui::DragFloat4(name, glm::value_ptr(v));
    }
}

void Inspector::Inspect(std::string name, std::vector<std::string>& items) const
{
    static int selectedIndex = -1;
    static std::string newItem;
    static std::string editBuffer;

    ImGui::Text(name.c_str());
    // Show add item input box
    if (ImGui::Button(("Add to list##" + name).c_str()))
    {
        if (!newItem.empty())
        {
            items.push_back(newItem);
            newItem.clear();
        }
    }
    ImGui::SameLine();
    ImGui::InputText((std::string("Add Item") + "##" + name).c_str(), &newItem);

    // Show list view
    if (!items.empty())
    {
        for (int i = 0; i < items.size(); ++i)
        {
            ImGui::PushID(i);

            // Show item text
            ImGui::Text("%s", items[i].c_str());

            // Show edit button
            ImGui::SameLine();
            if (ImGui::Button(("Edit##edit" + name).c_str()))
            {
                selectedIndex = i;
                editBuffer = items[i];
            }

            // Show delete button
            ImGui::SameLine();
            if (ImGui::Button(("Delete##delete" + name).c_str()))
            {
                items.erase(items.begin() + i);
                if (selectedIndex == i)
                {
                    selectedIndex = -1;
                }
                ImGui::PopID();
                break;  // Break loop after deleting an item
            }

            ImGui::PopID();
        }

        // Show edit input box for selected item
        if (selectedIndex != -1)
        {
            ImGui::Text("Edit item");
            std::string& selected_item = items[selectedIndex];
            if (ImGui::InputText(("##Edit Item" + name).c_str(), &editBuffer, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (!editBuffer.empty())
                {
                    selected_item = editBuffer;
                    selectedIndex = -1;
                    editBuffer.clear();
                }
            }
        }
    }
}

void Inspector::Inspect(std::string name, std::vector<int>& items) const
{
    static int selectedIndex = -1;
    static int newItem = 0;

    ImGui::Text(name.c_str());
    // Show add item input box
    if (ImGui::Button(("Add to list##" + name).c_str()))
    {
        items.push_back(newItem);
    }
    ImGui::SameLine();
    ImGui::InputInt((std::string("Add Item") + "##" + name).c_str(), &newItem);

    // Show list view
    if (!items.empty())
    {
        for (int i = 0; i < items.size(); ++i)
        {
            ImGui::PushID(i);

            // Show item text
            ImGui::Text("%i", items[i]);

            // Show edit button
            ImGui::SameLine();
            if (ImGui::Button(("Edit##edit" + name).c_str()))
            {
                selectedIndex = i;
            }

            // Show delete button
            ImGui::SameLine();
            if (ImGui::Button(("Delete##delete" + name).c_str()))
            {
                items.erase(items.begin() + i);
                if (selectedIndex == i)
                {
                    selectedIndex = -1;
                }
                ImGui::PopID();
                break;  // Break loop after deleting an item
            }

            ImGui::PopID();
        }

        // Show edit input box for selected item
        if (selectedIndex != -1)
        {
            ImGui::Text("Edit item");
            int& selected_item = items[selectedIndex];
            ImGui::InputInt(("##Edit Item" + name).c_str(), &selected_item, ImGuiInputTextFlags_EnterReturnsTrue);

            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), false))
            {
                selectedIndex = -1;
            }
        }
    }
}

void Inspector::Inspect(std::string name, std::vector<float>& items) const
{
    static int selectedIndex = -1;
    static float newItem = 0.0f;

    ImGui::Text(name.c_str());
    // Show add item input box
    if (ImGui::Button(("Add to list##" + name).c_str()))
    {
        items.push_back(newItem);
    }
    ImGui::SameLine();
    ImGui::InputFloat((std::string("Add Item") + "##" + name).c_str(), &newItem);

    // Show list view
    if (!items.empty())
    {
        for (int i = 0; i < items.size(); ++i)
        {
            ImGui::PushID(i);

            // Show item text
            ImGui::Text("%i", items[i]);

            // Show edit button
            ImGui::SameLine();
            if (ImGui::Button(("Edit##edit" + name).c_str()))
            {
                selectedIndex = i;
            }

            // Show delete button
            ImGui::SameLine();
            if (ImGui::Button(("Delete##delete" + name).c_str()))
            {
                items.erase(items.begin() + i);
                if (selectedIndex == i)
                {
                    selectedIndex = -1;
                }
                ImGui::PopID();
                break;  // Break loop after deleting an item
            }

            ImGui::PopID();
        }

        // Show edit input box for selected item
        if (selectedIndex != -1)
        {
            ImGui::Text("Edit item");
            float& selected_item = items[selectedIndex];
            ImGui::InputFloat(("##Edit Item" + name).c_str(), &selected_item, ImGuiInputTextFlags_EnterReturnsTrue);

            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), false))
            {
                selectedIndex = -1;
            }
        }
    }
}
void bee::Inspector::UsingTheEditor(bool usingEditor) { m_insideEditor = usingEditor; }

void SetStyleDark()
{
    // Main
    auto* style = &ImGui::GetStyle();
    style->FrameRounding = 2.0f;
    style->WindowPadding = ImVec2(4.0f, 3.0f);
    style->FramePadding = ImVec2(4.0f, 4.0f);
    style->ItemSpacing = ImVec2(4.0f, 3.0f);
    style->IndentSpacing = 12;
    style->ScrollbarSize = 12;
    style->GrabMinSize = 9;

    // Sizes
    style->WindowBorderSize = 0.0f;
    style->ChildBorderSize = 0.0f;
    style->PopupBorderSize = 0.0f;
    style->FrameBorderSize = 0.0f;
    style->TabBorderSize = 0.0f;

    style->WindowRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->PopupRounding = 0.0f;
    style->GrabRounding = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->TabRounding = 0.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.17f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.22f, 0.24f, 0.25f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.28f, 0.27f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.63f, 0.17f, 0.84f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.06f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.05f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.11f, 0.06f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.25f, 0.08f, 0.27f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.28f, 0.27f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.63f, 0.17f, 0.84f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.34f, 0.25f, 0.34f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.63f, 0.17f, 0.84f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.21f, 0.17f, 0.23f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.63f, 0.17f, 0.84f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.63f, 0.17f, 0.84f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.63f, 0.17f, 0.84f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.17f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.19f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.43f, 0.12f, 0.57f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void SetStyleLight()
{
    // Main
    auto* style = &ImGui::GetStyle();
    style->FrameRounding = 2.0f;
    style->WindowPadding = ImVec2(4.0f, 3.0f);
    style->FramePadding = ImVec2(4.0f, 4.0f);
    style->ItemSpacing = ImVec2(4.0f, 3.0f);
    style->IndentSpacing = 12;
    style->ScrollbarSize = 12;
    style->GrabMinSize = 9;

    // Sizes
    style->WindowBorderSize = 0.0f;
    style->ChildBorderSize = 0.0f;
    style->PopupBorderSize = 0.0f;
    style->FrameBorderSize = 0.0f;
    style->TabBorderSize = 0.0f;

    style->WindowRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->PopupRounding = 0.0f;
    style->GrabRounding = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->TabRounding = 0.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.89f, 0.89f, 0.89f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.01f, 0.01f, 0.01f, 0.63f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Button] = ImVec4(0.83f, 0.83f, 0.83f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Header] = ImVec4(0.67f, 0.67f, 0.67f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Separator] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.18f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 0.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

#else

#include "tools/inspector.hpp"

using namespace std;
using namespace bee;

Inspector::Inspector() {}
Inspector::~Inspector() {}
void Inspector::Inspect(float dt)
{
    m_gameSize = glm::vec2(bee::Engine.Device().GetWidth(), bee ::Engine.Device().GetHeight());
    m_gamePos = glm::vec2(0.0f, 0.0f);
}
void Inspector::Inspect(Entity entity, Transform& transform, std::set<Entity>& inspected) {}
void Inspector::Inspect(const char* name, float& f) {}
void Inspector::Inspect(const char* name, int& i) {}
void Inspector::Inspect(const char* name, bool& b) {}
void Inspector::Inspect(const char* name, glm::vec2& v) {}
void Inspector::Inspect(const char* name, glm::vec3& v) {}
void Inspector::Inspect(const char* name, glm::vec4& v) {}

#endif  // BEE_INSPECTOR
