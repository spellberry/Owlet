#pragma once
#include "brush.hpp"

namespace lvle
{

class UnitBrush : public lvle::Brush
{
public:
    UnitBrush() : Brush()
    { 
        Brush::Brush();
        m_snapMode = SnapMode::NoSnap;
    }

    UnitBrush(bee::Entity entity) : Brush(entity)
    {
        Brush::Brush(entity);
        m_snapMode = SnapMode::NoSnap;
    }

    void SetPreviewModel(const std::string& objectHandle);
    void Update(glm::vec3& intersectionPoint, const std::string objectHandle = "") override;
    std::optional<bee::Entity> PlaceObject(const std::string& objectHandle, Team team) override;

private:  // functions
    bool IsHoveredTileWalkable(const glm::vec3& position);
    bool IsSpaceEmpty();

private:  // params
};

}  // namespace lvle