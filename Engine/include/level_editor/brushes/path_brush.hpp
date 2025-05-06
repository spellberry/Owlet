#pragma once
#include "brush.hpp"

namespace lvle
{

enum class PathBrushMode
{
    Traverse,
    NoGround,
    NoAir,
    NoBuild
};

class PathBrush : public lvle::Brush
{
public:
    PathBrush() : Brush()
    {
        Brush::Brush();
        m_snapMode = SnapMode::SnapToPoint;
        m_modes.push_back("Traversible");
        m_modes.push_back("No Ground");
        m_modes.push_back("No Air");
        m_modes.push_back("No Build");
    }

    PathBrush(bee::Entity entity) : Brush(entity)
    {
        Brush::Brush(entity);
        m_snapMode = SnapMode::SnapToPoint;
        m_modes.push_back("Traversible");
        m_modes.push_back("No Ground");
        m_modes.push_back("No Air");
        m_modes.push_back("No Build");
    }

    void Terraform(const float deltaTime) override;

private:  // functions
    void Traversible();
    void NoGroundTraverse();
    void NoAirTraverse();
    void NoBuild();
    // I'll add this if we decide we want water
    // void NoWaterTraverse(std::unique_ptr<Terrain>& terrain, );

private:  // params
};

}  // namespace lvle