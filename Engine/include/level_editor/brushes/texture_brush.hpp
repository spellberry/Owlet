#pragma once
#include "brush.hpp"

namespace lvle
{

enum class TextureBrushMode
{
    Grass,
    Dirt,
    Rock,
    Empty2
};

class TextureBrush : public lvle::Brush
{
public:
    TextureBrush() : Brush()
    {
        Brush::Brush();
        m_snapMode = SnapMode::SnapToPoint;
        m_modes.push_back("Grass");
        m_modes.push_back("Dirt");
        m_modes.push_back("Rock");
        m_modes.push_back("Empty 2");
    }

    TextureBrush(bee::Entity entity) : Brush(entity)
    {
        Brush::Brush(entity);
        m_snapMode = SnapMode::SnapToPoint;
        m_modes.push_back("Foundation");
        m_modes.push_back("Highlight");
    }

    void Terraform(const float deltaTime) override;

private:  // functions
    // NOTE: This will be very hard-coded and will be made under the assumption there's only two materials.
    void Grass(const float deltaTime);
    void Dirt(const float deltaTime);
    void Rock(const float deltaTime);
    void EmptySecond(const float deltaTime);

public:  // params
    float weight = 0.1f;
};

}  // namespace lvle