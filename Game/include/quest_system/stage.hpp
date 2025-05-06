#pragma once
#include "quest_system/objectives/kill_objective.hpp"
#include "quest_system/objectives/location_objective.hpp"
#include "quest_system/objectives/build_objective.hpp"

namespace bee
{
class Quest;
class Stage
{
public:
    Stage(const std::string& name);
    ~Stage() = default;
    void Update();
    void StartStage();
    const QuestState& GetStageState() const { return m_state; }

    void SetName(const bee::ui::UIElementID& elementID, const int stageIndex, const int previousStageObjectivesCount);
    void Initialize(const bee::ui::UIElementID& elementID, const int stageIndex, const int previousStageObjectivesCount);
    void Clear();

    bool CheckCompletion();
private: // functions

public: // params

    std::vector<std::shared_ptr<bee::Objective>> objectives = {};

private: // params
    std::string m_stageName = "Default stage name";
    QuestState m_state = QuestState::NotStarted;

    //UI
    bee::ui::UIComponentID stageText = -1;

    float m_fontSize = 0.1f;
    float m_stageOffset = 0.03f;
    float m_stageYOffset = 0.03f;
    float m_nextPointy = 0.13f;
};

}  // namespace bee