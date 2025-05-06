#pragma once
#include "ai/navigation_path.hpp"
#include "rendering/debug_render.hpp"

namespace bee::ai
{
    class NavigationGrid
    {
    public:
        NavigationGrid(const glm::vec2& position, const size_t tileSize, const size_t sizeX,const size_t sizeY);

        NavigationPath ComputePath(glm::vec2 a, glm::vec2 b) const;
        NavigationPath ComputePathManhattan(glm::vec2 a, glm::vec2 b) const;
        NavigationPath ComputePathStraightLine(glm::vec2 a, glm::vec2 b) const;

        void SetVertexPosition(const int index, const bee::graph::VertexWithPosition& v);
        glm::vec2 SampleWalkablePoint(glm::vec2 pos) const;
        void DebugDraw(DebugRenderer& renderer) const;
    private:
        graph::EuclideanGraph m_graph{};
        glm::vec2 m_startPosition{};

        int m_tileSize;
        int m_sizeX;
        int m_sizeY ;
    };
}
