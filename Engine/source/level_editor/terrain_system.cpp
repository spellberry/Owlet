#include "level_editor/terrain_system.hpp"

#include <algorithm>
#include <cereal/archives/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <iostream>
#include <limits>
#include <string>
#include <tinygltf/json.hpp>

#include "ai/grid_navigation_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "rendering/Render.hpp"
#include "rendering/debug_render.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"
#include "tools/inspector.hpp"
#include "tools/log.hpp"
#include "tools/serialization.hpp"
#include "tools/tools.hpp"
#include "tools/3d_utility_functions.hpp"
#include "physics/physics_components.hpp"
#include "actors/attributes.hpp"
#include "actors/actor_wrapper.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

bool lvle::GetTerrainHeightAtPoint(const float x, const float y, float& result)
{
    auto& terrain = Engine.ECS().GetSystem<lvle::TerrainSystem>();

    glm::vec3 hit;
    if (terrain.FindRayMeshIntersection(glm::vec3(x, y, 10000.0f), glm::vec3(0.0f, 0.0f, -1.0f), hit))
    {
        result = hit.z;
        return true;
    }
    return false;
}

//credit to stack overflow user: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

TerrainSystem::TerrainSystem()
{
    const auto entity = Engine.ECS().CreateEntity();
    // Creating TerrainDataComponent component
    auto& data = Engine.ECS().CreateComponent<TerrainDataComponent>(entity);
    m_data = make_shared<TerrainDataComponent>(data);
    // Creating Transform component
    auto& transform = Engine.ECS().CreateComponent<Transform>(entity);
    transform.Name = "Terrain Ground";
    transform.Scale = vec3(1.0f);
    // Creating the MeshRender component
    for (int i = 0; i < m_data->m_materialPaths.size(); i++)
    {
        auto material = Engine.Resources().Load<Material>(m_data->m_materialPaths[i]);
        m_data->m_materials[i] = material;
        //m_data->m_materials.push_back(Engine.Resources().Load<Material>(m_data->m_materialPaths[i]));
    }
    auto image = Engine.Resources().Load<Image>("textures/TerrainArt/VA/T_Grass_Stylized_D.png");
    auto normalMap = Engine.Resources().Load<Image>("textures/TerrainArt/VA/T_Grass_Stylized_N.png");
    auto metallicRoughness = Engine.Resources().Load<Image>("textures/TerrainArt/VA/T_Grass_Stylized_ORM.png");
    auto occlusion = Engine.Resources().Load<Image>("textures/TerrainArt/VA/T_Grass_Stylized_ORM.png");
    auto sampler = make_shared<Sampler>();
    m_mesh = Engine.Resources().Create<Mesh>();
    auto material = make_shared<Material>();
    material->BaseColorTexture = make_shared<bee::Texture>(image, sampler);
    material->UseBaseTexture = true;
    material->NormalTexture = make_shared<Texture>(normalMap, sampler);
    material->UseNormalTexture = true;
    material->MetallicRoughnessTexture = make_shared<Texture>(metallicRoughness, sampler);
    material->UseMetallicRoughnessTexture = true;
    material->MetallicFactor = 0.0f;
    material->RoughnessFactor = 1.0f;
    material->OcclusionTexture = make_shared<Texture>(occlusion, sampler);
    material->UseOcclusionTexture = true;
    material->ReceiveShadows = false;
    Engine.ECS().CreateComponent<MeshRenderer>(entity, m_mesh, material/*m_data->m_materials[3]*/);
    // Mark this mesh as a terrain mesh.
    auto& tag = Engine.ECS().CreateComponent<TerrainGroundTagComponent>(entity);
    // TODO: not sure if this would work
    CreatePlane(16, 16, 1.0f);
}

void lvle::TerrainSystem::UpdateTerrainDataComponent()
{
    // update the TerrainDataComponent (this is a bit backwards, but it is done for easy terrain access in the level
    // editor).
    const auto view = bee::Engine.ECS().Registry.view<lvle::TerrainDataComponent>();
    for (auto [entity, data] : view.each())
    {
        data = *m_data;
    }
}

