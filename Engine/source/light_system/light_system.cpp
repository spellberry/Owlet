#include "light_system/light_system.hpp"

#include <fstream>
#include <imgui/imgui.h>

#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/transform.hpp"
#include "rendering/render_components.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/utility.hpp>

using namespace bee;

// credit to stack overflow user:
// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

LightSystem::LightSystem()
{
    Title = "Light System";
}
#ifdef BEE_INSPECTOR
void LightSystem::Inspect()
{
    System::Inspect();
    ImGui::Begin(Title.c_str());
    if(ImGui::Button("Add light"))
    {
        AddLight();
    }
    ImGui::End();
}
#endif

void LightSystem::AddLight()
{
    Entity entity = Engine.ECS().CreateEntity();
    Transform newTransform;
    newTransform.Name = "Light";
    Engine.ECS().CreateComponent<Transform>(entity,newTransform);
    Engine.ECS().CreateComponent<Light>(entity);
}

void bee::LightSystem::SaveLights(const std::string& fileName)
{
    std::vector<std::pair<bee::Transform, Light>> lights;
    auto view = bee::Engine.ECS().Registry.view<bee::Transform, Light>();
    for (auto [entity, transform, light] : view.each())
    {
        lights.push_back(std::pair<bee::Transform, Light>(transform, light));
    }

    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_Lights.json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(lights));
}

void bee::LightSystem::LoadLights(const std::string& fileName)
{
    // remove existing lights
    auto view = bee::Engine.ECS().Registry.view</*bee::Transform, */Light>();
    for (auto [entity, light] : view.each())
    {
        bee::Engine.ECS().DeleteEntity(entity);
    }

    // load new lights
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_Lights.json")))
    {
        std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_Lights.json"));
        cereal::JSONInputArchive archive(is);

        std::vector<std::pair<bee::Transform, Light>> lights;
        archive(CEREAL_NVP(lights));
        for (const auto& light : lights)
        {
            auto entity = bee::Engine.ECS().CreateEntity();
            auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(entity);
            transform = light.first;
            auto& lightData = bee::Engine.ECS().CreateComponent<Light>(entity);
            lightData = light.second;
        }
    }
}

entt::view<entt::get_t< Transform, Light>> LightSystem::ReturnRelevantLights()
{
    auto view = Engine.ECS().Registry.view<Transform, Light>();
    return view;
}
