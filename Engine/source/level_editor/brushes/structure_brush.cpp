#include "actors/structures/structure_manager_system.hpp"
#include "actors/props/resource_system.hpp"
#include "core/audio.hpp"

#include "level_editor/brushes/structure_brush.hpp"
#include "level_editor/terrain_system.hpp"

#include "tools/tools.hpp"
#include "physics/physics_components.hpp"

// hopefully temp...
#include "rendering/debug_render.hpp"
#include "tools/debug_metric.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

void lvle::StructureBrush::SetPreviewModel(const std::string& objectHandle)
{
    if (!m_enabled) return;
    if (objectHandle != "")
    {
        auto& structureManager = Engine.ECS().GetSystem<StructureManager>();

        auto [transform, diskCollider] = Engine.ECS().Registry.get<Transform, bee::physics::DiskCollider>(m_previewEntity);
        transform.Scale = vec3(structureManager.GetStructureTemplate(objectHandle).GetAttribute(BaseAttributes::Scale));
        transform.Rotation = glm::identity<glm::quat>();
        diskCollider.radius = structureManager.GetStructureTemplate(objectHandle).GetAttribute(BaseAttributes::DiskScale);
        m_objectDimensions = structureManager.GetStructureTemplate(objectHandle).tileDimensions;

        RemovePreviewModel();
        auto& structureTemplate = structureManager.GetStructureTemplate(objectHandle);
        auto modelEntity = Engine.ECS().CreateEntity();
        auto& modelTransform = Engine.ECS().CreateComponent<Transform>(modelEntity);
            modelTransform.Name = "Model";
            modelTransform.SetParent(m_previewEntity);
            modelTransform.Translation += structureTemplate.modelOffset.Translation;
        auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
        auto modelOffsetEuler = glm::eulerAngles(structureTemplate.modelOffset.Rotation);
        modelEuler += modelOffsetEuler;
            modelTransform.Rotation = glm::quat(modelEuler);
        auto& previewModelTag = Engine.ECS().CreateComponent<PreviewModelTag>(modelEntity);
            previewModelTag.previewType = objectHandle;
        auto model = Engine.Resources().Load<Model>(structureTemplate.modelPath);
        ConstantBufferData constantData;
        constantData.opacity = 0.65f;
        model->Instantiate(modelEntity, constantData);

        m_flipped = false;
    }
}

void lvle::StructureBrush::Update(glm::vec3& intersectionPoint, const std::string objectHandle)
{
    Brush::Update(intersectionPoint, objectHandle);

    // update preview model
    if (m_previewEntity != entt::null)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        transform.Translation = intersectionPoint;
        auto& structureManager = Engine.ECS().GetSystem<StructureManager>();
        if (objectHandle != "")
        {
            auto& structureTemplate = structureManager.GetStructureTemplate(objectHandle);
            transform.Scale = vec3(structureTemplate.GetAttribute(BaseAttributes::Scale));
        }
    }

    // buildable
    if (!AreHoveredTilesBuildable())
    {
        m_brushColor = vec4(1.0, 0.0, 0.0, 1.0);
        m_brushFlags &= ~TileFlags::Traversible;
        m_brushFlags |= TileFlags::NoBuild;
    }
}

int lvle::StructureBrush::CalculateSmallGridIndex(const glm::vec3& intersectionPoint)
{
    glm::vec3 pnt = intersectionPoint;
    SnapBrush(pnt);
    return m_hoveredSmallPointIndex;
}

void lvle::StructureBrush::RotatePreviewModel(const float& degreesOffset)
{
    if (!Engine.ECS().Registry.get<Transform>(m_previewEntity).HasChildern()) return;
    if (m_previewEntity != entt::null)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        glm::vec3 rotation_euler = eulerAngles(transform.Rotation);
        if (degreesOffset != 0.0f)
        {
            rotation_euler.z += radians(90.0f);
            Swap(m_objectDimensions.x, m_objectDimensions.y);
            m_flipped = !m_flipped;
        }
        transform.Rotation = quat(rotation_euler);
    }
}

