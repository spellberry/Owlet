#include "user_interface/user_interface.hpp"

#include <tinygltf/stb_image.h>  // Implementation of stb_image is in gltf_loader.cpp

#include <chrono>

#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/input.hpp"
#include "core/transform.hpp"
#include "rendering/ui_renderer.hpp"
#include "tools/inspector.hpp"
#include "tools/log.hpp"
#include "tools/profiler.hpp"
#include "user_interface/font_handler.hpp"
#include "user_interface/user_interface_serializer.hpp"
// credits to Viktor Chlumsk� for the msdfgen and msdf-atlas-gen
// https://github.com/Chlumsky/msdfgen
// https://github.com/Chlumsky/msdf-atlas-gen

using namespace bee;
using namespace ui;
using namespace internal;

UserInterface::UserInterface()
{
    //  load default font
    Title = "UserInterface";
    m_fontHandler = std::make_unique<FontHandler>();
    m_renderer = std::make_unique<UIRenderer>();
    serialiser = std::make_unique<UISerialiser>();
#ifdef BEE_INSPECTOR
    m_currentItem = "left/top";
#endif

    m_selectedElementOverlay = CreateUIElement(top, left);
    serialiser->LoadOverlays(this);
}

UserInterface::~UserInterface()
{
    m_fontHandler.reset();
    m_renderer.reset();
    const auto view = Engine.ECS().Registry.view<UIElement>();
    for (auto [ent, el] : view.each())
    {
        Engine.ECS().DeleteEntity(ent);
    }
    m_componentMap.clear();
    m_toResetButtons.clear();
    for (auto& el : m_items)
    {
        for (auto& items : el.second)
        {
            items.second.release();
        }
    }
}

void UserInterface::Render()
{
    BEE_PROFILE_FUNCTION();
    bool execute = false;
    m_renderer->StartFrame(execute);
    const auto view2 = Engine.ECS().Registry.view<UIElement>();

    for (auto& [ID, upper] : m_updates)
    {
        if (!Engine.ECS().Registry.valid(upper.element))
        {
            continue;
        }
        auto& el = view2.get<UIElement>(upper.element);
        switch (upper.type)
        {
            case ComponentType::progressbar:
            {
                auto& bar = el.progressBarimageComponents.at(abs(upper.location));
                m_renderer->ReplaceBuffers(bar.renderData, bar.vertices.size(), bar.indices.size(), &bar.vertices.at(0),
                                           &bar.indices.at(0), 16);
                break;
            }
            case ComponentType::image:
            {
                auto& comp = el.imageComponents.at(upper.location);
                m_renderer->ReplaceBuffers(comp.renderData, comp.vertices.size(), comp.indices.size(), &comp.vertices.at(0),
                                           &comp.indices.at(0), 5);
                break;
            }
            case ComponentType::text:
            {
                auto& comp = el.textComponents.at(upper.location);
                m_renderer->ReplaceBuffers(comp.renderData, comp.vertices.size(), comp.indices.size(), &comp.vertices.at(0),
                                           &comp.indices.at(0), 9);
                break;
            }
            case ComponentType::button:
            case ComponentType::slider:
            {
                Log::Error("Tried to render update a button or slider??????");
            }
        }
    }
    m_updates.clear();
    m_renderer->EndFrame(execute);

    m_renderer->Render();
}

