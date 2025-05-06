#include "user_interface/user_interface_editor_structs.hpp"

#include <imgui/imgui.h>

#include <user_interface/user_interface_editor.hpp>

#include "core/engine.hpp"
#include "imgui/imgui_stdlib.h"
#include "platform/pc/core/device_pc.hpp"
#include "tools/asset_explorer_system.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface_serializer.hpp"
using namespace bee;
using namespace ui;

constexpr float E_POS_SPEED = 0.01;
constexpr float E_SIZE_MIN = 0;
constexpr int E_ATLAS_SPEED = 1;
constexpr int E_ATLAS_MIN = 0;
constexpr int E_LAYER_SPEED = 1;
constexpr int E_LAYER_MIN = 0;

void bee::ui::Image::ShowInfo(const UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();
    auto& editor = Engine.ECS().GetSystem<UIEditor>();

    ImGui::InputText("Component name", &name);
    if (ImGui::DragInt("layer", &layer, E_LAYER_SPEED, E_LAYER_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat2("position", &start.x, E_POS_SPEED))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat2("size", &size.x, E_POS_SPEED, E_SIZE_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    int atla[4] = {static_cast<int>(atlas.x), static_cast<int>(atlas.y), static_cast<int>(atlas.z), static_cast<int>(atlas.w)};
    if (ImGui::DragInt4("atlas", &atla[0], E_ATLAS_SPEED, E_ATLAS_MIN))
    {
        ui.DeleteComponent(ID);
        atlas.x = static_cast<float>(atla[0]);
        atlas.y = static_cast<float>(atla[1]);
        atlas.z = static_cast<float>(atla[2]);
        atlas.w = static_cast<float>(atla[3]);
        RemakeWBounds(el);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Edit Mode", &editingAtlas))
    {
        if (editingAtlas)
        {
            editor.EnableAtlasEditingMode(&atlas);
        }
        else
        {
            editor.DisableAtlasEditingMode();
        }
    }
    if (ImGui::InputText("texture", &imageFile))
    {
        ui.DeleteComponent(ID);
        float w, h;
        GetPngImageDimensions(imageFile, w, h);
        atlas = glm::vec4(0.0f, 0.0f, w, h);
        RemakeWBounds(el);
    }
    std::filesystem::path path;
    if (assetSystem.SetDragDropTarget(path, {".png"}))
    {
        imageFile = path.string();
        ui.DeleteComponent(ID);
        float w, h;
        GetPngImageDimensions(imageFile, w, h);
        atlas = glm::vec4(0.0f, 0.0f, w, h);
        RemakeWBounds(el);
    }
}

void bee::ui::Image::Move(const UIElementID el, glm::vec2 delta)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    const auto& element = Engine.ECS().Registry.get<internal::UIElement>(el);
    if (element.topOrBot == bottom)
    {
        delta.y = -delta.y;
    }
    if (element.leftOrRight == right)
    {
        delta.x = -delta.x;
    }
    ui.DeleteComponent(ID);
    start = start + delta;
    RemakeWBounds(el);
}

void bee::ui::Image::Remake(const UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    if (ID != -1)
    {
        ui.DeleteComponent(ID, false);
    }
    element = el;
    const auto image = ui.LoadTexture(imageFile);
    ID = ui.CreateTextureComponentFromAtlas(el, image, atlas, start, size, layer, name);
    type = ComponentType::image;
}
void bee::ui::Image::RemakeWBounds(const UIElementID el)
{
    const auto& editor = Engine.ECS().GetSystem<UIEditor>();

    Remake(el);
    glm::vec2 cstart = start, csize = size;
    editor.AdjustCoordinates(el, cstart, csize);
    bounds = glm::vec4(cstart, cstart + csize);
}

void bee::ui::Image::Delete()
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    ui.DeleteComponent(ID);
}

void bee::ui::Text::ShowInfo(const UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();

    ImGui::InputText("Component name", &name);
    if (ImGui::DragInt("layer", &layer, E_LAYER_SPEED, E_LAYER_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat2("position", &start.x, E_POS_SPEED))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat("size", &size, E_POS_SPEED, E_SIZE_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::InputText("Text", &text))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat4("colour", &colour[0], 1, 0, 1))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::InputText("font", &font))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    std::filesystem::path path;
    if (assetSystem.SetDragDropTarget(path, {".sff"}))
    {
        font = path.string();
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("file reference");
        ImGui::EndTooltip();
    }
}
void bee::ui::Text::Move(const UIElementID el, glm::vec2 delta)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    const auto& element = Engine.ECS().Registry.get<internal::UIElement>(el);

    ui.DeleteComponent(ID);
    if (element.topOrBot == bottom)
    {
        delta.y = -delta.y;
    }
    if (element.leftOrRight == right)
    {
        delta.x = -delta.x;
    }
    start = start + delta;
    RemakeWBounds(el);
}

