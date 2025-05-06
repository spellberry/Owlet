#include "level_editor/brushes/area_brush.hpp"

void lvle::AreaBrush::Terraform(const float deltaTime)
{
    Brush::Terraform(deltaTime);
    SetAreaIndex(areaIndex);
}

void lvle::AreaBrush::SetAreaIndex(int index) 
{
    auto& terrain = bee::Engine.ECS().GetSystem<lvle::TerrainSystem>();
    auto& data = terrain.m_data;
    for (int i = 0; i < m_tileIndices.size(); i++)
    {
        data->m_tiles[m_tileIndices[i]].area = index;
    }
}