void UserInterface::Update(float dt)
{
    BEE_PROFILE_FUNCTION();
    const auto start = std::chrono::high_resolution_clock::now();

    bool execute = false;
    m_renderer->StartFrame(execute);

    for (int i = 0; i < m_toResetButtons.size(); i++)
    {
        ResetButtonInternal(m_toResetButtons.at(i));
        m_toResetButtons.erase(m_toResetButtons.begin() + i);
    }
    for (int i = 0; i < m_toDeleteElements.size(); i++)
    {
        DeleteUIElementInternal(m_toDeleteElements.at(i));
    }
    m_toDeleteElements.clear();
    auto view = Engine.ECS().Registry.view<UIElement>();
    bool currentlySelected = false;
    m_controllerTimer += dt;
    switch (m_inputMode)
    {
        case controller:
        {
            if (m_componentMap.count(m_CselectedComponent) == 0)
            {
                m_CselectedComponent = -1;
            }
            if (m_CselectedComponent == -1)
            {
                if (m_componentMap.size() > 0)
                {
                    bool found = false;
                    for (auto& component : m_componentMap)  // find a valid button or slider
                    {
                        const auto& compo = component.second;
                        if (compo.image == -200000 || compo.image == -300000)
                        {
                            if (compo.image == -200000)
                            {
                                if (!Engine.ECS().Registry.get<UIElement>(compo.element).buttons.at(compo.ID).hoverable)
                                {
                                    continue;
                                }
                            }
                            m_CselectedComponent = component.first;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        // no valid buttons or sliders were found, no input required
                        break;
                    }
                }
                else
                {
                    // no ui, no input required
                    break;
                }
            }
            // first check if currently selected interactable is being pressed?
            if (m_componentMap.count(m_CselectedComponent) == 0)
            {
                m_CselectedComponent = -1;
            }
            if (m_CselectedComponent == -1)
            {
                if (m_componentMap.size() > 0)
                {
                    bool found = false;
                    for (auto& component : m_componentMap)  // find a valid button or slider
                    {
                        const auto& compo = component.second;
                        if (compo.image == -200000 || compo.image == -300000)
                        {
                            m_CselectedComponent = component.first;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        // no valid buttons or sliders were found, no input required
                        break;
                    }
                }
                else
                {
                    // no ui, no input required
                    break;
                }
            }

            // interactions
            auto& compo = m_componentMap.find(m_CselectedComponent)->second;

            auto& element = Engine.ECS().Registry.get<UIElement>(compo.element);

            switch (static_cast<int>(compo.atlas.w))
            {
                case 1:
                {
                    // button
                    auto& inter = element.buttons.at(compo.ID);
                    bool lclicking = Engine.InputWrapper().GetDigitalData(m_clickAction).pressed;

                    if (inter.type == ButtonType::single)
                    {
                        inter.clicking = lclicking;
                        if (inter.lastClicking == true && inter.clicking != inter.lastClicking)
                        {
                            TriggerButton(element, inter, click, false);
                            TriggerButton(element, inter, hover, true);
                        }
                        else
                        {
                            currentlySelected = true;
                            TriggerButton(element, inter, hover, false);
                        }
                        inter.lastClicking = lclicking;
                    }
                    else
                    {
                        if (lclicking)
                        {
                            TriggerButton(element, inter, click, false);
                            TriggerButton(element, inter, hover, true);
                        }
                        else
                        {
                            currentlySelected = true;
                            TriggerButton(element, inter, hover, false);
                        }
                    }

                    break;
                }
                case 2:
                {
                    // slider
                    auto& inter = element.sliders.at(compo.ID);
                    auto axis = Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickLeftX);
                    if (axis > m_deadzone || axis < -m_deadzone)
                    {
                        float diff = inter.bounds.z - inter.bounds.x;
                        float& ref = *inter.linkedVar;
                        float diffperc = inter.max / (ref + (axis * inter.speed));
                        float newref = inter.max / diffperc;
                        if (newref > inter.max || newref < 0.0f)
                        {
                        }
                        else
                        {
                            ref = newref;
                            SetSliderSlideLoc(element, inter, diffperc, diff);
                        }
                    }
                    else
                    {
                        currentlySelected = true;
                    }

                    break;
                }
                default:
                {
                    Log::Critical("selected component was not correct?");
                }
            }
            if (m_controllerTimer > m_controllerDelay)
            {
                // switching between components / elements
                auto xaxis = Engine.InputWrapper()
                                 .GetAnalogData(m_buttonMovementAction)
                                 .x; /* Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickLeftX);*/
                auto yaxis = -Engine.InputWrapper()
                                  .GetAnalogData(m_buttonMovementAction)
                                  .y; /*Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickLeftY);*/
                bool changed = false;
                if (xaxis > m_deadzone)
                {
                    switch (static_cast<int>(compo.atlas.w))
                    {
                        case 1:
                        {
                            if (element.buttons.at(compo.ID).neighbours.right != -1)
                                m_CselectedComponent = element.buttons.at(compo.ID).neighbours.right;
                            else if (element.neighbours.right != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialright;
                            }

                            break;
                        }
                        case 2:
                        {
                            if (element.sliders.at(compo.ID).neighbours.right != -1)
                                m_CselectedComponent = element.sliders.at(compo.ID).neighbours.right;
                            else if (element.neighbours.right != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialright;
                            }
                            break;
                        }
                        default:
                        {
                            Log::Critical("selected component was not correct?");
                        }
                    }
                    changed = true;
                }
                else if (xaxis < -m_deadzone)
                {
                    switch (static_cast<int>(compo.atlas.w))
                    {
                        case 1:
                        {
                            if (element.buttons.at(compo.ID).neighbours.left != -1)
                                m_CselectedComponent = element.buttons.at(compo.ID).neighbours.left;
                            else if (element.neighbours.left != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialleft;
                            }
                            break;
                        }
                        case 2:
                        {
                            if (element.sliders.at(compo.ID).neighbours.left != -1)
                                m_CselectedComponent = element.sliders.at(compo.ID).neighbours.left;
                            else if (element.neighbours.left != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialleft;
                            }
                            break;
                        }
                        default:
                        {
                            Log::Critical("selected component was not correct?");
                        }
                    }
                    changed = true;
                }
                else if (yaxis < -m_deadzone)
                {
                    switch (static_cast<int>(compo.atlas.w))
                    {
                        case 1:
                        {
                            if (element.buttons.at(compo.ID).neighbours.top != -1)
                                m_CselectedComponent = element.buttons.at(compo.ID).neighbours.top;
                            else if (element.neighbours.top != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialtop;
                            }
                            break;
                        }
                        case 2:
                        {
                            if (element.sliders.at(compo.ID).neighbours.top != -1)
                                m_CselectedComponent = element.sliders.at(compo.ID).neighbours.top;
                            else if (element.neighbours.top != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialtop;
                            }
                            break;
                        }
                        default:
                        {
                            Log::Critical("selected component was not correct?");
                        }
                    }

                    changed = true;
                }
                else if (yaxis > m_deadzone)
                {
                    switch (static_cast<int>(compo.atlas.w))
                    {
                        case 1:
                        {
                            if (element.buttons.at(compo.ID).neighbours.bottom != -1)
                                m_CselectedComponent = element.buttons.at(compo.ID).neighbours.bottom;
                            else if (element.neighbours.bottom != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialbottom;
                            }
                            break;
                        }
                        case 2:
                        {
                            if (element.sliders.at(compo.ID).neighbours.bottom != -1)
                                m_CselectedComponent = element.sliders.at(compo.ID).neighbours.bottom;
                            else if (element.neighbours.bottom != entt::null)
                            {
                                m_CselectedComponent = element.neighbours.initialbottom;
                            }
                            break;
                        }
                        default:
                        {
                            Log::Critical("selected component was not correct?");
                        }
                    }
                    changed = true;
                }
                if (changed)
                {
                    m_controllerTimer = 0.0f;
                }
            }
            break;
        }
        case mouseAndKeyboard:
        {
#ifdef BEE_INSPECTOR
            m_CselectedComponent = (overrideSel != -1) ? overrideSel : -1;
#else
            m_CselectedComponent = -1;
#endif

            // M & KB
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

            bool lclicking = Engine.Input().GetMouseButton(bee::Input::MouseButton::Left);
            for (auto [ent, element] : view.each())
            {
                if (element.input)
                {
                    auto& trans = Engine.ECS().Registry.get<Transform>(ent);
                    const auto mat = trans.WorldMatrix;
                    for (auto& interactable : element.buttons)
                    {
                        auto& inter = interactable.second;
                        glm::vec4 translatedBounds1 = mat * glm::vec4(inter.bounds.x, inter.bounds.y, 0, 1);
                        glm::vec4 translatedBounds2 = mat * glm::vec4(inter.bounds.z, inter.bounds.w, 0, 1);
                        glm::vec4 translatedBounds =
                            glm::vec4(translatedBounds1.x, translatedBounds1.y, translatedBounds2.x, translatedBounds2.y);
                        if (isInBounds(translatedBounds, newMousePos))
                        {
                            inter.clicking = lclicking;

                            if (inter.type == ButtonType::single)
                            {
                                if (inter.lastClicking == true && lclicking == false)
                                {
                                    TriggerButton(element, inter, hover, true);
                                    if (inter.hoverable)
                                    {
                                        TriggerButton(element, inter, click, false);
                                    }
                                }
                                else
                                {
                                    if (inter.hoverable)
                                    {
                                        m_CselectedComponent = inter.ID;
                                        currentlySelected = true;
                                        TriggerButton(element, inter, hover, false);
                                    }
                                }
                            }
                            else
                            {
                                if (inter.clicking)
                                {
                                    TriggerButton(element, inter, hover, true);
                                    if (inter.hoverable)
                                    {
                                        TriggerButton(element, inter, click, false);
                                    }
                                }
                                else
                                {
                                    if (inter.hoverable)
                                    {
                                        TriggerButton(element, inter, hover, false);
                                        if (inter.HoverOverlay.name != "none")
                                        {
                                            currentlySelected = true;
                                            m_CselectedComponent = inter.ID;
                                        }
                                    }
                                }
                            }
                            inter.lastClicking = lclicking;
                        }
                        else
                        {
                            if (inter.isbeingHovered)
                            {
                                if (!inter.hasbeenReplaced)
                                {
                                    ResetButton(inter.ID);
                                }
                            }
                            inter.isbeingHovered = false;
                            inter.clicking = false;
                            inter.lastClicking = false;
                        }
                    }
                    for (auto& interactable : element.sliders)
                    {
                        auto& inter = interactable.second;
                        glm::vec4 translatedBounds1 = glm::vec4(inter.bounds.x, inter.bounds.y, 0, 1) * mat;
                        glm::vec4 translatedBounds2 = glm::vec4(inter.bounds.z, inter.bounds.w, 0, 1) * mat;
                        glm::vec4 translatedBounds =
                            glm::vec4(translatedBounds1.x, translatedBounds1.y, translatedBounds2.x, translatedBounds2.y);
                        if (isInBounds(translatedBounds, newMousePos))
                        {
                            if (lclicking)
                            {
                                float perc = newMousePos.x - translatedBounds.x;
                                float diff = translatedBounds.z - translatedBounds.x;

                                float& ref = *inter.linkedVar;
                                ref = inter.max / (diff / perc);
                            }
                            else
                            {
                                m_CselectedComponent = inter.ID;
                                currentlySelected = true;
                            }
                        }
                    }
                }
            }
            break;
        }
    }

    for (auto [ent, element] : view.each())
    {
        for (auto& interactable : element.sliders)
        {
            auto& inter = interactable.second;
            if (inter.lastVal != *inter.linkedVar)
            {
                float diff = inter.bounds.z - inter.bounds.x;
                inter.lastVal = *inter.linkedVar;
                float diffperc = inter.max / inter.lastVal;
                SetSliderSlideLoc(element, inter, diffperc, diff);
            }
        }
    }
    if (m_CselectedComponent != m_lastCselectedComponent && m_CselectedComponent != -1)
    {
        const auto compo = m_componentMap.find(m_CselectedComponent)->second;
        const auto& selectedUiel = Engine.ECS().Registry.get<UIElement>(compo.element);
        UIImageElement inter;
        switch (static_cast<int>(compo.atlas.w))
        {
            case 1:  // button
            {
                inter = selectedUiel.buttons.at(compo.ID).HoverOverlay;
                break;
            }
            case 2:  // slider
            {
                inter = selectedUiel.sliders.at(compo.ID).HoverOverlay;
                break;
            }
            default:
            {
                Log::Error("tried to get overlay for interactable in update but composition was not button or slider???");
            }
        }

        auto& overlayElement = Engine.ECS().Registry.get<UIElement>(m_selectedElementOverlay);

        if (overlayElement.imageComponents.count(inter.Img) == 0)
        {
            UIImageComponent imgcomp;
            std::string name = std::to_string(static_cast<int>(overlayElement.ID));
            m_renderer->genImageComp(ImgType::rawImage, imgcomp, name);
            m_nextID++;
            imgcomp.updaterID = m_nextID;
            overlayElement.imageComponents.emplace(inter.Img, imgcomp);
        }
        m_selectedComponentOverlay.image = inter.Img;
        if (inter.name != "none") MakeOverlay(inter);
    }

    else if (m_CselectedComponent == -1 && m_lastCselectedComponent != -1)
    {
        ResetOverlay();
    }
    m_lastCselectedComponent = m_CselectedComponent;
    m_selected = currentlySelected;
    if (m_CselectedComponent != -1)
    {
        m_CselectedElement = m_componentMap.at(m_CselectedComponent).element;
    }
    else
    {
        m_CselectedElement = entt::null;
    }
    const auto stop = std::chrono::high_resolution_clock::now();
    m_UpdateTime = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count());

    m_renderer->EndFrame(execute);
}

//
// ELEMENTS
//
UIElementID UserInterface::CreateUIElement(const Alignment topOrBot, const Alignment leftOrRight)
{
    auto id = Engine.ECS().CreateEntity();
    UIElement element;
    element.topOrBot = topOrBot;
    element.leftOrRight = leftOrRight;

    m_renderer->GenElement(element);

    const std::string str0 = "UI element " + std::to_string(static_cast<int>(id));
    auto& trans = Engine.ECS().CreateComponent<Transform>(id);
    trans.Name = str0;
    element.name = str0;
    element.ID = id;
    Engine.ECS().CreateComponent<UIElement>(id, element);
    m_items.emplace(id, std::unordered_map<std::string, std::unique_ptr<Item>>());

    return id;
}
void UserInterface::DeleteUIElement(const UIElementID element) { m_toDeleteElements.push_back(element); }
void UserInterface::DeleteUIElementInternal(const UIElementID element)
{
    Engine.ECS().Registry.destroy(element);

    // delete components in map
    std::vector<UIComponentID> toDeleteComponents;
    for (const auto& component : m_componentMap)
    {
        if (component.second.element == element)
        {
            toDeleteComponents.push_back(component.first);
        }
    }
    for (const auto& deleteComponent : toDeleteComponents)
    {
        m_componentMap.erase(deleteComponent);
    }

    // delete components in the reset buffer (for the tiny chance that this happens)
    for (int i = 0; i < m_toResetButtons.size(); i++)
    {
        for (const auto& deleteComponent : toDeleteComponents)
        {
            if (m_toResetButtons.at(i) == deleteComponent)
            {
                m_toResetButtons.erase(m_toResetButtons.begin() + i);
            }
        }
    }
    for (auto& item : m_items.find(element)->second)
    {
        item.second.release();
    }
    m_items.erase(element);
}

