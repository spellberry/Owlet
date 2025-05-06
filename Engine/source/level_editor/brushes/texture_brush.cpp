#include "level_editor/brushes/texture_brush.hpp"

#include "tools/tools.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

void lvle::TextureBrush::Terraform(const float deltaTime)
{
    Brush::Terraform(deltaTime);
    switch (m_brushMode)
    {
        case static_cast<int>(TextureBrushMode::Grass):
        {
            Grass(deltaTime);
            break;
        }
        case static_cast<int>(TextureBrushMode::Dirt):
        {
            Dirt(deltaTime);
            break;
        }
        case static_cast<int>(TextureBrushMode::Rock):
        {
            Rock(deltaTime);
            break;
        }
        case static_cast<int>(TextureBrushMode::Empty2):
        {
            EmptySecond(deltaTime);
            break;
        }
        default:
            break;
    }
}

void lvle::TextureBrush::Grass(const float deltaTime)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& data = terrain.m_data;
    for (auto vIndex : m_vertexIndices)
    {
        data->m_vertices[vIndex].materialWeights[0] = std::clamp(data->m_vertices[vIndex].materialWeights[0] + weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[1] = std::clamp(data->m_vertices[vIndex].materialWeights[1] - weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[2] = std::clamp(data->m_vertices[vIndex].materialWeights[2] - weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[3] = std::clamp(data->m_vertices[vIndex].materialWeights[3] - weight, 0.0f, 1.0f);
    }
}

void lvle::TextureBrush::Dirt(const float deltaTime)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& data = terrain.m_data;
    for (auto vIndex : m_vertexIndices)
    {
        data->m_vertices[vIndex].materialWeights[0] =
            std::clamp(data->m_vertices[vIndex].materialWeights[0] - weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[1] =
            std::clamp(data->m_vertices[vIndex].materialWeights[1] + weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[2] =
            std::clamp(data->m_vertices[vIndex].materialWeights[2] - weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[3] =
            std::clamp(data->m_vertices[vIndex].materialWeights[3] - weight, 0.0f, 1.0f);
    }
}

void lvle::TextureBrush::Rock(const float deltaTime)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();
    auto& data = terrain.m_data;
    for (auto vIndex : m_vertexIndices)
    {
        data->m_vertices[vIndex].materialWeights[0] =
            std::clamp(data->m_vertices[vIndex].materialWeights[0] - weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[1] =
            std::clamp(data->m_vertices[vIndex].materialWeights[1] - weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[2] =
            std::clamp(data->m_vertices[vIndex].materialWeights[2] + weight, 0.0f, 1.0f);
        data->m_vertices[vIndex].materialWeights[3] =
            std::clamp(data->m_vertices[vIndex].materialWeights[3] - weight, 0.0f, 1.0f);
    }
}

void lvle::TextureBrush::EmptySecond(const float deltaTime) {}