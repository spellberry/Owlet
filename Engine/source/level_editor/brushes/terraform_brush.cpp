#include "level_editor/brushes/terraform_brush.hpp"

#include <random>

#include "actors/attributes.hpp"
#include "math/math.hpp"
#include "tools/3d_utility_functions.hpp"

using namespace bee;
using namespace lvle;
using namespace std;
using namespace glm;

void lvle::TerraformBrush::Terraform(const float deltaTime)
{
    Brush::Terraform(deltaTime);
    switch (m_brushMode)
    {
        case static_cast<int>(TerraformBrushMode::Raise):
        {
            Raise(deltaTime);
            break;
        }
        case static_cast<int>(TerraformBrushMode::Lower):
        {
            Lower(deltaTime);
            break;
        }
        case static_cast<int>(TerraformBrushMode::Plateau):
        {
            Plateau(deltaTime);
            break;
        }
        case static_cast<int>(TerraformBrushMode::Smooth):
        {
            Smooth(deltaTime);
            break;
        }
        default:
            break;
    }

    // Update actors' vertical positions
    static float dtSum;
    dtSum += deltaTime;
    if (dtSum >= m_fixedDeltaTime)
    {
        // UpdateActorsVerticalPos();
        // UpdateFoliageVerticalPos();
        dtSum -= m_fixedDeltaTime;
    }
}

void lvle::TerraformBrush::Raise(const float deltaTime)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    auto data = terrain.m_data;
    if (m_doAveraging)
    {
        float avgHeight = CalculateAverageHeightInBrush();
        for (auto& index : m_vertexIndices)
        {
            if (data->m_vertices[index].position.z <= avgHeight)
            {
                data->m_vertices[index].position.z += m_intensity * deltaTime;
            }
        }
    }
    else
    {
        float spread = static_cast<float>(m_radius) / 2.4f;
        for (auto& index : m_vertexIndices)
        {
            vec2 indexCoord =
                vec2(static_cast<int>(index % (data->m_width + 1)), static_cast<int>(index / (data->m_height + 1)));
            vec2 centerIndexCoord = vec2(static_cast<int>(m_hoveredVertexIndex % (data->m_width + 1)),
                                         static_cast<int>(m_hoveredVertexIndex / (data->m_height + 1)));
            double distanceSquared = (indexCoord.x - centerIndexCoord.x) * (indexCoord.x - centerIndexCoord.x) +
                                     (indexCoord.y - centerIndexCoord.y) * (indexCoord.y - centerIndexCoord.y);
            double heightChange = std::exp(-0.5 * distanceSquared / (spread * spread));

            data->m_vertices[index].position.z += heightChange * m_intensity * deltaTime;
        }
    }
}

void lvle::TerraformBrush::Lower(const float deltaTime)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    auto data = terrain.m_data;
    if (m_doAveraging)
    {
        float avgHeight = CalculateAverageHeightInBrush();
        for (auto& index : m_vertexIndices)
        {
            if (data->m_vertices[index].position.z >= avgHeight)
            {
                data->m_vertices[index].position.z -= m_intensity * deltaTime;
            }
        }
    }
    else
    {
        float spread = static_cast<float>(m_radius) / 2.4f;
        for (auto& index : m_vertexIndices)
        {
            vec2 indexCoord =
                vec2(static_cast<int>(index % (data->m_width + 1)), static_cast<int>(index / (data->m_height + 1)));
            vec2 centerIndexCoord = vec2(static_cast<int>(m_hoveredVertexIndex % (data->m_width + 1)),
                                         static_cast<int>(m_hoveredVertexIndex / (data->m_height + 1)));
            double distanceSquared = (indexCoord.x - centerIndexCoord.x) * (indexCoord.x - centerIndexCoord.x) +
                                     (indexCoord.y - centerIndexCoord.y) * (indexCoord.y - centerIndexCoord.y);
            double heightChange = std::exp(-0.5 * distanceSquared / (spread * spread));

            data->m_vertices[index].position.z -= heightChange * m_intensity * deltaTime;
        }
    }
}

void lvle::TerraformBrush::Plateau(const float deltaTime)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    auto& data = terrain.m_data;
    float center_height = data->m_vertices[m_hoveredVertexIndex].position.z;
    for (auto& index : m_vertexIndices)
    {
        data->m_vertices[index].position.z = center_height;
    }
}