void bee::ui::UserInterface::MakeOverlay(const UIImageElement& inter)
{
    const auto compo = m_componentMap.find(m_CselectedComponent)->second;
    const auto& selectedUiel = Engine.ECS().Registry.get<UIElement>(compo.element);
    auto view = Engine.ECS().Registry.view<UIElement>();

    auto& overlayElement = Engine.ECS().Registry.get<UIElement>(m_selectedElementOverlay);
    glm::vec4 uvs = glm::vec4(0);
    glm::vec4 oldUvs = inter.GetAtlas();
    uvs = inter.GetAtlas();
    uvs.x = oldUvs.x / m_renderer->images.at(inter.Img).width;
    uvs.y = oldUvs.w / m_renderer->images.at(inter.Img).height;
    uvs.z = oldUvs.z / m_renderer->images.at(inter.Img).width;
    uvs.w = oldUvs.y / m_renderer->images.at(inter.Img).height;

    auto& comp = overlayElement.imageComponents.find(inter.Img)->second;
    // build new selected component buffer
    comp.vertices.clear();
    comp.indices.clear();

    if (!view.empty())
    {
        glm::vec4 bounds = glm::vec4(0.0f);

        switch (static_cast<int>(compo.atlas.w))
        {
            case 1:
                // button
                bounds = selectedUiel.buttons.at(compo.ID).bounds;
                break;
            case 2:
                // slider
                bounds = selectedUiel.sliders.at(compo.ID).bounds;
                break;
            default:
                Log::Error("selected component is invalid");
        }
        addTextureComponentIndices(comp, 5u);

        float x0 = 0.0f, x1 = 0.0f, y0 = 0.0f, y1 = 0.0f;
        x0 = bounds.x;
        x1 = bounds.z;
        y1 = bounds.y;
        y0 = bounds.w;
        const float localVertices[] = {
            x0, y0, 0.01f, uvs.x, uvs.y,  // top left
            x0, y1, 0.01f, uvs.x, uvs.w,  // top right
            x1, y1, 0.01f, uvs.z, uvs.w,  // bottom right
            x1, y0, 0.01f, uvs.z, uvs.y,
        };  // bottom left
        for (const float vert : localVertices)
        {
            comp.vertices.push_back(vert);
        }

        m_renderer->ReplaceBuffers(comp.renderData, comp.vertices.size(), comp.indices.size(), &comp.vertices.at(0),
                                   &comp.indices.at(0), 5);
    }
}

void bee::ui::UserInterface::ResetOverlay()
{
    auto& overlayCompo = m_selectedComponentOverlay;
    auto& element = Engine.ECS().Registry.get<UIElement>(m_selectedElementOverlay);
    auto& comp = element.imageComponents.find(overlayCompo.image)->second;
    // build new selected component buffer
    comp.vertices.clear();
    comp.indices.clear();

    for (int i = 0; i < 20; i++)
    {
        comp.vertices.push_back(0);
    }
    for (int i = 0; i < 6; i++)
    {
        comp.indices.push_back(0);
    }

    m_renderer->ReplaceBuffers(comp.renderData, comp.vertices.size(), comp.indices.size(), &comp.vertices.at(0),
                               &comp.indices.at(0), 5);
}

void UserInterface::SetDrawStateUIelement(const UIElementID element, const bool newState) const
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);

    uiel.drawing = newState;
    uiel.input = newState;
}
bool UserInterface::GetDrawStateUIelement(const UIElementID element) const
{
    const auto& uiel = Engine.ECS().Registry.get<UIElement>(element);

    return uiel.drawing;
}

void UserInterface::SetInputStateUIelement(const UIElementID element, bool newState) const
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);

    uiel.input = newState;
}
bool UserInterface::GetInputStateUIelement(const UIElementID element) const
{
    const auto& uiel = Engine.ECS().Registry.get<UIElement>(element);

    return uiel.input;
}
void UserInterface::SetElementOpacity(const UIElementID element, const float newOpacity) const
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);
    uiel.opacity = newOpacity;
}
void UserInterface::DeleteComponent(const UIComponentID id, bool DeleteItem)
{
    if (m_componentMap.count(id) == 0)
    {
        return;
    }
    auto& compo = m_componentMap.find(id)->second;
    auto& uiel = Engine.ECS().Registry.get<UIElement>(compo.element);
    if (compo.type == ComponentType::text)  // text
    {
        uiel.textComponents.at(compo.image).freespaces.push_back(glm::vec2(compo.ID, compo.atlas.x));
        for (int i = 0; i < compo.atlas.x; i++)
        {
            for (int j = 0; j < 36; j++)
            {
                const float compValue = compo.ID + (i * 36) + j;
                if (compValue < uiel.textComponents.at(compo.image).vertices.size())
                    uiel.textComponents.at(compo.image).vertices.at(compValue) = 0;
            }
        }
        if (m_updates.count(uiel.textComponents.at(compo.image).updaterID) == 0)
        {
            Updater upper;
            upper.element = compo.element;
            upper.location = compo.image;
            upper.type = ComponentType::text;
            m_updates.emplace(uiel.textComponents.at(compo.image).updaterID, upper);
        }
    }
    else if (compo.type == ComponentType::button)  // button
    {
        uiel.buttons.erase(compo.ID);
    }
    else if (compo.type == ComponentType::slider)  // slider
    {
        DeleteComponent(static_cast<unsigned int>(compo.atlas.x));
        DeleteComponent(static_cast<unsigned int>(compo.atlas.y));

        uiel.sliders.erase(compo.ID);
    }
    else if (compo.type == ComponentType::image)  // normal texture component
    {
        for (int i = 0; i < 20; i++)
        {
            const int atVal = compo.ID + i;
            uiel.imageComponents.find(compo.image)->second.vertices.at(atVal) = 0;
        }
        uiel.imageComponents.find(compo.image)->second.freespaces.push_back(compo.ID);
        if (m_updates.count(uiel.imageComponents.find(compo.image)->second.updaterID) == 0)
        {
            Updater upper;
            upper.element = compo.element;
            upper.location = compo.image;
            upper.type = ComponentType::image;
            m_updates.emplace(uiel.imageComponents.find(compo.image)->second.updaterID, upper);
        }
    }
    else if (compo.type == ComponentType::progressbar)  // progressbar
    {
        compo.image = abs(compo.image);
        auto& bar = uiel.progressBarimageComponents.find(compo.image)->second;
        bar.freespaces.push_back(compo.ID);
        for (int i = 0; i < 64; i++)
        {
            const int atVal = compo.ID + i;
            bar.vertices.at(atVal) = 0;
        }
        if (m_updates.count(bar.updaterID) == 0)
        {
            Updater upper;
            upper.element = compo.element;
            upper.location = compo.image;
            upper.type = ComponentType::progressbar;
            m_updates.emplace(bar.updaterID, upper);
        }
        compo.image = -compo.image;
    }
    if (DeleteItem)
    {
        m_items.find(compo.element)->second.find(compo.name)->second.release();
        m_items.find(compo.element)->second.erase(compo.name);
    }
    m_componentMap.erase(id);
}
//
// TEXT
//
UIComponentID UserInterface::CreateString(const UIElementID element, int fontindex, const std::string& str,
                                          const glm::vec2 position, const float size, glm::vec4 colour, int layer,
                                          const int reserveLetters, const std::string& name)
{
    float throwaway = 0;
    float throwaway2 = 0;
    return CreateString(element, fontindex, str, position, size, colour, throwaway, throwaway2, false, layer, reserveLetters,
                        name);
}
UIComponentID UserInterface::CreateString(const UIElementID element, int fontindex, const std::string& str, glm::vec2 position,
                                          const float size, glm::vec4 colour, float& xNextPositionAdvance,
                                          float& yNextPositionLineGap, const bool fullLine, int layer, const int reserveLetters,
                                          const std::string& name)
{
    auto& el = Engine.ECS().Registry.get<UIElement>(element);
    const Font& font = m_fontHandler->GetFont(fontindex);
    if (el.textComponents.count(fontindex) == 0)
    {
        UITextComponent imgcomp;
        std::string name = std::to_string(static_cast<int>(el.ID));
        m_renderer->genTextComp(ImgType::rawImage, imgcomp, name);
        m_nextID++;
        imgcomp.updaterID = m_nextID;
        el.textComponents.emplace(fontindex, imgcomp);
    }
    UITextComponent& tComp = el.textComponents.at(fontindex);
    const float begin = static_cast<float>(tComp.vertices.size());
    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;
    float length = static_cast<float>(str.length()) + static_cast<float>(reserveLetters);
    int inplace = -1;
    int inplaceSave = -1;

    for (int i = tComp.freespaces.size(); i > 0; i--)
    {
        const int atVal = i - 1;
        const auto& space = tComp.freespaces.at(atVal);
        if (space.y >= length)
        {
            inplace = space.x;
            inplaceSave = space.x;
            length += (space.y - length);
            tComp.freespaces.erase(tComp.freespaces.begin() + i - 1);
            break;
        }
    }
    float advance = 0.0f;

    if (el.leftOrRight == right)
    {
        float preAdvance = 0.0f;
        for (const auto letter : str)
        {
            preAdvance += font.advance.at(letter) * ((font.fontSize / 10) * size);
        }
        for (const auto letter : str)
        {
            addLetter(el, tComp, font, &letter, (norWidth - position.x) - preAdvance, position.y, advance, size, inplace, layer,
                      colour);
        }
    }
    else
    {
        for (auto letter : str)
        {
            addLetter(el, tComp, font, &letter, position.x, position.y, advance, size, inplace, layer, colour);
        }
    }
    UIComponent returner;
    // send data to rendeerer
    if (inplaceSave == -1)
    {
        returner.ID = static_cast<unsigned int>(begin);
    }
    else
    {
        returner.ID = inplaceSave;
    }
    if (m_updates.count(tComp.updaterID) == 0)
    {
        Updater upper;
        upper.element = element;
        upper.location = fontindex;
        upper.type = ComponentType::text;
        m_updates.emplace(tComp.updaterID, upper);
    }
    for (int i = 0; i < reserveLetters; i++)
    {
        if (inplace != -1)
        {
            for (int j = 0; j < 36; j++)
            {
                const int atVal = inplace + j;
                tComp.vertices.at(atVal) = 0;
            }
            for (int j = 0; j < 6; j++)
            {
                tComp.indices.at(std::floor(static_cast<int>((static_cast<float>(inplace) / 20) * 6) + j)) = 0;
            }
            inplace += 36;
        }
        else
        {
            for (int j = 0; j < 36; j++)
            {
                tComp.vertices.push_back(0);
            }
            for (int j = 0; j < 6; j++)
            {
                tComp.indices.push_back(0);
            }
        }
    }
    std::string actualName = "";
    if (name.empty())
        actualName = "string" + std::to_string(m_nextID);
    else
        actualName = name;
    returner.name = actualName;
    returner.element = element;
    returner.image = fontindex;
    returner.atlas.x = length;
    returner.atlas.y = size;
    returner.atlas.z = position.x;
    returner.atlas.w = position.y;
    returner.type = ComponentType::text;
    m_nextID++;
    m_componentMap.emplace(m_nextID, returner);

    xNextPositionAdvance = position.x + advance;

    switch (el.topOrBot)
    {
        case top:
        {
            if (fullLine)
            {
                yNextPositionLineGap = position.y + (font.lineSpace * ((font.fontSize / 10) * size));
            }
            else
            {
                yNextPositionLineGap = position.y + ((font.lineSpace * ((font.fontSize / 10) * size)) / 2);
            }
            break;
        }
        case bottom:
        {
            if (fullLine)
            {
                yNextPositionLineGap = position.y - (font.lineSpace * ((font.fontSize / 10) * size));
            }
            else
            {
                yNextPositionLineGap = position.y - ((font.lineSpace * ((font.fontSize / 10) * size)) / 2);
            }
            break;
        }
        case right:
        case left:
        {
            Log::Error("right/left Alignment of element is not correct");
        }
    }

    // item
    Text* item = new Text;
    item->name = actualName;
    item->element = element;
    item->nextLine = glm::vec2(xNextPositionAdvance, yNextPositionLineGap);
    item->size = size;
    item->start = position;
    item->text = str;
    item->ID = m_nextID;
    item->type = ComponentType::text;
    m_items.find(element)->second.emplace(actualName, item);

    return m_nextID;
}
glm::vec2 bee::ui::UserInterface::PreCalculateStringEnds(UIElementID element, int fontIndex, const std::string& str,
                                                         glm::vec2 position, float size, bool fullLine) const
{
    auto& el = Engine.ECS().Registry.get<UIElement>(element);
    const Font& font = m_fontHandler->GetFont(fontIndex);

    glm::vec2 end = glm::vec2(0.0f);

    end.x = position.x;

    if (el.leftOrRight == left)
    {
        for (const auto letter : str)
        {
            end.x += font.advance.at(letter) * ((font.fontSize / 10) * size);
        }
    }

    switch (el.topOrBot)
    {
        case top:
        {
            if (fullLine)
            {
                end.y = position.y + (font.lineSpace * ((font.fontSize / 10) * size));
            }
            else
            {
                end.y = position.y + ((font.lineSpace * ((font.fontSize / 10) * size)) / 2);
            }
            break;
        }
        case bottom:
        {
            if (fullLine)
            {
                end.y = position.y - (font.lineSpace * ((font.fontSize / 10) * size));
            }
            else
            {
                end.y = position.y - ((font.lineSpace * ((font.fontSize / 10) * size)) / 2);
            }
            break;
        }
    }

    return end;
}

