// #include "game_ui/resource_ui.hpp"
//
// #include <imgui/imgui.h>
//
// #include <cereal/archives/json.hpp>
// #include <cereal/cereal.hpp>
// #include <cereal/types/unordered_map.hpp>
// #include <glm/glm.hpp>
// #include <unordered_map>
//
// #include "actors/props/resource_system.hpp"
// #include "core/engine.hpp"
// #include "core/fileio.hpp"
// #include "imgui/imgui_stdlib.h"
// #include "tools/tools.hpp"
// #include "user_interface/user_interface.hpp"
//
// using namespace bee;
// using namespace ui;
//
// void CallbackFunc(const GameResourceType type, const int amount)
//{
//     Engine.ECS().GetSystem<ResourceUI>().UpdateUI(
//         type, Engine.ECS().GetSystem<ResourceSystem>().playerResourceData.resources.at(type));
// }
//
// ResourceUI::ResourceUI()
//{
//     Title = "resourceUI";
//     LoadIcons();
//
//     Engine.ECS().GetSystem<ResourceSystem>().SetCallback(CallbackFunc);
//     auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
//     const ui::UIElementID loElement = element = ui.CreateUIElement(ui::Alignment::top, ui::Alignment::left);
//     std::map<GameResourceType, bee::ui::UIComponentID> loComponents;
//     auto locIcons = m_icons;
//
//     glm::vec2 loc = glm::vec2(0.01, 0.1);
//     magic_enum::enum_for_each<GameResourceType>(
//         [&ui, &loElement, &loComponents, &loc, &locIcons](auto val)
//         {
//             constexpr GameResourceType resource = val;
//             if (resource != GameResourceType::None)
//             {
//                 const std::string name = std::string(magic_enum::enum_name(resource)) + ":";
//
//                 auto image = ui.LoadTexture(Engine.FileIO().GetPath(FileIO::Directory::Asset,
//                 locIcons.at(resource).iconPath)); glm::vec2 size = glm::vec2(0.04, 0.04);
//                 ui.CreateTextureComponentFromAtlas(loElement, image, locIcons.at(resource).iconTextureCoordinates,
//                                                    glm::vec2(loc.x, loc.y - size.x), size);
//                 float xNext = loc.x + size.x;
//                 const std::string amount = "1000";
//                 const std::string amountstr = std::string(magic_enum::enum_name(resource)) + "string";
//                 loComponents.emplace(resource, ui.CreateString(loElement, 0, amount, glm::vec2(xNext, loc.y), textSize,
//                                                                glm::vec4(1), 0, 0, amountstr));
//                 loc.x += xNext + (size.x * 2);
//             }
//         });
//     m_components = loComponents;
//     ui.SetElementLayer(element, 5);
// }
// ResourceUI::~ResourceUI() {}
//
// void const ResourceUI::UpdateUI(const GameResourceType type, const int newAmount)
//{
//     auto& ui = Engine.ECS().GetSystem<ui::UserInterface>();
//
//     float throwaway, throwaway2 = 0.0f;
//     if (m_components.find(type) == m_components.end()) return;
//     ui.ReplaceString(m_components.at(type), std::to_string(newAmount), throwaway, throwaway2, false);
// }
//
// void ResourceUI::SaveIcons()
//{
//     std::unordered_map<GameResourceType, bee::ui::Icon>& icons = m_icons;
//
//     std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, "ResourceIcons"));
//     cereal::JSONOutputArchive archive(os);
//     archive(CEREAL_NVP(icons));
// }
// void ResourceUI::LoadIcons()
//{
//     if (bee::fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, "ResourceIcons")))
//     {
//         std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, "ResourceIcons"));
//         cereal::JSONInputArchive archive(is);
//
//         std::unordered_map<GameResourceType, bee::ui::Icon> icons;
//         archive(CEREAL_NVP(icons));
//         m_icons = icons;
//     }
//     // if new enums are made emplace them automatically
//     {
//         std::unordered_map<GameResourceType, bee::ui::Icon>& icons = m_icons;
//
//         magic_enum::enum_for_each<GameResourceType>(
//             [&icons](auto val)
//             {
//                 constexpr GameResourceType resource = val;
//
//                 if (resource != GameResourceType::None)
//                 {
//                     if (icons.count(resource) == 0)
//                     {
//                         bee::ui::Icon icon;
//
//                         icons.emplace(resource, icon);
//                     }
//                 }
//             });
//         m_icons = icons;
//         SaveIcons();
//     }
// }
//
// #ifdef BEE_INSPECTOR
// void ResourceUI::Inspect()
//{
//     ImGui::Begin("ResourceUI");
//     auto icons = m_icons;
//     magic_enum::enum_for_each<GameResourceType>(
//         [&icons](auto val)
//         {
//             constexpr GameResourceType resource = val;
//             if (resource != GameResourceType::None)
//             {
//                 Icon& icon = icons.at(resource);
//                 ImGui::Text(std::string(magic_enum::enum_name(resource)).c_str());
//                 if (ImGui::InputText(std::string(std::string("Icon") + std::string(" ##Resource") +
//                                                  std::string(magic_enum::enum_name(resource)))
//                                          .c_str(),
//                                      &icon.iconPath))
//                 {
//                     if (fileExists(Engine.FileIO().GetPath(FileIO::Directory::Asset, icon.iconPath)))
//                     {
//                         float width = 0, height = 0;
//                         GetPngImageDimensions(Engine.FileIO().GetPath(FileIO::Directory::Asset, icon.iconPath), width,
//                         height); icon.iconTextureCoordinates.z = width; icon.iconTextureCoordinates.w = height;
//                     }
//                 }
//                 ImGui::InputFloat2(std::string(std::string("coordinates (in pixels)") + std::string(" ##Resource") +
//                                                std::string(magic_enum::enum_name(resource)))
//                                        .c_str(),
//                                    &icon.iconTextureCoordinates.x);
//                 ImGui::InputFloat2(std::string(std::string("size (in pixels)") + std::string(" ##Resource") +
//                                                std::string(magic_enum::enum_name(resource)))
//                                        .c_str(),
//                                    &icon.iconTextureCoordinates.z);
//                 ImGui::Separator();
//             }
//         });
//     m_icons = icons;
//     if (ImGui::Button("Apply##Resource"))
//     {
//         SaveIcons();
//     }
//     ImGui::End();
// }
// #endif