void TerrainSystem::CreatePlane(int width, int height, float step)
{
    m_data->m_indices.clear();
    m_data->m_vertices.clear();
    m_data->m_smallGridPoints.clear();
    m_data->m_tiles.clear();

    if (width % 2 != 0)
    {
        width++;
        bee::Log::Warn("plane width set to {}", width);
    }
    if (height % 2 != 0)
    {
        height++;
        bee::Log::Warn("plane height set to {}", height);
    }

    m_data->m_step = step;
    m_data->m_width = width;
    m_data->m_height = height;

    // actor snap grid
    float halfStep = step / 2.0f;
    int agpWidth = m_data->m_width * 2 + 1;
    int agpHeight = m_data->m_height * 2 + 1;
    for (int y = -(agpHeight / 2); y <= agpHeight / 2; y++)
    {
        for (int x = -(agpWidth / 2); x <= agpWidth / 2; x++)
        {
            float xPos = x * halfStep;
            float yPos = y * halfStep;
            m_data->m_smallGridPoints.push_back(vec2(xPos, yPos));
        }
    }

    // vertices
    for (int y = -m_data->m_height / 2; y <= m_data->m_height / 2; y++)
    {
        for (int x = -m_data->m_width / 2; x <= m_data->m_width / 2; x++)
        {
            TVertex v;
            v.position = vec3(x * m_data->m_step, y * m_data->m_step, 0.0f);
            v.normal = vec3(0.0f, 0.0f, 1.0f);
            v.uv = vec2(0.5f, 0.5f);
            m_data->m_vertices.push_back(v);
        }
    }

    // UVs
    vec2 atlasStep = vec2(1.0f / m_data->m_width, 1.0f / m_data->m_height);
    for (int y = 0; y < m_data->m_height + 1; y++)
    {
        for (int x = 0; x < m_data->m_width + 1; x++)
        {
            int index = CoordsToIndex(vec2(x, y), m_data->m_width + 1);
            m_data->m_vertices[index].uv.x = x*0.125f;
            m_data->m_vertices[index].uv.y = y * 0.125f;
           /* if (x % 2 == 0)
                m_data->m_vertices[index].uv.x = 0;
            else
                m_data->m_vertices[index].uv.x = 1;

             if (y % 2 == 0)
                m_data->m_vertices[index].uv.y = 0;
            else
                m_data->m_vertices[index].uv.y = 1;*/

            /*int index = CoordsToIndex(vec2(x, y), m_data->m_width + 1);
            auto uv = vec2(x * atlasStep.x, y * atlasStep.y);
            m_data->m_vertices[index].uv = uv;*/
        }
    }

    // indices
    int vWidth = m_data->m_width + 1;
    int vHeight = m_data->m_height + 1;
    for (int y = 0; y < m_data->m_height; y++)
    {
        for (int x = 0; x < m_data->m_width; x++)
        {
            // D      C
            //   ----
            //  |   /|
            //  |  / |
            //  | /  |
            //   ----
            // A      B

            int index = x + y * vWidth;

            // CAB
            m_data->m_indices.push_back(index + vWidth + 1);
            m_data->m_indices.push_back(index);
            m_data->m_indices.push_back(index + 1);
            // ACD
            m_data->m_indices.push_back(index);
            m_data->m_indices.push_back(index + vWidth + 1);
            m_data->m_indices.push_back(index + vWidth);
        }
    }

    // tiles
    for (int y = 0; y < m_data->m_height; y++)
    {
        for (int x = 0; x < m_data->m_width; x++)
        {
            tile tile;
            tile.index = x + y * m_data->m_width;
            m_data->m_tiles.push_back(tile);
        }
    }

    // SetAllUVs();
    //m_data->m_vertices[18].materialWeights = {0, 1, 0, 0};

    UpdatePlane();
    m_mesh->SetIndices(m_data->m_indices);
}

