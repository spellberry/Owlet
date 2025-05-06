#pragma once
#include "brush.hpp"

namespace lvle
{

enum class TerraformBrushMode
{
    Raise,
    Lower,
    Plateau,
    Smooth
};

class TerraformBrush : public lvle::Brush
{
public:
    TerraformBrush() : Brush()
    {
        Brush::Brush();
        m_snapMode = SnapMode::SnapToPoint;
        m_modes.push_back("Raise");
        m_modes.push_back("Lower");
        m_modes.push_back("Plateau");
        m_modes.push_back("Smooth");
    }

    TerraformBrush(bee::Entity entity) : Brush(entity)
    {
        Brush::Brush(entity);
        m_snapMode = SnapMode::SnapToPoint;
        m_modes.push_back("Raise");
        m_modes.push_back("Lower");
        m_modes.push_back("Plateau");
        m_modes.push_back("Smooth");
    }

    void Terraform(const float deltaTime) override;

    const bool GetDoAveraging() const { return m_doAveraging; }
    void SetDoAveraging(const bool doAveraging) { m_doAveraging = doAveraging; }

    const float GetIntensity() const { return m_intensity; }
    void SetIntensity(const float intensity) { m_intensity = intensity; }

    const float GetMaxIntensity() const { return m_maxIntensity; }

    void UpdateActorsVerticalPos();
    void UpdateFoliageVerticalPos();

private:  // functions
    void Raise(const float deltaTime);
    void Lower(const float deltaTime);
    void Plateau(const float deltaTime);
    void Smooth(const float deltaTime);

    // helper functions
    float CalculateHighestPointInBrush();
    float CalculateLowestPointInBrush();
    float CalculateAverageHeightInBrush();


private:  // params
    float m_intensity = 5.0f;
    float m_maxIntensity = 30.0f;
    bool m_doAveraging = false;

    float m_fixedDeltaTime = 0.5f;
};

}  // namespace lvle