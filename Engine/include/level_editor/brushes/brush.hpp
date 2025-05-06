#pragma once
#include <optional>

#include "level_editor/terrain_system.hpp"
#include "actors/actor_utils.hpp"

namespace lvle
{

struct PreviewModelTag
{
    std::string previewType = "";
};

class Brush
{
public:
    Brush();
    Brush(bee::Entity entity)
    {
        m_previewEntity = entity;
    }
    virtual ~Brush();

    const int GetMode() const { return m_brushMode; }
    void SetMode(const int mode) { m_brushMode = mode; }

    void Enable() { m_enabled = true; }
    void Disable() { m_enabled = false; }
    bool IsEnabled() { return m_enabled; }

    void Activate() { m_activate = true; }
    void Deactivate() { m_activate = false; }
    const bool IsActive() const { return m_activate; }

    const int GetRadius() const { return m_radius; }
    void SetRadius(const int radius) { m_radius = radius; }

    const int GetHoveredVertexIndex() const { return m_hoveredVertexIndex; }
    void SetHoveredVertexIndex(const int hoveredPointIndex) { m_hoveredVertexIndex = hoveredPointIndex; }

    const int GetHoveredSmallPointIndex() const { return m_hoveredSmallPointIndex; }
    void SetHoveredSmallPointIndex(const int hoveredSmallPointIndex) { m_hoveredSmallPointIndex = hoveredSmallPointIndex; }

    void SetObjectDimensions(const glm::vec2& objectDimensions)
    {
        m_objectDimensions = objectDimensions;
    }

    const std::vector<int>& GetBrushVertexIndices() const { return m_vertexIndices; }
    const std::vector<int>& GetBrushTiles() const { return m_tileIndices; }

    const std::vector<std::string>& GetAllModes() const
    {
        return m_modes;
    }  // Returns the names of all modes this brush can have.

    const bee::Entity GetPreviewEntity() const { return m_previewEntity; }

    virtual void SetPreviewModel(const std::string& objectHandle) {}
    void RemovePreviewModel();

    /// <summary>
    /// This function snaps a point to the grid based on the brush's snap mode. This might change the value of the output
    /// argument. The function also "prepares" the brush for use by recalculating the hovered grid points, hovered small grid
    /// points or hovered tiles (based on the snap mode).
    /// </summary>
    /// <param name="intersectionPoint">The intersection point between the mouse raycast and the terrain.</param>
    void SnapBrush(glm::vec3& intersectionPoint);

    void Clear();

    virtual void RotatePreviewModel(const float& degreesOffset);

    /// <summary>
    /// This will update the brush's preview model (if it needs one). It will also snap the brush depending on the snap mode.
    /// This might change the value of the output argument. The function also "prepares" the brush for use by recalculating the hovered grid points, hovered small grid
    /// points or hovered tiles (based on the snap mode).
    /// </summary>
    /// <param name="intersectionPoint">The intersection point between the mouse raycast and the terrain.</param>
    /// <param name="objectHandle">The actor template which the preview model should display.</param>
    virtual void Update(glm::vec3& intersectionPoint, const std::string objectHandle = "");

    void Render(const glm::vec3& intersectionPoint);

    virtual void Terraform(const float deltaTime)
    {
        if (!m_enabled) return;
    }
    virtual std::optional<bee::Entity> PlaceObject(const std::string& objectHandle, Team team)
    {
        return std::nullopt;
    }
    virtual std::optional<bee::Entity> PlaceObject(const std::string& objectHandle,
                                                   const glm::vec3& position, Team team)
    {
        return std::nullopt;
    }

protected:  // functions

    void FindBrushCoveredVertexIndices();
    void FindCoveredTileIndices();

protected:  // params
    int m_brushMode = 0;
    SnapMode m_snapMode = SnapMode::NoSnap;
    int m_brushFlags = TileFlags::Traversible;
    glm::vec4 m_brushColor = glm::vec4(1.0, 0.0, 1.0, 1.0);
    glm::vec2 m_objectDimensions = glm::vec2(1, 1);
    bee::Entity m_previewEntity = entt::null;  // The semi-transparent model that will display if the brush places an object.
    int m_radius = 1;                          // this is a square brush and the radius is measured in "vertices"
    int m_hoveredVertexIndex = 0;         // the hovered vertex's index
    std::vector<int> m_vertexIndices;          // the indices of the vertices surrounding the hovered one (including it)
    int m_hoveredSmallPointIndex = 0;          // used only for placing structures and props
    std::vector<int> m_tileIndices;            // the indices of the tiles affected by the brush.
    std::vector<std::string> m_modes;          // the names of all the modes the brush can have (different for each brush).

    // flags
    bool m_activate = false;
    bool m_enabled = false;
};

}  // namespace lvle