void bee::ui::Text::Remake(const UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    if (ID != -1)
    {
        ui.DeleteComponent(ID, false);
    }
    element = el;
    type = ComponentType::text;

    float nx = 0;
    float ny = 0;
    auto fontt = ui.LoadFont(font);
    ID = ui.CreateString(el, fontt, text, start, size, colour, nextLine.x, nextLine.y, fullline, 0, layer, name);
}
void bee::ui::Text::RemakeWBounds(UIElementID el)
{
    const auto& editor = Engine.ECS().GetSystem<UIEditor>();

    Remake(el);
    glm::vec2 cstart = glm::vec2(start), csize = nextLine;
    editor.AdjustCoordinates(el, cstart, csize);

    const auto& element = Engine.ECS().Registry.get<internal::UIElement>(el);

    const float x0 = cstart.x;
    float x1;
    if (element.leftOrRight == right)
    {
        const float height = static_cast<float>(Engine.Device().GetHeight());
        const float width = static_cast<float>(Engine.Device().GetWidth());
        const float norWidth = width / height;
        x1 = norWidth - nextLine.x;
    }
    else
    {
        x1 = nextLine.x;
    }

    const float y0 = cstart.y - 0.2f * size;
    if (element.topOrBot == bottom)
    {
        nextLine.y = 1.0f - nextLine.y;
    }
    const float y1 = cstart.y + ((nextLine.y - cstart.y) / 2);

    bounds = glm::vec4(x0, y0, x1, y1);
}

void bee::ui::Text::Delete()
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    ui.DeleteComponent(ID);
}

