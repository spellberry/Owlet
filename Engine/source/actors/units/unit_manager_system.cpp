#include "actors/units/unit_manager_system.hpp"

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <utility>

#include "actors/attributes.hpp"

#include "ai/ai_behavior_selection_system.hpp"
#include "ai/grid_navigation_system.hpp"
#include "ai/navmesh_agent.hpp"

#include "animation/animation_state.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"

#include "level_editor/terrain_system.hpp"

#include "magic_enum/magic_enum_utility.hpp"

#include "physics/physics_components.hpp"

#include "platform/dx12/skeleton.hpp"

#include "rendering/model.hpp"

#include "tools/3d_utility_functions.hpp"
#include "tools/log.hpp"
#include "tools/serialize_glm.h"
#include "tools/tools.hpp"

#include "platform/dx12/skeleton.hpp"

#include "animation/animation_state.hpp"

#include "level_editor/terrain_system.hpp"
#include "particle_system/particle_emitter.hpp"
#include "particle_system/particle_system.hpp"
#include "tools/debug_metric.hpp"

// credit to stack overflow user:
// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void UnitManager::AddNewUnitTemplate(UnitTemplate unitTemplate)
{
    if (m_Units.find(unitTemplate.name) != m_Units.end())
    {
        bee::Log::Warn("Unit Name " + unitTemplate.name + " is already used. Try another name.");
        return;
    }
    unitTemplate.model = bee::Engine.Resources().Load<bee::Model>(unitTemplate.modelPath);
    unitTemplate.corpseModel = bee::Engine.Resources().Load<bee::Model>(unitTemplate.corpsePath);

    if (unitTemplate.materialPaths.empty() && unitTemplate.materials.empty())
    {
        unitTemplate.materialPaths.resize(unitTemplate.model->GetMeshes().size());
        unitTemplate.materials.resize(unitTemplate.model->GetMeshes().size());

        for (int i = 0; i < unitTemplate.materialPaths.size(); i++)
        {
            unitTemplate.materialPaths[i] = "materials/Empty.pepimat";
            unitTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(unitTemplate.materialPaths[i]);
        }
    }
    else
    {
        unitTemplate.materials.resize(unitTemplate.model->GetMeshes().size());

        for (int i = 0; i < unitTemplate.materialPaths.size(); i++)
        {
            unitTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(unitTemplate.materialPaths[i]);
        }
    }

    m_Units.insert(std::pair<std::string, UnitTemplate>(unitTemplate.name, unitTemplate));
}

void UnitManager::RemoveUnitTemplate(const std::string& unitHandle)
{
    if (m_Units.find(unitHandle) == m_Units.end())
    {
        bee::Log::Warn("There is no unit type with the name " + unitHandle + ". Try another name.");
        return;
    }
    m_Units.erase(unitHandle);
}

UnitTemplate& UnitManager::GetUnitTemplate(const std::string& unitHandle)
{
    auto unit = m_Units.find(unitHandle);
    if (unit == m_Units.end())
    {
        bee::Log::Warn("There is no unit type with the name " + unitHandle + ". Try another name.");
        // return {};
        return defaultUnitTemplate;
    }
    return unit->second;
}

void UnitManager::Update(float dt)
{
    const auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent>();

    for (const auto entity : view)
    {
        const auto& agent = view.get<bee::ai::StateMachineAgent>(entity);
        auto& attributes = view.get<AttributesComponent>(entity);
        if (attributes.GetValue(BaseAttributes::HitPoints) <= 0)
        {
            agent.context.blackboard->SetData("IsDead", true);
        }
    }

    UpdateProps();
}

void UnitManager::LoadUnitData(const std::string& fileName)
{
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_attributes.json")))
    {
        std::ifstream is1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_attributes.json"));
        cereal::JSONInputArchive archive1(is1);

        std::vector<std::pair<AttributesComponent, bee::Transform>> attributes;

        archive1(CEREAL_NVP(attributes));
        for (int i = 0; i < attributes.size(); i++)
        {
            if (m_Units.find(attributes[i].first.GetEntityType()) == m_Units.end()) continue;
            auto unitEntity1 = SpawnUnit(attributes[i].first.GetEntityType(), attributes[i].second.Translation,
                                         static_cast<Team>(attributes[i].first.GetTeam()));
            if (!unitEntity1.has_value()) continue;
            bee::Engine.ECS().Registry.get<bee::Transform>(unitEntity1.value()).Rotation = attributes[i].second.Rotation;
            bee::Engine.ECS().Registry.get<bee::Transform>(unitEntity1.value()).Scale = attributes[i].second.Scale;
        }
    }
}