void UserInterface::addLetter(UIElement& element, UITextComponent& comp, const Font& font, const char* letter, const float xpos,
                              const float ypos, float& advance, const float size, int& inplace, int layer, glm::vec4 c)
{
    const unsigned int letterIndex = static_cast<unsigned char>(*letter);
    if (font.advance.count(letterIndex) != 1)
    {
        Log::Error("Letter not found in the font. Are you using a diacritic/emoji/non standard ascii letter?");
        advance += font.advance.at(32u) * ((font.fontSize / 10) * size);
        return;
    }

    const float pl = font.quadPlaneBounds.at(letterIndex).x;
    const float pb = font.quadPlaneBounds.at(letterIndex).y;
    const float pr = font.quadPlaneBounds.at(letterIndex).z;
    const float pt = font.quadPlaneBounds.at(letterIndex).w;
    const float il = font.quadAtlasBounds.at(letterIndex).x;
    const float ib = font.quadAtlasBounds.at(letterIndex).y;
    const float ir = font.quadAtlasBounds.at(letterIndex).z;
    const float it = font.quadAtlasBounds.at(letterIndex).w;

    float y0 = 0.0f, y1 = 0.0f;
    // world coordinates
    switch (element.topOrBot)
    {
        case bottom:
            y1 = 1.0f - (ypos + pb * (font.fontSize / 10.0f) * size);
            y0 = 1.0f - (ypos + pt * (font.fontSize / 10.0f) * size);
            break;
        case top:
            y1 = ypos - (pb * (font.fontSize / 10.0f) * size);
            y0 = ypos - (pt * (font.fontSize / 10.0f) * size);
            break;
        case left:
        case right:
            Log::Error("element has wrong Alignment");
    }

    const float x0 = (xpos + advance + ((pl) * ((font.fontSize / 10.0f) * size)));
    const float x1 = (xpos + advance + ((pr) * ((font.fontSize / 10.0f) * size)));
    // UV Coordinates
    const float tcx0 = il / font.width;
    const float tcy0 = it / font.height;
    const float tcx1 = ir / font.width;
    const float tcy1 = ib / font.height;
    const float layercalc = static_cast<float>(layer) * 0.01f;
    const float localVertices[] = {
        x0, y0, -layercalc, tcx0, tcy0, c.x, c.y, c.z, c.w,  //
        x0, y1, -layercalc, tcx0, tcy1, c.x, c.y, c.z, c.w,  //
        x1, y1, -layercalc, tcx1, tcy1, c.x, c.y, c.z, c.w,  //
        x1, y0, -layercalc, tcx1, tcy0, c.x, c.y, c.z, c.w,
    };  // bottom left
    const unsigned int counter = comp.vertices.size() / 9u;

    if (inplace > -1)
    {
        for (int i = 0; i < 36; i++)
        {
            const int atVal = inplace + i;
            comp.vertices.at(atVal) = localVertices[i];
        }
        inplace += 36;
    }
    else
    {
        for (const float vert : localVertices)
        {
            comp.vertices.push_back(vert);
        }
        for (const unsigned int ind : DEFAULTINDICES)
        {
            comp.indices.push_back(ind + counter);
        }
    }
    advance += font.advance.at(letterIndex) * ((font.fontSize / 10) * size);
};
UIComponentID UserInterface::ReplaceString(const UIComponentID id, const std::string& str, float& xNextPositionAdvance,
                                           float& yNextPositionLineGap, const glm::vec2 position, const bool fullLine)
{
    auto compo = m_componentMap.find(id)->second;
    DeleteComponent(id, false);
    auto* t = dynamic_cast<Text*>(m_items.find(compo.element)->second.find(compo.name)->second.get());
    const auto newId = CreateString(compo.element, compo.image, str, position, compo.atlas.y, t->colour, xNextPositionAdvance,
                                    yNextPositionLineGap, fullLine, 0, 0, compo.name);
    m_componentMap.emplace(id, m_componentMap.at(newId));
    m_componentMap.erase(newId);
    m_items.find(m_componentMap.find(id)->second.element)->second.find(m_componentMap.find(id)->second.name)->second->ID = id;
    return id;
}

UIComponentID UserInterface::ReplaceString(const UIComponentID id, const std::string& str, float& xNextPositionAdvance,
                                           float& yNextPositionLineGap, const bool fullLine)
{
    const auto& compo = m_componentMap.find(id)->second;
    return ReplaceString(id, str, xNextPositionAdvance, yNextPositionLineGap, glm::vec2(compo.atlas.z, compo.atlas.w),
                         fullLine);
}
UIComponentID UserInterface::ReplaceString(const UIComponentID id, const std::string& str)
{
    float throwaway = 0.0f;
    const auto& compo = m_componentMap.find(id)->second;
    return ReplaceString(id, str, throwaway, throwaway, glm::vec2(compo.atlas.z, compo.atlas.w), false);
}

//
// IMAGE HANDLING
//

int UserInterface::LoadTexture(const std::string& imglocation) { return LoadTexture(imglocation.c_str()); }
int UserInterface::LoadTexture(const char* imglocation)
{
    std::string str0 = imglocation;
    if (m_loadedTexturesMap.count(str0) > 0)
    {
        // already loaded texture
        return m_loadedTexturesMap.find(str0)->second;
    }

    std::ifstream f(imglocation);
    if (!f.good())
    {
        const std::string str1(imglocation);
        const std::string str = "failed to get load texture at location: " + str1;
        Log::Warn(str);
        f.close();
        // return -1;
    }
    f.close();
    int x, y, c;
    unsigned char* imgData = stbi_load(imglocation, &x, &y, &c, 0);
    UIImage img;
    img.width = x;
    img.height = y;
    std::string name = std::string(imglocation);
    m_renderer->GenTexture(img, c, imgData, name);
    m_renderer->images.push_back(img);
    stbi_image_free(imgData);
    m_loadedTexturesMap.emplace(str0, m_renderer->images.size() - 1);
    return static_cast<int>(m_renderer->images.size() - 1);
}
void UserInterface::DeleteTexture(const int image) const {}

