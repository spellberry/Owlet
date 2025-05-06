#include "actors/props/prop_manager_system.hpp"

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
#include "magic_enum/magic_enum_utility.hpp"
#include "physics/physics_components.hpp"
#include "rendering/model.hpp"
#include "tools/log.hpp"
#include "tools/serialize_glm.h"
#include "tools/tools.hpp"
#include "actors/props/resource_system.hpp"
#include "level_editor/terrain_system.hpp"
#include "level_editor/utils.hpp"

//credit to stack overflow user: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void PropManager::AddNewPropTemplate(PropTemplate propTemplate)
{
    if (m_Props.find(propTemplate.name) != m_Props.end())
    {
        bee::Log::Warn("Prop Name " + propTemplate.name + " is already used. Try another name.");
        return;
    }
    propTemplate.model = bee::Engine.Resources().Load<bee::Model>(propTemplate.modelPath);

    if (propTemplate.materialPaths.empty() && propTemplate.materials.empty())
    {
        propTemplate.materialPaths.resize(propTemplate.model->GetMeshes().size());
        propTemplate.materials.resize(propTemplate.model->GetMeshes().size());

        for (int i = 0; i < propTemplate.materialPaths.size(); i++)
        {
            propTemplate.materialPaths[i] = "materials/Empty.pepimat";
            propTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(propTemplate.materialPaths[i]);
        }
    }
    else
    {
        propTemplate.materials.resize(propTemplate.model->GetMeshes().size());

        for (int i = 0; i < propTemplate.materialPaths.size(); i++)
        {
            propTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(propTemplate.materialPaths[i]);
        }
    }

    m_Props.insert(std::pair<std::string, PropTemplate>(propTemplate.name, propTemplate));
}

void PropManager::RemovePropTemplate(std::string propHandle)
{
    if (m_Props.find(propHandle) == m_Props.end())
    {
        bee::Log::Warn("There is no prop type with the name " + propHandle + ". Try another name.");
        return;
    }
    m_Props.erase(propHandle);
}

PropTemplate& PropManager::GetPropTemplate(std::string propHandle)
{
    auto prop = m_Props.find(propHandle);
    if (prop == m_Props.end())
    {
        bee::Log::Warn("There is no prop type with the name " + propHandle + ". Try another name.");
        // return {};
        return defaultPropTemplate;
    }
    return prop->second;
}

void PropManager::Update(float dt) {}

void PropManager::SavePropData()
{
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();

    std::ofstream os1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, "propAttributes"));
    cereal::JSONOutputArchive archive1(os1);

    std::vector<std::pair<AttributesComponent, bee::Transform>> props;

    for (auto& entity : view)
    {
        auto [propAttributes, transform] = view.get(entity);
        props.push_back(std::pair<AttributesComponent, bee::Transform>(propAttributes, transform));
    }
    archive1(CEREAL_NVP(props));
}

void PropManager::LoadPropData(const std::string& fileName)
{
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_attributes.json")))
    {
        std::ifstream is1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_attributes.json"));
        cereal::JSONInputArchive archive1(is1);

        std::vector<std::pair<AttributesComponent, bee::Transform>> attributes;

        archive1(CEREAL_NVP(attributes));
        for (int i = 0; i < attributes.size(); i++)
        {
            if (m_Props.find(attributes[i].first.GetEntityType()) == m_Props.end()) continue;
            auto propEntity1 = SpawnProp(attributes[i].first.GetEntityType(), attributes[i].second.Translation,
                                         attributes[i].first.smallGridIndex, attributes[i].first.flipped);
            bee::Engine.ECS().Registry.get<bee::Transform>(propEntity1.value()).Rotation = attributes[i].second.Rotation;
            bee::Engine.ECS().Registry.get<bee::Transform>(propEntity1.value()).Scale = attributes[i].second.Scale;
        }
    }
}

void PropManager::SavePropTemplates(const std::string& fileName)
{
    std::unordered_map<std::string, PropTemplate> templateData = m_Props;
    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_PropTemplates.json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(templateData));
}

void PropManager::LoadPropTemplates(const std::string& fileName)
{
    m_Props.clear();
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_PropTemplates.json")))
    {
        std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_PropTemplates.json"));
        cereal::JSONInputArchive archive(is);

        std::unordered_map<std::string, PropTemplate> templateData;
        archive(CEREAL_NVP(templateData));
        for (auto it = templateData.begin(); it != templateData.end(); ++it)
        {
            AddNewPropTemplate(it->second);
        }
    }
}

