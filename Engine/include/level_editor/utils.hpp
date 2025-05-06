#pragma once
#include <optional>
#include <glm/glm.hpp>
#include <vector>

#include "cereal/cereal.hpp"
#include <cereal/types/string.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/archives/json.hpp>

namespace lvle
{

// --- Terrain Editor ---------------------------------------------------------------

enum TileFlags
{
    Traversible = 1 << 0,
    NoGroundTraverse = 1 << 1,
    NoAirTraverse = 1 << 2,
    NoBuild = 1 << 3,
    NoNavalTraverse = 1 << 4,
};

// Represents a vertex on the terrain.
struct TVertex
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec2 uv = glm::vec2(0.0f);
    glm::vec3 tangent = glm::vec3(0.0f);
    glm::vec3 bitangent = glm::vec3(0.0f);

    int cliffLevel = 0;
    std::vector<float> materialWeights = {1, 0, 0, 0};

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(position), CEREAL_NVP(normal), CEREAL_NVP(uv), CEREAL_NVP(cliffLevel), CEREAL_NVP(materialWeights));
    }
};

// Represents a tile on the terrain
// Tiles are used for pathing.
struct tile
{
    int index;
    int tileFlags = TileFlags::Traversible;
    int area= 0;

    std::vector<std::reference_wrapper<TVertex>> vertices;
    glm::vec3 centralPos = glm::vec3(0.0f);

    // TODO (sure buddy)
    std::vector<int> GetVerticesIndices()
    {
        /*
        auto& data = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>().m_data;
        int x = index % data->m_width;
        int y = index / data->m_width;
        int a = x + y * data->m_width + y;
        int b = a + 1;
        int c = a + (data->m_width + 1) + 1;
        int d = a + (data->m_width + 1);
        std::vector<std::reference_wrapper<lvle::vertex>> returnVector;
        returnVector.push_back(data->m_vertices[a]);
        returnVector.push_back(data->m_vertices[b]);
        returnVector.push_back(data->m_vertices[c]);
        returnVector.push_back(data->m_vertices[d]);
        return returnVector;
        */
    }

    template <class Archive>
    void save(Archive& archive)const
    {
        archive(CEREAL_NVP(index), CEREAL_NVP(tileFlags), CEREAL_NVP(centralPos), CEREAL_NVP(area));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(index), CEREAL_NVP(tileFlags), CEREAL_NVP(centralPos),CEREAL_NVP(area));
    }
};

// --------------------------------------------------------------------------------

// --- Level Editor Utils ---------------------------------------------------------

struct EditorDebugCategory
{
    enum Enum
    {
        Wireframe = 1 << 0,
        Normals = 1 << 1,
        Cross = 1 << 2,
        Mouse = 1 << 3,
        Pathing = 1 << 4,
        Areas = 1 << 5,
    };
};

// --------------------------------------------------------------------------------

// --- Brush Utils ----------------------------------------------------------------

enum class SnapMode
{
    NoSnap,           // used for units
    SnapToPoint,      // used for terraforming
    SnapObject        // used for props/structures
};

enum class BrushType
{
    Terraform=0,
    Texturing=1,
    Path=2,
    GameplayArea =3,
    Cliff,
};

// --------------------------------------------------------------------------------
}  // namespace lvle