#pragma once

#include <imgui/imgui.h>

#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/glm.hpp>
#include <map>
#include <set>
#include <string>

#include "core/engine.hpp"
#include "core/fwd.hpp"
#include "tools/serializable.hpp"
#include "tools/serialization.hpp"
//The guizmo if part of the ImGuizmo library https://github.com/CedricGuillemet/ImGuizmo
#include "tools/GuizmoData.hpp"

namespace bee
{
struct Transform;
class Inspector
{
public:
    Inspector();
    ~Inspector();
    void SaveToFile();
    void InitFromFile();
    void SetVisible(bool visible) { m_visible = visible; }
    bool GetVisible() const { return m_visible; }
    glm::vec2 GetGameSize() const { return m_gameSize; }
    glm::vec2 GetGamePos() const { return m_gamePos; }
    bool IsGameWindowFocused() const { return m_gameFocused; };
    void Inspect(float dt);
    void Inspect(Entity entity, Transform& transform, std::set<Entity>& inspected);
    void Inspect(const char* name, float& f);
    void Inspect(const char* name, int& i);
    void Inspect(const char* name, bool& b);
    void Inspect(const char* name, glm::vec2& v);
    void Inspect(const char* name, glm::vec3& v);
    void Inspect(const char* name, glm::vec4& v);
    void Inspect(std::string name , std::vector<std::string>& items) const;
    void Inspect(std::string name , std::vector<int>& items) const;
    void Inspect(std::string name , std::vector<float>& items) const;
    
    void UsingTheEditor(bool usingEditor);
    Entity SelectedEntity() { return m_selectedEntity; }
    bool InInspect() { return m_inInspectCall; }
    template <typename T>
    inline void operator()(const char* name, T& value)
    {
        Inspect(name, value);
    }

    template <typename T>
    inline void Inspect(T& value)
    {
        visit_struct::for_each(value, Engine.Inspector());
        if constexpr (std::is_convertible_v<T, Serializable>)
        {
            if (Engine.Serializer().HasItem(value))
            {
                if (ImGui::Button("Save"))
                {
                    Engine.Serializer().SerializeTo(value);
                }
                ImGui::SameLine();
                ImGui::LabelText("Path", "%s", Engine.Serializer().GetMapping(value).c_str());
            }
        }
    }

    unsigned int InspectorColorbuffer = 0;
    GuizmoData data;

private:
    Entity m_selectedEntity = entt::null;
    bool m_showImguiTest = false;
    bool m_config = false;
    bool m_visible = true;
    std::map<std::string, bool> m_openWindows;
    bool m_insideEditor = false;

    glm::vec2 m_gameSize = glm::vec2(0.0f, 0.0f);  // size of the game window in the editor
    glm::vec2 m_gamePos = glm::vec2(0, 0);         // position of game window in the editor
    bool m_gameFocused = false;                    // TRUE if the ImGui Game window is currently focused.
    bool m_inInspectCall = false;                  // to be able to keep track of if the program is inside the inspector loop
};


}  // namespace bee