void TerrainSystem::UpdatePlane()
{
    UpdateNormals();
    UpdateTangents();
    std::vector<Vertex> vertexList;
    for (auto& v : m_data->m_vertices)
    {
        Vertex vert;

        vert.pos.x = v.position.x;
        vert.pos.y = v.position.y;
        vert.pos.z = v.position.z;
        vert.normal.x = v.normal.x;
        vert.normal.y = v.normal.y;
        vert.normal.z = v.normal.z;
        vert.texCoord.x = v.uv.x;
        vert.texCoord.y = v.uv.y;
        vert.tangent.x = v.tangent.x;
        vert.tangent.y = v.tangent.y;
        vert.tangent.z = v.tangent.z;
        vert.bitangent.x = v.bitangent.x;
        vert.bitangent.y = v.bitangent.y;
        vert.bitangent.z = v.bitangent.z;
        vert.jointids = DirectX::XMUINT4(0, 1, 2, 3);
        vert.weights.x = v.materialWeights[0];
        vert.weights.y = v.materialWeights[1];
        vert.weights.z = v.materialWeights[2];
        vert.weights.w = v.materialWeights[3];

        vertexList.push_back(vert);
    }
    m_mesh->SetAttribute(vertexList);
}

// The normals of the vertices along the terrain edges are not updated!
void TerrainSystem::UpdateNormals()
{
    int vWidth = m_data->m_width + 1;
    int vHeight = m_data->m_height + 1;
    for (int y = 1; y < vHeight - 1; y++)
    {
        for (int x = 1; x < vWidth - 1; x++)
        {
            // index of the current point
            int index = x + y * vWidth;
            vec3 u = m_data->m_vertices[index + vWidth].position;
            vec3 d = m_data->m_vertices[index - vWidth].position;
            vec3 l = m_data->m_vertices[index - 1].position;
            vec3 r = m_data->m_vertices[index + 1].position;
            float c_u = u.z;
            float c_d = d.z;
            float c_l = l.z;
            float c_r = r.z;

            const vec3 normal = normalize(vec3(-2.0f * m_data->m_step * (c_r - c_l), 2.0f * m_data->m_step * (c_d - c_u),
                                               4.0f * m_data->m_step * m_data->m_step));
            m_data->m_vertices[index].normal = normal;
        }
    }
}

void lvle::TerrainSystem::UpdateTangents()
{
    for (size_t i = 0; i < m_data->m_indices.size(); i += 3)
    {
        TVertex& v0 = m_data->m_vertices[m_data->m_indices[i]];
        TVertex& v1 = m_data->m_vertices[m_data->m_indices[i + 1]];
        TVertex& v2 = m_data->m_vertices[m_data->m_indices[i + 2]];

        vec3 edge1 = v1.position - v0.position;
        vec3 edge2 = v2.position - v0.position;

        vec2 deltaUV1 = v1.uv - v0.uv;
        vec2 deltaUV2 = v2.uv - v0.uv;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        auto vector1 = vec3(deltaUV2.y) * edge1;
        auto vector2 = vec3(deltaUV1.y * edge2);
        vec3 tangent = vec3(vector1 - vector2) * f;

        vector1 = vec3(deltaUV1.x) * edge2;
        vector2 = vec3(deltaUV2.x) * edge1;
        vec3 bitangent = vec3(vector1 - vector2) * f;

        v0.tangent = tangent;
        v1.tangent = tangent;
        v2.tangent = tangent;

        v0.bitangent = bitangent;
        v1.bitangent = bitangent;
        v2.bitangent = bitangent;
    }

    for (TVertex& v : m_data->m_vertices)
    {
        v.tangent = glm::normalize(v.tangent);
        v.bitangent = glm::normalize(v.bitangent);
    }
}

void lvle::TerrainSystem::CalculateTileCentralPosition(int tileIndex)
{
    float halfStep = m_data->m_step / 2.0f;

    int x = tileIndex % m_data->m_width;
    int y = tileIndex / m_data->m_width;
    int vIndex = x + y * m_data->m_width + y;
    // calculate x and y of the central pos
    vec3 centralPos = m_data->m_vertices[vIndex].position + vec3(halfStep, halfStep, 0.0f);
    float result = 0.0f;
    if (GetTerrainHeightAtPoint(centralPos.x, centralPos.y, result))
    {
        centralPos.z = result;
        m_data->m_tiles[tileIndex].centralPos = centralPos;
    }
}

