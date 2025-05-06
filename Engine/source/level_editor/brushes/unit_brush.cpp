#include "level_editor/brushes/unit_brush.hpp"

#include "actors/units/unit_manager_system.hpp"
#include "level_editor/terrain_system.hpp"
#include "physics/physics_components.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

void lvle::UnitBrush::SetPreviewModel(const std::string& objectHandle)
{
    if (!m_enabled) return;
    if (objectHandle != "")
    {
        auto& unitManager = Engine.ECS().GetSystem<UnitManager>();

        auto [transform, diskCollider] = Engine.ECS().Registry.get<Transform, bee::physics::DiskCollider>(m_previewEntity);
        transform.Scale = vec3(unitManager.GetUnitTemplate(objectHandle).GetAttribute(BaseAttributes::Scale));
        transform.Rotation = glm::identity<glm::quat>();
        diskCollider.radius = unitManager.GetUnitTemplate(objectHandle).GetAttribute(BaseAttributes::DiskScale);

        RemovePreviewModel();
        auto& unitTemplate = unitManager.GetUnitTemplate(objectHandle);
        auto modelEntity = Engine.ECS().CreateEntity();
        auto& modelTransform = Engine.ECS().CreateComponent<Transform>(modelEntity);
        modelTransform.Name = "Model";
        modelTransform.SetParent(m_previewEntity);
        modelTransform.Translation += unitTemplate.modelOffset.Translation;
        auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
        auto modelOffsetEuler = glm::eulerAngles(unitTemplate.modelOffset.Rotation);
        modelEuler += modelOffsetEuler;
        modelTransform.Rotation = glm::quat(modelEuler);
        auto& previewModelTag = Engine.ECS().CreateComponent<PreviewModelTag>(modelEntity);
        previewModelTag.previewType = objectHandle;
        auto model = Engine.Resources().Load<Model>(unitTemplate.modelPath);
        ConstantBufferData constantData;
        constantData.opacity = 0.65f;
        model->Instantiate(modelEntity, constantData);
    }
}

void lvle::UnitBrush::Update(glm::vec3& intersectionPoint, const std::string objectHandle)
{
    Brush::Update(intersectionPoint, objectHandle);

    // update preview model
    if (m_previewEntity != entt::null)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        transform.Translation = intersectionPoint;
        auto& unitManager = Engine.ECS().GetSystem<UnitManager>();
        if (objectHandle != "")
        {
            auto& unitTemplate = unitManager.GetUnitTemplate(objectHandle);
            transform.Scale = vec3(unitTemplate.GetAttribute(BaseAttributes::Scale));
        }
    }

    // ground traverse
    if (!IsHoveredTileWalkable(intersectionPoint))
    {
        m_brushColor = vec4(1.0, 0.0, 0.0, 1.0);
        m_brushFlags &= ~TileFlags::Traversible;
        m_brushFlags |= TileFlags::NoGroundTraverse;
    }

    if(!IsSpaceEmpty())
    {
        m_brushColor = vec4(1.0, 0.0, 0.0, 1.0);
        m_brushFlags &= ~TileFlags::Traversible;
        m_brushFlags |= TileFlags::NoGroundTraverse;
    }
}

std::optional<bee::Entity> lvle::UnitBrush::PlaceObject(const std::string& objectHandle, Team team)
{
    if (!m_enabled) return std::nullopt;

    auto& unitManager = Engine.ECS().GetSystem<UnitManager>();

    auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
    if (!(m_brushFlags & TileFlags::NoGroundTraverse))
    {
        auto unitEntity = unitManager.SpawnUnit(objectHandle, transform.Translation, team);
        if (unitEntity != nullopt)
        {
            auto& unitTransform = Engine.ECS().Registry.get<Transform>(unitEntity.value());
            unitTransform.Rotation = transform.Rotation;
            return unitEntity;
        }
    }
    return nullopt;
}

bool lvle::UnitBrush::IsHoveredTileWalkable(const glm::vec3& position)
{
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();
    vec2 terrain_size = vec2(terrainSystem.m_data->m_width * terrainSystem.m_data->m_step,
                             terrainSystem.m_data->m_height * terrainSystem.m_data->m_step);
    int x = static_cast<int>((position.x + terrain_size.x / 2.0f) / terrainSystem.m_data->m_step);
    int y = static_cast<int>((position.y + terrain_size.y / 2.0f) / terrainSystem.m_data->m_step);
    int gridPointIndex = x + y * (terrainSystem.m_data->m_width + 1);
    int tileIndex = gridPointIndex - y;
    if (tileIndex >= 0 && tileIndex < terrainSystem.m_data->m_tiles.size())
    {
        return terrainSystem.m_data->m_tiles[tileIndex].tileFlags & TileFlags::Traversible;
    }
    return false;
}

bool lvle::UnitBrush::IsSpaceEmpty()
{
    auto view = Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();
    auto [brushCollider, brushTransform] = Engine.ECS().Registry.get<bee::physics::DiskCollider, bee::Transform>(m_previewEntity);

    for(auto [unit, attributes, actorTransform] : view.each())
    {
        if (attributes.HasAttribute(BaseAttributes::DiskScale))
        {
            const auto actorRadius = attributes.GetValue(BaseAttributes::DiskScale);
            glm::vec2 centerDiff = brushTransform.Translation - actorTransform.Translation;
            float l1 = glm::length2(centerDiff);
            float radiusSum = brushCollider.radius + actorRadius;
            if (l1 < radiusSum * radiusSum) return false;
        }
    }
    return true;
}