void PropManager::ReloadPropsFromTemplate(std::string propHandle)
{
    auto model_view = bee::Engine.ECS().Registry.view<bee::Transform, PropModelTag>();
    for (auto model_entity : model_view)
    {
        auto [transform, modelTag] = model_view.get(model_entity);
        if (modelTag.propTemplate == propHandle)
        {
            bee::Engine.ECS().DeleteEntity(model_entity);
        }
    }

    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();
    for (auto entity : view)
    {
        auto [propAttribute, transform] = view.get(entity);
        if (propAttribute.GetEntityType() == propHandle)
        {
            PropTemplate& propTemplate = GetPropTemplate(propAttribute.GetEntityType());
            // update attributes
            propAttribute.SetAttributes(GetPropTemplate(propAttribute.GetEntityType()).GetAttributes());
            // update model
            transform.NoChildren();
            transform.Scale = glm::vec3(m_Props.at(propAttribute.GetEntityType()).GetAttribute(BaseAttributes::Scale));
            auto modelEntity = bee::Engine.ECS().CreateEntity();
            auto& modelTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(modelEntity);
            modelTransform.Name = "Model";
            modelTransform.SetParent(entity);
            modelTransform.Translation += propTemplate.modelOffset.Translation;
            auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
            auto modelOffsetEuler = glm::eulerAngles(propTemplate.modelOffset.Rotation);
            modelEuler += modelOffsetEuler;
            modelTransform.Rotation = glm::quat(modelEuler);
            auto& modelTag = bee::Engine.ECS().CreateComponent<PropModelTag>(modelEntity);
            modelTag.propTemplate = propAttribute.GetEntityType();
            auto model = bee::Engine.Resources().Load<bee::Model>(GetPropTemplate(propAttribute.GetEntityType()).modelPath);
            auto path = GetPropTemplate(propAttribute.GetEntityType()).modelPath;
            model->Instantiate(modelEntity);
            int index = 0;
            bee::UpdateMeshRenderer(modelEntity, propTemplate.materials, index);
        }
    }
    bee::Engine.Resources().CleanUp();
}