void lvle::TerrainSystem::CalculateAllTilesCentralPositions()
{
    float halfStep = m_data->m_step / 2.0f;
    for (int i = 0; i < m_data->m_tiles.size(); i++)
    {
        int x = i % m_data->m_width;
        int y = i / m_data->m_width;
        int vIndex = x + y * m_data->m_width + y;
        // calculate x and y of the central pos
        vec3 centralPos = m_data->m_vertices[vIndex].position + vec3(halfStep, halfStep, 0.0f);
        float result = 0.0f;
        if (GetTerrainHeightAtPoint(centralPos.x, centralPos.y, result))
        {
            centralPos.z = result;
            m_data->m_tiles[i].centralPos = centralPos;
        }
    }
}

void lvle::TerrainSystem::CalculateTerrainColliders()
{
    // represent the terrain tiles as ints. 0 for walkable, 1 for unwalkable.
    std::vector<int> allTiles;
    for (const auto& tile : m_data->m_tiles)
    {
        if (tile.tileFlags & TileFlags::NoGroundTraverse)
            allTiles.push_back(1);
        else
            allTiles.push_back(0);
    }

    // -----------------------------------------------------
    // Step 1: Find the separate groups of unwalkable tiles.
    // -----------------------------------------------------
    // the groups of unwalkable tiles
    std::vector<std::vector<int>> tileGroups;

    std::vector<int> directions({1, -1, -1, 1});  // Up, down, left, right
    for (int i = 0; i < allTiles.size(); i++)
    {
        std::vector<int> group;
        FindColliderTileGroupRecursive(allTiles, group, i, directions);
        if (!group.empty())
        {
            tileGroups.push_back(group);
        }
    }

    // ----------------------------------------------------------
    // Step 2: Find outer edges for each outer tile in the group.
    // ----------------------------------------------------------
    std::vector<std::vector<std::pair<int, int>>> groupsEdges;

    for (const auto& group : tileGroups)
    {
        std::vector<std::pair<int, int>> tileEdges;
        FindColliderEdgesPerTileInTileGroup(group, tileEdges);
        if (!tileEdges.empty()) groupsEdges.push_back(tileEdges);
    }

    // -------------------------------------------------
    // Step 3: Reduce points by taking only the corners.
    // -------------------------------------------------
    std::vector<std::vector<int>> groupsColliderPoints;

    for (const auto& edgeGroup : groupsEdges)
    {
        std::vector<int> colliderCornerPoints;
        FindColliderCornerPoints(edgeGroup, colliderCornerPoints);
        if (!colliderCornerPoints.empty()) groupsColliderPoints.push_back(colliderCornerPoints);
    }

    // -----------------------------------
    // Step 4: Create polygonal colliders.
    // -----------------------------------
    for (const auto& colliderPointsIndices : groupsColliderPoints)
    {
        CreateTerrainColliderEntity(colliderPointsIndices, "TerrainCollider");
    }
}

bee::Entity lvle::TerrainSystem::CreateTerrainColliderEntity(const std::vector<int>& pointsIndices, const std::string& name)
{
    // make a vector with the actual coordinates
    std::vector<glm::vec2> colliderPoints;
    for (const auto index : pointsIndices) colliderPoints.push_back(m_data->m_vertices[index].position);

    auto colliderEntity = Engine.ECS().CreateEntity();
    auto& transform = Engine.ECS().CreateComponent<bee::Transform>(colliderEntity);
    transform.Translation = vec3(0.0f);
    transform.Name = name;
    auto& body = Engine.ECS().CreateComponent<bee::physics::Body>(colliderEntity, bee::physics::Body::Type::Static, 1.0f, 0.0f);
    auto& polygonCollider = Engine.ECS().CreateComponent<bee::physics::PolygonCollider>(colliderEntity, colliderPoints);

    return colliderEntity;
}