void UnitManager::SaveUnitTemplates(const std::string& fileName)
{
    std::unordered_map<std::string, UnitTemplate> templateData = m_Units;
    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_UnitTemplates.json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(templateData));
}

void UnitManager::LoadUnitTemplates(const std::string& fileName)
{
    m_Units.clear();
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_UnitTemplates.json")))
    {
        std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_UnitTemplates.json"));
        cereal::JSONInputArchive archive(is);

        std::unordered_map<std::string, UnitTemplate> templateData;
        archive(CEREAL_NVP(templateData));
        for (auto it = templateData.begin(); it != templateData.end(); ++it)
        {
            AddNewUnitTemplate(it->second);
        }
    }
}

void UnitManager::ReloadUnitsFromTemplate(const std::string& unitHandle)
{
    // delete all Model entities which are children to the unit entities of the type in the function's argument.
    auto model_view = bee::Engine.ECS().Registry.view<bee::Transform, UnitModelTag>();
    for (auto model_entity : model_view)
    {
        auto [transform, modelTag] = model_view.get(model_entity);
        if (modelTag.unitType == unitHandle)
        {
            bee::Engine.ECS().DeleteEntity(model_entity);
        }
    }

    // delete all StateMachineAgent components
    auto stateMachine_view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent, bee::Transform>();
    for (auto entity : stateMachine_view)
    {
        auto [unitAttribute, stateMachineAgent, transform] = stateMachine_view.get(entity);
        if (unitAttribute.GetEntityType() == unitHandle)
        {
            bee::Engine.ECS().Registry.remove<bee::ai::StateMachineAgent>(entity);
        }
    }

    // delete all AnimationAgent components
    auto animation_view = bee::Engine.ECS().Registry.view<AttributesComponent, AnimationAgent, bee::Transform>();
    for (auto entity : animation_view)
    {
        auto [unitAttribute, animationAgent, transform] = animation_view.get(entity);
        if (unitAttribute.GetEntityType() == unitHandle)
        {
            bee::Engine.ECS().Registry.remove<AnimationAgent>(entity);
        }
    }

    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();
    for (auto entity : view)
    {
        auto [unitAttribute, transform] = view.get(entity);
        if (unitAttribute.GetEntityType() == unitHandle)
        {
            UnitTemplate& unitTemplate = GetUnitTemplate(unitAttribute.GetEntityType());
            // update attributes
            unitAttribute.SetAttributes(unitTemplate.GetAttributes());
            // update fsm
            const std::shared_ptr<bee::ai::FiniteStateMachine> fsm =
                bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>(unitTemplate.fsmPath);

            const std::shared_ptr<bee::ai::FiniteStateMachine> animationController =
                bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>(unitTemplate.animationControllerPath);

            if (!unitTemplate.animationControllerPath.empty())
            {
                const auto& animator = bee::Engine.ECS().CreateComponent<AnimationAgent>(entity, animationController);
                const auto ids = animator.fsm->GetStateIDsOfType<AnimationState>();

                for (const auto id : ids)
                {
                    auto& state = animator.fsm->GetState(id);
                    const auto animState = dynamic_cast<AnimationState*>(&state);
                    if (!animState) continue;
                    animState->LoadAnimations();
                }
                animator.context.blackboard->SetData("MoveSpeed", 0.0f);
            }

            InitAnimators(unitHandle);
            if (!bee::Engine.ECS().Registry.try_get<bee::ai::StateMachineAgent>(entity))
            {
                auto& agent = bee::Engine.ECS().CreateComponent<bee::ai::StateMachineAgent>(entity, *fsm);
                agent.context.entity = entity;
            }

            // agent.fsm.Execute(agent.context);
            // agent.SetStateOfType<IdleState>();

            // update model
            transform.NoChildren();
            transform.Scale = glm::vec3(m_Units.at(unitAttribute.GetEntityType()).GetAttribute(BaseAttributes::Scale));
            auto modelEntity = bee::Engine.ECS().CreateEntity();
            auto& modelTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(modelEntity);
            modelTransform.Name = "Model";
            modelTransform.SetParent(entity);
            modelTransform.Translation += unitTemplate.modelOffset.Translation;
            auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
            auto modelOffsetEuler = glm::eulerAngles(unitTemplate.modelOffset.Rotation);
            modelEuler += modelOffsetEuler;
            modelTransform.Rotation = glm::quat(modelEuler);
            auto& modelTag = bee::Engine.ECS().CreateComponent<UnitModelTag>(modelEntity);
            modelTag.unitType = unitAttribute.GetEntityType();
            auto model = bee::Engine.Resources().Load<bee::Model>(unitTemplate.modelPath);
            auto path = unitTemplate.modelPath;
            model->Instantiate(modelEntity);
            int index = 0;
            bee::UpdateMeshRenderer(modelEntity, unitTemplate.materials, index);
            
        }
    }

    bee::Engine.Resources().CleanUp();

    auto agentView = bee::Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent>();
    for (const auto entity : agentView)
    {
        const auto& agent = agentView.get<bee::ai::StateMachineAgent>(entity);
        if (view.get<AttributesComponent>(entity).GetValue(BaseAttributes::HitPoints) <= 0)
        {
            agent.context.blackboard->SetData("IsDead", true);
        }
    }
}

