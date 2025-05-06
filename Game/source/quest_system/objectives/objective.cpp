#include "quest_system/objectives/objective.hpp"

#include "core/engine.hpp"
#include "user_interface/user_interface.hpp"

void bee::Objective::Update()
{
    if (quantity <= currentObjectiveProgress) state = QuestState::Completed;
    UpdateUI();
}

void bee::Objective::SetName(const bee::ui::UIElementID& elementID, const int objectiveIndex, const int stageIndex,
                             const float stageOffset)
{
    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    float yPos =
        static_cast<float>(stageIndex) * stageOffset + static_cast<float>(objectiveIndex) * m_objectiveYOffset + m_nextPointy;
    descriptionMax.y = yPos;
    objectiveNameText = UI.CreateString(elementID, 0, objectiveName, glm::vec2(m_objectiveOffset, yPos), m_fontSize,
                                        glm::vec4(1), descriptionMax.x, m_nextPointy, false, 0);
}

void bee::Objective::SetText(const bee::ui::UIElementID& elementID)
{
    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    std::string m_progressText = " " + std::to_string(currentObjectiveProgress) + " / " + std::to_string(quantity);
    int reserves = std::to_string(quantity).length();
    objectiveText = UI.CreateString(elementID, 0, m_progressText, descriptionMax, m_fontSize, glm::vec4(1), reserves - 1);
}

void bee::Objective::UpdateUI()
{
    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    std::string m_progressText = " " + std::to_string(currentObjectiveProgress) + " / " + std::to_string(quantity);
    float throwaway = 0;
    float throwaway2 = 0;
    objectiveText = UI.ReplaceString(objectiveText, m_progressText, throwaway, throwaway2, false);
}
