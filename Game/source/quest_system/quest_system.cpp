#include "quest_system/quest_system.hpp"

#include "actors/structures/structure_manager_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/transform.hpp"
#include "magic_enum/magic_enum.hpp"
#include "physics/physics_components.hpp"
#include "quest_system/objectives/collect_objective.hpp"
#include "quest_system/objectives/train_units_objective.hpp"
#include "quest_system/timer.hpp"
#include "rendering/debug_render.hpp"
#include "tools/log.hpp"
#include "user_interface/font_handler.hpp"
#include "user_interface/user_interface.hpp"

using namespace bee;

QuestSystem::QuestSystem()
{
    Title = "QuestSystem";

    auto& UI = Engine.ECS().GetSystem<bee::ui::UserInterface>();
    // ui::Font font;
    m_elementID = UI.CreateUIElement(ui::Alignment::top, ui::Alignment::left);
}

bee::Quest& QuestSystem::CreateQuest(const std::string& questName)
{
    m_totalQuests.push_back(Quest(questName));
    return m_totalQuests.back();
}

bee::Stage& QuestSystem::CreateStage(bee::Quest& quest, const std::string& stageName)
{
    quest.stages.push_back(bee::Stage(stageName));
    return quest.stages.back();
}

void bee::QuestSystem::CreateObjective(bee::Stage& stage, const std::string& objectiveName,
                                       const bee::ObjectiveType objectiveType, const int objectiveQuantity,
                                       const std::string& lookForHandle, const Team objectiveTeam)
{
    switch (objectiveType)
    {
        case ObjectiveType::Kill:
        {
            auto newKillObjective =
                std::make_shared<KillObjective>(objectiveName, objectiveQuantity, lookForHandle, objectiveTeam);
            stage.objectives.push_back(newKillObjective);
            break;
        }
        case ObjectiveType::Build:
        {
            auto newBuildObjective =
                std::make_shared<BuildObjective>(objectiveName, objectiveQuantity, lookForHandle, objectiveTeam);
            stage.objectives.push_back(newBuildObjective);
            break;
        }
        case ObjectiveType::Collect:
        {
            auto newCollectObjective =
                std::make_shared<CollectObjective>(objectiveName, objectiveQuantity, lookForHandle, objectiveTeam);
            stage.objectives.push_back((newCollectObjective));
            break;
        }
        case ObjectiveType::TrainUnits:
        {
            auto newTrainObjective =
                std::make_shared<TrainUnitsObjective>(objectiveName, objectiveQuantity, lookForHandle, objectiveTeam);
            stage.objectives.push_back((newTrainObjective));
            break;
        }
        default:
        {
            auto objectiveTypeName = magic_enum::enum_name(objectiveType);
            Log::Error("Can't create an objective of type {} with this constructor.", objectiveTypeName.data());
            break;
        }
    }
}

void QuestSystem::CreateObjective(bee::Stage& stage, const std::string& objectiveName, bee::ObjectiveType objectiveType,
                                  const int objectiveQuantity, const std::string& lookForHandle, const Team objectiveTeam,
                                  const glm::vec3& position, const float radius)
{
    switch (objectiveType)
    {
        case ObjectiveType::Location:
        {
            auto newLocationObjective = std::make_shared<LocationObjective>(objectiveName, objectiveQuantity, lookForHandle,
                                                                            objectiveTeam, position, radius);
            stage.objectives.push_back(newLocationObjective);
            break;
        }
        default:
        {
            auto objectiveTypeName = magic_enum::enum_name(objectiveType);
            Log::Error("Can't create an objective of type {} with this constructor.", objectiveTypeName.data());
            break;
        }
    }
}
void QuestSystem::Update(float dt)
{
    if (m_currentQuestIndex < m_totalQuests.size())
    {
        auto& currentQuest = m_totalQuests[m_currentQuestIndex];
        currentQuest.Update(m_elementID);

        if (currentQuest.GetQuestState() == QuestState::Completed)
        {
            m_currentQuestIndex++;
            if (m_currentQuestIndex < m_totalQuests.size())
            {
                auto& nextQuest = m_totalQuests[m_currentQuestIndex];
                nextQuest.Initialize(m_elementID);
                nextQuest.StartQuest();
            }
        }
    }
}

void QuestSystem::Render()
{
    if (m_currentQuestIndex < m_totalQuests.size())
    {
        for (const auto& stage : m_totalQuests[m_currentQuestIndex].stages)
        {
            if (stage.GetStageState() == QuestState::InProgress)
            {
                for (const auto& objective : stage.objectives)
                {
                    if (auto locationObjective = std::dynamic_pointer_cast<LocationObjective>(objective))
                    {
                        locationObjective->Render();
                    }
                }
            }
        }
    }
}