void lvle::TerrainSystem::FindColliderTileGroupRecursive(std::vector<int>& allTiles, std::vector<int>& tileGroup,
                                                         const int tileIndexToInspect, const std::vector<int>& directions)
{
    if (allTiles[tileIndexToInspect] == 1)
    {
        tileGroup.push_back(tileIndexToInspect);
        allTiles[tileIndexToInspect] = 0;
        for (int i = 0; i < directions.size(); i++)
        {
            auto tileCoords = IndexToCoords(tileIndexToInspect, m_data->m_width);
            if (i < 2)
                tileCoords.y += directions[i];
            else
                tileCoords.x += directions[i];

            if (IsTileInTerrain(tileCoords))
            {
                int newIndexToInspect = CoordsToIndex(tileCoords, m_data->m_width);
                FindColliderTileGroupRecursive(allTiles, tileGroup, newIndexToInspect, directions);
            }
        }
    }
}

void lvle::TerrainSystem::FindColliderEdgesPerTileInTileGroup(const std::vector<int>& group,
                                                              std::vector<std::pair<int, int>>& result)
{
    for (const auto tileIndex : group)
    {
        std::vector<int> directions = {1, -1, -1, 1}; // Up, down, left, right
        for (int i = 0; i < directions.size(); i++)
        {
            auto tileCoords = IndexToCoords(tileIndex, m_data->m_width);
            if (i < 2)
                tileCoords.y += directions[i];
            else
                tileCoords.x += directions[i];

            int tileIndexToCheck = CoordsToIndex(tileCoords, m_data->m_width);
            // add vertices if the neighbor tile isn't part of the group.
            if (std::find(group.begin(), group.end(), tileIndexToCheck) == group.end() || !IsTileInTerrain(tileCoords))
            {
                enum Directions
                {
                    Up,
                    Down,
                    Left,
                    Right
                };
                // vertex indices (A, B, C, D)
                int x = tileIndex % m_data->m_width;
                int y = tileIndex / m_data->m_width;
                int a = x + y * m_data->m_width + y;
                switch (i)
                {
                    case static_cast<int>(Up):  // C, D
                    {
                        int c = a + (m_data->m_width + 1) + 1;
                        int d = a + (m_data->m_width + 1);
                        result.push_back(std::pair<int, int>(c, d));
                        break;
                    }
                    case static_cast<int>(Down):  // A, B
                    {
                        int b = a + 1;
                        result.push_back(std::pair<int, int>(a, b));
                        break;
                    }
                    case static_cast<int>(Left):  // D, A
                    {
                        int d = a + (m_data->m_width + 1);
                        result.push_back(std::pair<int, int>(d, a));
                        break;
                    }
                    case static_cast<int>(Right):  // B, C
                    {
                        int b = a + 1;
                        int c = a + (m_data->m_width + 1) + 1;
                        result.push_back(std::pair<int, int>(b, c));
                        break;
                    }
                    default:
                        break;
                }
                //
            }
        }
    }
}

void lvle::TerrainSystem::FindColliderCornerPoints(const std::vector<std::pair<int, int>>& edgeGroup,
                                                   std::vector<int>& result)
{
    int firstCornerIndex = edgeGroup[0].first;
    result.push_back(firstCornerIndex);
    int currentSecondPoint = edgeGroup[0].second;
    enum Direction
    {
        Up, Down, Left, Right
    };
    Direction currentDirection = Direction::Right;
    std::vector<int> directions = {m_data->m_width, -m_data->m_width, -1, 1};

    while (firstCornerIndex != currentSecondPoint)
    {
        // D  <-  C
        //   *--*
        // | |  | ^
        // v |  | |
        //   *--*
        // A  ->  B
        // find next tile edge ( A->B, B->C, C->D, D->A)
        auto it = std::find_if(edgeGroup.begin(), edgeGroup.end(),
                               [currentSecondPoint](const std::pair<int, int>& element)
                               { return element.first == currentSecondPoint; });
        currentSecondPoint = it->second;
        if (it != edgeGroup.end())
        {
            int indexDifference = it->second - it->first;
            Direction edgeDirection = Direction::Up;
            for (int i = 0; i < directions.size(); i++)
            {
                if (indexDifference == directions[i]) edgeDirection = static_cast<Direction>(i);
            }
            if (currentDirection != edgeDirection)
            {
                currentDirection = edgeDirection;
                result.push_back(it->first);
            }
        }
    }
}

