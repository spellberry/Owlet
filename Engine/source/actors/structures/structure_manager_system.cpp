#include "actors/structures/structure_manager_system.hpp"

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <utility>

#include "ai/ai_behavior_selection_system.hpp"
#include "ai/grid_navigation_system.hpp"
#include "ai/navmesh_agent.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "level_editor/terrain_system.hpp"
#include "level_editor/utils.hpp"
#include "magic_enum/magic_enum_utility.hpp"
#include "particle_system/particle_emitter.hpp"
#include "particle_system/particle_system.hpp"
#include "physics/physics_components.hpp"
#include "rendering/model.hpp"
#include "tools/debug_metric.hpp"
#include "tools/log.hpp"
#include "tools/serialize_glm.h"
#include "tools/tools.hpp"

// credit to stack overflow user:
// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void StructureManager::AddNewStructureTemplate(StructureTemplate structureTemplate)
{
    if (m_Structures.find(structureTemplate.name) != m_Structures.end())
    {
        bee::Log::Warn("Strucuture Name " + structureTemplate.name + " is already used. Try another name.");
        return;
    }
    structureTemplate.model = bee::Engine.Resources().Load<bee::Model>(structureTemplate.modelPath);
    structureTemplate.corpseModel = bee::Engine.Resources().Load<bee::Model>(structureTemplate.corpsePath);

    if (structureTemplate.materialPaths.empty() && structureTemplate.materials.empty())
    {
        structureTemplate.materialPaths.resize(structureTemplate.model->GetMeshes().size());
        structureTemplate.materials.resize(structureTemplate.model->GetMeshes().size());

        for (int i = 0; i < structureTemplate.materialPaths.size(); i++)
        {
            structureTemplate.materialPaths[i] = "materials/Empty.pepimat";
            structureTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(structureTemplate.materialPaths[i]);
        }
    }
    else
    {
        structureTemplate.materials.resize(structureTemplate.model->GetMeshes().size());

        for (int i = 0; i < structureTemplate.materialPaths.size(); i++)
        {
            structureTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(structureTemplate.materialPaths[i]);
        }
    }

    m_Structures.insert(std::pair<std::string, StructureTemplate>(structureTemplate.name, structureTemplate));
}

void StructureManager::RemoveStructureTemplate(const std::string& structureHandle)
{
    if (m_Structures.find(structureHandle) == m_Structures.end())
    {
        bee::Log::Warn("There is no unit type with the name " + structureHandle + ". Try another name.");
        return;
    }
    m_Structures.erase(structureHandle);
}

StructureTemplate& StructureManager::GetStructureTemplate(const std::string& structureHandle)
{
    auto structure = m_Structures.find(structureHandle);
    if (structure == m_Structures.end())
    {
        bee::Log::Warn("There is no unit type with the name " + structureHandle + ". Try another name.");
        return defaultStructureTemplate;
    }
    return structure->second;
}


std::pair<std::string,StructureTemplate> StructureManager::FindTemplateOfLevel(const double level, const StructureTypes type)
{
    auto& pairs = m_Structures;
    for(auto& pair : pairs)
    {
        if (pair.second.GetAttribute(BaseAttributes::BuildingLevel) == level && pair.second.structureType == type)
        {
            return pair;
        }
    }
    return {};
}

void StructureManager::Update(float dt)
{
    const auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent,AllyStructure,bee::Transform>();

    for (const auto entity : view)
    {
        const auto& agent = view.get<bee::ai::StateMachineAgent>(entity);
        auto& attributes = view.get<AttributesComponent>(entity);
        auto& buildingTransform = view.get<bee::Transform>(entity);

        if (attributes.GetValue(BaseAttributes::HitPoints) <= 0)
        {
            agent.context.blackboard->SetData("IsDead", true);
        }

        if (attributes.GetValue(BaseAttributes::HitPoints) /GetStructureTemplate(attributes.GetEntityType()).GetAttribute(BaseAttributes::HitPoints) <= 0.5)
        {
            const auto hurtBuildingVFX = bee::Engine.ECS().Registry.try_get<HurtBuildingVFX>(entity);
            if (!hurtBuildingVFX)
            {
                auto& vfx = bee::Engine.ECS().CreateComponent<HurtBuildingVFX>(entity);
                const int numberOfFires = bee::GetRandomNumberInt(2,6);
                for (int i = 0; i < numberOfFires; i++)
                {
                    const auto particle = bee::Engine.ECS().CreateEntity();
                    auto& particleTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(particle);
                    auto& emitter = bee::Engine.ECS().CreateComponent<bee::ParticleEmitter>(particle);

                    emitter.AssignEntity(particle);

                    particleTransform.Translation = buildingTransform.Translation;
                    particleTransform.Translation.z += bee::GetRandomNumber(1.0f,4.0f);
                    particleTransform.Translation.x += bee::GetRandomNumber(-attributes.GetValue(BaseAttributes::DiskScale),
                                                                            attributes.GetValue(BaseAttributes::DiskScale));
                    particleTransform.Translation.y += bee::GetRandomNumber(-attributes.GetValue(BaseAttributes::DiskScale),
                                                                            attributes.GetValue(BaseAttributes::DiskScale));

                    bee::Engine.ECS().GetSystem<bee::ParticleSystem>().LoadEmitterFromTemplate(emitter, "E_Fire.pepitter");
                    vfx.vfxEntities.push_back(particle);
                }
            }
        }
    }
}