UIComponentID UserInterface::CreateTextureComponentFromAtlas(const UIElementID element, const int image,
                                                             const glm::vec4 texCoord, const glm::vec2 position,
                                                             const glm::vec2 size, int layer, const std::string& name)
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);

    if (image > m_renderer->images.size() - 1)
    {
        Log::Error("image int not found, are you passing the correct variable?");
        return 0u;
    }
    if (uiel.imageComponents.count(image) == 0)
    {
        UIImageComponent imgcomp;
        std::string name = std::to_string(static_cast<int>(uiel.ID));
        m_renderer->genImageComp(ImgType::rawImage, imgcomp, name);
        m_nextID++;
        imgcomp.updaterID = m_nextID;
        uiel.imageComponents.emplace(image, imgcomp);
    }
    glm::vec4 uvs = glm::vec4(0.0f);
    // UV Coordinates

    uvs.x = texCoord.z / m_renderer->images.at(image).width;
    uvs.w = texCoord.y / m_renderer->images.at(image).height;
    uvs.z = texCoord.x / m_renderer->images.at(image).width;
    uvs.y = texCoord.w / m_renderer->images.at(image).height;

    /* UVs.x = 1;
     UVs.y = 1;
     UVs.z = 0;
     UVs.w = 0;*/
    UIComponent id;

    if (uiel.imageComponents.find(image)->second.freespaces.size() > 0)
    {
        std::vector<float> vertices;
        addTextureComponentVertices(uiel, position.x, position.y, size.x, size.y, uvs, vertices, true, layer);
        for (int i = 0; i < vertices.size(); i++)
        {
            uiel.imageComponents.find(image)->second.vertices.at(uiel.imageComponents.find(image)->second.freespaces.back() +
                                                                 i) = vertices.at(i);
        }
        id.ID = uiel.imageComponents.find(image)->second.freespaces.back();
        uiel.imageComponents.find(image)->second.freespaces.pop_back();
    }
    else
    {
        addTextureComponentIndices(uiel.imageComponents.find(image)->second, 5u);
        id = addTextureComponentVertices(uiel, position.x, position.y, size.x, size.y, uvs,
                                         uiel.imageComponents.find(image)->second.vertices, true, layer);
    }

    if (m_updates.count(uiel.imageComponents.find(image)->second.updaterID) == 0)
    {
        Updater upper;
        upper.element = element;
        upper.location = image;
        upper.type = ComponentType::image;
        m_updates.emplace(uiel.imageComponents.find(image)->second.updaterID, upper);
    }
    std::string actualName = "";
    if (name.empty())
        actualName = "Image" + std::to_string(m_nextID);
    else
        actualName = name;
    id.name = actualName;
    id.element = element;
    id.image = image;
    id.atlas = texCoord;
    id.type = ComponentType::image;
    m_nextID++;
    m_componentMap.emplace(m_nextID, id);

    Image* item = new Image;
    item->start = position;
    item->atlas = texCoord;
    for (auto& foo : m_loadedTexturesMap)
    {
        if (foo.second == image)
        {
            item->imageFile = foo.first;
            break;
        }
    }
    item->size = size;
    item->name = actualName;
    item->ID = m_nextID;
    item->element = element;
    item->type = ComponentType::image;
    item->layer = layer;
    m_items.at(element).emplace(actualName, item);
    return m_nextID;
}

UIComponent UserInterface::addTextureComponentVertices(const UIElement& element, const float xpos, const float ypos,
                                                       const float sizex, const float sizey, const glm::vec4 UVs,
                                                       std::vector<float>& vertices, const bool useOrientation, int layer) const
{
    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;
    // world coordinates
    float x0 = 0.0f, x1 = 0.0f, y0 = 0.0f, y1 = 0.0f;
    // world coordinates

    if (!useOrientation)
    {
        x0 = xpos;
        x1 = sizex;
        y1 = ypos;
        y0 = sizey;
    }
    else
    {
        switch (element.topOrBot)
        {
            case bottom:
                y1 = 1.0f - (ypos + sizey);
                y0 = 1.0f - ypos;
                break;
            case top:
                y1 = ypos;
                y0 = sizey + ypos;
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
                x0 = norWidth - xpos;
                x1 = norWidth - (xpos + sizex);
                break;

            case left:
                x0 = sizex + xpos;
                x1 = xpos;
                break;
            case top:
            case bottom:
            {
                Log::Error("element rightOrLeft orientation is wrong");
            }
        }
    }
    const float layercalc = static_cast<float>(layer) * 0.0001f;

    float localVertices[] = {
        x0, y0, -layercalc, UVs.x, UVs.y,  // top left
        x0, y1, -layercalc, UVs.x, UVs.w,  // top right
        x1, y1, -layercalc, UVs.z, UVs.w,  // bottom right
        x1, y0, -layercalc, UVs.z, UVs.y,
    };  // bottom left
    UIComponent id;
    id.ID = vertices.size();
    for (float vert : localVertices)
    {
        vertices.push_back(vert);
    }

    return id;
}

void UserInterface::addTextureComponentIndices(UIImageComponent& img, const unsigned int verticesOffset) const
{
    const unsigned int counter = img.vertices.size() / verticesOffset;
    for (const unsigned int ind : DEFAULTINDICES)
    {
        img.indices.push_back(ind + counter);
    }
}

UIComponentID UserInterface::ReplaceAtlasPostition(const UIComponentID id, const float pixcoordx, const float pixcoordy,
                                                   const float pixcoordx2, const float pixcoordy2)
{
    auto& compo = m_componentMap.find(id)->second;
    auto& element = Engine.ECS().Registry.get<UIElement>(compo.element);
    dynamic_cast<Image*>(m_items.at(compo.element).at(compo.name).get())->atlas =
        glm::vec4(pixcoordx, pixcoordy, pixcoordx2, pixcoordy2);

    std::vector<float> newData;
    const glm::vec4 uv = glm::vec4(
        pixcoordx / m_renderer->images.at(compo.image).width, pixcoordy / m_renderer->images.at(compo.image).height,
        pixcoordx2 / m_renderer->images.at(compo.image).width, pixcoordy2 / m_renderer->images.at(compo.image).height);

    const int plus1 = compo.ID + 1;
    const int plus2 = compo.ID + 2;
    const int plus6 = compo.ID + 6;
    const int plus10 = compo.ID + 10;
    addTextureComponentVertices(element, element.imageComponents.find(compo.image)->second.vertices.at(compo.ID),
                                element.imageComponents.find(compo.image)->second.vertices.at(plus1),
                                element.imageComponents.find(compo.image)->second.vertices.at(plus10),
                                element.imageComponents.find(compo.image)->second.vertices.at(plus6), uv, newData, false,
                                std::abs(element.imageComponents.find(compo.image)->second.vertices.at(plus2) * 10000));
    m_renderer->ReplaceImg(element, compo, newData);
    return id;
}

//
// INTERACTABLES
//

bool UserInterface::isInBounds(const glm::vec4 bounds, const glm::vec2 position) const
{
    if (position.x > bounds.x && position.x < bounds.z && position.y > bounds.y && position.y < bounds.w) return true;
    if (position.x < bounds.x && position.x > bounds.z && position.y > bounds.y && position.y < bounds.w) return true;
    if (position.x < bounds.x && position.x > bounds.z && position.y < bounds.y && position.y > bounds.w) return true;
    if (position.x > bounds.x && position.x < bounds.z && position.y < bounds.y && position.y > bounds.w) return true;
    return false;
}

void UserInterface::SetInputMode(const InputMode newMode) { m_inputMode = newMode; }
InputMode UserInterface::GetInputMode() const { return m_inputMode; }

void UserInterface::SetComponentNeighbours(const UIComponentID id, const ControllerComponentNeighbours& neighbourStruct) const
{
    const auto& compo = m_componentMap.find(id)->second;
    auto& el = Engine.ECS().Registry.get<UIElement>(compo.element);
    switch (static_cast<int>(compo.atlas.w))
    {
        case 1:
            // button
            el.buttons.at(compo.ID).neighbours = neighbourStruct;
            break;
        case 2:
            // slider
            el.sliders.at(compo.ID).neighbours = neighbourStruct;
            break;
        default:
        {
            Log::Error("composition was wrong when trying to set component neighbours");
        }
    }
}
ControllerComponentNeighbours UserInterface::GetNeighbours(const UIComponentID id) const
{
    const auto& compo = m_componentMap.find(id)->second;
    const auto& el = Engine.ECS().Registry.get<UIElement>(compo.element);
    switch (static_cast<int>(compo.atlas.w))
    {
        case 1:
            // button
            return el.buttons.at(compo.ID).neighbours;
        case 2:
            // slider
            return el.sliders.at(compo.ID).neighbours;
        default:
        {
            Log::Error("composition was wrong when getting neighbours");
        }
    }
    ControllerComponentNeighbours null;
    null.top = -1;
    null.right = -1;
    null.bottom = -1;
    null.left = -1;
    return null;
}
void UserInterface::SetElementNeighbours(const UIElementID element, const ControllerElementNeighbours& neighbourStruct) const
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);
    uiel.neighbours = neighbourStruct;
}
ControllerElementNeighbours UserInterface::GetElementNeighbours(const UIElementID element) const
{
    return Engine.ECS().Registry.get<UIElement>(element).neighbours;
}
void UserInterface::SetSelectedInteractable(const UIComponentID ID) { m_CselectedComponent = ID; }

