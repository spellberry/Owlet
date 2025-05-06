#include "user_interface/user_interface_serializer.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/types/unordered_map.hpp>

#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "tools/log.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_structs.hpp"

using namespace bee;
using namespace ui;
using namespace internal;
bee::ui::UISerialiser::UISerialiser()
{
    LoadFunctions();
    if (ButtonFuncs.size() != 0)
    {
        SaveFunctions();
    }
}
void bee::ui::UISerialiser::LoadOverlays()
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    LoadOverlays(&ui);
}
void bee::ui::UISerialiser::LoadOverlays(UserInterface* ui)
{
    if (bee::fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "default/Overlays.json")))

    {
        {
            std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "default/Overlays.json"));
            cereal::JSONInputArchive archive(is);

            std::unordered_map<unsigned int, UIImageElement> loverlays;

            archive(CEREAL_NVP(loverlays));
            overlays.clear();
            ui->m_overlaymap.clear();
            ui->m_overlaymap = loverlays;
            for (auto& overlay : ui->m_overlaymap)
            {
                overlays.emplace(overlay.second.name, overlay.first);
                overlay.second.Img = ui->LoadTexture(overlay.second.file);
            }
        }
    }
}

void bee::ui::UISerialiser::SaveOverlays() const
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    std::unordered_map<unsigned int, UIImageElement>& loverlays = ui.m_overlaymap;

    const std::string path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "default/Overlays.json");
    std::ofstream os(path);
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(loverlays));
}

bee::ui::UISerialiser::~UISerialiser() {}

void bee::ui::UISerialiser::LoadFunctions()
{
    for (int i = 0; i < ButtonFuncs.size(); i++)
    {
        functions.emplace(ButtonFuncs.at(i)->funcName, i);
    }
}

void bee::ui::UISerialiser::LoadFunctionsEditor()
{
    std::vector<std::string> list;

    std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "default/Functions.json"));
    cereal::JSONInputArchive archive(is);
    archive(CEREAL_NVP(list));

    for (auto& listEntry : list)
    {
        if (functions.count(listEntry) == 0)
        {
            functions.emplace(listEntry, -1);
        }
    }
}

void bee::ui::UISerialiser::SaveFunctions()
{
    std::vector<std::string> list;

    for (auto& func : functions)
    {
        list.push_back(func.first);
    }
    const std::string path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, "default/Functions.json");
    std::ofstream os(path);
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(list));
}

UIElementID bee::ui::UISerialiser::LoadElement(const std::string& name)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();
    sElement element;

    std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, name + ".json "));
    cereal::JSONInputArchive archive(is);
    archive(CEREAL_NVP(element));

    UIElementID ID = ui.CreateUIElement(element.tobs, element.lors);
    Engine.ECS().Registry.get<Transform>(ID).Name = element.name;
    Engine.ECS().Registry.get<UIElement>(ID).name = element.name;
    ui.m_items.emplace(ID, std::unordered_map<std::string, std::unique_ptr<Item>>());
    Engine.ECS().Registry.get<UIElement>(ID).opacity = element.opacity;
    LoadSElement(ID, element);

    Engine.ECS().Registry.get<Transform>(ID).Translation.z = static_cast<float>(element.layer) / 100.0f;
    return ID;
}

void bee::ui::UISerialiser::LoadIntoElement(UIElementID ID, const std::string& name)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    sElement element;

    std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, name + ".json "));
    cereal::JSONInputArchive archive(is);
    archive(CEREAL_NVP(element));

    if (ui.m_items.count(ID) == 0)
    {
        ui.m_items.emplace(ID, std::unordered_map<std::string, std::unique_ptr<Item>>());
    }
    LoadSElement(ID, element);
}

void bee::ui::UISerialiser::LoadSElement(UIElementID ID, sElement element)
{
    auto& ui = Engine.ECS().GetSystem<UserInterface>();

    for (auto& pb : element.progressbars)
    {
        pb.Remake(ID);
    }
    for (auto& tx : element.texts)
    {
        tx.Remake(ID);
    }
    for (auto& img : element.images)
    {
        img.Remake(ID);
    }
    for (auto& slid : element.sliders)
    {
        slid.Remake(ID);
    }
    for (auto& but : element.buttons)
    {
        but.Remake(ID);
    }
}

void bee::ui::UISerialiser::SaveElement(const sElement& element) const
{
    const std::string path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::UserInterface, element.name + ".json");
    std::ofstream os(path);
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(element));
}