std::optional<bee::Entity> UnitManager::SpawnUnit(const std::string& unitTemplateHandle, const glm::vec3& position, Team team)
{
    if (m_Units.find(unitTemplateHandle) == m_Units.end())
    {
        bee::Log::Warn("There is no unit type with the name " + unitTemplateHandle + ". Try another name.");
        return {};
    }
    auto& unitTemplate = m_Units.find(unitTemplateHandle)->second;
    auto unitEntity = bee::Engine.ECS().CreateEntity();

    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(unitEntity);
        transform.Translation = position;
        transform.Name = unitTemplateHandle;

    auto& terrain_system = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();

    glm::vec3 cameraPosition;
    const auto screen = bee::Engine.ECS().Registry.view<bee::Transform, bee::Camera>();
    for (auto& entity : screen)
    {
        auto [transform, camera] = screen.get(entity);
        cameraPosition = transform.Translation;
    }

    glm::vec3 point;
    const bool result = terrain_system.FindRayMeshIntersection(cameraPosition, position - cameraPosition, point);

    if (result)
    {
        transform.Translation.z = point.z;
    }

    transform.Scale = glm::vec3(GetUnitTemplate(unitTemplateHandle).GetAttribute(BaseAttributes::Scale));

    auto& baseAttributes = bee::Engine.ECS().CreateComponent<AttributesComponent>(unitEntity);
    baseAttributes.SetAttributes(unitTemplate.GetAttributes());
    baseAttributes.SetEntityType(unitTemplate.name);

    const std::shared_ptr<bee::ai::FiniteStateMachine> fsm =
        bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>(unitTemplate.fsmPath);

    if (!unitTemplate.animationControllerPath.empty())
    {
        const std::shared_ptr<bee::ai::FiniteStateMachine> animationController =bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>(unitTemplate.animationControllerPath);
        const auto& animator = bee::Engine.ECS().CreateComponent<AnimationAgent>(unitEntity, animationController);
        const auto ids = animator.fsm->GetStateIDsOfType<AnimationState>();

        for (const auto id : ids)
        {
            auto& state = animator.fsm->GetState(id);
            const auto animState = dynamic_cast<AnimationState*>(&state);
            if (!animState) continue;
            animState->LoadAnimations();
        }
        animator.context.blackboard->SetData("MoveSpeed", 0.0f);
    }

    auto& agent = bee::Engine.ECS().CreateComponent<bee::ai::StateMachineAgent>(unitEntity, *fsm);
        agent.context.entity = unitEntity;
    auto& gridAgent = bee::Engine.ECS().CreateComponent<bee::ai::GridAgent>(
        unitEntity, 0.5f, baseAttributes.GetValue(BaseAttributes::MovementSpeed),
        baseAttributes.GetValue(BaseAttributes::Height));
    gridAgent.verticalPosition = transform.Translation.z;
    auto& body =
        bee::Engine.ECS().CreateComponent<bee::physics::Body>(unitEntity, bee::physics::Body::Type::Dynamic, 1.0f, 0.0f);
    auto& disk = bee::Engine.ECS().CreateComponent<bee::physics::DiskCollider>(
        unitEntity, GetUnitTemplate(unitTemplateHandle).GetAttribute(BaseAttributes::DiskScale));

    body.SetPosition(transform.Translation);
    baseAttributes.SetTeam(static_cast<int>(team));

    // interactable

    auto modelEntity = bee::Engine.ECS().CreateEntity();
    auto& modelTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(modelEntity);
        modelTransform.Name = unitTemplateHandle;
        modelTransform.SetParent(unitEntity);
        modelTransform.Translation += unitTemplate.modelOffset.Translation;
    auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
    auto modelOffsetEuler = glm::eulerAngles(unitTemplate.modelOffset.Rotation);
        modelEuler += modelOffsetEuler;
        modelTransform.Rotation = glm::quat(modelEuler);
    auto& modelTag = bee::Engine.ECS().CreateComponent<UnitModelTag>(modelEntity);
        modelTag.unitType = unitTemplateHandle;
    auto model = bee::Engine.Resources().Load<bee::Model>(unitTemplate.modelPath);
        model->Instantiate(modelEntity);

    InitAnimators(unitTemplateHandle);
    int index = 0;
    bee::UpdateMeshRenderer(modelEntity, unitTemplate.materials, index);

    if (team == Team::Ally)
    {
        bee::Engine.ECS().CreateComponent<AllyUnit>(unitEntity);
        auto interactable = bee::Engine.ECS().CreateComponent<bee::physics::Interactable>(unitEntity);
    }
    else if (team == Team::Enemy)
    {
        bee::Engine.ECS().CreateComponent<EnemyUnit>(unitEntity);
    }
    else if (team == Team::Neutral)
    {
        bee::Engine.ECS().CreateComponent<NeutralUnit>(unitEntity);
    }

    if (team == Team::Ally)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
            if (debugMetric.allyUnitsSpawned.find(unitTemplateHandle) == debugMetric.allyUnitsSpawned.end())
                debugMetric.allyUnitsSpawned.insert(std::pair<std::string, int>(unitTemplateHandle, 1));
        else
                debugMetric.allyUnitsSpawned[unitTemplateHandle]++;
    }
    else if (team == Team::Enemy)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
            if (debugMetric.enemyUnitsSpawned.find(unitTemplateHandle) == debugMetric.enemyUnitsSpawned.end())
                debugMetric.enemyUnitsSpawned.insert(std::pair<std::string, int>(unitTemplateHandle, 1));
        else
                debugMetric.enemyUnitsSpawned[unitTemplateHandle]++;
    }
    AttachProp(modelEntity, modelTransform);

    if (baseAttributes.GetValue(BaseAttributes::WoodBounty) > 0)
    {
        auto& emitter = bee::Engine.ECS().CreateComponent<bee::ParticleEmitter>(unitEntity);
        emitter.AssignEntity(unitEntity);
        bee::Engine.ECS().GetSystem<bee::ParticleSystem>().LoadEmitterFromTemplate(emitter, "E_Leaf.pepitter");
        emitter.SetLoop(true);
    }

    return unitEntity;
}

