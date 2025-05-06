#pragma once
#include "brush.hpp"

namespace lvle
{

class PropBrush : public lvle::Brush
{
public:
    PropBrush() : Brush()
    { 
        Brush::Brush();
        m_snapMode = SnapMode::SnapObject;
    }

    PropBrush(bee::Entity entity) : Brush(entity)
    {
        Brush::Brush(entity);
        m_snapMode = SnapMode::SnapObject;
    }

    void SetPreviewModel(const std::string& objectHandle) override;
    void Update(glm::vec3& intersectionPOint, const std::string objectHandle = "") override;
    void RotatePreviewModel(const float& degreesOffset) override;
    // Usually used for placing structures.
    std::optional<bee::Entity> PlaceObject(const std::string& objectHandle, Team team) override;
    // Usually used for placing structures.
    std::optional<bee::Entity> PlaceObject(const std::string& objectHandle, const glm::vec3& position, Team team) override;

    bool GetFlipped() { return m_flipped; }

private:  // functions
    bool AreHoveredTilesBuildable(const glm::vec3& position);

private:  // params
    bool m_flipped = false;
};

}  // namespace lvle