void UserInterface::SwitchInteractable(const Alignment direction)
{
    const auto& compo = m_componentMap.find(m_CselectedComponent)->second;
    const auto& element = Engine.ECS().Registry.get<UIElement>(m_componentMap.find(m_CselectedComponent)->second.element);
    switch (direction)
    {
        case right:
        {
            switch (static_cast<int>(compo.atlas.w))
            {
                case 1:  // buttons
                {
                    m_CselectedComponent = element.buttons.at(compo.ID).neighbours.right;
                    break;
                }
                case 2:  // sliders
                {
                    m_CselectedComponent = element.sliders.at(compo.ID).neighbours.right;
                    break;
                }
                default:
                {
                    Log::Error("tried to switch interactable but composition was not button or slider");
                }
            }
            break;
        }
        case left:
        {
            switch (static_cast<int>(compo.atlas.w))
            {
                case 1:  // buttons
                {
                    m_CselectedComponent = element.buttons.at(compo.ID).neighbours.left;
                    break;
                }
                case 2:  // sliders
                {
                    m_CselectedComponent = element.sliders.at(compo.ID).neighbours.left;
                    break;
                }
                default:
                {
                    Log::Error("tried to switch interactable but composition was not button or slider");
                }
            }
            break;
        }
        case top:
        {
            switch (static_cast<int>(compo.atlas.w))
            {
                case 1:  // buttons
                {
                    m_CselectedComponent = element.buttons.at(compo.ID).neighbours.top;
                    break;
                }
                case 2:  // sliders
                {
                    m_CselectedComponent = element.sliders.at(compo.ID).neighbours.top;
                    break;
                }
                default:
                {
                    Log::Error("tried to switch interactable but composition was not button or slider");
                }
            }
            break;
        }
        case bottom:
        {
            switch (static_cast<int>(compo.atlas.w))
            {
                case 1:  // buttons
                {
                    m_CselectedComponent = element.buttons.at(compo.ID).neighbours.bottom;
                    break;
                }
                case 2:  // sliders
                {
                    m_CselectedComponent = element.sliders.at(compo.ID).neighbours.bottom;
                    break;
                }
                default:
                {
                    Log::Error("tried to switch interactable but composition was not button or slider");
                }
            }
            break;
        }
    }
}
void UserInterface::SetComponentNeighbourInDirection(const UIComponentID id, const UIComponentID neighbourid,
                                                     const Alignment direction) const
{
    const auto& compo = m_componentMap.find(id)->second;
    auto& element = Engine.ECS().Registry.get<UIElement>(compo.element);

    switch (static_cast<int>(compo.atlas.w))
    {
        case 1:  // buttons
        {
            switch (direction)
            {
                case right:
                {
                    element.buttons.at(compo.ID).neighbours.right = neighbourid;
                    break;
                }
                case left:
                {
                    element.buttons.at(compo.ID).neighbours.left = neighbourid;
                    break;
                }
                case top:
                {
                    element.buttons.at(compo.ID).neighbours.top = neighbourid;
                    break;
                }
                case bottom:
                {
                    element.buttons.at(compo.ID).neighbours.bottom = neighbourid;
                    break;
                }
            }
            break;
        }
        case 2:  // sliders
        {
            switch (direction)
            {
                case right:
                {
                    element.sliders.at(compo.ID).neighbours.right = neighbourid;
                    break;
                }
                case left:
                {
                    element.sliders.at(compo.ID).neighbours.left = neighbourid;
                    break;
                }
                case top:
                {
                    element.sliders.at(compo.ID).neighbours.top = neighbourid;
                    break;
                }
                case bottom:
                {
                    element.sliders.at(compo.ID).neighbours.bottom = neighbourid;
                    break;
                }
            }
            break;
        }
        default:
        {
            Log::Error("tried to set neighbour for a interactable but composition was not button or slider");
        }
    }
}

// buttons
UIComponentID UserInterface::CreateButton(const UIElementID element, const glm::vec2 position, const glm::vec2 size,
                                          const interaction& onClick, const interaction& onHover, const ButtonType type,
                                          unsigned int hoverimg, const bool hoverable, const UIComponentID linkedComponent,
                                          const std::string& name)
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);

    Button inter;
    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;
    inter.hoverable = hoverable;
    if (hoverimg != -1)
    {
        inter.HoverOverlay = m_overlaymap.find(hoverimg)->second;
    }
    else
    {
        inter.HoverOverlay.name = "none";
    }

    switch (uiel.topOrBot)
    {
        case bottom:
            inter.bounds.y = 1.0f - (position.y + size.y);
            inter.bounds.w = 1.0f - (position.y);
            break;
        case top:
            inter.bounds.y = position.y;
            inter.bounds.w = position.y + size.y;
            break;
        case right:
        case left:
        {
            Log::Error("top/bot Alignment of element is not correct");
        }
    }
    switch (uiel.leftOrRight)
    {
        case right:
            inter.bounds.x = norWidth - (position.x + size.x);
            inter.bounds.z = norWidth - position.x;
            break;
        case left:
            inter.bounds.x = position.x;
            inter.bounds.z = position.x + size.x;
            break;
        case top:
        case bottom:
        {
            Log::Error("right/left Alignment of element is not correct");
        }
    }
    inter.type = type;
    if (m_componentMap.count(linkedComponent) == 0 || linkedComponent == -1)
    {
        // no interaction changing so
    }
    else
    {
        const auto& comp = m_componentMap.find(linkedComponent)->second;

        if (comp.image == -2)
        {
            // linked to text
            inter.onReset = none;
        }
        else
        {
            // image with atlas component
            inter.onReset = newAtlasPos;
        }
        if (onClick.type != SwitchType::none)
        {
            inter.atlasOnClick = onClick.newAtlas;
        }
        if (onHover.type != SwitchType::none)
        {
            inter.atlasOnHover = onHover.newAtlas;
        }
    }
    inter.onClick = onClick.type;
    if (onClick.funcName.size() != 0 && onClick.funcName != "none")
        inter.buttonFuncIndexClick = serialiser->functions.at(onClick.funcName);

    inter.onHover = onHover.type;
    if (onHover.funcName.size() != 0 && onHover.funcName != "none")
        inter.buttonFuncIndexHover = serialiser->functions.at(onHover.funcName);

    // inter.mid = glm::vec2(bounds.z - bounds.x, bounds.w - bounds.y);
    std::string actualName = "";
    if (name.empty())
        actualName = "Slider" + std::to_string(m_nextID);
    else
        actualName = name;

    UIComponent returner;
    returner.name = actualName;
    returner.element = element;
    returner.image = -200000;  // -200000 = button
    returner.atlas.w = 1;
    m_nextID++;
    inter.ID = m_nextID;
    uiel.buttons.emplace(m_nextID, inter);

    returner.ID = m_nextID;
    returner.type = ComponentType::button;
    m_componentMap.emplace(m_nextID, returner);

    sButton* item = new sButton;
    item->click = onClick;
    item->hover = onHover;
    item->hoverable = hoverable;
    for (auto over : serialiser->overlays)
    {
        if (over.second == hoverimg)
        {
            item->hoverimg = over.first;
            break;
        }
    }
    if (linkedComponent != -1)
    {
        item->linkedComponent = linkedComponent;
        item->linkedComponentstr = m_componentMap.find(linkedComponent)->second.name;
    }
    item->size = size;
    item->start = position;
    item->typein = type;
    item->element = element;
    item->ID = m_nextID;
    item->type = ComponentType::button;
    item->name = actualName;
    m_items.at(element).emplace(item->name, item);
    return m_nextID;
}

void bee::ui::UserInterface::SetUiInputActions(const std::string& buttonMovementAction, const std::string& clickAction)
{
    m_buttonMovementAction = buttonMovementAction;
    m_clickAction = clickAction;
}

void bee::ui::UserInterface::SetElementLayer(const UIElementID element, const int layer) const
{
    Engine.ECS().Registry.get<Transform>(element).Translation.z = static_cast<float>(layer) / 100.0f;
}

int bee::ui::UserInterface::GetElementLayer(const UIElementID element) const
{
    return static_cast<int>(Engine.ECS().Registry.get<Transform>(element).Translation.z * 100.0f);
}

void bee::ui::UserInterface::Clean()
{
    auto view = Engine.ECS().Registry.view<UIElement>();
    for (auto [ent, el] : view.each())
    {
        if (ent != m_selectedElementOverlay) m_toDeleteElements.push_back(ent);
    }
    m_componentMap.clear();
    m_toResetButtons.clear();
    m_CselectedComponent = -1;
    m_lastCselectedComponent = -1;
    m_CselectedComponent = -1;

    auto& element = Engine.ECS().Registry.get<UIElement>(m_selectedElementOverlay);
    element.imageComponents.clear();
}

void UserInterface::TriggerButton(UIElement& element, Button& inter, const interactionType type, const bool funcOnly)
{
    switch (type)
    {
        case click:
        {
            // clicking
            if (inter.buttonFuncIndexClick != -1)
            {
                ButtonFuncs.at(inter.buttonFuncIndexClick)->Func(element.ID, inter.ID);
            }
            if (!funcOnly)
            {
                // stop replacing it after
                if (!inter.hasbeenReplaced)
                {
                    SwitchButtonLook(element, inter.linkedComponent, inter, interactionType::click);
                }
            }
            break;
        }
        case hover:
        {
            // hovering
            if (inter.buttonFuncIndexHover != -1)
            {
                ButtonFuncs.at(inter.buttonFuncIndexHover)->Func(element.ID, inter.ID);
            }
            if (!funcOnly)
            {
                SwitchButtonLook(element, inter.linkedComponent, inter, interactionType::hover);
            }
            break;
        }
        case reset:
        {
            Log::Warn("passed reset on trigger button?");
        }
    }
}
void UserInterface::TriggerButton(const UIComponentID buttonid, const interactionType type)
{
    const auto compo = m_componentMap.find(buttonid)->second;
    auto& uiel = Engine.ECS().Registry.get<UIElement>(compo.element);

    auto& inter = uiel.buttons.at(compo.ID);
    TriggerButton(uiel, inter, type, false);
}
void UserInterface::SwitchButtonLook(UIElement& element, const UIComponentID id, Button& butt, const interactionType type)
{
    switch (type)
    {
        case hover:
        {
            if (!butt.isbeingHovered)
            {
                switch (butt.onHover)
                {
                    case newAtlasPos:
                    {
                        ReplaceAtlasPostition(id, butt.atlasOnHover.x, butt.atlasOnHover.y, butt.atlasOnHover.z,
                                              butt.atlasOnHover.w);
                        butt.isbeingHovered = true;
                        break;
                    }
                    case none:
                    {
                        // Log::Error("tried to swith button look but type was none?");
                    }
                }
            }
            else
            {
                // hover has been replaced, so no more replacing :)
            }
            break;
        }
        case click:
        {
            switch (butt.onClick)
            {
                case newAtlasPos:
                {
                    ReplaceAtlasPostition(id, butt.atlasOnClick.x, butt.atlasOnClick.y, butt.atlasOnClick.z,
                                          butt.atlasOnClick.w);
                    butt.hasbeenReplaced = true;
                    break;
                }
                case none:
                {
                    // Log::Error("tried to swith button look but type was none?");
                }
            }
            break;
        }
        case reset:
        {
            switch (butt.onReset)
            {
                case newAtlasPos:
                {
                    ReplaceAtlasPostition(id, m_componentMap.find(butt.linkedComponent)->second.atlas.x,
                                          m_componentMap.find(butt.linkedComponent)->second.atlas.y,
                                          m_componentMap.find(butt.linkedComponent)->second.atlas.z,
                                          m_componentMap.find(butt.linkedComponent)->second.atlas.w);
                    butt.hasbeenReplaced = false;
                    break;
                }
                case none:
                {
                    // Log::Error("tried to swith button look but type was none?");
                }
            }
            break;
        }
    }
}
void UserInterface::ResetButton(const UIComponentID buttonid) { m_toResetButtons.push_back(buttonid); }
void UserInterface::ResetButtonInternal(const UIComponentID id)
{
    const auto& comp = m_componentMap.find(id)->second;
    auto& uiel = Engine.ECS().Registry.get<UIElement>(comp.element);
    const auto& inter = uiel.buttons.at(comp.ID);

    SwitchButtonLook(uiel, uiel.buttons.at(m_componentMap.find(id)->second.ID).linkedComponent,
                     uiel.buttons.at(m_componentMap.find(id)->second.ID), interactionType::reset);
}
bool UserInterface::GetButtonState(const UIComponentID buttonid) const
{
    const auto& comp = m_componentMap.find(buttonid)->second;
    const auto& uiel = Engine.ECS().Registry.get<UIElement>(comp.element);

    return uiel.buttons.at(comp.ID).hasbeenReplaced;
}

