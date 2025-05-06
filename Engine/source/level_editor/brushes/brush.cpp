#include "level_editor/brushes/brush.hpp"

#include "actors/selection_system.hpp"
#include "rendering/debug_render.hpp"
#include "tools/tools.hpp"
#include "tools/3d_utility_functions.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

lvle::Brush::Brush()
{
    m_previewEntity = Engine.ECS().CreateEntity();
    auto& transform = Engine.ECS().CreateComponent<Transform>(m_previewEntity);
    Engine.ECS().CreateComponent<bee::physics::DiskCollider>(m_previewEntity, m_radius);
    transform.Name = "Preview Actor";
}

lvle::Brush::~Brush()
{
    //RemovePreviewModel();
    Engine.ECS().DeleteEntity(m_previewEntity);
}

void lvle::Brush::FindBrushCoveredVertexIndices()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    m_vertexIndices.clear();
    int gp_width = terrain.m_data->m_width + 1;
    int gp_height = terrain.m_data->m_height + 1;
    for (int y = -m_radius; y <= m_radius; y++)
    {
        for (int x = -m_radius; x <= m_radius; x++)
        {
            int index = (m_hoveredVertexIndex + x) + y * gp_width;
            int indexX = m_hoveredVertexIndex % gp_width + x;
            int indexY = m_hoveredVertexIndex / gp_width + y;
            if (indexX >= 0 && indexX < gp_width && indexY >= 0 && indexY < gp_height)
            {
                m_vertexIndices.push_back(index);
            }
        }
    }
}

