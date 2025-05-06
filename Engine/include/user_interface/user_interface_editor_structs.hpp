#pragma once

#include "user_interface.hpp"
#include "user_interface_structs.hpp"
namespace bee
{
namespace ui
{

struct Item
{
    virtual void ShowInfo(UIElementID el){};
    virtual void Move(UIElementID el, glm::vec2 delta){};
    virtual void Remake(UIElementID el){};
    virtual void Delete(){};
    virtual void RemakeWBounds(UIElementID el){};

    glm::vec4 bounds;
    ComponentType type;
    UIComponentID ID = -1;
    UIElementID element;
    std::string name = "New Component";
    int layer = 0;
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(type);
    }
};

struct Image : public Item
{
    Image(){};
    Image(UIElementID eID) { Remake(eID); };
    void ShowInfo(UIElementID el) override;
    void Move(UIElementID el, glm::vec2 delta) override;
    void Remake(UIElementID el) override;
    void Delete() override;
    void RemakeWBounds(UIElementID el);

    glm::vec2 start = glm::vec2(0.5f);
    glm::vec2 size = glm::vec2(0.1f);
    glm::vec4 atlas = glm::vec4(0.0f, 0.0f, 64.0f, 64.0f);
    std::string imageFile = "assets/Textures/checkerboard.png";
    bool editingAtlas = false;
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name), CEREAL_NVP(start), CEREAL_NVP(size), CEREAL_NVP(atlas), CEREAL_NVP(imageFile), CEREAL_NVP(layer));
    }
};

struct Text : public Item
{
    Text(){};
    Text(UIElementID eID) { Remake(eID); };

    void ShowInfo(UIElementID el) override;
    void Move(UIElementID el, glm::vec2 delta) override;
    void Remake(UIElementID el) override;
    void Delete() override;
    void RemakeWBounds(UIElementID el);

    glm::vec2 start = glm::vec2(0.5f, 0.5f);
    float size = 1.0f;
    std::string text = "Text";
    glm::vec2 nextLine = glm::vec2(0.0f);
    glm::vec4 colour = glm::vec4(1);
    std::string font = "assets/fonts/DroidSans.sff";
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name), CEREAL_NVP(start), CEREAL_NVP(size), CEREAL_NVP(text), CEREAL_NVP(layer), CEREAL_NVP(colour),
           CEREAL_NVP(font));
    }
    bool fullline = false;
};
struct sButton : public Item
{
    sButton(){};
    sButton(UIElementID eID) { Remake(eID); };
    void ShowInfo(UIElementID el) override;
    void Move(UIElementID el, glm::vec2 delta) override;
    void Remake(UIElementID el) override;
    void Delete() override;
    void RemakeWBounds(UIElementID el);

    glm::vec2 start = glm::vec2(0.5f);
    glm::vec2 size = glm::vec2(0.1f);
    UIComponentID visOverlay = -1;

    UIComponentID linkedComponent = -1;

    interaction hover = interaction(SwitchType::none);
    std::string hoverst = "none";
    interaction click = interaction(SwitchType::none);
    std::string clickst = "none";

    ButtonType typein = single;
    std::string typeDrop = "single";

    std::string hoverimg = "none";
    std::string linkedComponentstr = "";
    bool hoverable = true;
    bool preview = false;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(start), CEREAL_NVP(size), CEREAL_NVP(hoverst), CEREAL_NVP(click), CEREAL_NVP(clickst), CEREAL_NVP(hover),
           CEREAL_NVP(typein), CEREAL_NVP(hoverimg), CEREAL_NVP(hoverable), CEREAL_NVP(name), CEREAL_NVP(linkedComponentstr),
           CEREAL_NVP(layer));
    }
};

struct sProgressBar : public Item
{
    sProgressBar(){};
    sProgressBar(UIElementID eID) { Remake(eID); };
    void ShowInfo(UIElementID el) override;
    void Move(UIElementID el, glm::vec2 delta) override;
    void Remake(UIElementID el) override;
    void Delete() override;
    void RemakeWBounds(UIElementID el);

    glm::vec2 start = glm::vec2(0.5f);
    glm::vec2 size = glm::vec2(0.2f);
    glm::vec4 atlas = glm::vec4(0.0f, 0.0f, 64.0f, 64.0f);
    bool editingAtlas = false;

    std::string imageFile = "assets/Textures/checkerboard.png";
    glm::vec4 fColor = glm::vec4(1.0f);
    glm::vec4 bColor = glm::vec4(0.0f);

    float value = 100.0f;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(start), CEREAL_NVP(size), CEREAL_NVP(atlas), CEREAL_NVP(imageFile), CEREAL_NVP(fColor),
           CEREAL_NVP(bColor), CEREAL_NVP(name), CEREAL_NVP(value), CEREAL_NVP(layer));
    }
};

struct sSlider : public Item
{
    sSlider(){};
    sSlider(UIElementID eID) { Remake(eID); };
    void ShowInfo(UIElementID el) override;
    void Move(UIElementID el, glm::vec2 delta) override;
    void Remake(UIElementID el) override;
    void Delete() override;
    void RemakeWBounds(UIElementID el);

    glm::vec2 start = glm::vec2(0.5f);
    glm::vec2 size = glm::vec2(0.2f);
    std::string imageFile = "assets/Textures/checkerboard.png";

    glm::vec4 slidAtlas = glm::vec4(0.0f, 0.0f, 64.0f, 64.0f);
    bool editingAtlasslid = false;
    glm::vec4 barAtlas = glm::vec4(0.0f, 0.0f, 64.0f, 64.0f);
    bool editingAtlasbar = false;
    glm::vec2 slidSize = glm::vec2(0.05f, 0.3f);

    float speed = 0.5f;
    float max = 100.0f;
    float tempVar = 50.0f;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name), CEREAL_NVP(start), CEREAL_NVP(size), CEREAL_NVP(imageFile), CEREAL_NVP(slidAtlas),
           CEREAL_NVP(barAtlas), CEREAL_NVP(slidSize), CEREAL_NVP(speed), CEREAL_NVP(max), CEREAL_NVP(layer));
    }
};

struct sElement
{
    Alignment tobs;
    Alignment lors;

    std::string name;

    std::vector<sSlider> sliders;
    std::vector<sProgressBar> progressbars;
    std::vector<sButton> buttons;
    std::vector<Text> texts;
    std::vector<Image> images;
    int layer = 0;
    float opacity = 1.0f;
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name), CEREAL_NVP(tobs), CEREAL_NVP(lors), CEREAL_NVP(sliders), CEREAL_NVP(progressbars),
           CEREAL_NVP(buttons), CEREAL_NVP(texts), CEREAL_NVP(images), CEREAL_NVP(layer), CEREAL_NVP(opacity));
    }
};
}  // namespace ui
}  // namespace bee
