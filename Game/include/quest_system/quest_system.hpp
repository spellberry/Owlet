#pragma once
#include <string>
#include <vector>

#include "core/ecs.hpp"
#include "glm/vec3.hpp"
#include "quest.hpp"
#include "user_interface/user_interface_structs.hpp"

class Timer;

namespace bee
{

class QuestSystem : public bee::System
{
public:
    QuestSystem();
    void Update(float dt) override;
    void Render() override;

    bee::Quest& CreateQuest(const std::string& questname);
    bee::Stage& CreateStage(bee::Quest& quest, const std::string& stageName);
    void CreateObjective(bee::Stage& stage, const std::string& objectiveName, const bee::ObjectiveType objectiveType,
                         const int objectiveQuantity, const std::string& lookForHandle, const Team objectiveTeam);
    void CreateObjective(bee::Stage& stage, const std::string& objectiveName, const bee::ObjectiveType objectiveType,
                         const int objectiveQuantity, const std::string& lookForHandle, const Team objectiveTeam,
                         const glm::vec3& position, const float radius);

    bee::ui::UIElementID& GetUIElementID() { return m_elementID; }

private:
    bee::Entity m_newEntity;
    std::vector<bee::Quest> m_totalQuests;
    int m_currentQuestIndex = 0;

    bee::ui::UIElementID m_elementID;
};

}  // namespace bee
