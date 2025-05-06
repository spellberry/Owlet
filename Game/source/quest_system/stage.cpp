#include "quest_system/stage.hpp"

#include "core/engine.hpp"
#include "quest_system/quest.hpp"
#include "user_interface/user_interface.hpp"
using namespace bee;

bee::Stage::Stage(const std::string& name) { m_stageName = name; }

void bee::Stage::Update()
{
    // Update Objectives
    if (m_state == QuestState::InProgress)
    {
        for (auto& objective : objectives)
        {
            objective->Update();
        }
    }

    // Check if we stage should be marked as Completed.
    if (CheckCompletion()) m_state = QuestState::Completed;
}

void bee::Stage::StartStage()
{
    m_state = QuestState::InProgress;
    for (auto& objective : objectives)
    {
        objective->StartObjective();
    }
}

void bee::Stage::SetName(const bee::ui::UIElementID& elementID, const int stageIndex, const int previousStageObjectivesCount)
{
    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    float xThrowAway = 0.0f;
    float yPos = static_cast<float>(stageIndex) * m_stageYOffset + objectives[0]->GetYOffset() * previousStageObjectivesCount +
                 m_nextPointy;
    stageText = UI.CreateString(elementID, 0, m_stageName, glm::vec2(m_stageOffset, yPos), m_fontSize, glm::vec4(1), xThrowAway,
                                m_nextPointy, false, 0);
}

void bee::Stage::Initialize(const bee::ui::UIElementID& elementID, const int stageIndex, const int previousStageObjectivesCount)
{
    SetName(elementID, stageIndex, previousStageObjectivesCount);
    for (int i = 0; i < objectives.size(); i++)
    {
        float yPos =
            static_cast<float>(stageIndex) * m_stageYOffset + objectives[0]->GetYOffset() * previousStageObjectivesCount;
        objectives[i]->SetName(elementID, i, stageIndex, yPos);
        objectives[i]->SetText(elementID);
    }
}

void bee::Stage::Clear()
{
    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    UI.DeleteComponent(stageText);
    for (auto& objective : objectives)
    {
        UI.DeleteComponent(objective->objectiveText);
        UI.DeleteComponent(objective->objectiveNameText);
    }
}

bool bee::Stage::CheckCompletion()
{
    for (auto& objective : objectives)
    {
        if (objective->GetObjectiveState() != QuestState::Completed) return false;
    }
    return true;
}