void lvle::TerraformBrush::Smooth(const float deltaTime)
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    auto& data = terrain.m_data;

    int vWidth = data->m_width + 1;
    // int vHeight = data->m_height + 1;
    for (const auto index : m_vertexIndices)
    {
        auto vertexPos = data->m_vertices[index].position;
        float smoothedHeight = vertexPos.z;
        int neighborIndices[4] = {index + vWidth, index - vWidth, index - 1, index + 1};  // up, down, left, right
        // sum heights
        for (const auto neighborIndex : neighborIndices)
        {
            ivec2 neighborVertexCoords = ivec2(static_cast<int>(neighborIndex % (data->m_width + 1)),
                                               static_cast<int>(neighborIndex / (data->m_height + 1)));
            if (neighborVertexCoords.x >= 0 && neighborVertexCoords.x < data->m_width + 1 && neighborVertexCoords.y >= 0 &&
                neighborVertexCoords.y < data->m_height + 1)
            {
                smoothedHeight += data->m_vertices[neighborIndex].position.z;
            }
            else
                smoothedHeight += 0.0f;
        }

        // average heights
        smoothedHeight /= 5.0f;

        data->m_vertices[index].position.z =
            mix(vertexPos.z, smoothedHeight, Remap(0.0f, m_maxIntensity * deltaTime, 0.0f, 1.0f, m_intensity * deltaTime));
    }
}

// helper functions

float lvle::TerraformBrush::CalculateHighestPointInBrush()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    auto& data = terrain.m_data;
    float maxHeight = data->m_vertices[m_hoveredVertexIndex].position.z;
    int vertexAtMaxHeightCnt = 0;  // number of vertices which are at max brush height
    for (auto& index : m_vertexIndices)
    {
        float currentHeight = data->m_vertices[index].position.z;
        if (currentHeight == maxHeight)
            vertexAtMaxHeightCnt++;
        else if (currentHeight > maxHeight)
            maxHeight = currentHeight;
    }
    if (vertexAtMaxHeightCnt == m_vertexIndices.size()) return 100000.0f;
    return maxHeight;
}

float lvle::TerraformBrush::CalculateLowestPointInBrush()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    auto& data = terrain.m_data;
    float minHeight = data->m_vertices[m_hoveredVertexIndex].position.z;
    int vertexAtMinHeightCnt = 0;  // number of vertices which are at min brush height
    for (auto& index : m_vertexIndices)
    {
        if (data->m_vertices[index].position.z == minHeight)
            vertexAtMinHeightCnt++;
        else if (data->m_vertices[index].position.z < minHeight)
            minHeight = data->m_vertices[index].position.z;
    }
    if (vertexAtMinHeightCnt == m_vertexIndices.size()) return -100000.0f;
    return minHeight;
}

float lvle::TerraformBrush::CalculateAverageHeightInBrush()
{
    auto& terrain = Engine.ECS().GetSystem<TerrainSystem>();

    auto& data = terrain.m_data;
    float avgHeight = 0.0f;
    for (auto& index : m_vertexIndices)
    {
        float currentHeight = data->m_vertices[index].position.z;
        avgHeight += currentHeight;
    }
    return (avgHeight / static_cast<float>(m_vertexIndices.size()));
}

void lvle::TerraformBrush::UpdateActorsVerticalPos()
{
    auto unitView = Engine.ECS().Registry.view<Transform, AttributesComponent>();
    for (auto [entity, transform, attributes] : unitView.each())
    {
        float newHeight = 0.0f;
        if (GetTerrainHeightAtPoint(transform.Translation.x, transform.Translation.y, newHeight))
        {
            transform.Translation.z = newHeight;
        }
    }
}

void lvle::TerraformBrush::UpdateFoliageVerticalPos()
{
    auto foliageView = Engine.ECS().Registry.view<Transform, FoliageComponent>();
    for (auto [entity, transform, attributes] : foliageView.each())
    {
        float newHeight = 0.0f;
        if (GetTerrainHeightAtPoint(transform.Translation.x, transform.Translation.y, newHeight))
        {
            transform.Translation.z = newHeight;
        }
    }
}