bool bee::ui::UserInterface::GetButtonLastClick(UIComponentID buttonid) const
{
    const auto& comp = m_componentMap.find(buttonid)->second;
    const auto& uiel = Engine.ECS().Registry.get<UIElement>(comp.element);

    return uiel.buttons.at(comp.ID).lastClicking;
}

void bee::ui::UserInterface::SetButtonLastClick(UIComponentID buttonid, bool newState) const
{
    const auto& comp = m_componentMap.find(buttonid)->second;
    auto& uiel = Engine.ECS().Registry.get<UIElement>(comp.element);

    uiel.buttons.at(comp.ID).lastClicking = newState;
}

// sliders
UIComponentID UserInterface::CreateSlider(const UIElementID element, float& var, const float max, const float speed,
                                          const glm::vec2 position, const glm::vec2 size, const glm::vec2 slideSize,
                                          const int imgAtlas, const glm::vec4 atlasForBar, const glm::vec4 atlasForSlider,
                                          int layer, const std::string& name)
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);

    Slider inter;
    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;
    switch (uiel.topOrBot)
    {
        case bottom:
            inter.bounds.y = 1.0f - position.y;
            inter.bounds.w = 1.0f - position.y + size.y;
            break;
        case top:
            inter.bounds.y = position.y;
            inter.bounds.w = position.y + size.y;
            break;
        case right:
        case left:
        {
            Log::Error("top/bot Alignment of element is not correct");
        }
    }
    switch (uiel.leftOrRight)
    {
        case right:
            inter.bounds.x = norWidth - position.x - size.x;
            inter.bounds.z = norWidth - position.x;
            break;
        case left:
            inter.bounds.x = position.x;
            inter.bounds.z = position.x + size.x;
            break;
        case top:
        case bottom:
        {
            Log::Error("element rightOrLeft orientation is wrong");
        }
    }
    inter.slideSize = glm::vec2(slideSize.x, slideSize.y);
    inter.linkedVar = &var;

    inter.img = imgAtlas;
    inter.max = max;
    inter.linkedElement = element;

    // add vertex data to image
    if (imgAtlas > m_renderer->images.size() - 1)
    {
        Log::Error("image passed onto font is not within the image vector");
        return 0u;
    }
    // UV Coordinates
    inter.atlasForBar.x = atlasForBar.x / m_renderer->images.at(imgAtlas).width;
    inter.atlasForBar.y = atlasForBar.y / m_renderer->images.at(imgAtlas).height;
    inter.atlasForBar.z = atlasForBar.z / m_renderer->images.at(imgAtlas).width;
    inter.atlasForBar.w = atlasForBar.w / m_renderer->images.at(imgAtlas).height;
    inter.atlasForSlider.x = atlasForSlider.x / m_renderer->images.at(imgAtlas).width;
    inter.atlasForSlider.y = atlasForSlider.y / m_renderer->images.at(imgAtlas).height;
    inter.atlasForSlider.z = atlasForSlider.z / m_renderer->images.at(imgAtlas).width;
    inter.atlasForSlider.w = atlasForSlider.w / m_renderer->images.at(imgAtlas).height;

    auto barid = CreateTextureComponentFromAtlas(uiel.ID, imgAtlas, atlasForBar, position, size, layer);
    UIComponentID sliderID;
    switch (uiel.leftOrRight)
    {
        case right:
            sliderID = CreateTextureComponentFromAtlas(uiel.ID, imgAtlas, atlasForSlider,
                                                       glm::vec2(position.x - (slideSize.x / 2) + size.x, position.y),
                                                       glm::vec2(slideSize.x, slideSize.y), layer);
            break;
        case left:
            sliderID = CreateTextureComponentFromAtlas(uiel.ID, imgAtlas, atlasForSlider,
                                                       glm::vec2(position.x - (slideSize.x / 2), position.y),
                                                       glm::vec2(slideSize.x, slideSize.y), layer);
            break;
        case top:
        case bottom:
        {
            Log::Error("element rightOrLeft orientation is wrong");
        }
    }

    inter.locInVertexArray = m_componentMap.find(sliderID)->second.ID;
    // inter.mid = glm::vec2(barBounds.z - position.x, barBounds.w - barBounds.y);

    UIComponent component;
    component.image = -300000;
    component.element = element;
    component.atlas.x = static_cast<float>(barid);
    component.atlas.y = static_cast<float>(sliderID);
    component.atlas.w = 2.0f;
    component.type = ComponentType::slider;
    m_nextID++;
    inter.ID = m_nextID;
    uiel.sliders.emplace(uiel.sliders.size(), inter);
    component.ID = uiel.sliders.size() - 1u;
    std::string actualName = "";
    if (name.empty())
        actualName = "Slider" + std::to_string(m_nextID);
    else
        actualName = name;
    component.name = actualName;
    m_componentMap.emplace(m_nextID, component);

    sSlider* item = new sSlider;
    item->name = actualName;
    item->element = element;
    for (auto& foo : m_loadedTexturesMap)
    {
        if (foo.second == imgAtlas)
        {
            item->imageFile = foo.first;
            break;
        }
    }
    item->start = position;
    item->barAtlas = atlasForBar;
    item->max = max;
    item->size = size;
    item->slidAtlas = atlasForSlider;
    item->slidSize = slideSize;
    item->speed = speed;
    item->ID = m_nextID;
    item->layer = layer;
    item->type = ComponentType::slider;
    m_items.at(element).emplace(actualName, item);
    return m_nextID;
}

float UserInterface::GetSliderMax(const UIComponentID ID) const
{
    const auto& comp = m_componentMap.find(ID)->second;
    const auto& uiel = Engine.ECS().Registry.get<UIElement>(comp.element);
    const auto& slider = uiel.sliders.at(comp.ID);

    return slider.max;
}

UIComponentID bee::ui::UserInterface::CreateSlider(UIElementID element, float max, float speed, glm::vec2 position,
                                                   glm::vec2 size, glm::vec2 slideSize, int imgAtlas, glm::vec4 atlasForBar,
                                                   glm::vec4 atlasForSlider, int layer, const std::string& name)
{
    float varthrowawy = 0.0f;
    return CreateSlider(element, varthrowawy, max, speed, position, size, slideSize, imgAtlas, atlasForBar, atlasForSlider,
                        layer, name);
}

void bee::ui::UserInterface::SetSliderVar(UIComponentID ID, float& var) const
{
    const auto& comp = m_componentMap.find(ID)->second;
    auto& uiel = Engine.ECS().Registry.get<UIElement>(comp.element);
    auto& slider = uiel.sliders.at(comp.ID);
    slider.linkedVar = &var;
}

void UserInterface::SetSliderMax(const UIComponentID ID, const float newMax) const
{
    const auto& compo = m_componentMap.find(ID)->second;
    auto& el = Engine.ECS().Registry.get<UIElement>(compo.element);
    dynamic_cast<sSlider*>(m_items.at(compo.element).at(compo.name).get())->max = newMax;

    el.sliders.at(compo.ID).max = newMax;
}
void UserInterface::SetSliderSpeed(const UIComponentID ID, const float newSpeed) const
{
    const auto& compo = m_componentMap.find(ID)->second;
    auto& el = Engine.ECS().Registry.get<UIElement>(compo.element);
    dynamic_cast<sSlider*>(m_items.at(compo.element).at(compo.name).get())->speed = newSpeed;

    el.sliders.at(compo.ID).speed = newSpeed;
}

void UserInterface::SetSliderSlideLoc(UIElement& element, const Slider& inter, const float diffperc, const float diff)
{
    const float rawPercentage = ((100.0f / diffperc) / 100.0f);
    const float adder = diff * rawPercentage - (0.5f * inter.slideSize.x);
    auto& comp = element.imageComponents.find(inter.img)->second;

    std::vector<float> newData;
    addTextureComponentVertices(element, inter.bounds.x + adder, inter.bounds.y, inter.bounds.x + adder + inter.slideSize.x,
                                inter.bounds.y + inter.slideSize.y, inter.atlasForSlider, newData, false,
                                std::abs(comp.vertices.at(inter.locInVertexArray + 2) * 10000));
    for (int i = 0; i < newData.size(); i++)
    {
        const int atVal = inter.locInVertexArray + i;
        comp.vertices.at(atVal) = newData[i];
    }
    if (m_updates.count(comp.updaterID) == 0)
    {
        Updater upper;
        upper.element = element.ID;
        upper.location = inter.img;
        upper.type = ComponentType::image;
        m_updates.emplace(comp.updaterID, upper);
    }
}