void lvle::Brush::FindCoveredTileIndices()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto data = terrain.m_data;

    m_tileIndices.clear();
    switch (m_snapMode)
    {
        case SnapMode::SnapToPoint:
        {
            vec2 hoveredVertexCoord =
                vec2(m_hoveredVertexIndex % (data->m_width + 1), m_hoveredVertexIndex / (data->m_width + 1));
            vec2 firstHoveredVertexCoord = vec2(hoveredVertexCoord.x - static_cast<float>(m_radius + 1),
                                                   hoveredVertexCoord.y - static_cast<float>(m_radius + 1));
            for (int y = static_cast<int>(firstHoveredVertexCoord.y);
                 y <= static_cast<int>(firstHoveredVertexCoord.y) + m_radius * 2 + 1; y++)
            {
                for (int x = static_cast<int>(firstHoveredVertexCoord.x);
                     x <= static_cast<int>(firstHoveredVertexCoord.x) + m_radius * 2 + 1; x++)
                {
                    if (x >= 0 && x < data->m_width && y >= 0 && y < data->m_height)
                    {
                        int vertexIndex = x + y * (data->m_width + 1);
                        int tileIndex = vertexIndex - y;
                        m_tileIndices.push_back(tileIndex);
                    }
                }
            }
            break;
        }
        case SnapMode::SnapObject:
        {
            int smallGridWidth = terrain.m_data->m_width * 2 + 1;
            vec2 hoveredSmallPointCoord = vec2(static_cast<float>(m_hoveredSmallPointIndex % smallGridWidth),
                                                   static_cast<float>(m_hoveredSmallPointIndex / smallGridWidth));
            vec2 firstSmallPointCoord =
                vec2(hoveredSmallPointCoord.x - m_objectDimensions.x, hoveredSmallPointCoord.y - m_objectDimensions.y);
            int firstTileIndex = (firstSmallPointCoord.x / 2) + (firstSmallPointCoord.y / 2) * terrain.m_data->m_width;
            vec2 firstTileCoord = vec2(firstTileIndex % terrain.m_data->m_width, firstTileIndex / terrain.m_data->m_width);
            for (int y = static_cast<int>(firstTileCoord.y); y < static_cast<int>(firstTileCoord.y + m_objectDimensions.y); y++)
            {
                for (int x = static_cast<int>(firstTileCoord.x); x < static_cast<int>(firstTileCoord.x + m_objectDimensions.x);
                     x++)
                {
                    if (x >= 0 && x < terrain.m_data->m_width && y >= 0 && y < terrain.m_data->m_height)
                    {
                        int tileIndex = x + y * terrain.m_data->m_width;
                        m_tileIndices.push_back(tileIndex);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void lvle::Brush::RemovePreviewModel()
{
    auto view = Engine.ECS().Registry.view<bee::Transform, PreviewModelTag>();
    for (auto entity : view)
    {
        auto [transform, previewModelTag] = view.get(entity);
        if(transform.GetParent() == m_previewEntity);
        {
            Engine.ECS().DeleteEntity(entity);
            auto& previewTransform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
            previewTransform.NoChildren();
        }
    }
}

void lvle::Brush::SnapBrush(glm::vec3& intersectionPoint)
{
    Clear();

    switch (m_snapMode)
    {
        case SnapMode::NoSnap:
        {
            // No need to snap it to anything. :)
            break;
        }
        case SnapMode::SnapToPoint:
        {
            auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

            // grid points
            vec2 terrainSize =
                vec2(terrain.m_data->m_width * terrain.m_data->m_step, terrain.m_data->m_height * terrain.m_data->m_step);
            int x = static_cast<int>((intersectionPoint.x + terrainSize.x / 2.0f) / terrain.m_data->m_step + 0.5f);
            int y = static_cast<int>((intersectionPoint.y + terrainSize.y / 2.0f) / terrain.m_data->m_step + 0.5f);
            m_hoveredVertexIndex = x + y * (terrain.m_data->m_width + 1);
            intersectionPoint = terrain.m_data->m_vertices[m_hoveredVertexIndex].position;
            FindBrushCoveredVertexIndices();
            FindCoveredTileIndices();
            break;
        }
        case SnapMode::SnapObject:
        {
            auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
            vec2 terrainSize =
                vec2(terrain.m_data->m_width * terrain.m_data->m_step, terrain.m_data->m_height * terrain.m_data->m_step);
            int x = static_cast<int>((intersectionPoint.x + terrainSize.x / 2.0f) / (terrain.m_data->m_step / 2.0f) + 0.5f);
            int y = static_cast<int>((intersectionPoint.y + terrainSize.y / 2.0f) / (terrain.m_data->m_step / 2.0f) + 0.5f);
            static vec2 hoveredSmallPointCoord = vec2(-1, -1);

            // odd x dimensions
            if (static_cast<int>(m_objectDimensions.x) % 2 != 0)
            {
                // snap x to odd
                if (x % 2 != 0) hoveredSmallPointCoord.x = static_cast<float>(x);
            }
            // even x dimensions
            else
            {
                // snap x to even
                if (x % 2 == 0) hoveredSmallPointCoord.x = static_cast<float>(x);
            }
            // odd y dimensions
            if (static_cast<int>(m_objectDimensions.y) % 2 != 0)
            {
                // snap y to odd
                if (y % 2 != 0) hoveredSmallPointCoord.y = static_cast<float>(y);
            }
            // even y dimensions
            else
            {
                // snap y to even
                if (y % 2 == 0) hoveredSmallPointCoord.y = static_cast<float>(y);
            }
            if (static_cast<int>(hoveredSmallPointCoord.x) != -1 && static_cast<int>(hoveredSmallPointCoord.y) != -1)
            {
                m_hoveredSmallPointIndex =
                    hoveredSmallPointCoord.x + hoveredSmallPointCoord.y * (terrain.m_data->m_width * 2 + 1);
                vec2 sgpPos = vec2(terrain.m_data->m_smallGridPoints[m_hoveredSmallPointIndex].x,
                               terrain.m_data->m_smallGridPoints[m_hoveredSmallPointIndex].y);
                float result = 0.0f;
                if (GetTerrainHeightAtPoint(sgpPos.x, sgpPos.y, result))
                {
                    intersectionPoint = vec3(sgpPos.x, sgpPos.y, result);
                }
            }
            FindCoveredTileIndices();
            break;
        }
        default:
            break;
    }
}

void lvle::Brush::Clear()
{
    m_vertexIndices.clear();
    m_tileIndices.clear();
}

void lvle::Brush::RotatePreviewModel(const float& degreesOffset)
{
    if (!Engine.ECS().Registry.get<Transform>(m_previewEntity).HasChildern()) return;
    if (m_previewEntity != entt::null)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_previewEntity);
        glm::vec3 rotationEuler = eulerAngles(transform.Rotation);
        rotationEuler.z += radians(degreesOffset);
        transform.Rotation = quat(rotationEuler);
    }
}

void lvle::Brush::Update(glm::vec3& intersectionPoint, const std::string objectHandle)
{
    SnapBrush(intersectionPoint);

    // reset flags and color
    m_brushColor = vec4(1.0, 0.0, 1.0, 1.0);
    m_brushFlags &= ~TileFlags::NoGroundTraverse;
    m_brushFlags &= ~TileFlags::NoAirTraverse;
    m_brushFlags &= ~TileFlags::NoBuild;
    m_brushFlags |= TileFlags::Traversible;
}

void lvle::Brush::Render(const glm::vec3& intersectionPoint)
{
    if (!m_enabled) return;

    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    switch (m_snapMode)
    {
        case SnapMode::NoSnap:
        {
            if (Engine.ECS().Registry.get<bee::Transform>(m_previewEntity).HasChildern())
            {
                auto& diskCollider = Engine.ECS().Registry.get<bee::physics::DiskCollider>(m_previewEntity);
                Engine.DebugRenderer().AddCircle(DebugCategory::Editor, intersectionPoint + vec3(0.0f, 0.0f, 0.05f),
                                                 diskCollider.radius, m_brushColor);
            }
            else
            {
                Engine.DebugRenderer().AddCircle(
                    DebugCategory::Editor, intersectionPoint + vec3(0.0f, 0.0f, 0.05f),
                    static_cast<float>(m_radius) * terrain.m_data->m_step + (terrain.m_data->m_step * 0.5f), m_brushColor);
            }
            break;
        }
        case SnapMode::SnapToPoint:
        {
            for (int i = 0; i < m_vertexIndices.size(); i++)
            {
                Engine.DebugRenderer().AddLine(
                    DebugCategory::Editor, terrain.m_data->m_vertices[m_vertexIndices[i]].position,
                    terrain.m_data->m_vertices[m_vertexIndices[i]].position + vec3(0.0f, 0.0f, 1.0f),
                    m_brushColor);
            }
            Engine.DebugRenderer().AddCircle(
                DebugCategory::Editor, intersectionPoint + vec3(0.0f, 0.0f, 0.05f),
                static_cast<float>(m_radius) * terrain.m_data->m_step + (terrain.m_data->m_step * 0.5f), m_brushColor);
            break;
        }
        case SnapMode::SnapObject:
        {
            auto& diskCollider = Engine.ECS().Registry.get<bee::physics::DiskCollider>(m_previewEntity);
            Engine.DebugRenderer().AddCircle(DebugCategory::Editor, intersectionPoint + vec3(0.0f, 0.0f, 0.05f),
                                             diskCollider.radius, m_brushColor);
            for (int i = 0; i < m_tileIndices.size(); i++)
            {
                Engine.DebugRenderer().AddCircle(DebugCategory::Editor, terrain.m_data->m_tiles[m_tileIndices[i]].centralPos,
                                                 terrain.m_data->m_step / 2.0f, m_brushColor);
            }
            break;
        }
        default:
            break;
    }
}