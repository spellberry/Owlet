#pragma once
#include "brush.hpp"

namespace lvle
{

    class AreaBrush : public lvle::Brush
    {
    public:
        AreaBrush() : Brush()
        {
            m_snapMode = lvle::SnapMode::SnapToPoint;
            m_modes.push_back("Area Brush");
        }

        AreaBrush(bee::Entity entity) : Brush(entity)
        {
            Brush::Brush(entity);
            m_snapMode = lvle::SnapMode::SnapToPoint;
            m_modes.push_back("Area Brush");
        }

        void Terraform(const float deltaTime) override;
        int areaIndex = 0;
    private:
        void SetAreaIndex(int index);
    };
}  // namespace lvle