void UnitManager::InitAnimators(const std::string& unitTemplateHandle)
{
    auto view = bee::Engine.ECS().Registry.view<bee::MeshRenderer, bee::Transform>();
    const auto& unitTemplate = GetUnitTemplate(unitTemplateHandle);

    if (!unitTemplate.animationControllerPath.empty())
    {
        for (const auto entity : view)
        {
            auto t = bee::Engine.ECS().Registry.try_get<bee::Transform>(entity);
            auto& renderer = view.get<bee::MeshRenderer>(entity);

            bee::Entity current = entity;
            while (t->HasParent())
            {
                if (!bee::Engine.ECS().Registry.try_get<bee::Transform>(current)) break;
                t = bee::Engine.ECS().Registry.try_get<bee::Transform>(current);
                const AnimationAgent* agent = bee::Engine.ECS().Registry.try_get<AnimationAgent>(current);

                // This happens in UpdateMeshRenderers (int tools.hpp) anyway.
                /*if (bee::Engine.ECS().Registry.try_get<AttributesComponent>(current))
                {
                    if (!unitTemplate.materialPath.empty() &&
                        bee::Engine.ECS().Registry.try_get<AttributesComponent>(current)->GetEntityType() == unitTemplateHandle)
                    {
                        renderer.Material = unitTemplate.material;
                    }
                }*/

                if (agent)
                {
                    agent->context.blackboard->SetData<std::shared_ptr<bee::Skeleton>>("Skeleton", renderer.Skeleton);
                    break;
                }
                current = t->GetParent();
            }
        }
    }
}

