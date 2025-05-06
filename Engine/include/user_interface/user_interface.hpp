#pragma once

#include <glm/glm.hpp>
#include <string>

#include "core/ecs.hpp"
#include "font_handler.hpp"
#include "user_interface/user_interface_structs.hpp"

namespace bee
{
namespace ui
{
struct Item;

constexpr unsigned int DEFAULTINDICES[] = {
    0u, 1u, 3u,  // first triangle
    1u, 2u, 3u   // second triangle
};

enum InputMode
{
    mouseAndKeyboard = 1,
    controller = 2
};
enum interactionType
{
    hover = 1,
    click = 2,
    reset = 3
};

struct interaction
{
    interaction(const SwitchType switchType) : type(switchType){};
    interaction(const SwitchType switchType, const std::string& funcName) : type(switchType), funcName(funcName){};
    interaction(const SwitchType switchType, const glm::vec4 newAtlas) : type(switchType), newAtlas(newAtlas){};
    interaction(const SwitchType switchType, const glm::vec4 newAtlas, const std::string& funcName)
        : type(switchType), newAtlas(newAtlas), funcName(funcName){};
    interaction();
    SwitchType type = none;

    glm::vec4 newAtlas = glm::vec4(-1.0f);
    std::string funcName = "";

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(type), CEREAL_NVP(newAtlas), CEREAL_NVP(funcName));
    }
};

class FontHandler;
class UIRenderer;
class UISerialiser;

class UserInterface : public bee::System
{
public:
    // default system functions
    UserInterface();
    ~UserInterface() override;
    void Render() override;
    void Update(float dt) override;
    std::unique_ptr<UISerialiser> serialiser;

    // Create a UI element. returns the ID of the element.
    [[nodiscard]] UIElementID CreateUIElement(Alignment topOrBot, Alignment leftOrRight);
    void DeleteUIElement(UIElementID element);
    // enable or disable drawing of a certaine eleme nt
    void SetDrawStateUIelement(UIElementID element, bool newState) const;
    [[nodiscard]] bool GetDrawStateUIelement(UIElementID element) const;
    void SetInputStateUIelement(UIElementID element, bool newState) const;
    [[nodiscard]] bool GetInputStateUIelement(UIElementID element) const;
    void SetElementOpacity(UIElementID element, float newOpacity) const;

    void DeleteComponent(UIComponentID id, bool DeleteItem = true);

    //
    // text
    //
    [[nodiscard]] const Font& GetFont(const int id) const { return m_fontHandler->GetFont(id); };
    UIFontID LoadFont(const std::string& fontLocation) const { return m_fontHandler->LoadFont(fontLocation); };

    UIComponentID CreateString(UIElementID element, int fontindex, const std::string& str, glm::vec2 position, float size,
                               glm::vec4 colour, int layer = 0, int reserveLetters = 0, const std::string& name = "");
    UIComponentID CreateString(UIElementID element, int fontindex, const std::string& str, glm::vec2 position, float size,
                               glm::vec4 colour, float& xNextPositionAdvance, float& yNextPositionLineGap, bool fullLine,
                               int layer = 0, int reserveLetters = 0, const std::string& name = "");
    glm::vec2 PreCalculateStringEnds(UIElementID element, int fontIndex, const std::string& str, glm::vec2 position, float size,
                                     bool fullLine) const;
    // string has top be same size in glyphs, if not delete the string and create a new one.
    UIComponentID ReplaceString(UIComponentID id, const std::string& str, float& xNextPositionAdvance,
                                float& yNextPositionLineGap, bool fullLine);
    UIComponentID ReplaceString(UIComponentID id, const std::string& str, float& xNextPositionAdvance,
                                float& yNextPositionLineGap, glm::vec2 position, bool fullLine);
    UIComponentID ReplaceString(UIComponentID id, const std::string& str);

    //
    // image/texture
    //

    // Load a texture for future use, returns the ID of the image as int.
    [[nodiscard]] int LoadTexture(const char* imglocation);
    // Load a texture for future use, returns the ID of the image as int.
    [[nodiscard]] int LoadTexture(const std::string& imglocation);
    void DeleteTexture(int image) const;