void StructureManager::SaveStructureData()
{
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();

    std::ofstream os1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, "structureAttributes"));
    cereal::JSONOutputArchive archive1(os1);

    std::vector<std::pair<AttributesComponent, bee::Transform>> structures;

    for (auto& entity : view)
    {
        auto [structureAttirbutes, transform] = view.get(entity);
        structures.push_back(std::pair<AttributesComponent, bee::Transform>(structureAttirbutes, transform));
    }
    archive1(CEREAL_NVP(structures));
}

void StructureManager::LoadStructureData(const std::string& fileName)
{
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_attributes.json")))
    {
        std::ifstream is1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_attributes.json"));
        cereal::JSONInputArchive archive1(is1);

        std::vector<std::pair<AttributesComponent, bee::Transform>> attributes;

        archive1(CEREAL_NVP(attributes));
        for (int i = 0; i < attributes.size(); i++)
        {
            if (m_Structures.find(attributes[i].first.GetEntityType()) == m_Structures.end()) continue;
            auto unitEntity1 = SpawnStructure(attributes[i].first.GetEntityType(), attributes[i].second.Translation,
                               attributes[i].first.smallGridIndex, attributes[i].first.flipped, static_cast<Team>(attributes[i].first.GetTeam()));
            bee::Engine.ECS().Registry.get<bee::Transform>(unitEntity1.value()).Rotation = attributes[i].second.Rotation;
            bee::Engine.ECS().Registry.get<bee::Transform>(unitEntity1.value()).Scale = attributes[i].second.Scale;
        }
    }
}

void StructureManager::SaveStructureTemplates(const std::string& fileName)
{
    std::unordered_map<std::string, StructureTemplate> templateData = m_Structures;
    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_StructureTemplates.json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(templateData));
}

void StructureManager::LoadStructureTemplates(const std::string& fileName)
{
    m_Structures.clear();
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_StructureTemplates.json")))
    {
        std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_StructureTemplates.json"));
        cereal::JSONInputArchive archive(is);

        std::unordered_map<std::string, StructureTemplate> templateData;
        archive(CEREAL_NVP(templateData));
        for (auto it = templateData.begin(); it != templateData.end(); ++it)
        {
            AddNewStructureTemplate(it->second);
        }
    }
}

