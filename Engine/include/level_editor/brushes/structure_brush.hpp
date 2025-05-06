#pragma once

#include "brush.hpp"
#include "actors/props/resource_type.hpp"

namespace lvle
{

class StructureBrush : public lvle::Brush
{
public:
    StructureBrush() : Brush()
    {
        Brush::Brush();
        m_snapMode = SnapMode::SnapObject;
    }

    StructureBrush(bee::Entity entity) : Brush(entity)
    {
        Brush::Brush(entity);
        m_snapMode = SnapMode::SnapObject;
    }

    void SetPreviewModel(const std::string& objectHandle) override;
    void Update(glm::vec3& intersectionPoint, const std::string objectHandle = "") override;
    int CalculateSmallGridIndex(const glm::vec3& intersectionPoint);
    void RotatePreviewModel(const float& degreesOffset) override;
    // Usually used for placing structures.
    std::optional<bee::Entity> PlaceObject(const std::string& objectHandle, Team team) override;

    // Usuaully used for placing structures.
    std::optional<bee::Entity> PlaceObject(const std::string& objectHandle, const glm::vec3& position, Team team);

    // Usually used for placing structures.
    std::optional<bee::Entity> PlaceObject(const std::string& objectHandle, const glm::vec3& position, const glm::quat& rotation, Team team);

    void PlaceMultipleObjects(const std::string& objectHandle, const std::vector<glm::vec3>& positions, const glm::quat& rotation, const size_t amount, GameResourceType resource);



    bool GetFlipped() { return m_flipped; }
    void SetFlipped(bool flag) { m_flipped = flag; }

private:  // functions
    bool AreHoveredTilesBuildable();
    bool AreHoveredTilesBuildableWithPosition(glm::vec3& position); // this will also adjust the position of the wall to be exactly on the small grid
    std::vector<int> FindCoveredTileIndicesFromPosition(glm::vec3& position, const glm::ivec2& tileDims);

private:  // params
    bool m_flipped = false;
};

}  // namespace lvle