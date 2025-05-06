#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <entt/entt.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "core/ecs.hpp"
#include "core/transform.hpp"
#include "rendering/ui_render_data.hpp"
#include "tools/serialize_glm.h"

// #define OPEN_GL
#define DX12
namespace skye
{
template <typename T>
struct AdvancedT
{
public:
    AdvancedT(T t) : m_value(t), m_resetValue(t) {}
    T Get() { return m_value; }
    void Set(T t) { m_value = t; }
    void SetReset(T t)
    {
        m_resetValue = t;
        m_value = t;
    }
    void Reset() { m_value = m_resetValue; };
    bool Changed() { return (m_value == m_resetValue) ? true : false; }

private:
    T m_value;
    T m_resetValue;
};
}  // namespace skye
namespace bee
{
namespace ui
{
typedef unsigned int UIComponentID;
typedef unsigned int UIFontID;
typedef entt::entity UIElementID;

struct ButtonFunc
{
    virtual void Func(const bee::ui::UIElementID element, const bee::ui::UIComponentID component){};
    std::string funcName;
};

#define UI_FUNCTION(internalName, name, code)                                                        \
    struct name : public bee::ui::ButtonFunc                                                         \
    {                                                                                                \
        name()                                                                                       \
        {                                                                                            \
            funcName = #name;                                                                        \
            bee::ui::ButtonFuncs.push_back(this);                                                    \
        };                                                                                           \
        void Func(const bee::ui::UIElementID element, const bee::ui::UIComponentID component){code}; \
    };                                                                                               \
    name internalName;

inline extern std::vector<ButtonFunc*> ButtonFuncs = std::vector<bee::ui::ButtonFunc*>();

enum class ComponentType
{
    text = 0,
    image = 1,

    button = 2,
    slider = 3,
    progressbar = 4,
};

struct Icon
{
    std::string iconPath = "assets/textures/checkerboard.png";
    glm::vec4 iconTextureCoordinates = glm::vec4(0, 0, 1, 1);

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(iconPath), CEREAL_NVP(iconTextureCoordinates));
    }
};

struct UIImageElement
{
    UIImageElement(const glm::vec2 leftCorner, const glm::vec2 size) : m_leftCornerAnchor(leftCorner), m_size(size){};
    [[nodiscard]] glm::vec4 GetAtlas() const
    {
        return glm::vec4(m_leftCornerAnchor.x, m_leftCornerAnchor.y, m_leftCornerAnchor.x + m_size.x,
                         m_leftCornerAnchor.y + m_size.y);
    }
    [[nodiscard]] glm::vec2 GetNormSize(const float modifier) const { return glm::normalize(m_size) * modifier; }
    int Img = 0;
    std::string file = "assets/textures/checkerboard.png";
    std::string name = "New UI Image";
    glm::vec2 m_leftCornerAnchor = glm::vec2(0.0f);
    glm::vec2 m_size = glm::vec2(64.0f);
    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(m_leftCornerAnchor), CEREAL_NVP(m_size), CEREAL_NVP(file), CEREAL_NVP(name));
    }
    UIImageElement(){};

private:
    friend class UserInterface;
    friend class UISerialiser;
};

struct Font
{
    float width = 0;
    float height = 0;
    float pixelrange = 0.0f;
    float fontSize = 2.0f;
    float lineSpace = 1.0f;
    std::unordered_map<unsigned int, float> advance;
    std::unordered_map<unsigned int, glm::vec4> quadPlaneBounds;
    std::unordered_map<unsigned int, glm::vec4> quadAtlasBounds;

    std::vector<float> pepi;

    UITexture tex;
    std::string name;
    UIFontID id = -1;
    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(width), CEREAL_NVP(lineSpace), CEREAL_NVP(height), CEREAL_NVP(pixelrange), CEREAL_NVP(fontSize),
                CEREAL_NVP(name), CEREAL_NVP(advance), CEREAL_NVP(quadPlaneBounds), CEREAL_NVP(quadAtlasBounds),
                CEREAL_NVP(pepi));
    }
    void Save(const std::string& path);
    void Load(const std::string& path);
};