std::vector<bee::Entity> UnitManager::SpawnUnits(std::vector<std::string> unitTemplateHandles, std::vector<glm::vec3> positions, std::vector<Team> teams)
{
    if (unitTemplateHandles.size() != positions.size() && positions.size() != teams.size())
    {
        bee::Log::Warn(
            "There is a difference in the number of unitTemplateHandles elements, positions and teams. UnitTemplateHandles "
            "number: i%, "
            "Positions number: i%, Teams number: i%",
            unitTemplateHandles.size(), positions.size(), teams.size());
    }
    int i = 0;
    while (i < unitTemplateHandles.size())
    {
        if (m_Units.find(unitTemplateHandles[i]) == m_Units.end())
        {
            bee::Log::Warn("There is no unit type with the name " + unitTemplateHandles[i] + ". Try another name.");
            unitTemplateHandles.erase(std::next(unitTemplateHandles.begin(), i));
            if (positions.size() > i)
            {
                positions.erase(std::next(positions.begin(), i));
            }
            if (teams.size() > i)
            {
                teams.erase(std::next(teams.begin(), i));
            }
        }
        else
        {
            ++i;
        }
    }

    for (int i = 0; i < unitTemplateHandles.size(); i++)
    {
        if (positions.size() < i + 1)
        {
            positions.push_back(glm::vec3(0.0f));
        }
        if (teams.size() < i + 1)
        {
            teams.push_back(Team::Ally);
        }
    }

    std::vector<entt::entity> entities;
    entities.resize(unitTemplateHandles.size());
    bee::Engine.ECS().Registry.create(entities.begin(), entities.end());

    bee::Engine.ECS().Registry.insert<AttributesComponent>(entities.begin(), entities.end());
    bee::Engine.ECS().Registry.insert<bee::Transform>(entities.begin(), entities.end());

    AttributesComponent baseAttributes{};

    for (int index = 0; index < entities.size(); index++)
    {
        baseAttributes.SetAttributes(m_Units.find(unitTemplateHandles[index])->second.GetAttributes());
        baseAttributes.SetEntityType(m_Units.find(unitTemplateHandles[index])->second.name);
        const std::shared_ptr<bee::ai::FiniteStateMachine> fsm =
            bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>(m_Units.find(unitTemplateHandles[index])->second.fsmPath);

        auto& agent = bee::Engine.ECS().CreateComponent<bee::ai::StateMachineAgent>(entities[index], *fsm);
        agent.context.entity = entities[index];

        bee::Engine.ECS().CreateComponent<bee::ai::GridAgent>(entities[index], 0.5f,
                                                              baseAttributes.GetValue(BaseAttributes::MovementSpeed), 0.85f);
        bee::Engine.ECS().CreateComponent<bee::physics::Body>(entities[index], bee::physics::Body::Type::Kinematic, 1.0f, 1.0f);

        bee::Engine.ECS().Registry.get<AttributesComponent>(entities[index]) = baseAttributes;
        bee::Engine.ECS().Registry.get<bee::Transform>(entities[index]).Translation = positions[index];

        if (teams[index] == Team::Ally)
        {
            bee::Engine.ECS().CreateComponent<AllyUnit>(entities[index]);
        }
        else
        {
            bee::Engine.ECS().CreateComponent<EnemyUnit>(entities[index]);
        }
    }
    return entities;
}

