#pragma once
#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "level_editor/level_editor_components.hpp"
#include "rendering/mesh.hpp"
#include "tools/log.hpp"

namespace lvle
{

    //struct TerrainDataComponent;

    bool GetTerrainHeightAtPoint(const float x, const float y, float& result);
/// <summary>
/// Upon creating the system, a "terrain" entity will be created as well. The entity has a Transform, MeshRenderer, TerrainDataComponent and TerrainGroundComponent.
/// </summary>
class TerrainSystem : public bee::System
{
public:
    TerrainSystem();

    void UpdateTerrainDataComponent();

    bool FindRayMeshIntersection(const glm::vec3& rayStart, const glm::vec3& rayDir, glm::vec3& result);

    void SaveLevel(std::string& fileName);
    void LoadLevel(const std::string& fileName);

    void CalculateAllTilesCentralPositions();
    void CalculateTileCentralPosition(int tileIndex);
    void CalculateTerrainColliders();

    bee::Entity CreateTerrainColliderEntity(const std::vector<int>& pointsIndices, const std::string& name);

    bool CanBuildOnTile(const int index) const { return !(m_data->m_tiles[index].tileFlags & TileFlags::NoBuild); }
    void SetTileFlags(const int index, const TileFlags flagsToSet) const { m_data->m_tiles[index].tileFlags = flagsToSet; }

    // Based on Brush::FindCoveredTileIndices, SnapToObject case
    void GetOccupiedTilesFromObject(const int smallGridIndex, int tileDimsX, int tileDimsY, const bool flipped, std::vector<int>& result);

    void DrawWireframe(glm::vec4 color);
    void DrawNormals(glm::vec4 color);
    void DrawPathing();
    void DrawAreas() const;

    glm::ivec2 IndexToCoords(int index, int width);
    int CoordsToIndex(const glm::vec2 coords, int width);

    bool IsTileInTerrain(const glm::ivec2& tileCoords);
    bool IsVertexInTerrain(const glm::ivec2& vertexCoords);

    int GetWidthInTiles() { return m_data->m_width; }
    int GetHeightInTiles() { return m_data->m_height; }
    const int GetSmallGridIndexFromPosition(const glm::vec3& position, int tileDimsX, int tileDimsY) const;

protected:

    /// <summary>
    /// Creates a flat plane (aka a starting point for the terrain)
    /// </summary>
    /// <param name="width">: Quad segments on the x axis</param>
    /// <param name="height">: Quad segments on the y axis</param>
    /// <param name="step"> Size of a single tile. (both dimensions, they're squares)</param>
    /// <param name="atlasWidth">Quad segments on the x axis of the atlas</param>
    /// <param name="atlasHeight">Quad segments on the y axis of the atlas</param>
    /// <param name="atlasStep">Size of a single atlas tile. (both dimensions, they're squares)</param>
    void CreatePlane(int width, int height, float step);
    void CreatePlaneLite(const TerrainDataComponent& data);
    void UpdatePlane();
    void UpdateNormals();
    void UpdateTangents();

    void FindColliderTileGroupRecursive(std::vector<int>& allTiles, std::vector<int>& tileGroup, const int tileIndexToInspect,
                                        const std::vector<int>& directions);
    void FindColliderEdgesPerTileInTileGroup(const std::vector<int>& group, std::vector<std::pair<int, int>>& result);
    void FindColliderCornerPoints(const std::vector<std::pair<int, int>>& edgeGroup, std::vector<int>& result);

protected:
    std::shared_ptr<lvle::TerrainDataComponent> m_data;
    std::shared_ptr<bee::Mesh> m_mesh;

    bool m_loadLevel = false;

private:
    friend class LevelEditor;
    friend class Brush;
    friend class TerraformBrush;
    friend class TextureBrush;
    friend class PathBrush;
    friend class UnitBrush;
    friend class PropBrush;
    friend class StructureBrush;
    friend class AreaBrush;
    friend class FoliageBrush;
};

}  // namespace lvle
