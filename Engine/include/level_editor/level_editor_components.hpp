#include <cereal/types/unordered_map.hpp>
#include <filesystem>

#include "platform/dx12/DeviceManager.hpp"
#include "rendering/render_components.hpp"
#include "tools/serialize_glm.h"
#include "utils.hpp"

namespace lvle
{

// This component indicates the entity that has it, is the ground plane of the terrain.
struct TerrainGroundTagComponent
{
    char dummy = 'd';
};

// This component indicates the entity that has it, is the cliffs of the terrain.
struct TerrainCliffTagComponent
{
    char dummy = 'd';
};

struct AreaPreset
{
    float color[3]{0};

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(color));
    }
};

struct FoliageComponent
{
    std::string name = "";
};

struct TerrainDataComponent
{
    int m_width = 0, m_height = 0;  // terrain dimensions in tiles
    float m_step = 0;               // dimensions of a tile

    std::vector<DWORD> m_indices;  // triangles are described counter-clockwise (CCW)
    std::vector<TVertex> m_vertices;
    std::vector<glm::vec2> m_smallGridPoints;  // a smaller grid using for brush snapping for structures and props.
    std::vector<tile> m_tiles;
    std::vector<AreaPreset> m_areaPresets;
    std::vector<std::string> m_materialPaths = {"materials/TerrainGrass.pepimat", "materials/TerrainDirt.pepimat",
                                                "materials/TerrainRock.pepimat", "materials/Empty.pepimat"};
    std::vector<std::shared_ptr<bee::Material>> m_materials = {nullptr, nullptr, nullptr, nullptr};

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(m_width), CEREAL_NVP(m_height), CEREAL_NVP(m_step), CEREAL_NVP(m_indices), CEREAL_NVP(m_vertices),
                CEREAL_NVP(m_tiles), CEREAL_NVP(m_areaPresets), CEREAL_NVP(m_materialPaths));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(m_width), CEREAL_NVP(m_height), CEREAL_NVP(m_step), CEREAL_NVP(m_indices), CEREAL_NVP(m_vertices),
                CEREAL_NVP(m_tiles), CEREAL_NVP(m_areaPresets), CEREAL_NVP(m_materialPaths));
    }
};

}  // namespace lvle