#ifdef BEE_INSPECTOR

void UnitManager::Inspect(bee::Entity e)
{
    /*ImGui::Text("AttributesComponent");
    AttributesComponent* attributes = bee::Engine.ECS().Registry.try_get<AttributesComponent>(e);
    if (!attributes) return;*/
    /*magic_enum::enum_for_each<BaseAttributes>(
        [attributes](auto val)
        {
            constexpr BaseAttributes attribute = val;
            const auto& it = attributes->attributes.find(attribute);
            if (it != attributes->attributes.end())
            ImGui::InputDouble(std::string(magic_enum::enum_name(attribute)).c_str(), &it->second);
        });*/
}
#endif

void UnitManager::RemoveUnitsOfTemplate(const std::string& unitHandle)
{
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent>();
    for (auto unit : view)
    {
        auto [attrbiutes] = view.get(unit);
        if (attrbiutes.GetEntityType() == unitHandle)
        {
            bee::Engine.ECS().DeleteEntity(unit);
        }
    }
}

void UnitManager::RemoveUnit(bee::Entity unit) { bee::Engine.ECS().DeleteEntity(unit); }

void UnitManager::AttachProp(bee::Entity parentEntity, const bee::Transform& parentTransform)
{
    const std::string nameMage = "Mage", nameWarrior = "Warrior";

    if (parentTransform.Name == nameMage)
    {
        ConfigureMage(parentEntity, parentTransform);
    }
    else if (parentTransform.Name == nameWarrior)
    {
        ConfigureWarrior(parentEntity, parentTransform);
    }
}

void UnitManager::ConfigureMage(bee::Entity parentEntity, const bee::Transform& parentTransform)
{
    // Need to get the skeleton from the mesh renderer component not the skeleton directly
    // Also the mesh renderer component is always a child to whatever entity you instantiated �\_(-_-#)_/�
    auto& tempTransform = bee::Engine.ECS().Registry.get<bee::Transform>(parentEntity);
    bee::MeshRenderer* skeletonMeshRenderer = nullptr;

    for (auto child = tempTransform.begin(); child != tempTransform.end(); ++child)
    {
        if (bee::Engine.ECS().Registry.all_of<bee::MeshRenderer>(*child))
        {
            skeletonMeshRenderer = bee::Engine.ECS().Registry.try_get<bee::MeshRenderer>(*child);
        }
    }

    // Early out if for some reason there is no skeleton
    if (skeletonMeshRenderer == nullptr)
    {
        printf("Entity Mage %i did not get their skeleton configured properly.\n", parentEntity);
        return;
    }

    const int index = skeletonMeshRenderer->Skeleton->GiveNameGetIndex("j_wingRoll_B_L");          // Getting the joint index for mages
    const glm::mat4 boneTransform = skeletonMeshRenderer->Skeleton->GiveIndexGetMatrixGLM(index);  // Getting bone glm matrix based on bone index
    const glm::mat4 worldTransform = parentTransform.World();          // Getting mage transform
    const glm::mat4 finalTransform = worldTransform * boneTransform;   // multiplying the matrices

    // Preparing the model and material
    auto model = bee::Engine.Resources().Load<bee::Model>("models/Unit Props/Staff.glb");

    // Creating prop entity
    bee::Entity propEntity = bee::Engine.ECS().CreateEntity();
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(propEntity);
        transform.Name = "Staff";
        transform.Translation = glm::vec3(finalTransform[3][0], finalTransform[3][1], finalTransform[3][2]);
        transform.Scale = glm::vec3(0.9f, 0.9f, 0.9f);
        transform.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    InstantiateProp(propEntity, *model.get(), "materials/M_Mage_Prop.pepimat");

    // Assigning the prop to the parent entity
    UnitPropTagR& propTag = bee::Engine.ECS().CreateComponent<UnitPropTagR>(parentEntity);
        propTag.propRight = propEntity;
        propTag.jointRightArm = index;
}