glm::ivec2 lvle::TerrainSystem::IndexToCoords(int index, int width)
{
    return glm::ivec2(index % width, index / width); }

int lvle::TerrainSystem::CoordsToIndex(const glm::vec2 coords, int width) { return coords.x + coords.y * width; }

bool lvle::TerrainSystem::IsTileInTerrain(const glm::ivec2& tileCoords)
{
    int width = m_data->m_width;
    int height = m_data->m_height;
    return (tileCoords.x >= 0 && tileCoords.x < width && tileCoords.y >= 0 && tileCoords.y < height);
}

bool lvle::TerrainSystem::IsVertexInTerrain(const glm::ivec2& vertexCoords)
{
    int width = m_data->m_width + 1;
    int height = m_data->m_height + 1;
    return (vertexCoords.x >= 0 && vertexCoords.x < width && vertexCoords.y >= 0 && vertexCoords.y < height);
}

void lvle::TerrainSystem::GetOccupiedTilesFromObject(const int smallGridIndex, int tileDimsX, int tileDimsY, const bool flipped, std::vector<int>& result)
{
    if (flipped) Swap(tileDimsX, tileDimsY);

    int smallGridWidth = m_data->m_width * 2 + 1;

    vec2 hoveredSmallGridPointCoord = vec2(static_cast<float>(smallGridIndex % smallGridWidth), static_cast<float>(smallGridIndex / smallGridWidth));
    vec2 firstSmallGridPointCoord = vec2(hoveredSmallGridPointCoord.x - tileDimsX, hoveredSmallGridPointCoord.y - tileDimsY);

    int firstTileIndex = (firstSmallGridPointCoord.x / 2) + (firstSmallGridPointCoord.y / 2) * m_data->m_width;
    vec2 firstTileCoord = vec2(firstTileIndex % m_data->m_width, firstTileIndex / m_data->m_width);

    for (int y = static_cast<int>(firstTileCoord.y); y < static_cast<int>(firstTileCoord.y + tileDimsY); y++)
    {
        for (int x = static_cast<int>(firstTileCoord.x); x < static_cast<int>(firstTileCoord.x + tileDimsX); x++)
        {
            if (x >= 0 && x < m_data->m_width && y >= 0 && y < m_data->m_height)
            {
                int tileIndex = x + y * m_data->m_width;
                result.push_back(tileIndex);

            }
        }
    }
}

void lvle::TerrainSystem::DrawWireframe(glm::vec4 color)
{
    // horizontal lines
    int vWidth = m_data->m_width+ 1;
    int vHeight = m_data->m_height+ 1;
    for (int y = 0; y < vHeight; y++)
    {
        for (int x = 0; x < vWidth - 1; x++)
        {
            int indexA = x + y * vHeight;
            int indexB = indexA + 1;
            vec3 aPos = m_data->m_vertices[indexA].position + vec3(0.0f, 0.0f, 0.05f);
            vec3 bPos = m_data->m_vertices[indexB].position + vec3(0.0f, 0.0f, 0.05f);
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, aPos, bPos, color);
        }
    }
    // vertical lines
    for (int x = 0; x < vWidth; x++)
    {
        for (int y = 0; y < vHeight - 1; y++)
        {
            int indexA = x + y * vHeight;
            int indexB = indexA + vHeight;
            vec3 aPos = m_data->m_vertices[indexA].position + vec3(0.0f, 0.0f, 0.05f);
            vec3 bPos = m_data->m_vertices[indexB].position + vec3(0.0f, 0.0f, 0.05f);
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, aPos, bPos, color);
        }
    }
}

void TerrainSystem::DrawNormals(glm::vec4 color)
{
    for (int i = 0; i < m_data->m_vertices.size(); i++)
    {
        Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[i].position,
                                       m_data->m_vertices[i].position + m_data->m_vertices[i].normal,
                                       color);
    }
}

