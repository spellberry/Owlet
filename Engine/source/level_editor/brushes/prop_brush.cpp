#include "level_editor/brushes/prop_brush.hpp"

#include "actors/props/prop_manager_system.hpp"
#include "level_editor/terrain_system.hpp"
#include "tools/tools.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

void lvle::PropBrush::SetPreviewModel(const std::string& objectHandle)
{
    if (!m_enabled) return;
    if (objectHandle != "")
    {
        auto& propManager = Engine.ECS().GetSystem<PropManager>();

        auto [transform, diskCollider] = Engine.ECS().Registry.get<Transform, bee::physics::DiskCollider>(m_previewEntity);
        transform.Scale = vec3(propManager.GetPropTemplate(objectHandle).GetAttribute(BaseAttributes::Scale));
        transform.Rotation = glm::identity<glm::quat>();
        diskCollider.radius = propManager.GetPropTemplate(objectHandle).GetAttribute(BaseAttributes::DiskScale);

        RemovePreviewModel();
        auto& propTemplate = propManager.GetPropTemplate(objectHandle);
        auto modelEntity = Engine.ECS().CreateEntity();
        auto& modelTransform = Engine.ECS().CreateComponent<Transform>(modelEntity);
        modelTransform.Name = "Model";
        modelTransform.SetParent(m_previewEntity);
        modelTransform.Translation += propTemplate.modelOffset.Translation;
        auto modelEuler = glm::eulerAngles(modelTransform.Rotation);
        auto modelOffsetEuler = glm::eulerAngles(propTemplate.modelOffset.Rotation);
        modelEuler += modelOffsetEuler;
        modelTransform.Rotation = glm::quat(modelEuler);
        auto& previewModelTag = Engine.ECS().CreateComponent<PreviewModelTag>(modelEntity);
        previewModelTag.previewType = objectHandle;
        auto model = Engine.Resources().Load<Model>(propTemplate.modelPath);
        ConstantBufferData constantData;
        constantData.opacity = 0.65f;
        model->Instantiate(modelEntity, constantData);

        m_flipped = false;
    }
}

void lvle::PropBrush::Update(glm::vec3& intersectionPoint, const std::string objectHandle)
{
    Brush::Update(intersectionPoint, objectHandle);

    // update preview model
    if (m_previewEntity != entt::null)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        transform.Translation = intersectionPoint;
        auto& propManager = Engine.ECS().GetSystem<PropManager>();
        if (objectHandle != "")
        {
            auto& propTemplate = propManager.GetPropTemplate(objectHandle);
            transform.Scale = vec3(propTemplate.GetAttribute(BaseAttributes::Scale));
        }
    }

    // buildable
    if (!AreHoveredTilesBuildable(intersectionPoint))
    {
        m_brushColor = vec4(1.0, 0.0, 0.0, 1.0);
        m_brushFlags &= ~TileFlags::Traversible;
        m_brushFlags |= TileFlags::NoBuild;
    }
}

void lvle::PropBrush::RotatePreviewModel(const float& degreesOffset)
{
    if (!Engine.ECS().Registry.get<Transform>(m_previewEntity).HasChildern()) return;
    if (m_previewEntity != entt::null)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        glm::vec3 rotationEuler = eulerAngles(transform.Rotation);
        if (degreesOffset != 0.0f)
        {
            rotationEuler.z += radians(90.0f);
            Swap(m_objectDimensions.x, m_objectDimensions.y);
            m_flipped = !m_flipped;

        }
        transform.Rotation = quat(rotationEuler);
    }
}

std::optional<bee::Entity> lvle::PropBrush::PlaceObject(const std::string& objectHandle, Team team)
{
    if (!m_enabled) return std::nullopt;

    auto& propManager = Engine.ECS().GetSystem<PropManager>();
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();

    if (!(m_brushFlags & TileFlags::NoBuild))
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        auto propEntity = propManager.SpawnProp(objectHandle, transform.Translation, m_hoveredSmallPointIndex, m_flipped);
        if (propEntity != nullopt)
        {
            auto& propTransform = Engine.ECS().Registry.get<Transform>(propEntity.value());
            propTransform.Rotation = transform.Rotation;
            for (auto tileIndex : m_tileIndices)
            {
                int flag = 0;
                flag &= ~TileFlags::Traversible;
                flag |= TileFlags::NoGroundTraverse;
                flag |= TileFlags::NoBuild;
                terrainSystem.SetTileFlags(tileIndex, static_cast<TileFlags>(flag));
            }
            return propEntity;
        }
    }
    return nullopt;
}

std::optional<bee::Entity> lvle::PropBrush::PlaceObject(const std::string& objectHandle,
                                                        const glm::vec3& position, Team team)
{
    auto& propManager = Engine.ECS().GetSystem<PropManager>();
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();

    if (!(m_brushFlags & TileFlags::NoBuild))
    {
        auto propManagerEntity = propManager.SpawnProp(objectHandle, position, m_hoveredSmallPointIndex, m_flipped);
        for (auto tileIndex : m_tileIndices)
        {
            int flag = 0;
            flag &= ~TileFlags::Traversible;
            flag |= TileFlags::NoGroundTraverse;
            flag |= TileFlags::NoBuild;
            terrainSystem.SetTileFlags(tileIndex, static_cast<TileFlags>(flag));
        }
        return propManagerEntity;
    }
    return nullopt;
}

bool lvle::PropBrush::AreHoveredTilesBuildable(const glm::vec3& position)
{
    auto& terrainSystem = Engine.ECS().GetSystem<TerrainSystem>();

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
