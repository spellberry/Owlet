#pragma once
#include <string>
#include <vector>
#include <memory>
#include "quest_system/stage.hpp"

namespace bee
{
class Stage;
class Timer;
class Quest
{
public:
    Quest(const std::string& name);
    void Update(const bee::ui::UIElementID& elementID);
    void StartQuest();
    const QuestState& GetQuestState() const { return m_state; }

    void SetName(const bee::ui::UIElementID& elementID);
    void Initialize(const bee::ui::UIElementID& elementID);
    void Clear();

private: // functions
    bool CheckCompletion();

public: // params
    std::vector<Stage> stages;
    int m_currentStageIndex = 0;

private: // params
    std::string m_questName = "Default quest name";
    QuestState m_state = QuestState::NotStarted;
    bool m_timerStarted = false;
    bool m_shouldDisplay = false;

    std::unique_ptr<Timer> m_timer;
    //UI
    bee::ui::UIComponentID m_questText = -1;

    float m_fontSize = 0.1f;
    float m_questOffset = 0.01f;
    float m_nextPointy = 0.11f;
};
}  // namespace bee