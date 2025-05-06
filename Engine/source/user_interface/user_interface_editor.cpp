#include "user_interface/user_interface_editor.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <glm/vec2.hpp>
#include <platform/dx12/DeviceManager.hpp>
#include <platform/dx12/RenderPipeline.hpp>

#include "core/device.hpp"
#include "core/engine.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "rendering/debug_render.hpp"
#include "tools/asset_explorer_system.hpp"
#include "tools/log.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_editor_structs.hpp"
#include "user_interface/user_interface_serializer.hpp"

using namespace bee;
using namespace ui;

UIEditor::UIEditor()
{
    Title = "User interface Editor";
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    auto image = ui.LoadTexture(Engine.FileIO().GetPath(FileIO::Directory::Asset, "Textures/checkerboard.png"));

    m_defFont = ui.LoadFont(Engine.FileIO().GetPath(FileIO::Directory::Asset, "fonts/DroidSans.sff"));
    // float height = static_cast<float>(Engine.Device().GetHeight());
    // float width = static_cast<float>(Engine.Device().GetWidth());
    // ui.CreateTextureComponentFromAtlas(ui.m_selectedElementOverlay, image, glm::vec4(12, 64, 12, 64), glm::vec2(0.0f, 0.0f),
    //                                    glm::vec2(height, width), -0.2f);
    // ui.SetDrawStateUIelement(ui.m_selectedElementOverlay, false);

    m_autoSizes.resize(5);

    ui.serialiser->LoadFunctionsEditor();
}
UIEditor::~UIEditor()
{
    for (const auto& item : m_items)
    {
        delete item;
    }

    m_items.clear();
}
#ifdef BEE_INSPECTOR
void UIEditor::Inspect()
{
    ViewPort();
    Inspector();
    Lines();
}
#endif