void UnitManager::ConfigureWarrior(bee::Entity parentEntity, const bee::Transform& parentTransform)
{
    // Need to get the skeleton from the mesh renderer component not the skeleton directly
    // Also the mesh renderer component is always a child to whatever entity you instantiated �\_(-_-#)_/�
    auto& tempTransform = bee::Engine.ECS().Registry.get<bee::Transform>(parentEntity);
    bee::MeshRenderer* skeletonMeshRenderer = nullptr;

    for (auto child = tempTransform.begin(); child != tempTransform.end(); ++child)
    {
        if (bee::Engine.ECS().Registry.all_of<bee::MeshRenderer>(*child))
        {
            skeletonMeshRenderer = bee::Engine.ECS().Registry.try_get<bee::MeshRenderer>(*child);
        }
    }

    // Early out if for some reason there is no skeleton
    if (skeletonMeshRenderer == nullptr)
    {
        printf("Entity Warrior %i did not get their skeleton configured properly.\n", parentEntity);
        return;
    }

    const int indexL = skeletonMeshRenderer->Skeleton->GiveNameGetIndex("j_wingRoll_B_L");           // Getting the joint index for mages
    const glm::mat4 boneTransformL = skeletonMeshRenderer->Skeleton->GiveIndexGetMatrixGLM(indexL);  // Getting bone glm matrix based on bone index
    const glm::mat4 worldTransform = parentTransform.World();           // Getting mage transform
    const glm::mat4 finalTransformL = worldTransform * boneTransformL;  // multiplying the matrices

    // Loading the new material
    auto modelSpear = bee::Engine.Resources().Load<bee::Model>("models/Unit Props/Spear.glb");
    auto modelShield = bee::Engine.Resources().Load<bee::Model>("models/Unit Props/Shield.glb");

    // Creating prop entity
    bee::Entity propEntityL = bee::Engine.ECS().CreateEntity();
    auto& transformL = bee::Engine.ECS().CreateComponent<bee::Transform>(propEntityL);
        transformL.Name = "Spear";
        transformL.Translation = glm::vec3(finalTransformL[3][0], finalTransformL[3][1], finalTransformL[3][2]);
        transformL.Scale = glm::vec3(1.0f, 1.0f, 1.0f);
        transformL.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    InstantiateProp(propEntityL, *modelSpear.get(), "materials/M_Warrior_Prop.pepimat");

    // Assigning the prop to the parent entity
    UnitPropTagL& propTagL = bee::Engine.ECS().CreateComponent<UnitPropTagL>(parentEntity);
        propTagL.propLeft = propEntityL;
        propTagL.jointLeftArm = indexL;

    // Redoing everything but with the shield
    const int indexR = skeletonMeshRenderer->Skeleton->GiveNameGetIndex("j_wingRoll_B_R");           // Getting the joint index for mages
    const glm::mat4 boneTransformR = skeletonMeshRenderer->Skeleton->GiveIndexGetMatrixGLM(indexR);  // Getting bone glm matrix based on bone index
    const glm::mat4 finalTransformR = worldTransform * boneTransformR;                               // multiplying the matrices

    // Creating prop entity
    bee::Entity propEntityR = bee::Engine.ECS().CreateEntity();
    auto& transformR = bee::Engine.ECS().CreateComponent<bee::Transform>(propEntityR);
        transformR.Name = "Shield";
        transformR.Translation = glm::vec3(finalTransformR[3][0], finalTransformR[3][1], finalTransformR[3][2]);
        transformR.Scale = glm::vec3(1.2f, 1.2f, 1.2f);
        transformR.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    InstantiateProp(propEntityR, *modelShield.get(), "materials/M_Warrior_Prop.pepimat");

    // Assigning the prop to the parent entity
    UnitPropTagR& propTagR = bee::Engine.ECS().CreateComponent<UnitPropTagR>(parentEntity);
        propTagR.propRight = propEntityR;
        propTagR.jointRightArm = indexR;
}