std::optional<bee::Entity> PropManager::SpawnProp(std::string propTemplateHandle, glm::vec3 position, const int smallGridIndex, const bool flipped)
{
    if (m_Props.find(propTemplateHandle) == m_Props.end())
    {
        bee::Log::Warn("There is no prop type with the name " + propTemplateHandle + ". Try another name.");
        return {};
    }

    // Prop Template
    auto& propTemplate = m_Props.find(propTemplateHandle)->second;
    auto propEntity = bee::Engine.ECS().CreateEntity();

    // Creating Transform
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(propEntity);
    transform.Translation = position;
    transform.Scale = glm::vec3(GetPropTemplate(propTemplateHandle).GetAttribute(BaseAttributes::Scale));
    transform.Name += (" " + propTemplateHandle);

    // Creating Base Attributes
    auto& baseAttributes = bee::Engine.ECS().CreateComponent<AttributesComponent>(propEntity);
    baseAttributes.SetAttributes(propTemplate.GetAttributes());
    baseAttributes.smallGridIndex = smallGridIndex;
    baseAttributes.flipped = flipped;
    baseAttributes.SetEntityType(propTemplate.name);

    //Creating Resourse Type
    if (propTemplate.resourceType != GameResourceType::None)
    {
        auto& propResourceType = bee::Engine.ECS().CreateComponent<PropResourceComponent>(propEntity);
        propResourceType.type = propTemplate.resourceType;
    }

    // Creating Body and Disk Collider
    auto& body =
        bee::Engine.ECS().CreateComponent<bee::physics::Body>(propEntity, bee::physics::Body::Type::Static, 1.0f, 0.0f);
    auto& disk = bee::Engine.ECS().CreateComponent<bee::physics::DiskCollider>(
        propEntity, GetPropTemplate(propTemplateHandle).GetAttribute(BaseAttributes::DiskScale));
    body.SetPosition(transform.Translation);

    // Creating Model entity and model instantiation
    auto modelEntity = bee::Engine.ECS().CreateEntity();
    auto& modelTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(modelEntity);
    modelTransform.Name = "Model";
    modelTransform.SetParent(propEntity);
    modelTransform.Translation += propTemplate.modelOffset.Translation;
    auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
    auto modelOffsetEuler = glm::eulerAngles(propTemplate.modelOffset.Rotation);
    modelEuler += modelOffsetEuler;
    modelTransform.Rotation = glm::quat(modelEuler);
    auto& modelTag = bee::Engine.ECS().CreateComponent<PropModelTag>(modelEntity);
    modelTag.propTemplate = propTemplateHandle;
    auto model = bee::Engine.Resources().Load<bee::Model>(propTemplate.modelPath);
    model->Instantiate(modelEntity);
    int index = 0;
    bee::UpdateMeshRenderer(modelEntity, propTemplate.materials, index);

    // mark tiles underneath
    auto& terrainSystem = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    std::vector<int> occupiedTiles{};
    terrainSystem.GetOccupiedTilesFromObject(smallGridIndex, propTemplate.tileDimensions.x, propTemplate.tileDimensions.y,
                                                flipped, occupiedTiles);

    for (auto tileIndex : occupiedTiles)
    {
        int flag = 0;
        flag &= ~lvle::TileFlags::Traversible;
        flag |= lvle::TileFlags::NoGroundTraverse;
        flag |= lvle::TileFlags::NoBuild;
        terrainSystem.SetTileFlags(tileIndex, static_cast<lvle::TileFlags>(flag));
    }

    // create prop collider
    if (propTemplate.createCollider)
    {
        int colliderTileA = 0;
        int colliderTileB = propTemplate.tileDimensions.x - 1;
        int colliderTileC =
            (propTemplate.tileDimensions.x - 1) + (propTemplate.tileDimensions.y - 1) * propTemplate.tileDimensions.y;
        int colliderTileD = (propTemplate.tileDimensions.y - 1) * propTemplate.tileDimensions.y;

        glm::ivec2 tileGridCoordsA = terrainSystem.IndexToCoords(occupiedTiles[colliderTileA], terrainSystem.GetWidthInTiles());
        glm::ivec2 tileGridCoordsB = terrainSystem.IndexToCoords(occupiedTiles[colliderTileB], terrainSystem.GetWidthInTiles());
        glm::ivec2 tileGridCoordsC = terrainSystem.IndexToCoords(occupiedTiles[colliderTileC], terrainSystem.GetWidthInTiles());
        glm::ivec2 tileGridCoordsD = terrainSystem.IndexToCoords(occupiedTiles[colliderTileD], terrainSystem.GetWidthInTiles());

        int colliderPointA = tileGridCoordsA.x + tileGridCoordsA.y * terrainSystem.GetWidthInTiles() + tileGridCoordsA.y;
        int colliderPointB = (tileGridCoordsB.x + tileGridCoordsB.y * terrainSystem.GetWidthInTiles() + tileGridCoordsB.y) + 1;
        int colliderPointC = (tileGridCoordsC.x + tileGridCoordsC.y * terrainSystem.GetWidthInTiles() + tileGridCoordsC.y) +
                             (terrainSystem.GetWidthInTiles() + 1) + 1;
        int colliderPointD = (tileGridCoordsD.x + tileGridCoordsD.y * terrainSystem.GetWidthInTiles() + tileGridCoordsD.y) +
                             (terrainSystem.GetWidthInTiles() + 1);

        std::string name = propTemplateHandle + "PropCollider";
        auto colliderEntity =
            terrainSystem.CreateTerrainColliderEntity({colliderPointA, colliderPointB, colliderPointC, colliderPointD}, name);
        auto& colliderTransform = bee::Engine.ECS().Registry.get<bee::Transform>(colliderEntity);
        colliderTransform.SetParent(propEntity);
    }

    return propEntity;
}

#ifdef BEE_INSPECTOR
void PropManager::Inspect(bee::Entity e)
{
   /* System::Inspect(e);
    ImGui::Text("Attributes");
    AttributesComponent* attributes = bee::Engine.ECS().Registry.try_get<AttributesComponent>(e);
    if (!attributes) return;
    for (auto element : attributes->attributes)
    {
        ImGui::InputDouble(std::string(magic_enum::enum_name(element.first)).c_str(), &attributes->attributes.at(element.first));
    }*/

}
#endif

void PropManager::RemovePropsOfTemplate(std::string propHandle)
{
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent>();
    for (auto prop : view)
    {
        auto [attrbiutes] = view.get(prop);
        if (attrbiutes.GetEntityType() == propHandle)
        {
            bee::Engine.ECS().DeleteEntity(prop);
        }
    }
}

void PropManager::RemoveGameProp(bee::Entity prop)
{
    // Change the flags of previously occupied tiles
    auto& terrainSystem = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    auto& attributes = bee::Engine.ECS().Registry.get<AttributesComponent>(prop);
    auto& propTemplate = GetPropTemplate(attributes.GetEntityType());
    std::vector<int> occupiedTiles{};
    terrainSystem.GetOccupiedTilesFromObject(attributes.smallGridIndex, propTemplate.tileDimensions.x,
                                                propTemplate.tileDimensions.y, attributes.flipped, occupiedTiles);

    for (auto tileIndex : occupiedTiles)
    {
        terrainSystem.SetTileFlags(tileIndex, lvle::TileFlags::Traversible);
    }

    // delete entity
    bee::Engine.ECS().DeleteEntity(prop);
}