void bee::ui::UIEditor::Inspector()
{
    auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    // main inspector
    {
        int windowCounter = 0;
        ImGui::Begin("UI Editor");
        ImVec2 cursorPos = ImGui::GetCursorPos();
        int cPosx = static_cast<int>(cursorPos.x);
        BeginChild("separate windows", windowCounter, cursorPos);
        {
            if (ImGui::Button("Font Manager"))
            {
                m_fontEditor = !m_fontEditor;
            }
            ResizeIfNeeded(windowCounter);
            if (ImGui::Button("Hover Manager"))
            {
                m_hoverEditor = !m_hoverEditor;
            }
            ResizeIfNeeded(windowCounter);
            ImGui::Checkbox("Debug Drawing", &m_debugDrawing);
        }
        EndChild(windowCounter, cursorPos);

        BeginChild("general", windowCounter, cursorPos);
        {
            if (ImGui::Button("Get Scene element"))
            {
                if (Engine.ECS().Registry.try_get<internal::UIElement>(Engine.Inspector().SelectedEntity()))
                {
                    m_curSelectedEl = Engine.Inspector().SelectedEntity();
                    auto& element = Engine.ECS().Registry.get<internal::UIElement>(m_curSelectedEl);
                    if (element.topOrBot == top)
                    {
                        m_tobs = "Top";
                    }
                    else
                    {
                        m_tobs = "Bottom";
                    }
                    if (element.leftOrRight == left)
                    {
                        m_lors = "Left";
                    }
                    else
                    {
                        m_lors = "Right";
                    }
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Load selected element in the scene window");
                ImGui::EndTooltip();
            }
            ResizeIfNeeded(windowCounter);
            if (ImGui::Button("New Element"))
            {
                m_curSelectedEl = ui.CreateUIElement(top, left);
                m_lors = "Left";
                m_tobs = "Top";
                ui.SetInputStateUIelement(m_curSelectedEl, false);
            }
            ResizeIfNeeded(windowCounter);
            ImGui::SetNextItemWidth(100.0f);

            if (ImGui::InputText("element file", &m_elementFile))
            {
                std::string name = m_elementFile;
                RemoveSubstring(name, ".json");
                RemoveSubstring(name, "assets/userinterface\\");
                RemoveSubstring(name, "assets/userinterface/");
                m_curSelectedEl = LoadElement(name, entt::null);
            }
            std::filesystem::path path;
            if (assetSystem.SetDragDropTarget(path, {".json"}))
            {
                m_elementFile = path.string();
                std::string name = m_elementFile;
                RemoveSubstring(name, ".json");
                RemoveSubstring(name, "assets/userinterface\\");
                m_curSelectedEl = LoadElement(name, entt::null);
            }
            ResizeIfNeeded(windowCounter);
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputText("into elementFile", &m_elementFile))
            {
                std::string name = m_elementFile;
                RemoveSubstring(name, ".json");
                RemoveSubstring(name, "assets/userinterface\\");
                RemoveSubstring(name, "assets/userinterface/");
                m_curSelectedEl = LoadElement(name, m_curSelectedEl);
            }
            std::filesystem::path paths;
            if (assetSystem.SetDragDropTarget(paths, {".json"}))
            {
                m_elementFile = paths.string();
                std::string name = m_elementFile;
                RemoveSubstring(name, ".json");
                RemoveSubstring(name, "assets/userinterface\\");
                m_curSelectedEl = LoadElement(name, m_curSelectedEl);
            }
            ResizeIfNeeded(windowCounter);
            if (m_curSelectedEl != entt::null)
            {
                if (ImGui::Button("Delete element"))
                {
                    ui.DeleteUIElement(m_curSelectedEl);
                    for (int i = 0; i < m_items.size(); i++)
                    {
                        if (m_items.at(i)->element == m_curSelectedEl)
                        {
                            m_items.erase(m_items.begin() + i);
                            i--;
                        }
                    }
                    m_curSelectedEl = entt::null;
                    m_selectedComp = -1;
                }
                ResizeIfNeeded(windowCounter);
                if (ImGui::Button("Reload Element"))
                {
                    UIElementID oldUI = m_curSelectedEl;

                    auto& el = Engine.ECS().Registry.get<internal::UIElement>(oldUI);
                    m_curSelectedEl = ui.CreateUIElement(el.topOrBot, el.leftOrRight);
                    auto& newel = Engine.ECS().Registry.get<internal::UIElement>(m_curSelectedEl);
                    newel.name = el.name;
                    ui.DeleteUIElement(oldUI);
                    for (int i = 0; i < m_items.size(); i++)
                    {
                        if (m_items.at(i)->element == oldUI)
                        {
                            m_items.at(i)->RemakeWBounds(m_curSelectedEl);
                        }
                    }
                }
                ResizeIfNeeded(windowCounter);
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Needed to reflect certain changes like font or alignment");
                    ImGui::EndTooltip();
                }
                if (ImGui::Button("Save Element"))
                {
                    sElement el;
                    auto& newel = Engine.ECS().Registry.get<internal::UIElement>(m_curSelectedEl);
                    el.opacity = newel.opacity;
                    if (m_lors == "Left")
                    {
                        el.lors = left;
                    }
                    else if (m_lors == "Right")
                    {
                        el.lors = right;
                    }
                    if (m_tobs == "Top")
                    {
                        el.tobs = top;
                    }
                    else if (m_tobs == "Bottom")
                    {
                        el.tobs = bottom;
                    }
                    el.name = newel.name;
                    for (auto item : m_items)
                    {
                        if (item->element == m_curSelectedEl)
                        {
                            switch (item->type)
                            {
                                case ComponentType::text:
                                {
                                    el.texts.push_back(*dynamic_cast<Text*>(item));
                                    break;
                                }
                                case ComponentType::image:
                                {
                                    el.images.push_back(*dynamic_cast<Image*>(item));
                                    break;
                                }
                                case ComponentType::button:
                                {
                                    el.buttons.push_back(*dynamic_cast<sButton*>(item));
                                    break;
                                }
                                case ComponentType::slider:
                                {
                                    el.sliders.push_back(*dynamic_cast<sSlider*>(item));
                                    break;
                                }
                                case ComponentType::progressbar:
                                {
                                    el.progressbars.push_back(*dynamic_cast<sProgressBar*>(item));
                                    break;
                                }
                            }
                        }
                    }
                    el.layer = static_cast<int>(Engine.ECS().Registry.get<Transform>(m_curSelectedEl).Translation.z * 100);
                    el.opacity = newel.opacity;
                    ui.serialiser->SaveElement(el);
                }
            }
        }
        EndChild(windowCounter, cursorPos);

        if (m_curSelectedEl != entt::null)
        {
            BeginChild("currently selected ui element", windowCounter, cursorPos);
            {
                auto& element = Engine.ECS().Registry.get<internal::UIElement>(m_curSelectedEl);
                if (ImGui::InputText("Element Name", &element.name))
                {
                    auto& trans = Engine.ECS().Registry.get<Transform>(m_curSelectedEl);
                    trans.Name = element.name;
                }
                ResizeIfNeeded(windowCounter);

                const std::vector<std::string> tobl = {"Top", "Bottom"};
                if (DisplayDropDown(m_tobs, tobl, m_tobs))
                {
                    if (m_tobs == tobl[0])
                    {
                        element.topOrBot = top;
                    }
                    if (m_tobs == tobl[1])
                    {
                        element.topOrBot = bottom;
                    }
                    UIElementID oldUI = m_curSelectedEl;

                    auto& el = Engine.ECS().Registry.get<internal::UIElement>(oldUI);
                    m_curSelectedEl = ui.CreateUIElement(el.topOrBot, el.leftOrRight);
                    auto& newel = Engine.ECS().Registry.get<internal::UIElement>(m_curSelectedEl);
                    newel.name = el.name;
                    ui.DeleteUIElement(oldUI);
                    for (int i = 0; i < m_items.size(); i++)
                    {
                        if (m_items.at(i)->element == oldUI)
                        {
                            m_items.at(i)->RemakeWBounds(m_curSelectedEl);
                        }
                    }
                }
                ResizeIfNeeded(windowCounter);

                const std::vector<std::string> lorl = {"Left", "Right"};
                if (DisplayDropDown(m_lors, lorl, m_lors))
                {
                    if (m_lors == lorl[0])
                    {
                        element.leftOrRight = left;
                    }
                    if (m_lors == lorl[1])
                    {
                        element.leftOrRight = right;
                    }
                    UIElementID oldUI = m_curSelectedEl;

                    auto& el = Engine.ECS().Registry.get<internal::UIElement>(oldUI);
                    m_curSelectedEl = ui.CreateUIElement(el.topOrBot, el.leftOrRight);
                    auto& newel = Engine.ECS().Registry.get<internal::UIElement>(m_curSelectedEl);
                    newel.name = el.name;
                    ui.DeleteUIElement(oldUI);
                    for (int i = 0; i < m_items.size(); i++)
                    {
                        if (m_items.at(i)->element == oldUI)
                        {
                            m_items.at(i)->RemakeWBounds(m_curSelectedEl);
                        }
                    }
                }
                ResizeIfNeeded(windowCounter);

                ImGui::DragFloat("opacity", &element.opacity, 0.01f, 0.0f, 1.0f);
                ResizeIfNeeded(windowCounter);
                auto& trans = Engine.ECS().Registry.get<Transform>(m_curSelectedEl);
                int layer = static_cast<int>(trans.Translation.z * 100);
                ImGui::DragInt("layer", &layer, 1, 0, 100, "%d", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
                trans.Translation.z = static_cast<float>(layer) / 100;
            }
            EndChild(windowCounter, cursorPos);

            BeginChild("components", windowCounter, cursorPos);
            {
                if (ImGui::Button("New Text"))
                {
                    m_items.push_back(new Text());
                    m_items.back()->RemakeWBounds(m_curSelectedEl);
                    m_items.back()->name.append(std::to_string(m_componentNaneIndexer));
                    m_componentNaneIndexer++;
                    m_selectedComp = (m_items.size() - 1);
                }
                ResizeIfNeeded(windowCounter);
                if (ImGui::Button("New textureComponent"))
                {
                    m_items.push_back(new Image());
                    m_items.back()->RemakeWBounds(m_curSelectedEl);
                    m_items.back()->name.append(std::to_string(m_componentNaneIndexer));
                    m_componentNaneIndexer++;
                    m_selectedComp = (m_items.size() - 1);
                }
                ResizeIfNeeded(windowCounter);
                if (ImGui::Button("New Button"))
                {
                    m_items.push_back(new sButton());
                    m_items.back()->RemakeWBounds(m_curSelectedEl);
                    m_items.back()->name.append(std::to_string(m_componentNaneIndexer));
                    m_componentNaneIndexer++;
                    m_selectedComp = (m_items.size() - 1);
                }
                ResizeIfNeeded(windowCounter);
                if (ImGui::Button("New Progressbar"))
                {
                    m_items.push_back(new sProgressBar());
                    m_items.back()->RemakeWBounds(m_curSelectedEl);
                    m_items.back()->name.append(std::to_string(m_componentNaneIndexer));
                    m_componentNaneIndexer++;
                    m_selectedComp = (m_items.size() - 1);
                }
                ResizeIfNeeded(windowCounter);
                if (ImGui::Button("New slider"))
                {
                    m_items.push_back(new sSlider());
                    m_items.back()->RemakeWBounds(m_curSelectedEl);
                    m_items.back()->name.append(std::to_string(m_componentNaneIndexer));
                    m_componentNaneIndexer++;
                    m_selectedComp = (m_items.size() - 1);
                }
                ResizeIfNeeded(windowCounter);
                if (m_selectedComp != -1)
                {
                    if (ImGui::Button("delete component"))
                    {
                        m_items.at(m_selectedComp)->Delete();
                        delete m_items.at(m_selectedComp);
                        m_items.erase(m_items.begin() + m_selectedComp);
                        m_selectedComp = -1;
                    }
                }
                ResizeIfNeeded(windowCounter);
            }
            EndChild(windowCounter, cursorPos);

            BeginChild("compEditor", windowCounter, cursorPos);

            if (m_curSelectedEl != entt::null && m_selectedComp != -1)
            {
                m_items.at(m_selectedComp)->ShowInfo(m_curSelectedEl);
            }
            ResizeIfNeeded(windowCounter);

            EndChild(windowCounter, cursorPos);
        }
        else
        {
            ImGui::Text("No element selected");
            ResizeIfNeeded(windowCounter);
        }
    }
    ImGui::End();

    if (m_fontEditor)
    {
        ImGui::Begin("Font Manager", &m_fontEditor);

        ImGui::Text("Convert traditional font file to sff");

        if (ImGui::InputText("font", &m_curFontLoad))
        {
            if (fileExists(m_curFontLoad))
            {
                if (m_curFontLoad.find(".sff") != std::string::npos)
                {
                    m_curLoadedFont = ui.m_fontHandler->LoadssfEditor(m_curFontLoad);
                    m_curFontLoadsff = m_curFontLoad;
                }
                else
                {
                    if (ui.m_fontHandler->SerializeFont(m_curFontLoad, "NewFont"))
                    {
                        m_curFontLoadsff = Engine.FileIO().GetPath(FileIO::Directory::Asset, "fonts/NewFont.sff");
                        m_curLoadedFont = ui.m_fontHandler->LoadssfEditor(m_curFontLoadsff);
                    }
                }
                m_previewEl = ui.CreateUIElement(top, left);
                m_previewcomp = ui.CreateString(m_previewEl, 0, m_previewText, glm::vec2(0.5, 0.5), 0.2f, glm::vec4(1));
                m_LoadedFont = true;
            }
        }
        std::filesystem::path path;
        if (assetSystem.SetDragDropTarget(path, {".otf", ".ttf", ".sff"}))
        {
            m_curFontLoad = path.string();
            if (m_curFontLoad.find(".sff") != std::string::npos)
            {
                m_curLoadedFont = ui.m_fontHandler->LoadssfEditor(m_curFontLoad);
                m_curFontLoadsff = m_curFontLoad;
            }
            else
            {
                if (ui.m_fontHandler->SerializeFont(m_curFontLoad, "NewFont"))
                {
                    m_curFontLoadsff = Engine.FileIO().GetPath(FileIO::Directory::Asset, "fonts/NewFont.sff");
                    m_curLoadedFont = ui.m_fontHandler->LoadssfEditor(m_curFontLoadsff);
                }
            }
            m_previewEl = ui.CreateUIElement(top, left);
            m_previewcomp = ui.CreateString(m_previewEl, 0, m_previewText, glm::vec2(0.5, 0.5), 0.2f, glm::vec4(1));
            m_LoadedFont = true;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Accepts: otf, ttf, sff");
            ImGui::EndTooltip();
        }
        ImGui::Spacing();
        if (ImGui::InputText("preview text", &m_previewText))
        {
            if (m_LoadedFont) ui.ReplaceString(m_previewcomp, m_previewText);
        }
        ImGui::Separator();

        if (m_curLoadedFont != -1)
        {
            ImGui::Text("currently Loaded Font");
            if (ImGui::InputText("Font name", &ui.fontHandler().m_fonts.at(m_curLoadedFont).name))
            {
            }
            if (ImGui::InputFloat("Font size", &ui.fontHandler().m_fonts.at(m_curLoadedFont).fontSize))
            {
                std::ofstream os(m_curFontLoadsff);
                cereal::JSONOutputArchive archive(os);
                archive(CEREAL_NVP(ui.fontHandler().m_fonts.at(m_curLoadedFont)));
                os.close();
            }
        }
        else
        {
            ImGui::Text("No font loaded");
        }
        float x = ImGui::GetCursorPos().x;
        float height = ImGui::GetWindowHeight();
        auto& style = ImGui::GetStyle();
        height -= ImGui::GetFontSize() * 1 + style.FramePadding.y * 2;
        ImGui::SetCursorPos(ImVec2(x, height));
        if (ImGui::Button("Save"))
        {
            std::string str0 = Engine.FileIO().GetPath(FileIO::Directory::Asset,
                                                       "fonts/" + ui.fontHandler().m_fonts.at(m_curLoadedFont).name + ".sff");
            // std::ifstream opened(m_curFontLoadsff);
            // if (std::rename(m_curFontLoadsff.c_str(), str0.c_str()) == 0)
            //{
            //     // if (std::remove(m_curFontLoadsff.c_str()))
            //     //{
            //     // }
            //     std::cout << " succeed";
            // }
            m_curFontLoadsff = str0;

            Font font = ui.fontHandler().m_fonts.at(m_curLoadedFont);
            // std::ofstream os(m_curFontLoadsff);
            // cereal::JSONOutputArchive archive(os);
            // archive(CEREAL_NVP(font));
            if (fileExists(str0)) std::remove(m_curFontLoadsff.c_str());
            font.Save(str0);
            // os.close();
        }
        ImGui::End();
    }
    else if (m_LoadedFont)
    {
        ui.DeleteUIElement(m_previewEl);
        m_LoadedFont = false;
    }

    if (m_hoverEditor)
    {
        ImGui::Begin("hover editor", &m_hoverEditor);

        std::vector<std::string> vecstr;
        vecstr.push_back("none");
        for (auto& overlay : ui.serialiser->overlays)
        {
            vecstr.push_back(overlay.first);
        }
        DisplayDropDown(m_cursel, vecstr, m_cursel);
        ImGui::SameLine();
        if (ImGui::Button("New"))
        {
            ui.AddOverlayImage(UIImageElement());
            ui.m_overlaymap.at(ui.serialiser->overlays.at("New UI Image")).name.append(std::to_string(m_oCounter));
            ui.serialiser->overlays.emplace(ui.m_overlaymap.at(ui.serialiser->overlays.at("New UI Image")).name,
                                            ui.serialiser->overlays.at("New UI Image"));
            ui.serialiser->overlays.erase("New UI Image");
            m_oCounter++;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
        {
            if (m_cursel != "none")
            {
                ui.m_overlaymap.erase(ui.serialiser->overlays.at(m_cursel));
                ui.serialiser->overlays.erase(m_cursel);
                m_cursel = "none";
            }
        }
        if (m_cursel != "none")
        {
            auto& overl = ui.m_overlaymap.at(ui.serialiser->overlays.at(m_cursel));
            ImGui::InputText("name", &overl.name);
            int atla[4] = {static_cast<int>(overl.m_leftCornerAnchor.x), static_cast<int>(overl.m_leftCornerAnchor.y),
                           static_cast<int>(overl.m_size.x), static_cast<int>(overl.m_size.y)};
            if (ImGui::InputInt4("atlas", &atla[0]))
            {
                overl.m_leftCornerAnchor.x = atla[0];
                overl.m_leftCornerAnchor.y = atla[1];
                overl.m_size.x = atla[2];
                overl.m_size.y = atla[3];
            }
            if (ImGui::InputText("file path", &overl.file))
            {
                float w, h;
                GetPngImageDimensions(overl.file, w, h);
                overl.m_leftCornerAnchor = glm::vec2(0.0f, 0.0f);
                overl.m_size = glm::vec2(w, h);
            }
            std::filesystem::path path;
            if (assetSystem.SetDragDropTarget(path, {".png"}))
            {
                overl.file = path.string();
                float w, h;
                GetPngImageDimensions(overl.file, w, h);
                overl.m_leftCornerAnchor = glm::vec2(0.0f, 0.0f);
                overl.m_size = glm::vec2(w, h);
            }
        }
        float x = ImGui::GetCursorPos().x;
        float height = ImGui::GetWindowHeight();
        auto& style = ImGui::GetStyle();
        height -= ImGui::GetFontSize() * 1 + style.FramePadding.y * 2;
        ImGui::SetCursorPos(ImVec2(x, height));
        if (ImGui::Button("Save"))
        {
            ui.serialiser->SaveOverlays();
            m_cursel = ui.m_overlaymap.at(ui.serialiser->overlays.at(m_cursel)).name;
            ui.serialiser->LoadOverlays();
        }
        float newx = ImGui::GetItemRectSize().x + style.FramePadding.y * 2;
        ImGui::SetCursorPos(ImVec2(newx, height));
        if (ImGui::Button("Load"))
        {
            ui.serialiser->LoadOverlays();
            m_cursel = "none";
        }
        ImGui::End();
    }
    if (m_ishovered)
    {
        if (!m_atlasEditing)
        {
            if (Engine.Input().GetMouseButtonOnce(Input::MouseButton::Left))
            {
                m_selectedComp = -1;
                m_atlasEditing = false;
            }
            int counter = 0;
            for (int i = m_items.size() - 1; i >= 0;
                 i--)  // backwards so that newly created items take priority over older items.
            {
                auto& item = *m_items.at(i);
                if (ui.isInBounds(item.bounds, m_oldMPos))
                {
                    glm::vec2 p0 = glm::vec2(item.bounds.x, item.bounds.y);
                    p0 = glm::vec2((p0.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p0.y * (m_gameSize.y));
                    glm::vec2 p1 = glm::vec2(item.bounds.z, item.bounds.w);
                    p1 = glm::vec2((p1.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p1.y * (m_gameSize.y));

                    MakeBox(p0, p1, glm::vec4(1.0f));

                    //

                    if (Engine.Input().GetMouseButtonOnce(Input::MouseButton::Left))
                    {
                        m_selectedComp = (m_items.size() - 1) - counter;
                        m_clicking = true;
                        m_curSelectedEl = m_items.at(m_selectedComp)->element;
                        if (Engine.ECS().Registry.get<internal::UIElement>(m_items.at(m_selectedComp)->element).topOrBot == top)
                        {
                            m_tobs = "Top";
                        }
                        else
                        {
                            m_tobs = "Bottom";
                        }
                        if (Engine.ECS().Registry.get<internal::UIElement>(m_items.at(m_selectedComp)->element).leftOrRight ==
                            left)
                        {
                            m_lors = "Left";
                        }
                        else
                        {
                            m_lors = "Right";
                        }
                        break;
                    }
                }
                else
                {
                }
                counter++;
            }
            if (m_selectedComp != -1)
            {
                if (ui.isInBounds(m_items.at(m_selectedComp)->bounds, m_oldMPos) && m_clicking == true &&
                    Engine.Input().GetMouseButton(Input::MouseButton::Left))
                {
                    auto& item = *m_items.at(m_selectedComp);
                    glm::vec2 mousePos = Engine.Input().GetMousePosition();
                    glm::vec2 newMousePos2 = glm::vec2(mousePos.x - m_gamePos.x, mousePos.y - m_gamePos.y);
                    glm::vec2 newMpos = glm::vec2((newMousePos2.x / m_gameSize.x) * (m_gameSize.x / m_gameSize.y),
                                                  newMousePos2.y / (m_gameSize.y));

                    glm::vec2 diff = newMpos - m_oldMPos;

                    item.bounds = glm::vec4(glm::vec2(item.bounds.x, item.bounds.y) + diff,
                                            glm::vec2(item.bounds.z, item.bounds.w) + diff);

                    item.Move(m_curSelectedEl, diff);
                }
                else
                {
                    m_clicking = false;
                }
            }
        }

        else
        {
            // astlas editing
            bool edited = false;

            float wheel = Engine.Input().GetMouseWheel();
            float adjwheel = m_mouseWheel - wheel;
            m_mouseWheel = wheel;
            adjwheel *= 10.f;
            if (adjwheel > 0 || adjwheel < 0)
            {
                m_atlasptr->x -= adjwheel;
                m_atlasptr->y -= adjwheel;
                m_atlasptr->z += adjwheel;
                m_atlasptr->w += adjwheel;
                edited = true;
            }

            if (Engine.Input().GetMouseButton(Input::MouseButton::Left))
            {
                glm::vec2 mousePos = Engine.Input().GetMousePosition();
                glm::vec2 newMousePos2 = glm::vec2(mousePos.x - m_gamePos.x, mousePos.y - m_gamePos.y);
                glm::vec2 newMpos =
                    glm::vec2((newMousePos2.x / m_gameSize.x) * (m_gameSize.x / m_gameSize.y), newMousePos2.y / (m_gameSize.y));
                glm::vec2 diff = newMpos - m_oldMPos;
                diff *= 100;

                m_atlasptr->x += diff.x;
                m_atlasptr->y += diff.y;
                m_atlasptr->z += diff.x;
                m_atlasptr->w += diff.y;
                edited = true;
            }
            if (edited)
            {
                ui.DeleteComponent(m_items.at(m_selectedComp)->ID);
                m_items.at(m_selectedComp)->RemakeWBounds(m_curSelectedEl);
            }
        }
    }
    if (m_selectedComp != -1)
    {
        auto& item = *m_items.at(m_selectedComp);
        glm::vec2 p0 = glm::vec2(item.bounds.x, item.bounds.y);
        p0 = glm::vec2((p0.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p0.y * (m_gameSize.y));
        glm::vec2 p1 = glm::vec2(item.bounds.z, item.bounds.w);
        p1 = glm::vec2((p1.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p1.y * (m_gameSize.y));
        MakeBox(p0, p1, glm::vec4(1.0f));
        if (m_focusedWindow)
        {
            if (Engine.Input().GetKeyboardKeyOnce(Input::KeyboardKey::Delete))
            {
                m_items.at(m_selectedComp)->Delete();
                delete m_items.at(m_selectedComp);
                m_items.erase(m_items.begin() + m_selectedComp);
                m_selectedComp = -1;
                m_atlasEditing = false;
            }
        }
    }
}

void bee::ui::UIEditor::Lines()
{
    // MakeLine(glm::vec2(newMousePos2), glm::vec2(0.0f), glm::vec4(1.0f));
    // draw lines
    if (m_debugDrawing)
    {
        auto view = Engine.ECS().Registry.view<internal::UIElement>();

        for (auto [ent, el] : view.each())
        {
            for (auto& [in, comp] : el.textComponents)
            {
                for (int i = 0; i < comp.indices.size(); i += 3)
                {
                    const int vertexSize = 9;
                    glm::vec2 p0 = glm::vec2(comp.vertices.at(comp.indices.at(i + 0) * vertexSize),
                                             comp.vertices.at((comp.indices.at(i + 0) * vertexSize) + 1));
                    glm::vec2 p1 = glm::vec2(comp.vertices.at(comp.indices.at(i + 1) * vertexSize),
                                             comp.vertices.at((comp.indices.at(i + 1) * vertexSize) + 1));
                    glm::vec2 p2 = glm::vec2(comp.vertices.at(comp.indices.at(i + 2) * vertexSize),
                                             comp.vertices.at((comp.indices.at(i + 2) * vertexSize) + 1));
                    p0 = glm::vec2((p0.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p0.y * (m_gameSize.y));
                    p1 = glm::vec2((p1.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p1.y * (m_gameSize.y));
                    p2 = glm::vec2((p2.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p2.y * (m_gameSize.y));

                    MakeVertex(p0, p1, p2, glm::vec4(1.0f));
                }
            }
            for (auto& comp : el.imageComponents)
            {
                const int vertexSize = 5;
                auto compo = comp.second;
                for (int i = 0; i < compo.indices.size(); i += 3)
                {
                    glm::vec2 p0 = glm::vec2(compo.vertices.at(compo.indices.at(i + 0) * vertexSize),
                                             compo.vertices.at((compo.indices.at(i + 0) * vertexSize) + 1));
                    glm::vec2 p1 = glm::vec2(compo.vertices.at(compo.indices.at(i + 1) * vertexSize),
                                             compo.vertices.at((compo.indices.at(i + 1) * vertexSize) + 1));
                    glm::vec2 p2 = glm::vec2(compo.vertices.at(compo.indices.at(i + 2) * vertexSize),
                                             compo.vertices.at((compo.indices.at(i + 2) * vertexSize) + 1));
                    p0 = glm::vec2((p0.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p0.y * (m_gameSize.y));
                    p1 = glm::vec2((p1.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p1.y * (m_gameSize.y));
                    p2 = glm::vec2((p2.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p2.y * (m_gameSize.y));

                    MakeVertex(p0, p1, p2, glm::vec4(1.0f));
                }
            }
            for (auto& comp : el.progressBarimageComponents)
            {
                const int vertexSize = 16;

                auto compo = comp.second;
                for (int i = 0; i < compo.indices.size(); i += 3)
                {
                    glm::vec2 p0 = glm::vec2(compo.vertices.at(compo.indices.at(i + 0) * vertexSize),
                                             compo.vertices.at((compo.indices.at(i + 0) * vertexSize) + 1));
                    glm::vec2 p1 = glm::vec2(compo.vertices.at(compo.indices.at(i + 1) * vertexSize),
                                             compo.vertices.at((compo.indices.at(i + 1) * vertexSize) + 1));
                    glm::vec2 p2 = glm::vec2(compo.vertices.at(compo.indices.at(i + 2) * vertexSize),
                                             compo.vertices.at((compo.indices.at(i + 2) * vertexSize) + 1));
                    p0 = glm::vec2((p0.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p0.y * (m_gameSize.y));
                    p1 = glm::vec2((p1.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p1.y * (m_gameSize.y));
                    p2 = glm::vec2((p2.x * m_gameSize.x) / (m_gameSize.x / m_gameSize.y), p2.y * (m_gameSize.y));

                    MakeVertex(p0, p1, p2, glm::vec4(1.0f));
                }
            }
        }
    }

    ImGui::Begin("UI Editor Window");
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        for (const auto& line : m_lines)
        {
            draw_list->AddLine(ImVec2(line.p0.x + m_gamePos.x, line.p0.y + m_gamePos.y),
                               ImVec2(line.p1.x + m_gamePos.x, line.p1.y + m_gamePos.y),
                               IM_COL32(line.color.r * 255, line.color.g * 255, line.color.b * 255, line.color.a * 255), 1.0f);
        }
        m_lines.clear();
    }
    // general properties

    const glm::vec2 mousePos = Engine.Input().GetMousePosition();
    const glm::vec2 newMousePos2 = glm::vec2(mousePos.x - m_gamePos.x, mousePos.y - m_gamePos.y);
    m_oldMPos = glm::vec2((newMousePos2.x / m_gameSize.x) * (m_gameSize.x / m_gameSize.y), newMousePos2.y / (m_gameSize.y));

    ImGui::End();
}

void bee::ui::UIEditor::ViewPort()
{
    // auto& ui = Engine.ECS().GetSystem<UserInterface>();

    bool aintThatTrue = true;
    ImGui::Begin("UI Editor Window", &aintThatTrue, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::IsWindowFocused())
    {
        m_focusedWindow = true;
        // ui.SetDrawStateUIelement(ui.m_selectedElementOverlay, true);
    }
    else
    {
        m_focusedWindow = false;
        // ui.SetDrawStateUIelement(ui.m_selectedElementOverlay, false);
    }
    // general properties
    float width = ImGui::GetWindowWidth();
    float height = ImGui::GetWindowHeight();

    ImGui::SetCursorPos(ImVec2(0.0f, ImGui::GetFrameHeight()));

    m_gameSize = glm::vec2(width, height);
    m_gamePos = glm::vec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + ImGui::GetFrameHeight());

    const auto screenAspectRatio = 9.0f / 16.0f;
    if (height / width < screenAspectRatio)
        width = height * 1.0f / screenAspectRatio;
    else
        height = width * screenAspectRatio;

    m_gameSize = glm::vec2(width, height);
    // get rendertarget
    {
        DeviceManager* device_manager = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager();

        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStartHandle(
            device_manager->m_ImGui_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        const auto rtvDescriptorSize =
            device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        gpuStartHandle.Offset(rtvDescriptorSize * 1);

        ImGui::Image((ImTextureID)gpuStartHandle.ptr, ImVec2(width, height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    }
    m_wsize = glm::vec2(ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y);
    m_ishovered = ImGui::IsWindowHovered();

    ImGui::End();
}

void bee::ui::UIEditor::MakeLine(glm::vec2 p0, glm::vec2 p1, glm::vec4 color) { m_lines.push_back(Line(p0, p1, color)); }

void bee::ui::UIEditor::MakeBox(glm::vec2 p0, glm::vec2 p1, glm::vec4 color)
{
    MakeLine(p0, glm::vec2(p1.x, p0.y), color);
    MakeLine(glm::vec2(p1.x, p0.y), p1, color);
    MakeLine(p1, glm::vec2(p0.x, p1.y), color);
    MakeLine(glm::vec2(p0.x, p1.y), p0, color);
}

void bee::ui::UIEditor::MakeVertex(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec4 color)
{
    MakeLine(p0, p1, color);
    MakeLine(p1, p2, color);
    MakeLine(p2, p0, color);
}

void bee::ui::UIEditor::ResizeIfNeeded(int windowCounter)
{
    if (ImGui::GetItemRectSize().x > m_autoSizes.at(windowCounter))
    {
        m_autoSizes.at(windowCounter) = ImGui::GetItemRectSize().x;
    }
}

// thanks vlad :)
bool bee::ui::UIEditor::DisplayDropDown(const std::string& name, const std::vector<std::string>& list,
                                        std::string& selectedItem) const
{
    const float width =
        ImGui::CalcTextSize(name.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetNextItemWidth(width);
    bool newSelect = false;
    const std::string tag = "##" + name;
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

            // Set the initial focus when opening the combo (scrolling +
            // keyboard navigation focus)
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    return newSelect;
}

void bee::ui::UIEditor::BeginChild(const std::string& name, const int& windowCounter, const ImVec2& cursorPos) const
{
    ImGui::SetCursorPos(cursorPos);
    ImGui::BeginChild(name.c_str(), ImVec2(m_autoSizes.at(windowCounter), ImGui::GetWindowHeight() - cursorPos.y));
}

void bee::ui::UIEditor::EndChild(int& windowCounter, ImVec2& cursorPos) const
{
    const int spaceBetween = 10;

    ImGui::EndChild();
    cursorPos.x += m_autoSizes.at(windowCounter) + spaceBetween;
    windowCounter++;
}

void bee::ui::UIEditor::AdjustCoordinates(const UIElementID el, glm::vec2& posToAdjust, glm::vec2& sizeToAdjust) const
{
    const auto& element = Engine.ECS().Registry.get<internal::UIElement>(el);
    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;

    switch (element.topOrBot)
    {
        case bottom:
            posToAdjust.y = 1.0f - posToAdjust.y;
            sizeToAdjust.y = -sizeToAdjust.y;
            break;
        case top:
            // do nothing
            break;
        case right:
        case left:
        {
            Log::Error("element topOrBot orientation is wrong");
        }
    }
    switch (element.leftOrRight)
    {
        case right:
            posToAdjust.x = norWidth - posToAdjust.x;
            sizeToAdjust.x = -sizeToAdjust.x;
            break;
        case left:
            // do nothing
            break;
        case top:
        case bottom:
        {
            Log::Error("element rightOrLeft orientation is wrong");
        }
    }
}

std::vector<std::string> bee::ui::UIEditor::GetHoverables() const
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    std::vector<std::string> vecstr;
    vecstr.push_back("none");
    for (auto& overlay : ui.serialiser->overlays)
    {
        vecstr.push_back(overlay.first);
    }
    return vecstr;
}

std::vector<std::string> bee::ui::UIEditor::GetFunctions() const
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    std::vector<std::string> vecstr;
    vecstr.push_back("none");
    for (auto& func : ui.serialiser->functions)
    {
        vecstr.push_back(func.first);
    }
    return vecstr;
}

void bee::ui::UIEditor::SetoverrideSel(const UIComponentID ID) const
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
#ifdef BEE_INSPECTOR
    ui.overrideSel = ID;
#endif
}

UIElementID bee::ui::UIEditor::LoadElement(const std::string& name, UIElementID ID)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    sElement element;

    std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, name + ".json "));
    cereal::JSONInputArchive archive(is);
    archive(CEREAL_NVP(element));
    if (ID == entt::null)
    {
        ID = ui.CreateUIElement(element.tobs, element.lors);
    }

    Engine.ECS().Registry.get<internal::UIElement>(ID).name = name;
    Engine.ECS().Registry.get<Transform>(ID).Name = name;
    Engine.ECS().Registry.get<internal::UIElement>(ID).opacity = element.opacity;
    for (auto& but : element.buttons)
    {
        but.RemakeWBounds(ID);
        m_items.push_back(new sButton(but));
    }
    for (auto& pb : element.progressbars)
    {
        pb.RemakeWBounds(ID);
        m_items.push_back(new sProgressBar(pb));
    }
    for (auto& tx : element.texts)
    {
        tx.RemakeWBounds(ID);
        m_items.push_back(new Text(tx));
    }
    for (auto& img : element.images)
    {
        img.RemakeWBounds(ID);
        m_items.push_back(new Image(img));
    }
    for (auto& slid : element.sliders)
    {
        slid.RemakeWBounds(ID);
        m_items.push_back(new sSlider(slid));
    }
    Engine.ECS().Registry.get<Transform>(ID).Translation.z = static_cast<float>(element.layer) / 100.0f;
    if (element.tobs == top)
    {
        m_tobs = "Top";
    }
    else
    {
        m_tobs = "Bottom";
    }
    if (element.lors == left)
    {
        m_lors = "Left";
    }
    else
    {
        m_lors = "Right";
    }
    return ID;
}