std::optional<bee::Entity> lvle::StructureBrush::PlaceObject(const std::string& objectHandle, Team team)
{
    if (!m_enabled) return std::nullopt;

    auto& structureManager = Engine.ECS().GetSystem<StructureManager>();
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();

    if (!(m_brushFlags & TileFlags::NoBuild))
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        auto structureEntity = structureManager.SpawnStructure(objectHandle, transform.Translation, m_hoveredSmallPointIndex, m_flipped, team);
        if (structureEntity != nullopt)
        {
            auto& structureTransform = Engine.ECS().Registry.get<Transform>(structureEntity.value());
            structureTransform.Rotation = transform.Rotation;
            return structureEntity;
        }
    }
    return nullopt;
}

std::optional<bee::Entity> lvle::StructureBrush::PlaceObject(const std::string& objectHandle, const glm::vec3& position, Team team)
{
    auto& structureManager = Engine.ECS().GetSystem<StructureManager>();
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();

    if (!(m_brushFlags & TileFlags::NoBuild))
    {
        auto structureManagerEntity =
            structureManager.SpawnStructure(objectHandle, position, m_hoveredSmallPointIndex, m_flipped, team);
        return structureManagerEntity;
    }
    return nullopt;
}

std::optional<bee::Entity> lvle::StructureBrush::PlaceObject(const std::string& objectHandle, const glm::vec3& position, const glm::quat& rotation, Team team)
{
    if (!m_enabled) return std::nullopt;

    auto& structureManager = Engine.ECS().GetSystem<StructureManager>();
        auto& buildTemplate = structureManager.GetStructureTemplate(objectHandle);
        const glm::ivec2 dimensions = buildTemplate.tileDimensions;

    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();
        const int smallIndex = terrainSystem.GetSmallGridIndexFromPosition(position, dimensions.x, dimensions.y);

    if (!(m_brushFlags & TileFlags::NoBuild))
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        auto structureEntity = structureManager.SpawnStructure(objectHandle, position, smallIndex, m_flipped, team);
        if (structureEntity != nullopt)
        {
            auto& structureTransform = Engine.ECS().Registry.get<Transform>(structureEntity.value());
            structureTransform.Rotation = rotation;
            return structureEntity;
        }
    }
    return nullopt;
}

void lvle::StructureBrush::PlaceMultipleObjects(const std::string& objectHandle, const std::vector<glm::vec3>& positions, const glm::quat& rotation, const size_t amount, GameResourceType resource)
{
    // Used to access resource data
    auto& resources = Engine.ECS().GetSystem<ResourceSystem>();

    auto editablePositions = positions;
    for (int i = 0; i < static_cast<int>(amount); i++)
    {
        // End if player cannot afford building
        if (!resources.CanAfford(resource, 2))
        {
            bee::Engine.Audio().PlaySoundW("audio/not_possible.wav", 1.3f, true);
            return;
        }

        // If the wall was successfully placed
        if (!AreHoveredTilesBuildableWithPosition(editablePositions[i])) continue;
        if (PlaceObject(objectHandle, editablePositions[i], rotation, Team::Ally) != nullopt)
        {
            
            // Need to do something here with the Editor data to update which tiles are not moveable...
            resources.Spend(resource, 2);
            for (auto [entity, debugMetric] : bee::Engine.ECS().Registry.view<bee::DebugMetricData>().each())
                if (debugMetric.resourcesSpentOn.find(objectHandle) == debugMetric.resourcesSpentOn.end())
                    debugMetric.resourcesSpentOn.insert(std::pair<std::string, int>(objectHandle, 1));
            else
                    debugMetric.resourcesSpentOn[objectHandle]++;
        }
        else
        {
            printf("Oppa %i\n", i);
        }
    }
    bee::Engine.Audio().PlaySoundW("audio/building2.wav", 1.3f, true);
}

