// #pragma once
// #include <unordered_map>
//
// #include "actors/actor_utils.hpp"
// #include "actors/props/resource_system.hpp"
// #include "actors/props/resource_type.hpp"
// #include "core/ecs.hpp"
// #include "user_interface/user_interface_structs.hpp"
//
// void CallbackFunc(GameResourceType type, int amount);
//
// constexpr float textSize = 0.2f;
// class ResourceUI : public bee::System
//{
// public:
//     ResourceUI();
//     void const UpdateUI(GameResourceType type, int newAmount);
//     ~ResourceUI();
//
//     // This is a handler for the canvas with the resources
//     bee::ui::UIElementID element = entt::null;
//
// private:
//     friend class InGameUI;
//     std::map<GameResourceType, bee::ui::UIComponentID> m_components;
//
//     std::unordered_map<GameResourceType, bee::ui::Icon> m_icons;
//     void SaveIcons();
//     void LoadIcons();
// #ifdef BEE_INSPECTOR
//     void Inspect() override;
// #endif
// };