    // Adds a texture component from an atlas into the element.
    UIComponentID CreateTextureComponentFromAtlas(UIElementID element, int image, glm::vec4 texCoord, glm::vec2 position,
                                                  glm::vec2 size, int layer = 0, const std::string& name = "");

    UIComponentID ReplaceAtlasPostition(UIComponentID id, float pixcoordx, float pixcoordy, float pixcoordx2, float pixcoordy2);

    //
    // interactables
    //
    void SetInputMode(InputMode newMode);
    [[nodiscard]] InputMode GetInputMode() const;

    // Create a button with a linked texture component to use for interactions
    UIComponentID CreateButton(UIElementID element, glm::vec2 position, glm::vec2 size, const interaction& onClick,
                               const interaction& onHover, ButtonType type, unsigned int hoverimg = -1, bool hoverable = true,
                               UIComponentID linkedComponent = -1, const std::string& name = "");
    // simulates everything that happens when a button is pressed without the button being pressed
    void TriggerButton(UIComponentID buttonid, interactionType type);
    // Resets a button to its original state using the earlier passed linkedComponent
    void ResetButton(UIComponentID buttonid);
    [[nodiscard]] bool GetButtonState(UIComponentID buttonid) const;
    [[nodiscard]] bool GetButtonLastClick(UIComponentID buttonid) const;
    void SetButtonLastClick(UIComponentID buttonid, bool newState) const;

    // Create a slider from an Atlas texture
    UIComponentID CreateSlider(UIElementID element, float& var, float max, float speed, glm::vec2 position, glm::vec2 size,
                               glm::vec2 slideSize, int imgAtlas, glm::vec4 atlasForBar, glm::vec4 atlasForSlider,
                               int layer = 0, const std::string& name = "");
    // PLEASE SET VAR ON SAME CALL OTHERWISE IT WILL CAUSE ERRORS!!!!
    UIComponentID CreateSlider(UIElementID element, float max, float speed, glm::vec2 position, glm::vec2 size,
                               glm::vec2 slideSize, int imgAtlas, glm::vec4 atlasForBar, glm::vec4 atlasForSlider,
                               int layer = 0, const std::string& name = "");
    void SetSliderVar(UIComponentID ID, float& var) const;
    void SetSliderMax(UIComponentID ID, float newMax) const;
    [[nodiscard]] float GetSliderMax(UIComponentID ID) const;
    void SetSliderSpeed(UIComponentID ID, float newSpeed) const;

    // Create a progress bar
    UIComponentID CreateProgressBar(UIElementID element, glm::vec2 position, glm::vec2 size, int shapeImg, glm::vec4 texCoord,
                                    glm::vec4 foreGroundColour, glm::vec4 backGroundColour, int layer = 0, float value = 100.0f,
                                    const std::string& name = "");
    // Update the value of a progress bar
    void SetProgressBarValue(UIComponentID barid, float newValue);
    // Update the background and foreground colours of a progress bar
    void SetProgressBarColors(UIComponentID barid, glm::vec4 foreGroundColour, glm::vec4 backgroundColour);

    //
    // controller input
    //
    void SetComponentNeighbours(UIComponentID id, const ControllerComponentNeighbours& neighbourStruct) const;
    void SetElementNeighbours(UIElementID element, const ControllerElementNeighbours& neighbourStruct) const;
    [[nodiscard]] ControllerElementNeighbours GetElementNeighbours(UIElementID element) const;
    [[nodiscard]] ControllerComponentNeighbours GetNeighbours(UIComponentID id) const;
    void SetSelectedInteractable(UIComponentID ID);
    void SwitchInteractable(Alignment direction);
    void SetComponentNeighbourInDirection(UIComponentID id, UIComponentID neighbourid, Alignment direction) const;
    UIComponentID GetSelectedInteractable() const { return m_CselectedComponent; }
    UIElementID GetSelectedElement() const { return m_CselectedElement; }
    unsigned int AddOverlayImage(const UIImageElement& image);
    unsigned int GetOverlayIndex(const std::string& str) const;
    void SetUiInputActions(const std::string& buttonMovementAction, const std::string& clickAction);
    void SetElementLayer(UIElementID element, int layer) const;
    int GetElementLayer(UIElementID element) const;
    void Clean();