bool lvle::StructureBrush::AreHoveredTilesBuildable()
{
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();
    //FindCoveredTileIndicesFromPosition(position);

    if (m_tileIndices.size() != m_objectDimensions.x * m_objectDimensions.y)
    {
        return false;
    }
    bool canPlace = true;
    for (auto tileIndex : m_tileIndices)
    {
        if (!terrainSystem.CanBuildOnTile(tileIndex))
        {
            canPlace = false;
            return canPlace;
        }
    }
    return canPlace;
}

bool lvle::StructureBrush::AreHoveredTilesBuildableWithPosition(glm::vec3& position)
{
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();

    vector<int> occupiedTileIndices;
    //occupiedTileIndices = FindCoveredTileIndicesFromPosition(position, ivec2(3, 1));
    if (m_flipped)
        occupiedTileIndices = FindCoveredTileIndicesFromPosition(position, ivec2(1, 3));
    else
        occupiedTileIndices = FindCoveredTileIndicesFromPosition(position, ivec2(3, 1));

    /*if (m_tileIndices.size() != m_objectDimensions.x * m_objectDimensions.y)
    {
        return false;
    }*/
    bool canPlace = true;
    for (auto tileIndex : occupiedTileIndices)
    {
        if (!terrainSystem.CanBuildOnTile(tileIndex))
        {
            canPlace = false;
            return canPlace;
        }
    }
    return canPlace;
}

std::vector<int> lvle::StructureBrush::FindCoveredTileIndicesFromPosition(glm::vec3& position, const glm::ivec2& tileDims)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto data = terrain.m_data;
    
    std::vector<int> result;
    auto hoveredSmallPointIndex = terrain.GetSmallGridIndexFromPosition(position, tileDims.x, tileDims.y);
    vec2 sgpPos = vec2(terrain.m_data->m_smallGridPoints[hoveredSmallPointIndex].x,
                       terrain.m_data->m_smallGridPoints[hoveredSmallPointIndex].y);
    float heightResult = 0.0f;
    if (GetTerrainHeightAtPoint(sgpPos.x, sgpPos.y, heightResult))
    {
        position = vec3(sgpPos.x, sgpPos.y, heightResult);
    }

    switch (m_snapMode)
    {
        case SnapMode::SnapObject:
        {
            int smallGridWidth = terrain.m_data->m_width * 2 + 1;
            vec2 hoveredSmallGridPointCoord = vec2(static_cast<float>(hoveredSmallPointIndex % smallGridWidth),
                                                   static_cast<float>(hoveredSmallPointIndex / smallGridWidth));
            vec2 firstSmallGridPointCoord =
                vec2(hoveredSmallGridPointCoord.x - tileDims.x, hoveredSmallGridPointCoord.y - tileDims.y);
            int firstTileIndex = (firstSmallGridPointCoord.x / 2) + (firstSmallGridPointCoord.y / 2) * terrain.m_data->m_width;
            vec2 firstTileCoord = vec2(firstTileIndex % terrain.m_data->m_width, firstTileIndex / terrain.m_data->m_width);
            for (int y = static_cast<int>(firstTileCoord.y); y < static_cast<int>(firstTileCoord.y + tileDims.y); y++)
            {
                for (int x = static_cast<int>(firstTileCoord.x); x < static_cast<int>(firstTileCoord.x + tileDims.x);
                     x++)
                {
                    if (x >= 0 && x < terrain.m_data->m_width && y >= 0 && y < terrain.m_data->m_height)
                    {
                        int tileIndex = x + y * terrain.m_data->m_width;
                        result.push_back(tileIndex);
                        // terrain.m_data->m_tiles[tileIndex].centralPos =
                        // terrain.m_data->m_gridPoints[m_hoveredGridPointIndex].GetFirst().position;
                        // terrain.CalculateTileCentralPosition(tileIndex);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return result;
}