void UnitManager::UpdateProps()
{ 
    // Only get the units that have props
    const auto viewRight = bee::Engine.ECS().Registry.view<bee::Transform, UnitPropTagR>();
    for (const auto [parentEntity, parentTransform, parentProp] : viewRight.each())
    {
        bee::MeshRenderer* skeletonMeshRenderer = nullptr;
        for (auto child = parentTransform.begin(); child != parentTransform.end(); ++child)
        {
            if (bee::Engine.ECS().Registry.all_of<bee::MeshRenderer>(*child))
            {
                skeletonMeshRenderer = bee::Engine.ECS().Registry.try_get<bee::MeshRenderer>(*child);
            }
        }

        if (skeletonMeshRenderer == nullptr) continue;

        // Calculating the new transform of the proper
        const glm::mat4 boneTransform = skeletonMeshRenderer->Skeleton->GiveIndexGetMatrixGLM(parentProp.jointRightArm);
        const glm::mat4 worldTransform = parentTransform.WorldMatrix;
        const glm::mat4 finalTransform = worldTransform * boneTransform;

        glm::vec3 position = glm::vec3(0.0f), scale = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        bee::Decompose(finalTransform, position, scale, rotation);

        // Getting and setting the corresponding prop transform
        auto& propTransform = bee::Engine.ECS().Registry.get<bee::Transform>(parentProp.propRight);
            propTransform.Translation = position;
            propTransform.Rotation = rotation;
    }

    // Only get the units that have props
    const auto viewLeft = bee::Engine.ECS().Registry.view<bee::Transform, UnitPropTagL>();
    for (const auto [parentEntity, parentTransform, parentProp] : viewLeft.each())
    {
        bee::MeshRenderer* skeletonMeshRenderer = nullptr;
        for (auto child = parentTransform.begin(); child != parentTransform.end(); ++child)
        {
            if (bee::Engine.ECS().Registry.all_of<bee::MeshRenderer>(*child))
            {
                skeletonMeshRenderer = bee::Engine.ECS().Registry.try_get<bee::MeshRenderer>(*child);
            }
        }

        if (skeletonMeshRenderer == nullptr) continue;

        // Calculating the new transform of the proper
        const glm::mat4 boneTransform = skeletonMeshRenderer->Skeleton->GiveIndexGetMatrixGLM(parentProp.jointLeftArm);
        const glm::mat4 worldTransform = parentTransform.WorldMatrix;
        const glm::mat4 finalTransform = worldTransform * boneTransform;

        glm::vec3 position = glm::vec3(0.0f), scale = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        bee::Decompose(finalTransform, position, scale, rotation);

        // Getting and setting the corresponding prop transform
        auto& propTransform = bee::Engine.ECS().Registry.get<bee::Transform>(parentProp.propLeft);
            propTransform.Translation = position;
            propTransform.Rotation = rotation;
    }
}

void UnitManager::InstantiateProp(bee::Entity parentEntity, const bee::Model& model, const std::string& fileName)
{
    const auto& node = model.GetDocument().nodes[0];
    const auto entity = bee::Engine.ECS().CreateEntity();

    // Getting the transform from the node
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(entity);
    transform.Name = node.name;
    transform.SetParent(parentEntity);

    if (!node.matrix.empty())
    {
        glm::mat4 transformGLM = glm::make_mat4(node.matrix.data());
        bee::Decompose(transformGLM, transform.Translation, transform.Scale, transform.Rotation);
    }
    else
    {
        if (!node.scale.empty()) transform.Scale = bee::to_vec3(node.scale);
        if (!node.rotation.empty()) transform.Rotation = bee::to_quat(node.rotation);
        if (!node.translation.empty()) transform.Translation = bee::to_vec3(node.translation);
    }

    // Creating the mesh and material
    auto mesh = bee::Engine.Resources().Load<bee::Mesh>(model, static_cast<int>(0));
    auto material = bee::Engine.Resources().Load<bee::Material>(fileName);
    bee::Engine.ECS().CreateComponent<bee::MeshRenderer>(entity, mesh, material);
}