UIComponentID UserInterface::CreateProgressBar(const UIElementID element, const glm::vec2 position, const glm::vec2 size,
                                               const int shapeImg, const glm::vec4 texCoord, const glm::vec4 foreGroundColour,
                                               const glm::vec4 backGroundColour, int layer, float value,
                                               const std::string& name)
{
    auto& uiel = Engine.ECS().Registry.get<UIElement>(element);
    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;
    UIComponent id;
    if (uiel.progressBarimageComponents.count(shapeImg) == 0)
    {
        UIImageComponent imgComp;
        std::string name = std::to_string(static_cast<int>(uiel.ID));
        m_renderer->genImageComp(ImgType::ProgressBarBackground, imgComp, name);
        m_nextID++;
        imgComp.updaterID = m_nextID;
        uiel.progressBarimageComponents.emplace(shapeImg, imgComp);
    }

    auto& imgComp = uiel.progressBarimageComponents.find(shapeImg)->second;
    std::vector<float> vertices;
    glm::vec4 uvs = glm::vec4(0.0f);

    uvs.x = texCoord.x / m_renderer->images.at(shapeImg).width;
    uvs.w = texCoord.y / m_renderer->images.at(shapeImg).height;
    uvs.z = texCoord.z / m_renderer->images.at(shapeImg).width;
    uvs.y = texCoord.w / m_renderer->images.at(shapeImg).height;

    addTextureComponentVertices(uiel, position.x, position.y, size.x, size.y, uvs, vertices, true, layer);
    const int finalSize = (vertices.size() / 5) * 11 + vertices.size();
    for (int i = 5; i < finalSize; i += 16)
    {
        if (uiel.leftOrRight == right)
        {
            vertices.insert(vertices.begin() + i, norWidth - (position.x + size.x));
        }
        else
        {
            vertices.insert(vertices.begin() + i, position.x);
        }
        vertices.insert(vertices.begin() + i + 1, size.x);
        vertices.insert(vertices.begin() + i + 2, value);
        // bColor
        vertices.insert(vertices.begin() + i + 3, backGroundColour.x);
        vertices.insert(vertices.begin() + i + 4, backGroundColour.y);
        vertices.insert(vertices.begin() + i + 5, backGroundColour.z);
        vertices.insert(vertices.begin() + i + 6, backGroundColour.w);
        // fColour
        vertices.insert(vertices.begin() + i + 7, foreGroundColour.x);
        vertices.insert(vertices.begin() + i + 8, foreGroundColour.y);
        vertices.insert(vertices.begin() + i + 9, foreGroundColour.z);
        vertices.insert(vertices.begin() + i + 10, foreGroundColour.w);
    }
    if (uiel.progressBarimageComponents.find(shapeImg)->second.freespaces.size() > 0)
    {
        id.ID = uiel.progressBarimageComponents.find(shapeImg)->second.freespaces.back();
        for (int i = 0; i < vertices.size(); i++)
        {
            imgComp.vertices.at(uiel.progressBarimageComponents.find(shapeImg)->second.freespaces.back() + i) = vertices.at(i);
        }
        uiel.progressBarimageComponents.find(shapeImg)->second.freespaces.pop_back();
    }
    else
    {
        addTextureComponentIndices(imgComp, 16u);
        id.ID = imgComp.vertices.size();
        for (const auto& vert : vertices)
        {
            imgComp.vertices.push_back(vert);
        }
    }
    id.image = -shapeImg;
    id.element = element;
    id.type = ComponentType::progressbar;
    if (m_updates.count(imgComp.updaterID) == 0)
    {
        Updater upper;
        upper.element = element;
        upper.location = abs(shapeImg);
        upper.type = ComponentType::progressbar;
        m_updates.emplace(imgComp.updaterID, upper);
    }
    m_nextID++;
    std::string actualName = "";
    if (name.empty())
        actualName = "ProgressBar" + std::to_string(m_nextID);
    else
        actualName = name;
    id.name = actualName;
    m_componentMap.emplace(m_nextID, id);

    sProgressBar* item = new sProgressBar;
    for (auto& foo : m_loadedTexturesMap)
    {
        if (foo.second == shapeImg)
        {
            item->imageFile = foo.first;
            break;
        }
    }
    item->start = position;
    item->atlas = texCoord;
    item->bColor = backGroundColour;
    item->fColor = foreGroundColour;
    item->size = size;
    item->value = value;
    item->name = actualName;
    item->ID = m_nextID;
    item->type = ComponentType::progressbar;
    m_items.at(element).emplace(actualName, item);
    return m_nextID;
}

void UserInterface::SetProgressBarValue(const UIComponentID barid, const float newValue)
{
    auto compo = m_componentMap.find(barid)->second;
    auto& uiel = Engine.ECS().Registry.get<UIElement>(compo.element);
    compo.image = abs(compo.image);

    for (int i = 7; i < 64; i += 16)
    {
        const int atVal = compo.ID + i;
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(atVal) = newValue;
    }
    auto& bar = uiel.progressBarimageComponents.find(compo.image)->second;
    if (m_updates.count(bar.updaterID) == 0)
    {
        Updater upper;
        upper.element = compo.element;
        upper.location = compo.image;
        upper.type = ComponentType::progressbar;
        m_updates.emplace(bar.updaterID, upper);
    }
    compo.image = -compo.image;
}

void UserInterface::SetProgressBarColors(const UIComponentID barid, const glm::vec4 foreGroundColour,
                                         const glm::vec4 backgroundColour)
{
    auto compo = m_componentMap.find(barid)->second;
    auto& uiel = Engine.ECS().Registry.get<UIElement>(compo.element);
    compo.image = abs(compo.image);

    for (unsigned int i = 8; i < 64; i += 16)
    {
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i) = backgroundColour.x;
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i + 1u) = backgroundColour.y;
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i + 2) = backgroundColour.z;
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i + 3) = backgroundColour.w;

        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i + 4) = foreGroundColour.x;
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i + 5) = foreGroundColour.y;
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i + 6) = foreGroundColour.z;
        uiel.progressBarimageComponents.find(compo.image)->second.vertices.at(compo.ID + i + 7) = foreGroundColour.w;
    }
    auto& bar = uiel.progressBarimageComponents.find(compo.image)->second;
    if (m_updates.count(bar.updaterID) == 0)
    {
        Updater upper;
        upper.element = compo.element;
        upper.location = compo.image;
        upper.type = ComponentType::progressbar;
        m_updates.emplace(bar.updaterID, upper);
    }
    compo.image = -compo.image;
}
unsigned int UserInterface::AddOverlayImage(const UIImageElement& image)
{
    m_nextID++;
    m_overlaymap.emplace(m_nextID, image);
    serialiser->overlays.emplace(image.name, m_nextID);
    return m_nextID;
}

unsigned int bee::ui::UserInterface::GetOverlayIndex(const std::string& name) const
{
    if (serialiser->overlays.count(name) == 0) return -1;
    return serialiser->overlays.at(name);
}

UIComponentID bee::ui::UserInterface::GetComponentID(const UIElementID elementID, const std::string& key) const
{
    if (m_items.count(elementID) != 0)
    {
        if (m_items.at(elementID).count(key) != 0)
        {
            return m_items.at(elementID).find(key)->second->ID;
        }
        else
        {
            Log::Warn("serializer does not recognise component name:" + key);
        }
    }
    else
    {
        Log::Warn("serializer does not recognise element ID:" + std::to_string(static_cast<int>(elementID)));
    }
    return -1;
}

#ifdef BEE_INSPECTOR
#include "imgui/imgui.h"
void UserInterface::Inspect()
{
    ImGui::Begin("User interface");
    ImGui::Text("elements:");
    ImGui::SameLine();
    ImGui::Text(std::to_string(Engine.ECS().Registry.view<UIElement>().size()).c_str());
    ImGui::Text("Drawcalls:");
    ImGui::SameLine();
    ImGui::Text(std::to_string(m_renderer->drawCalls).c_str());
    m_renderer->drawCalls = 0;
    ImGui::Separator();
    glm::vec2 newMousePos;
    glm::vec2 mousePos = Engine.Input().GetMousePosition();
    const float height = static_cast<float>(Engine.Device().GetHeight());
    const float width = static_cast<float>(Engine.Device().GetWidth());
    const float norWidth = width / height;
    if (Engine.Inspector().GetVisible())
    {
        auto pos = Engine.Inspector().GetGamePos();
        auto size = Engine.Inspector().GetGameSize();
        glm::vec2 newMousePos2 = glm::vec2(mousePos.x - pos.x, mousePos.y - pos.y);
        newMousePos = glm::vec2((newMousePos2.x / size.x) * (size.x / size.y), newMousePos2.y / (size.y));
    }
    else
    {
        newMousePos = glm::vec2((mousePos.x / width) * (width / height), mousePos.y / (height));
    }

    if (ImGui::BeginCombo("##combo", m_currentItem))
    {
        for (auto& selectable : m_orientations)
        {
            const bool isSelected = (m_currentItem == selectable);
            if (ImGui::Selectable(selectable, isSelected)) m_currentItem = selectable;
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    int orientation = 0;
    if (m_currentItem == "left/top") orientation = 0;
    if (m_currentItem == "left/bottom") orientation = 1;
    if (m_currentItem == "right/top") orientation = 2;
    if (m_currentItem == "right/bottom") orientation = 3;

    switch (orientation)
    {
        case 0:
            break;
        case 1:
            newMousePos.y = 1.0f - newMousePos.y;
            break;
        case 2:
            newMousePos.x = norWidth - newMousePos.x;
            break;
        case 3:
            newMousePos.y = 1.0f - newMousePos.y;
            newMousePos.x = norWidth - newMousePos.x;
            break;
        default:
            Log::Warn("inspector orientation corrupted");
    }
    const auto mx = std::to_string(newMousePos.x);
    const auto my = std::to_string(newMousePos.y);
    std::string str1 = "Current mouse position: x:" + mx + " y:" + my;
    ImGui::Text(str1.c_str());
    ImGui::Separator();
    if (ImGui::Button("Save Functions"))
    {
        serialiser->SaveFunctions();
    }
    if (ImGui::Button("clean"))
    {
        Clean();
    }
    ImGui::End();
}
#endif