void StructureManager::ReloadStructuresFromTemplate(const std::string& structureHandle)
{
    auto model_view = bee::Engine.ECS().Registry.view<bee::Transform, StructureModelTag>();
    for (auto model_entity : model_view)
    {
        auto [transform, modelTag] = model_view.get(model_entity);
        if (modelTag.structureTemplate == structureHandle)
        {
            bee::Engine.ECS().DeleteEntity(model_entity);
        }
    }

    // delete all StateMachineAgent components
    auto sma_view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::ai::StateMachineAgent, bee::Transform>();
    for (auto entity : sma_view)
    {
        auto [structureAttribute, agent, transform] = sma_view.get(entity);
        if (structureAttribute.GetEntityType() == structureHandle)
        {
            bee::Engine.ECS().Registry.remove<bee::ai::StateMachineAgent>(entity);
        }
    }

    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();
    for (auto entity : view)
    {
        auto [structureAttribute, transform] = view.get(entity);
        if (structureAttribute.GetEntityType() == structureHandle)
        {
            StructureTemplate& structureTemplate = GetStructureTemplate(structureAttribute.GetEntityType());
            // update attributes
            structureAttribute.SetAttributes( GetStructureTemplate(structureAttribute.GetEntityType()).GetAttributes());
            // update fsm
            const std::shared_ptr<bee::ai::FiniteStateMachine> fsm = bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>(
                GetStructureTemplate(structureAttribute.GetEntityType()).fsmPath);
            auto& agent = bee::Engine.ECS().CreateComponent<bee::ai::StateMachineAgent>(entity, *fsm);
            agent.context.entity = entity;
            // update model
            transform.NoChildren();
            transform.Scale = glm::vec3(m_Structures.at(structureAttribute.GetEntityType()).GetAttribute(BaseAttributes::Scale));
            auto modelEntity = bee::Engine.ECS().CreateEntity();
            auto& modelTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(modelEntity);
            modelTransform.Name = "Model";
            modelTransform.SetParent(entity);
            modelTransform.Translation += structureTemplate.modelOffset.Translation;
            auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
            auto modelOffsetEuler = glm::eulerAngles(structureTemplate.modelOffset.Rotation);
            modelEuler += modelOffsetEuler;
            modelTransform.Rotation = glm::quat(modelEuler);
            auto& modelTag = bee::Engine.ECS().CreateComponent<StructureModelTag>(modelEntity);
            modelTag.structureTemplate = structureAttribute.GetEntityType();
            auto model =
                bee::Engine.Resources().Load<bee::Model>(GetStructureTemplate(structureAttribute.GetEntityType()).modelPath);
            auto path = GetStructureTemplate(structureAttribute.GetEntityType()).modelPath;
            model->Instantiate(modelEntity);
            int index = 0;
            bee::UpdateMeshRenderer(entity, structureTemplate.materials, index);
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

//std::optional<bee::Entity> StructureManager::SpawnStructure(const std::string& structureTemplateHandle, const glm::vec3& position, const glm::quat& rotation, Team team)
//{
//    std::optional<bee::Entity> entity = SpawnStructure(structureTemplateHandle, position, team);
//    if (entity.has_value())
//    {
//        auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(entity.value());
//        transform.Rotation = rotation;
//    }
//
//    return entity;
//}

std::optional<bee::Entity> StructureManager::SpawnStructure(const std::string& structureTemplateHandle, const glm::vec3& position, const int smallGridIndex, const bool flipped, Team team)
{
    if (m_Structures.find(structureTemplateHandle) == m_Structures.end())
    {
        bee::Log::Warn("There is no structures type with the name " + structureTemplateHandle + ". Try another name.");
        return {};
    }

    // structure template and entity
    auto& structureTemplate = m_Structures.find(structureTemplateHandle)->second;
    auto strucutureEntity = bee::Engine.ECS().CreateEntity();

    // creating transform
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(strucutureEntity);
    transform.Translation = position;
    transform.Scale = glm::vec3(GetStructureTemplate(structureTemplateHandle).GetAttribute(BaseAttributes::Scale));
    transform.Name += (" " + structureTemplateHandle);

    // creating base attributes
    auto& baseAttributes = bee::Engine.ECS().CreateComponent<AttributesComponent>(strucutureEntity);
    baseAttributes.SetAttributes( structureTemplate.GetAttributes());
    baseAttributes.SetEntityType( structureTemplate.name);
    baseAttributes.smallGridIndex = smallGridIndex;
    baseAttributes.flipped = flipped;
    baseAttributes.SetTeam(static_cast<int>(team));

    const std::shared_ptr<bee::ai::FiniteStateMachine> fsm =
        bee::Engine.Resources().Load<bee::ai::FiniteStateMachine>(structureTemplate.fsmPath);

    // agent
    auto& agent = bee::Engine.ECS().CreateComponent<bee::ai::StateMachineAgent>(strucutureEntity, *fsm);
    agent.context.entity = strucutureEntity;

    // physics body and collider
    auto& body =
        bee::Engine.ECS().CreateComponent<bee::physics::Body>(strucutureEntity, bee::physics::Body::Type::Static, 1.0f, 0.0f);
    auto& disk = bee::Engine.ECS().CreateComponent<bee::physics::DiskCollider>(
        strucutureEntity, GetStructureTemplate(structureTemplateHandle).GetAttribute(BaseAttributes::DiskScale));
    body.SetPosition(transform.Translation);

    // model entity
    auto modelEntity = bee::Engine.ECS().CreateEntity();

    // model transform
    auto& modelTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(modelEntity);
    modelTransform.Name = "Model";
    modelTransform.SetParent(strucutureEntity);
    modelTransform.Translation += structureTemplate.modelOffset.Translation;
    auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
    auto modelOffsetEuler = glm::eulerAngles(structureTemplate.modelOffset.Rotation);
    modelEuler += modelOffsetEuler;
    modelTransform.Rotation = glm::quat(modelEuler);

    // model tag
    auto& modelTag = bee::Engine.ECS().CreateComponent<StructureModelTag>(modelEntity);
    modelTag.structureTemplate = structureTemplateHandle;

    // model
    auto model = bee::Engine.Resources().Load<bee::Model>(structureTemplate.modelPath);
    model->Instantiate(modelEntity);
    int index = 0;
    bee::UpdateMeshRenderer(modelEntity, structureTemplate.materials, index);

    // training/production building
    for (const auto element : structureTemplate.availableOrders)
    {
        auto name = std::string(magic_enum::enum_name(element));

        for (auto& c : name)
        {
            c = std::tolower(c);
        }

        if (name.find("train") != std::string::npos)
        {
            auto& spawningStructure = bee::Engine.ECS().CreateComponent<SpawningStructure>(strucutureEntity);
            spawningStructure.rallyPoint = transform.Translation - glm::vec3(0, 2, 0) * static_cast<float>(baseAttributes.GetValue(BaseAttributes::DiskScale));
            const auto flagEntity = bee::Engine.ECS().CreateEntity();
            auto& flagTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(flagEntity);
            flagTransform.Translation = spawningStructure.rallyPoint;
            flagTransform.Name = "Flag";
            const auto flagModel = bee::Engine.Resources().Load<bee::Model>("models/flag.glb");
            spawningStructure.flagEntity = flagEntity;
            spawningStructure.spawnLimit = static_cast<int>(baseAttributes.GetValue(BaseAttributes::MaxTrainedUnitsQueued));
            flagModel->Instantiate(flagEntity);

            bee::GetComponentInChildren<bee::MeshRenderer>(flagEntity).Material =bee::Engine.Resources().Load<bee::Material>("materials/flag.pepimat");

            break;
        }
    }

    // assigning a team
    if (team == Team::Ally)
    {
        bee::Engine.ECS().CreateComponent<AllyStructure>(strucutureEntity);
        auto interactable = bee::Engine.ECS().CreateComponent<bee::physics::Interactable>(strucutureEntity);
    }
    else if (team == Team::Enemy)
    {
        bee::Engine.ECS().CreateComponent<EnemyStructure>(strucutureEntity);
    }
    else if (team == Team::Neutral)
    {
        bee::Engine.ECS().CreateComponent<NeutralStructure>(strucutureEntity);
    }

    // mark tiles underneath
    auto& terrainSystem = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    std::vector<int> occupiedTiles{};
    terrainSystem.GetOccupiedTilesFromObject(smallGridIndex, structureTemplate.tileDimensions.x, structureTemplate.tileDimensions.y, flipped, occupiedTiles);
    if (occupiedTiles.empty())
    {
        bee::Log::Warn("Couldn't create a polygon collider for {}", structureTemplate.name);
        return strucutureEntity;
    }

    for (auto tileIndex : occupiedTiles)
    {
        int flag = 0;
        flag &= ~lvle::TileFlags::Traversible;
        flag |= lvle::TileFlags::NoGroundTraverse;
        flag |= lvle::TileFlags::NoBuild;
        terrainSystem.SetTileFlags(tileIndex, static_cast<lvle::TileFlags>(flag));
    }
    terrainSystem.UpdateTerrainDataComponent();
    bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();

    // create structure collider
    int colliderTileA = 0;
    int colliderTileB = structureTemplate.tileDimensions.x - 1;
    int colliderTileC = (structureTemplate.tileDimensions.x - 1) + (structureTemplate.tileDimensions.y - 1) * structureTemplate.tileDimensions.x;
    int colliderTileD = (structureTemplate.tileDimensions.y - 1) * structureTemplate.tileDimensions.x;
    
    glm::ivec2 tileGridCoordsA = terrainSystem.IndexToCoords(occupiedTiles[colliderTileA], terrainSystem.GetWidthInTiles());
    glm::ivec2 tileGridCoordsB = terrainSystem.IndexToCoords(occupiedTiles[colliderTileB], terrainSystem.GetWidthInTiles());
    glm::ivec2 tileGridCoordsC = terrainSystem.IndexToCoords(occupiedTiles[colliderTileC], terrainSystem.GetWidthInTiles());
    glm::ivec2 tileGridCoordsD = terrainSystem.IndexToCoords(occupiedTiles[colliderTileD], terrainSystem.GetWidthInTiles());

    int colliderPointA = tileGridCoordsA.x + tileGridCoordsA.y * terrainSystem.GetWidthInTiles() + tileGridCoordsA.y;
    int colliderPointB = (tileGridCoordsB.x + tileGridCoordsB.y * terrainSystem.GetWidthInTiles() + tileGridCoordsB.y) + 1;
    int colliderPointC = (tileGridCoordsC.x + tileGridCoordsC.y * terrainSystem.GetWidthInTiles() + tileGridCoordsC.y) + (terrainSystem.GetWidthInTiles() + 1) + 1;
    int colliderPointD = (tileGridCoordsD.x + tileGridCoordsD.y * terrainSystem.GetWidthInTiles() + tileGridCoordsD.y) + (terrainSystem.GetWidthInTiles() + 1);

    std::string name = structureTemplateHandle + "StructureCollider";
    auto colliderEntity =
        terrainSystem.CreateTerrainColliderEntity({colliderPointA, colliderPointB, colliderPointC, colliderPointD}, name);
    auto& colliderTransform = bee::Engine.ECS().Registry.get<bee::Transform>(colliderEntity);
    colliderTransform.SetParent(strucutureEntity);

    if (structureTemplate.HasAttribute(BaseAttributes::BuffRange))
    {
        auto& tower = bee::Engine.ECS().CreateComponent<BuffStructure>(strucutureEntity);
        tower.buffType = structureTemplate.buffedAttribute;
        tower.buffModifier = StatModifier(ModifierType::Additive, structureTemplate.GetAttribute(BaseAttributes::BuffValue));
    }

    if (team == Team::Ally)
    {
        for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
            if (debugMetric.buildingSpawned.find(structureTemplateHandle) == debugMetric.buildingSpawned.end())
                debugMetric.buildingSpawned.insert(std::pair<std::string, int>(structureTemplateHandle, 1));
        else
                debugMetric.buildingSpawned[structureTemplateHandle]++;
    }

    return strucutureEntity;
}
#ifdef BEE_INSPECTOR
void StructureManager::Inspect(bee::Entity e)
{
    /*System::Inspect(e);
    ImGui::Text("Attributes");
    AttributesComponent* attributes = bee::Engine.ECS().Registry.try_get<AttributesComponent>(e);
    if (!attributes) return;
    for (auto element : attributes->attributes)
    {
        ImGui::InputDouble(std::string(magic_enum::enum_name(element.first)).c_str(),
                           &attributes->attributes.at(element.first));
    }*/

    SpawningStructure* spawning = bee::Engine.ECS().Registry.try_get<SpawningStructure>(e);
    if (spawning)
    {
        bee::Engine.DebugRenderer().AddCircle(bee::DebugCategory::Gameplay, spawning->rallyPoint, 1.0f, glm::vec4(1, 0, 0, 1));
    }
}
#endif

void StructureManager::RemoveStructuresOfTemplate(std::string structureHandle)
{
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent>();
    for (auto unit : view)
    {
        auto [attrbiutes] = view.get(unit);
        if (attrbiutes.GetEntityType() == structureHandle)
        {
            bee::Engine.ECS().DeleteEntity(unit);
        }
    }
}

void StructureManager::RemoveStructure(bee::Entity structure)
{
    // Change the flags of previously occupied tiles
    auto& terrainSystem = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(structure);
    const auto& structureTemplate = GetStructureTemplate(attributes.GetEntityType());
    std::vector<int> occupiedTiles{};

    terrainSystem.GetOccupiedTilesFromObject(attributes.smallGridIndex, structureTemplate.tileDimensions.x, structureTemplate.tileDimensions.y, attributes.flipped, occupiedTiles);

    for (const auto tileIndex : occupiedTiles)
    {
        terrainSystem.SetTileFlags(tileIndex, lvle::TileFlags::Traversible);
    }
    terrainSystem.UpdateTerrainDataComponent();
    bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().UpdateFromTerrain();

    // Remove rally point flag (if there is one)
    SpawningStructure* spawningStructure = bee::Engine.ECS().Registry.try_get<SpawningStructure>(structure);
    if (spawningStructure)
    {
        bee::Engine.ECS().DeleteEntity(spawningStructure->flagEntity);
    }
    // delete structure entity
    bee::Engine.ECS().DeleteEntity(structure);
}