// set to -1 to keep the neighbour empty
struct ControllerComponentNeighbours
{
    UIComponentID top = -1;
    UIComponentID right = -1;
    UIComponentID bottom = -1;
    UIComponentID left = -1;
};

// set to -1 to keep empty. initials are for which component to select when switchin to this element
struct ControllerElementNeighbours
{
    UIElementID top = entt::null;
    UIComponentID initialtop = -1;
    UIElementID right = entt::null;
    UIComponentID initialright = -1;
    UIElementID bottom = entt::null;
    UIComponentID initialbottom = -1;
    UIElementID left = entt::null;
    UIComponentID initialleft = -1;
};
enum ButtonType
{
    repeat = 1,
    single = 2
};
enum SwitchType
{
    none = 1,
    newAtlasPos = 2
};
enum Alignment
{
    bottom = 0,
    top = 1,
    left = 2,
    right = 3
};
enum ImgType
{
    rawImage,
    ProgressBarBackground
};
namespace internal
{

struct Button
{
    bool clicking = false;
    bool lastClicking = false;
    unsigned int buttonFuncIndexHover = -1;
    unsigned int buttonFuncIndexClick = -1;

    glm::vec4 bounds = glm::vec4(0.0f);

    SwitchType onReset = SwitchType::none;
    SwitchType onHover = SwitchType::none;
    SwitchType onClick = SwitchType::none;

    glm::vec4 atlasOnHover = glm::vec4(0.0f);
    glm::vec4 atlasOnClick = glm::vec4(0.0f);

    UIComponentID linkedComponent = -1;

    bool isbeingHovered = false;
    bool hasbeenReplaced = false;
    UIComponentID replacedID = -1;

    ControllerComponentNeighbours neighbours;
    UIComponentID ID = -1;
    ButtonType type = single;
    UIImageElement HoverOverlay = UIImageElement(glm::vec2(0.0), glm::vec2(0));
    bool hoverable = true;
};
struct Slider
{
    float* linkedVar = nullptr;
    float lastVal = 0.0f;
    glm::vec4 bounds = glm::vec4(0.0f);
    glm::vec2 slideSize = glm::vec2(0.0f);
    int img = -1;
    float speed = 1.0f;
    glm::vec4 atlasForBar = glm::vec4(0.0f);
    glm::vec4 atlasForSlider = glm::vec4(0.0f);

    UIElementID linkedElement = entt::null;

    float max = 0;
    int locInVertexArray = -1;

    ControllerComponentNeighbours neighbours;
    UIComponentID ID = -1;
    UIImageElement HoverOverlay = UIImageElement(glm::vec2(0.0), glm::vec2(0));
};
struct UIComponent
{
    unsigned int ID = 0;

    int image = -1;
    glm::vec4 atlas = glm::vec4(0.0f);
    UIElementID element = entt::null;
    ComponentType type;
    std::string name = "";
};

struct UICompo
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    RendererData renderData;
    int updaterID = -1;
};
struct UIImageComponent : public UICompo
{
    std::vector<unsigned int> freespaces;
};
struct UITextComponent : public UICompo
{
    std::vector<glm::vec2> freespaces;
};

struct UIImage
{
    int width = 0;
    int height = 0;

    UITexture tex;
};
struct Updater
{
    ComponentType type = ComponentType::text;
    int location = 0;
    UIElementID element = entt::null;
    bool Inplace = false;
};

struct UIElement
{
    ElementRenderData elRData;
    std::unordered_map<int, UIImageComponent> imageComponents;
    std::unordered_map<int, UIImageComponent> progressBarimageComponents;
    std::unordered_map<int, UITextComponent> textComponents;
    bool drawing = true;
    bool input = true;
    UIElementID ID = entt::null;
    std::unordered_map<int, Button> buttons;
    std::unordered_map<int, Slider> sliders;

    Alignment topOrBot = top;
    Alignment leftOrRight = left;

    ControllerElementNeighbours neighbours;
    float opacity = 1.0f;
    std::string name;
};
}  // namespace internal
}  // namespace ui
}  // namespace bee