    template <typename T, std::enable_if_t<std::is_base_of_v<Item, T>, std::nullptr_t> = nullptr>
    T& getComponentItem(UIElementID element, std::string name)
    {
        return *dynamic_cast<T*>(m_items.at(element).at(name).get());
    }
    UIComponentID GetComponentID(UIElementID elementID, const std::string& key) const;

private:
    friend class FontHandler;
    friend class UIRenderer;
    friend class UIEditor;
    friend class UISerialiser;
    [[nodiscard]] FontHandler& fontHandler() const { return *m_fontHandler; }
    std::unique_ptr<FontHandler> m_fontHandler;
    std::unique_ptr<UIRenderer> m_renderer;
    InputMode m_inputMode = mouseAndKeyboard;
    std::unordered_map<UIElementID, std::unordered_map<std::string, std::unique_ptr<Item>>> m_items;

    // Data handling
    UIComponentID m_nextID = 0;
    std::unordered_map<UIComponentID, internal::UIComponent> m_componentMap;
    std::vector<UIComponentID> m_toResetButtons;
    std::vector<UIElementID> m_toDeleteElements;
    // text
    void addLetter(internal::UIElement& element, internal::UITextComponent& comp, const Font& font, const char* letter,
                   float xpos, float ypos, float& advance, float size, int& inplace, int layer, glm::vec4 c);

    // image handling
    internal::UIComponent addTextureComponentVertices(const internal::UIElement& element, float xpos, float ypos, float sizex,
                                                      float sizey, glm::vec4 UVs, std::vector<float>& vertices,
                                                      bool useOrientation, int zValue = 0) const;
    void addTextureComponentIndices(internal::UIImageComponent& img, unsigned int verticesOffset) const;
    // interactables
    void TriggerButton(internal::UIElement& ID, internal::Button& inter, interactionType type, bool funcOnly);
    void SwitchButtonLook(internal::UIElement& element, UIComponentID id, internal::Button& butt, interactionType type);
    void ResetButtonInternal(UIComponentID id);
    bool isInBounds(glm::vec4 bounds, glm::vec2 position) const;
    void DeleteUIElementInternal(UIElementID element);

    void MakeOverlay(const UIImageElement& inter);
    void ResetOverlay();
    void SetSliderSlideLoc(internal::UIElement& element, const internal::Slider& inter, float diffperc, float diff);

    // controller/MKB Input
    internal::UIComponent m_selectedComponentOverlay;
    UIElementID m_selectedElementOverlay;
    int m_selectedComponentOverlayImage = -1;
    bool m_selected = false;
    bool m_lastselected = false;
    float m_controllerTimer = 1.0f;
    float m_deadzone = 0.5f;
    float m_controllerDelay = 0.2f;
    UIComponentID m_CselectedComponent = -1;
    UIComponentID m_lastCselectedComponent = -1;
    std::string m_buttonMovementAction;
    std::string m_clickAction;
    UIElementID m_CselectedElement = entt::null;
    float m_UpdateTime = 0.0f;

#ifdef BEE_INSPECTOR
    void Inspect() override;
    char m_textLoaderBuffer[128] = {" "};
    const char* m_currentItem;
    const char* m_orientations[4] = {"left/top", "left/bottom", "right/top", "right/bottom"};
    UIComponentID overrideSel = -1;
#endif
    std::unordered_map<std::string, int> m_loadedTexturesMap{};

    std::unordered_map<unsigned int, UIImageElement> m_overlaymap;

    bool m_hasToClean = false;
    std::unordered_map<int, internal::Updater> m_updates;
};
}  // namespace ui
}  // namespace bee