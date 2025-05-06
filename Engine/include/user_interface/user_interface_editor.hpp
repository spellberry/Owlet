#pragma once
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "core/ecs.hpp"
#include "user_interface.hpp"
#include "user_interface_editor_structs.hpp"
#include "user_interface_structs.hpp"

struct ImVec2;

namespace bee
{
namespace ui
{

struct Line
{
    Line(glm::vec2 p0, glm::vec2 p1, glm::vec4 color) : p0(p0), p1(p1), color(color){};
    glm::vec2 p0, p1;
    glm::vec4 color;
};

class UIEditor : public System
{
public:
    UIEditor();
    ~UIEditor() override;
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif

    void AdjustCoordinates(UIElementID el, glm::vec2& posToAdjust, glm::vec2& sizeToAdjust) const;
    void EnableAtlasEditingMode(glm::vec4* ptr)
    {
        m_atlasEditing = true;
        m_atlasptr = ptr;
    };
    void DisableAtlasEditingMode()
    {
        m_atlasEditing = false;
        m_atlasptr = nullptr;
    };

private:
    friend struct sButton;
    // windows
    void ViewPort();
    void Inspector();
    void Lines();
    // lines
    void MakeLine(glm::vec2 p0, glm::vec2 p1, glm::vec4 color);
    void MakeBox(glm::vec2 p0, glm::vec2 p1, glm::vec4 color);
    void MakeVertex(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec4 color);

    void ResizeIfNeeded(int windowCounter);
    bool DisplayDropDown(const std::string& name, const std::vector<std::string>& list, std::string& selectedItem) const;
    void BeginChild(const std::string& name, const int& windowCounter, const ImVec2& cursorPos) const;
    void EndChild(int& windowCounter, ImVec2& cursorPos) const;
    std::vector<std::string> GetHoverables() const;
    std::vector<std::string> GetFunctions() const;
    void SetoverrideSel(UIComponentID ID) const;
    std::vector<Line> m_lines;
    UIElementID LoadElement(const std::string& path, UIElementID ID);
    glm::vec2 m_wsize = glm::vec2(0.0f, 0.0f);
    bool m_clicking = false;
    bool m_debugDrawing = false;
    // atlas editing
    bool m_atlasEditing = false;
    float m_mouseWheel = 0;
    glm::vec4* m_atlasptr;

    // adjustment
    glm::vec2 m_oldMPos = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_gamePos = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_gameSize = glm::vec2(0.0f, 0.0f);
    bool m_ishovered = false;
    int m_componentNaneIndexer = 0;

    // font editor
    bool m_fontEditor = false;
    bool m_LoadedFont = false;
    std::string m_curFontLoad;
    std::string m_curFontLoadsff;
    UIFontID m_curLoadedFont = -1;
    UIElementID m_previewEl = entt::null;
    UIComponentID m_previewcomp = -1;
    std::string m_previewText = "Preview";

    // hover editor
    bool m_hoverEditor = false;
    std::string m_cursel = "none";
    int m_oCounter = 0;

    // editor
    UIElementID m_curSelectedEl = entt::null;
    std::vector<float> m_autoSizes;
    std::string m_tobs = "Top";
    std::string m_lors = "Left";
    UIFontID m_defFont = -1;
    bool m_focusedWindow = false;
    std::string m_elementFile;

    // component editor
    // unforge raw pointers for copy and cast compatibility
    std::vector<Item*> m_items;
    int m_selectedComp = -1;

    std::vector<std::string> m_functions;
};
}  // namespace ui
}  // namespace bee