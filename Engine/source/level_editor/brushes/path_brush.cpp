#include "level_editor/brushes/path_brush.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

void lvle::PathBrush::Terraform(const float deltaTime)
{
    Brush::Terraform(deltaTime);
    switch (m_brushMode)
    {
        case static_cast<int>(PathBrushMode::Traverse):
        {
            Traversible();
            break;
        }
        case static_cast<int>(PathBrushMode::NoGround):
        {
            NoGroundTraverse();
            break;
        }
        case static_cast<int>(PathBrushMode::NoAir):
        {
            NoAirTraverse();
            break;
        }
        case static_cast<int>(PathBrushMode::NoBuild):
        {
            NoBuild();
            break;
        }
        default:
            break;
    }
}

void lvle::PathBrush::Traversible()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& data = terrain.m_data;
    for (int i = 0; i < m_tileIndices.size(); i++)
    {
        data->m_tiles[m_tileIndices[i]].tileFlags = TileFlags::Traversible;
    }
}

void lvle::PathBrush::NoGroundTraverse()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& data = terrain.m_data;
    for (int i = 0; i < m_tileIndices.size(); i++)
    {
        data->m_tiles[m_tileIndices[i]].tileFlags &= ~TileFlags::Traversible;
        data->m_tiles[m_tileIndices[i]].tileFlags |= TileFlags::NoGroundTraverse;
    }
}

void lvle::PathBrush::NoAirTraverse()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& data = terrain.m_data;
    for (int i = 0; i < m_tileIndices.size(); i++)
    {
        data->m_tiles[m_tileIndices[i]].tileFlags &= ~TileFlags::Traversible;
        data->m_tiles[m_tileIndices[i]].tileFlags |= TileFlags::NoAirTraverse;
    }
}

void lvle::PathBrush::NoBuild()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& data = terrain.m_data;
    for (int i = 0; i < m_tileIndices.size(); i++)
    {
        data->m_tiles[m_tileIndices[i]].tileFlags |= TileFlags::NoBuild;
    }
}
