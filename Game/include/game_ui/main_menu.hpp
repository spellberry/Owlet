#pragma once

#include "core/game_base.hpp"
#include "user_interface/user_interface_structs.hpp"

enum state
{
    mainMenu = 1,
    options = 2,
    levels = 3
};

class MainMenu : public bee::GameBase
{
public:
    void Init() override;
    void ShutDown() override;
    void Update(float dt) override;

    void ToMenu(state stat) const;

private:

    void SetUpMenu();
    bee::ui::UIElementID m_menuButtons = entt::null;
    bee::ui::UIElementID m_menuBackground = entt::null;
    bee::ui::UIElementID m_menuPicture = entt::null;

    std::vector<entt::entity> m_toClearEntities;
    bee::ui::UIElementID m_element = entt::null;
    bee::ui::UIElementID m_options = entt::null;
    bee::ui::UIElementID m_levels = entt::null;
    bee::ui::UIImageElement m_buttonBack = bee::ui::UIImageElement(glm::vec2(6679, 4847), glm::vec2(1195, 144));
    bee::ui::UIImageElement m_hover = bee::ui::UIImageElement(glm::vec2(6695, 4191), glm::vec2(1195, 144));
    unsigned int m_hoverid = -1;
    state m_state = mainMenu;

    bee::ui::UIComponentID m_startSC;
    bee::ui::UIComponentID m_startTD;
    bee::ui::UIComponentID m_optionsReturnButt;
    bee::ui::UIComponentID m_levelsReturnButt;
    float m_elementSize = 0.4f;
};