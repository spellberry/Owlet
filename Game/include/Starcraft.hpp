#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/game_base.hpp"
#include "rendering/image.hpp"
#include "tools/log.hpp"
#include "user_interface/user_interface_structs.hpp"

class Starcraft : public bee::GameBase
{
public:
    void Init() override;
    void ShutDown() override;
    void Update(float dt) override;
    void Render() override;
    void Pause(bool pause) override;
    void EndScreen();

    //Upon Shutdown
    void SaveLeaderboardScore();
    void SaveDebugMetrics();


     bee::ui::UIElementID GetDebugMetricData() const { return m_debugMetricData; };
    bee::ui::UIElementID GetDebugMetricBackground() const { return m_debugMetricBackground; };
     bee::ui::UIElementID GetDebugMetricButtons() const { return m_debugMetricButtons; };
    bee::ui::UIElementID GetDebugMetricBasic() const { return m_debugMetricBasic; };
     bee::ui::UIElementID GetDebugMetricUnits() const { return m_debugMetricUnits; };
    bee::ui::UIElementID GetDebugMetricStructure() const { return m_debugMetricStructures; };

    bool IsInputNameEnabled() const { return m_inputName; }
    void ChangeInputNameToggle(const bool newState) { m_inputName = newState; }

    bool GetDebugMetricState() const { return m_metricEnabled; }
    void SetDebugMetricState(const bool newState) { m_metricEnabled = newState; }

private:

    void InitializeSounds();

private:
    bool m_orderUnitToggle = false;
    bool m_endScreen = false;
    bool m_pause = false;
    bool m_metricEnabled = false;
    bool m_inputName = false;

    std::vector<entt::entity> m_toClearEntities;
    bee::ui::UIElementID m_element = entt::null;
    bee::ui::UIElementID m_background = entt::null;
    bee::ui::UIElementID m_debugMetricData = entt::null;
    bee::ui::UIElementID m_debugMetricBackground = entt::null;
    bee::ui::UIElementID m_debugMetricButtons = entt::null;
    bee::ui::UIElementID m_debugMetricBasic = entt::null;
    bee::ui::UIElementID m_debugMetricUnits = entt::null;
    bee::ui::UIElementID m_debugMetricStructures = entt::null;
    bee::ui::UIImageElement m_buttonBack = bee::ui::UIImageElement(glm::vec2(6679, 4847), glm::vec2(1195, 144));
    bee::ui::UIImageElement m_hover = bee::ui::UIImageElement(glm::vec2(6695, 4191), glm::vec2(1195, 144));
    unsigned int m_hoverid = -1;

    bee::ui::UIComponentID m_mainMenu;
    bee::ui::UIComponentID m_startSC;
    float m_elementSize = 0.4f;
    std::string m_levelName = "L_Release_v0.6";

    bee::ui::UIElementID m_pauseMenu = entt::null;
    bee::ui::UIElementID m_pauseMenuOpacity = entt::null;
    bee::ui::UIElementID m_pauseMenuBackground = entt::null;


    int m_playerScoreIndex = -1;
};
