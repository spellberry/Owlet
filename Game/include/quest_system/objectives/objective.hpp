#pragma once
#include <string>

#include "user_interface/user_interface_structs.hpp"
#include "actors/actor_utils.hpp"

namespace bee
{

enum class QuestState
{
    NotStarted,
    InProgress,
    Completed,
};

enum class ObjectiveType
{
    Location,  // enumerator used for moving the army to a different position
    Kill,      // enumerator used for killing a certain number of enemies
    Build,
    Collect,
    TrainUnits
};

class Objective
{
public:
    Objective(const std::string& a_name, const int a_quantity, const std::string& a_handle, const Team a_team)
    {
        objectiveName = a_name;
        quantity = a_quantity;
        handle = a_handle;
        team = a_team;
    }
    ~Objective() = default;

    // Different implementation for each objective type. Call Objective::Update in the end of the Update function of the derived classes.
    virtual void Update();
    /*void SetObjectiveStateToNotStarted() { state = QuestState::NotStarted; }
    void SetObjectiveStateToInProgress() { state = QuestState::InProgress; }
    void SetObjectiveStateToCompleted() { state = QuestState::Completed; }*/

    // Different implementation for each objective type. Call Objective::StartObjective in the end of the Update function of the derived classes.
    virtual void StartObjective() { state = QuestState::InProgress; }
    const QuestState& GetObjectiveState() const { return state; }
    const float GetYOffset() const { return m_objectiveYOffset; }

    void SetName(const bee::ui::UIElementID& elementID, const int objectiveIndex, const int stageIndex, const float stageOffset);
    void SetText(const bee::ui::UIElementID& elementID);

protected:  // functions
    void UpdateUI();

public:  // params
    std::string objectiveName = "Default objective name";
    std::string handle = "";  // A handle for what type of objects/entities the objective should look for (ex: type of units to
                              // kill, type of resources to gather)
    Team team = Team::Ally;
    int quantity = 0;         // How many steps of the objective are completed (ex: 2/4 Fighters killed)
    int currentObjectiveProgress = 0;  // The amount of steps needed in order to complete the objective (ex: Kill 4 Fighters)
    QuestState state = QuestState::NotStarted;

    // ui
    bee::ui::UIComponentID objectiveText = -1;
    bee::ui::UIComponentID objectiveNameText = -1;
    glm::vec2 descriptionMax = glm::vec2(0.0f);

    float m_fontSize = 0.1f;
    float m_objectiveOffset = 0.05f;
    float m_objectiveYOffset = 0.02f;
    float m_nextPointy = 0.15f;

protected:  // params
};

}  // namespace bee