void lvle::TerrainSystem::DrawPathing()
{
    for (int i = 0; i < m_data->m_tiles.size(); i++)
    {
        int x = i % m_data->m_width;
        int y = i / m_data->m_width;
        int a = x + y * m_data->m_width + y;
        int b = a + 1;
        int c = a + (m_data->m_width + 1) + 1;
        int d = a + (m_data->m_width + 1);

        if (m_data->m_tiles[i].tileFlags & TileFlags::NoGroundTraverse)
        {
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[a].position,
                                           m_data->m_vertices[c].position, vec4(1.0, 0.0, 0.0, 1.0));
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[b].position,
                                           m_data->m_vertices[d].position, vec4(1.0, 0.0, 0.0, 1.0));
        }
        if (m_data->m_tiles[i].tileFlags & TileFlags::NoAirTraverse)
        {
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[a].position,
                                           m_data->m_vertices[c].position + vec3(0.0, 0.0, 0.02), vec4(0.0, 0.0, 1.0, 1.0));
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[b].position,
                                           m_data->m_vertices[d].position + vec3(0.0, 0.0, 0.02), vec4(0.0, 0.0, 1.0, 1.0));
        }
        if (m_data->m_tiles[i].tileFlags & TileFlags::NoBuild)
        {
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[a].position,
                                           m_data->m_vertices[c].position + vec3(0.0, 0.0, 0.04), vec4(1.0, 1.0, 0.0, 1.0));
            Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[b].position,
                                           m_data->m_vertices[d].position + vec3(0.0, 0.0, 0.04), vec4(1.0, 1.0, 0.0, 1.0));
        }
    }
}

void TerrainSystem::DrawAreas() const
{
    for (int i = 0; i < m_data->m_tiles.size(); i++)
    {
        if (m_data->m_tiles[i].area == 0) continue;

        int x = i % m_data->m_width;
        int y = i / m_data->m_width;
        int a = x + y * m_data->m_width + y;
        int b = a + 1;
        int c = a + (m_data->m_width + 1) + 1;
        int d = a + (m_data->m_width + 1);

        if (m_data->m_areaPresets.size() <= m_data->m_tiles[i].area)
        {
            m_data->m_areaPresets.resize(m_data->m_tiles[i].area + 1);
            return;
        }


        const auto color = m_data->m_areaPresets[m_data->m_tiles[i].area].color;
        Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[a].position, m_data->m_vertices[c].position,
                                       vec4(color[0], color[1], color[2], 1.0));
        Engine.DebugRenderer().AddLine(DebugCategory::Editor, m_data->m_vertices[b].position, m_data->m_vertices[d].position,
                                       vec4(color[0], color[1], color[2], 1.0));
    }
}

void lvle::TerrainSystem::SaveLevel(std::string& fileName)
{
    CalculateAllTilesCentralPositions();

    // unflag prop and structure tiles
    auto view = Engine.ECS().Registry.view<AttributesComponent>();
    for (auto [entity, attributes] : view.each())
    {
        if (actors::IsUnit(attributes.GetEntityType()))
        {
            
        }
        else if (actors::IsStructure(attributes.GetEntityType()))
        {
            auto structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
            auto& structureTemplate = structureManager.GetStructureTemplate(attributes.GetEntityType());
            std::vector<int> occupiedTiles{};
            GetOccupiedTilesFromObject(attributes.smallGridIndex, structureTemplate.tileDimensions.x, structureTemplate.tileDimensions.y,
                                       attributes.flipped, occupiedTiles);
            for (const auto tileIndex : occupiedTiles)
            {
                SetTileFlags(tileIndex, TileFlags::Traversible);
            }
        }
        else if (actors::IsProp(attributes.GetEntityType()))
        {
            auto propManager = bee::Engine.ECS().GetSystem<PropManager>();
            auto& propTemplate = propManager.GetPropTemplate(attributes.GetEntityType());
            std::vector<int> occupiedTiles{};
            GetOccupiedTilesFromObject(attributes.smallGridIndex, propTemplate.tileDimensions.x, propTemplate.tileDimensions.y,
                                       attributes.flipped, occupiedTiles);
            for (const auto tileIndex : occupiedTiles)
            {
                SetTileFlags(tileIndex, TileFlags::Traversible);
            }
        }
    }

    TerrainDataComponent data = *m_data;

    std::ofstream os(Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(data));
}

