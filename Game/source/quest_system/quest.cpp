#include "quest_system/quest.hpp"

#include "core/engine.hpp"
#include "quest_system/stage.hpp"
#include "quest_system/timer.hpp"
#include "user_interface/user_interface.hpp"

bee::Quest::Quest(const std::string& name)
{
    m_questName = name;
    m_timer = std::make_unique<Timer>();
}

void bee::Quest::Update(const bee::ui::UIElementID& elementID)
{
    // Update Stages
    if (m_state == QuestState::InProgress && m_currentStageIndex < stages.size())
    {
        stages[m_currentStageIndex].Update();
        if (stages[m_currentStageIndex].GetStageState() == QuestState::Completed)
        {
            m_currentStageIndex++;
            if (m_currentStageIndex < stages.size())
            {
                stages[m_currentStageIndex].Initialize(elementID, m_currentStageIndex,
                                                       stages[m_currentStageIndex - 1].objectives.size());
                stages[m_currentStageIndex].StartStage();
            }
        }
    }

    // Check if we quest should be marked as Completed.
    if (CheckCompletion())
    {
        if (!m_timerStarted)
        {
            m_timer->Start(std::chrono::milliseconds(1000));
            m_timerStarted = true;
        }
        if (m_timer->IsTimeUp())
        {
            m_state = QuestState::Completed;
            m_shouldDisplay = false;
            Clear();
        }
    }
}

void bee::Quest::StartQuest()
{
    m_state = QuestState::InProgress;
    if (!stages.empty())
    {
        stages.front().StartStage();  // Start only the first stage
    }
}

void bee::Quest::SetName(const bee::ui::UIElementID& elementID)
{
    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    float xThrowAway = 0.0f;
    m_questText = UI.CreateString(elementID, 0, m_questName, glm::vec2(m_questOffset, m_nextPointy), m_fontSize, glm::vec4(1),
                                  xThrowAway, m_nextPointy, false, 0);
}

void bee::Quest::Initialize(const bee::ui::UIElementID& elementID)
{
    SetName(elementID);
    if (!stages.empty())
    {
        // Initialize only the first stage
        stages[m_currentStageIndex].Initialize(elementID, m_currentStageIndex, 0);
    }
}

void bee::Quest::Clear()
{
    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    UI.DeleteComponent(m_questText);
    for (auto& stage : stages)
    {
        stage.Clear();
    }
}

bool bee::Quest::CheckCompletion()
{
    for (auto& stage : stages)
    {
        if (stage.GetStageState() != QuestState::Completed) return false;
    }
    return true;
}