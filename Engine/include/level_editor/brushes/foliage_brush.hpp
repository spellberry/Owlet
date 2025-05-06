#pragma once
#include "brush.hpp"
#include <tinygltf/stb_image.h>
#include <cereal/cereal.hpp>

namespace bee
{
class Material;
}

namespace lvle
{

struct FoliageTemplate
{
    std::string name = "FoliageType";
    std::string modelPath = "models/BoxAndCylinder.gltf";
    
    std::vector<std::string> materialPaths{} /* = {"materials/pop.pepimat"}*/;
    std::vector<std::shared_ptr<bee::Material>> materials{} /* = {nullptr}*/;
    std::shared_ptr<bee::Model> model = nullptr;
    bee::Transform modelOffset;
    float windMultiplier = 1.0f;

    bool active = true;

    int density = 100;
    float distanceRadius = 0.1f;
    float zOffsetMin = 0.0f;
    float zOffsetMax = 0.0f;
    float scaleRangeMin = 1.0f;
    float scaleRangeMax = 1.0f;
    glm::vec3 rotationRange = glm::vec3(0.0f, 0.0f, 180.0f);
    float colorOverlayIntensity = 0.0f;
    float noiseIntensity = 1.0f;

    glm::vec3 currentBrushstrokeColor = glm::vec3(1.0f);
    std::vector<glm::vec3> colorVariations;

private:
    friend class cereal::access;

    template<class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(windMultiplier), CEREAL_NVP(modelPath), CEREAL_NVP(materialPaths), CEREAL_NVP(modelOffset), CEREAL_NVP(colorVariations));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(name),  CEREAL_NVP(windMultiplier), CEREAL_NVP(modelPath), CEREAL_NVP(materialPaths), CEREAL_NVP(modelOffset), CEREAL_NVP(colorVariations));
    }
};

class FoliageBrush : public lvle::Brush
{
public:
    FoliageBrush() : Brush()
    {
        Brush::Brush();
        m_snapMode = SnapMode::NoSnap;
        m_modes.push_back("Apply");
        m_modes.push_back("Erase");

        LoadNoiseImage("assets/textures/TerrainArt/VA/T_Grass_Noise_Mask.png");
    }

    FoliageBrush(bee::Entity entity) : Brush(entity)
    {
        Brush::Brush(entity);
        m_snapMode = SnapMode::NoSnap;
        m_modes.push_back("Apply");
        m_modes.push_back("Erase");
    }

    ~FoliageBrush() override {
        Brush::~Brush();
        if (m_noiseImage != nullptr)
            stbi_image_free(m_noiseImage);
    }

    void Update(glm::vec3& intersectionPoint, const std::string objectHandle = "") override;
    void Terraform(float deltaTime) override;
    void Apply();
    void Erase();

    void AddFoliageType(FoliageTemplate foliageTemplate);
    void RemoveFoliageType(const std::string& foliageHandle);
    void RemoveAllFoliageFromType(const std::string& foliageHandle);
    FoliageTemplate& GetFoliageTemplate(const std::string& foliageHandle);

    void SaveFoliageTemplates(const std::string& filename);
    void SaveFoliageInstances(const std::string& filename);
    void LoadFoliageTemplates(const std::string& filename);
    void LoadFoliageInstances(const std::string& filename);

   glm::vec3 SampleNoiseMask(const glm::vec3& foliagePosition);

private:  // functions
    void PlaceFoliageInstance(const bee::Transform& randomizedTransform, const FoliageTemplate& foliageTemplate, const glm::vec3& color, const bool fromFile = false);
    
   bool GenerateRandomPointsInCircle(const glm::vec2& center, const float radius, glm::vec2& result);
   bool IsPointInCircle(const glm::vec2& p);
   bool IsPointFarEnough(const glm::vec2& genPoint, const glm::vec2& p, const float distance);
   int FoliageInstancesOfTypeInBrushRadius(const std::string& foliageType);

   float RandomNumberGenerator(const float min, const float max);

   void LoadNoiseImage(const std::string& path);

public: // params
    std::unordered_map<std::string, FoliageTemplate> m_foliageTypes;
    std::string m_selectedFoliage = "";
    bool m_erase = false;
    int noiseMaskSize = 32; // in tiles
    int noiseImageWidth = -1; // in pixels
    int noiseImageHeight = -1; // in pixels
    int noiseImageChannels = -1;

private:  // params
    FoliageTemplate m_defaultFoliageTemplate;
    glm::vec3 m_intersectionPoint = glm::vec3(0.0f);

    unsigned char* m_noiseImage;
};

}  // namespace lvle