// TODO: Rework this whole logic when you're done converting Terrain to system
void lvle::TerrainSystem::LoadLevel(const std::string& fileName)
{
    if (bee::fileExists(Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + ".json")))
    {
        TerrainDataComponent data{};
        std::ifstream is(Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + ".json"));
        cereal::JSONInputArchive archive(is);
        archive(CEREAL_NVP(data));

        //auto view = Engine.ECS().Registry.view<Transform, MeshRenderer, TerrainGroundTagComponent, TerrainDataComponent>();
        //for (auto& entity : view)
        //{
        //    auto [transform, meshRenderer, tag, terrainData] = view.get(entity);
        //    terrainData = data;
        //    m_data = make_shared<TerrainDataComponent>(terrainData);
        //    //auto image = Engine.Resources().Load<Image>("textures/TerrainArt/VA/T_Grass_Stylized_D.png");
        //    //meshRenderer.Material->BaseColorTexture->Image = image;
        //}

        // CreatePlaneLite(data);

        CreatePlane(data.m_width, data.m_height, data.m_step);
        // vertices
        for (int i = 0; i < m_data->m_vertices.size(); i++)
        {
            m_data->m_vertices[i] = data.m_vertices[i];
        }
        // indices
        for (int i = 0; i < m_data->m_indices.size(); i++)
        {
            m_data->m_indices[i] = data.m_indices[i];
        }

        // tiles
        for (int i = 0; i < m_data->m_tiles.size(); i++)
        {
            m_data->m_tiles[i] = data.m_tiles[i];
        }

        UpdatePlane();
        m_mesh->SetIndices(m_data->m_indices);
    }

    UpdateTerrainDataComponent();
}

bool TerrainSystem::FindRayMeshIntersection(const glm::vec3& rayStart, const glm::vec3& rayDir, glm::vec3& result)
{
    bool foundIntersection = false;
    for (int i = 0; i < m_data->m_indices.size(); i += 3)
    {
        if (!foundIntersection)
        {
            int a = m_data->m_indices[i];
            int b = m_data->m_indices[i + 1];
            int c = m_data->m_indices[i + 2];
            glm::vec3 bary = vec3(0.0f, 0.0f, 0.0f);
            if (foundIntersection = glm::intersectLineTriangle(rayStart, rayDir, m_data->m_vertices[a].position,
                                                                m_data->m_vertices[b].position, m_data->m_vertices[c].position, bary))
            {
                double u, v, w;
                v = bary.y;
                w = bary.z;
                u = 1.0f - (v + w);
                result.x = u * m_data->m_vertices[a].position.x + v * m_data->m_vertices[b].position.x + w * m_data->m_vertices[c].position.x;
                result.y = u * m_data->m_vertices[a].position.y + v * m_data->m_vertices[b].position.y + w * m_data->m_vertices[c].position.y;
                result.z = u * m_data->m_vertices[a].position.z + v * m_data->m_vertices[b].position.z + w * m_data->m_vertices[c].position.z;
            }
        }
        else
        {
            break;
        }
    }
    
    return foundIntersection;
}

const int lvle::TerrainSystem::GetSmallGridIndexFromPosition(const glm::vec3& position, int tileDimsX, int tileDimsY) const {
    vec2 terrainSize =
        vec2(m_data->m_width * m_data->m_step, m_data->m_height * m_data->m_step);
    int x = static_cast<int>((position.x + terrainSize.x / 2.0f) / (m_data->m_step / 2.0f) + 0.5f);
    int y = static_cast<int>((position.y + terrainSize.y / 2.0f) / (m_data->m_step / 2.0f) + 0.5f);
    static vec2 hoveredSmallPointCoord = vec2(-1, -1);

    // odd x dimensions
    if (static_cast<int>(tileDimsX) % 2 != 0)
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
    if (static_cast<int>(tileDimsY) % 2 != 0)
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
        int smallPointIndex = hoveredSmallPointCoord.x + hoveredSmallPointCoord.y * (m_data->m_width * 2 + 1);
        return smallPointIndex;
    }
    bee::Log::Error("TerrainSystem::GetSmallGridIndexFromPosition couldn't find a valid small grid index.");
    return -1;
}