void bee::ui::sButton::ShowInfo(const UIElementID el)
{
    auto& editor = Engine.ECS().GetSystem<UIEditor>();
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    ImGui::InputText("Component name", &name);
    if (ImGui::DragInt("layer", &layer, E_LAYER_SPEED, E_LAYER_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat2("position", &start.x, E_POS_SPEED))
    {
        ui.DeleteComponent(ID);
        ui.DeleteComponent(visOverlay);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat2("size", &size.x, E_POS_SPEED, E_SIZE_MIN))
    {
        ui.DeleteComponent(ID);
        ui.DeleteComponent(visOverlay);
        RemakeWBounds(el);
    }
    const std::vector<std::string> tlist = {"single", "repeat"};
    if (editor.DisplayDropDown(typeDrop, tlist, typeDrop))
    {
        if (typeDrop == "single")
        {
            typein = single;
        }
        if (typeDrop == "repeat")
        {
            typein = repeat;
        }
        ui.DeleteComponent(visOverlay);
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    ImGui::Checkbox("hoverable", &hoverable);
    ImGui::Separator();
    ImGui::InputText("linked component", &linkedComponentstr);
    if (ImGui::Button("Sync Button and texture"))
    {
        for (auto item : editor.m_items)
        {
            if (item->name == linkedComponentstr)
            {
                auto cItem = dynamic_cast<Image*>(item);
                ui.DeleteComponent(cItem->ID);
                cItem->start = start;
                cItem->size = size;
                cItem->RemakeWBounds(el);
                break;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("select linkedComp"))
    {
        for (int i = 0; i < editor.m_items.size(); i++)
        {
            if (editor.m_items.at(i)->name == linkedComponentstr)
            {
                editor.m_selectedComp = i;
                break;
            }
        }
    }
    const std::vector<std::string> hstlist = {"none", "new atlas positions"};
    if (editor.DisplayDropDown(hoverst + "##hover", hstlist, hoverst))
    {
        if (hoverst == "none")
        {
            hover.type = none;
        }
        if (hoverst == "new atlas positions")
        {
            hover.type = newAtlasPos;
        }
    }
    if (hover.type == newAtlasPos)
    {
        int atla[4] = {static_cast<int>(hover.newAtlas.x), static_cast<int>(hover.newAtlas.y),
                       static_cast<int>(hover.newAtlas.z), static_cast<int>(hover.newAtlas.w)};
        if (ImGui::DragInt4("new hover atlas", &atla[0], E_ATLAS_SPEED, E_ATLAS_MIN))
        {
            hover.newAtlas.x = static_cast<float>(atla[0]);
            hover.newAtlas.y = static_cast<float>(atla[1]);
            hover.newAtlas.z = static_cast<float>(atla[2]);
            hover.newAtlas.w = static_cast<float>(atla[3]);
        }
    }
    const std::vector<std::string> cstlist = {"none", "new atlas positions"};
    if (editor.DisplayDropDown(clickst + "##click", cstlist, clickst))
    {
        if (clickst == "none")
        {
            click.type = none;
        }
        if (clickst == "new atlas positions")
        {
            click.type = newAtlasPos;
        }
    }
    if (click.type == newAtlasPos)
    {
        int atla[4] = {static_cast<int>(click.newAtlas.x), static_cast<int>(click.newAtlas.y),
                       static_cast<int>(click.newAtlas.z), static_cast<int>(click.newAtlas.w)};
        if (ImGui::DragInt4("new click atlas", &atla[0], E_ATLAS_SPEED, E_ATLAS_MIN))
        {
            click.newAtlas.x = atla[0];
            click.newAtlas.y = atla[1];
            click.newAtlas.z = atla[2];
            click.newAtlas.w = atla[3];
        }
    }
    const auto hoverlist = editor.GetHoverables();
    if (editor.DisplayDropDown(hoverimg + "##hoverimg", hoverlist, hoverimg))
    {
        ui.DeleteComponent(ID);
        ui.DeleteComponent(visOverlay);
        RemakeWBounds(el);
    }
    ImGui::SameLine();
    ImGui::Text("hover img");
    ImGui::SameLine();
    if (ImGui::Checkbox("preview", &preview))
    {
        if (preview)
        {
            editor.SetoverrideSel(ID);
        }
        if (!preview)
        {
            editor.SetoverrideSel(-1);
        }
    }

    if (editor.DisplayDropDown(hover.funcName + "##hoverfunc", editor.GetFunctions(), hover.funcName))
    {
    }
    ImGui::SameLine();
    ImGui::Text("hover function");
    if (editor.DisplayDropDown(click.funcName + "##clickfunc", editor.GetFunctions(), click.funcName))
    {
    }
    ImGui::SameLine();
    ImGui::Text("click function");

    if (preview)
    {
        editor.SetoverrideSel(ID);
    }
}

void bee::ui::sButton::Move(UIElementID el, glm::vec2 delta)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    const auto& element = Engine.ECS().Registry.get<internal::UIElement>(el);

    ui.DeleteComponent(ID);
    ui.DeleteComponent(visOverlay);
    if (element.topOrBot == bottom)
    {
        delta.y = -delta.y;
    }
    if (element.leftOrRight == right)
    {
        delta.x = -delta.x;
    }
    start = start + delta;
    RemakeWBounds(el);
    if (preview)
    {
        const auto& editor = Engine.ECS().GetSystem<UIEditor>();
        editor.SetoverrideSel(ID);
    }
}

void bee::ui::sButton::Remake(UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    if (ID != -1)
    {
        ui.DeleteComponent(ID, false);
    }
    bounds = glm::vec4(start, start + size);
    element = el;
    linkedComponent = linkedComponentstr.empty() ? -1 : linkedComponent = ui.GetComponentID(element, linkedComponentstr);
    ID = ui.CreateButton(el, start, size, click, hover, typein, ui.GetOverlayIndex(hoverimg), hoverable, linkedComponent, name);
    type = ComponentType::button;
}

void bee::ui::sButton::Delete()
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    ui.DeleteComponent(ID);
    ui.DeleteComponent(visOverlay);
}

void bee::ui::sButton::RemakeWBounds(UIElementID el)
{
    const auto& editor = Engine.ECS().GetSystem<UIEditor>();
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    Remake(el);
    glm::vec2 cstart = start, csize = size;
    editor.AdjustCoordinates(el, cstart, csize);
    bounds = glm::vec4(cstart, cstart + csize);
    visOverlay = ui.CreateTextureComponentFromAtlas(el, 0, glm::vec4(0, 0, 1, 1), start, size, 0);
}

void bee::ui::sProgressBar::ShowInfo(UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();
    auto& editor = Engine.ECS().GetSystem<UIEditor>();

    ImGui::InputText("Component name", &name);
    if (ImGui::DragInt("layer", &layer, E_LAYER_SPEED, E_LAYER_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::SliderFloat("value", &value, 0, 100))
    {
        ui.SetProgressBarValue(ID, value);
    }
    if (ImGui::DragFloat2("position", &start.x, E_POS_SPEED))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat2("size", &size.x, E_POS_SPEED, E_SIZE_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    int atla[4] = {static_cast<int>(atlas.x), static_cast<int>(atlas.y), static_cast<int>(atlas.z), static_cast<int>(atlas.w)};
    if (ImGui::DragInt4("atlas", &atla[0], E_ATLAS_SPEED, E_ATLAS_MIN))
    {
        ui.DeleteComponent(ID);
        atlas.x = static_cast<float>(atla[0]);
        atlas.y = static_cast<float>(atla[1]);
        atlas.z = static_cast<float>(atla[2]);
        atlas.w = static_cast<float>(atla[3]);
        RemakeWBounds(el);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Edit Mode", &editingAtlas))
    {
        if (editingAtlas)
        {
            editor.EnableAtlasEditingMode(&atlas);
        }
        else
        {
            editor.DisableAtlasEditingMode();
        }
    }
    if (ImGui::InputText("texture", &imageFile))
    {
        ui.DeleteComponent(ID);
        float w, h;
        GetPngImageDimensions(imageFile, w, h);
        atlas = glm::vec4(0.0f, 0.0f, w, h);
        RemakeWBounds(el);
    }
    std::filesystem::path path;
    if (assetSystem.SetDragDropTarget(path, {".png"}))
    {
        imageFile = path.string();
        ui.DeleteComponent(ID);
        float w, h;
        GetPngImageDimensions(imageFile, w, h);
        atlas = glm::vec4(0.0f, 0.0f, w, h);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat4("foreground Color", &fColor.x, 1.0f / 255, 0, 1, "&.3f",
                          ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat4("background Color", &bColor.x, 1.0f / 255, 0, 1, "&.3f",
                          ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
}
void bee::ui::sProgressBar::Move(UIElementID el, glm::vec2 delta)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    const auto& element = Engine.ECS().Registry.get<internal::UIElement>(el);
    if (element.topOrBot == bottom)
    {
        delta.y = -delta.y;
    }
    if (element.leftOrRight == right)
    {
        delta.x = -delta.x;
    }
    ui.DeleteComponent(ID);
    start = start + delta;
    RemakeWBounds(el);
}
void bee::ui::sProgressBar::Remake(UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    if (ID != -1)
    {
        ui.DeleteComponent(ID, false);
    }
    const auto image = ui.LoadTexture(imageFile);
    element = el;

    ID = ui.CreateProgressBar(el, start, size, image, atlas, fColor, bColor, layer, value, name);
    type = ComponentType::progressbar;
}
void bee::ui::sProgressBar::Delete()
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    ui.DeleteComponent(ID);
}

void bee::ui::sProgressBar::RemakeWBounds(UIElementID el)
{
    Remake(el);

    const auto& editor = Engine.ECS().GetSystem<UIEditor>();

    glm::vec2 cstart = start, csize = size;
    editor.AdjustCoordinates(el, cstart, csize);
    bounds = glm::vec4(cstart, cstart + csize);
}

void bee::ui::sSlider::ShowInfo(UIElementID el)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    auto& assetSystem = Engine.ECS().GetSystem<AssetExplorer>();
    auto& editor = Engine.ECS().GetSystem<UIEditor>();

    ImGui::InputText("Component name", &name);
    if (ImGui::DragInt("layer", &layer, E_LAYER_SPEED, E_LAYER_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat("value", &tempVar, max / 100, 0, max))
    {
    }
    if (ImGui::DragFloat2("position", &start.x, E_POS_SPEED))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat2("size", &size.x, E_POS_SPEED, E_SIZE_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::InputText("texture", &imageFile))
    {
        ui.DeleteComponent(ID);
        float w, h;
        GetPngImageDimensions(imageFile, w, h);
        slidAtlas = glm::vec4(0.0f, 0.0f, w, h);
        barAtlas = glm::vec4(0.0f, 0.0f, w, h);
        RemakeWBounds(el);
    }
    std::filesystem::path path;
    if (assetSystem.SetDragDropTarget(path, {".png"}))
    {
        imageFile = path.string();
        ui.DeleteComponent(ID);
        float w, h;
        GetPngImageDimensions(imageFile, w, h);
        slidAtlas = glm::vec4(0.0f, 0.0f, w, h);
        barAtlas = glm::vec4(0.0f, 0.0f, w, h);
        RemakeWBounds(el);
    }
    int satla[4] = {static_cast<int>(slidAtlas.x), static_cast<int>(slidAtlas.y), static_cast<int>(slidAtlas.z),
                    static_cast<int>(slidAtlas.w)};
    if (ImGui::DragInt4("slider atlas", &satla[0], E_ATLAS_SPEED, E_ATLAS_MIN))
    {
        ui.DeleteComponent(ID);
        slidAtlas.x = static_cast<float>(satla[0]);
        slidAtlas.y = static_cast<float>(satla[1]);
        slidAtlas.z = static_cast<float>(satla[2]);
        slidAtlas.w = static_cast<float>(satla[3]);
        RemakeWBounds(el);
    }
    if (!editingAtlasbar)
    {
        ImGui::SameLine();
        if (ImGui::Checkbox("Edit Mode##slider", &editingAtlasslid))
        {
            if (editingAtlasslid)
            {
                editor.EnableAtlasEditingMode(&slidAtlas);
            }
            else
            {
                editor.DisableAtlasEditingMode();
            }
        }
    }
    int batla[4] = {static_cast<int>(barAtlas.x), static_cast<int>(barAtlas.y), static_cast<int>(barAtlas.z),
                    static_cast<int>(barAtlas.w)};
    if (ImGui::DragInt4("slider atlas", &batla[0], E_ATLAS_SPEED, E_ATLAS_MIN))
    {
        ui.DeleteComponent(ID);
        barAtlas.x = static_cast<float>(batla[0]);
        barAtlas.y = static_cast<float>(batla[1]);
        barAtlas.z = static_cast<float>(batla[2]);
        barAtlas.w = static_cast<float>(batla[3]);
        RemakeWBounds(el);
    }
    if (!editingAtlasslid)
    {
        ImGui::SameLine();
        if (ImGui::Checkbox("Edit Mode##bar", &editingAtlasbar))
        {
            if (editingAtlasbar)
            {
                editor.EnableAtlasEditingMode(&barAtlas);
            }
            else
            {
                editor.DisableAtlasEditingMode();
            }
        }
    }
    if (ImGui::DragFloat2("slider size", &slidSize.x, E_POS_SPEED, E_SIZE_MIN))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::DragFloat("speed", &speed))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("only for controller input");
        ImGui::EndTooltip();
    }
    if (ImGui::DragFloat("max", &max))
    {
        ui.DeleteComponent(ID);
        RemakeWBounds(el);
    }
}

void bee::ui::sSlider::Move(UIElementID el, glm::vec2 delta)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    auto& element = Engine.ECS().Registry.get<internal::UIElement>(el);
    if (element.topOrBot == bottom)
    {
        delta.y = -delta.y;
    }
    if (element.leftOrRight == right)
    {
        delta.x = -delta.x;
    }
    ui.DeleteComponent(ID);
    start = start + delta;
    RemakeWBounds(el);
}

void bee::ui::sSlider::Remake(UIElementID el)
{
    type = ComponentType::slider;
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    if (ID != -1)
    {
        ui.DeleteComponent(ID, false);
    }
    auto image = ui.LoadTexture(imageFile);
    ID = ui.CreateSlider(el, tempVar, max, speed, start, size, slidSize, image, barAtlas, slidAtlas, layer, name);
    element = el;
}
void bee::ui::sSlider::RemakeWBounds(UIElementID el)
{
    Remake(el);
    bounds = glm::vec4(start, start + size);
}

void bee::ui::sSlider::Delete()
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    ui.DeleteComponent(ID);
}
