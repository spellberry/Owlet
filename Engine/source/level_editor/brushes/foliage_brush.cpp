#include "level_editor/brushes/foliage_brush.hpp"

#include <tinygltf/stb_image.h>
#include "rendering/model.hpp"
#include "core/input.hpp"
#include "tools/tools.hpp"
#include "math/math.hpp"
#include <random>
#include <cmath>
#include <cereal/types/utility.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/unordered_map.hpp>

using namespace lvle;

// credit to stack overflow user:
// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void lvle::FoliageBrush::Update(glm::vec3& intersectionPoint, const std::string objectHandle)
{
    Brush::Update(intersectionPoint);
    m_intersectionPoint = intersectionPoint;
}

void lvle::FoliageBrush::Terraform(float deltaTime)
{
    if (!m_erase)
    {
        Apply();
    }
    else
    {
        Erase();
    }
}

void lvle::FoliageBrush::Apply()
{
    for (auto& foliage : m_foliageTypes)
    {
        auto& foliageTemplate = foliage.second;
        if (!foliageTemplate.active) continue;

        // 0) Calculate number of points to spawn
        int amountToSpawn = foliageTemplate.density - FoliageInstancesOfTypeInBrushRadius(foliageTemplate.name);
        if (amountToSpawn < 0) amountToSpawn = 0;

        // 1) Generate points, based on density.
        std::vector<glm::vec2> generatedPoints;
        for (int i = 0; i < amountToSpawn; i++)
        {
            glm::vec2 result;
            if (GenerateRandomPointsInCircle(m_intersectionPoint, m_radius, result))
            {
                // Remove points which are too close to other points that were generated now
                if (generatedPoints.empty()) generatedPoints.push_back(result);
                bool greenLight = true;
                for (const auto& genPoint : generatedPoints)
                {
                    if (!IsPointFarEnough(genPoint, result, foliageTemplate.distanceRadius))
                    {
                        greenLight = false;
                        break;
                    }
                }
                if (greenLight) generatedPoints.push_back(result);
            }
        }

        // 2) Remove points which are too close to existing foliage instances
        std::vector<glm::vec2> distributedPoints;
        auto view = bee::Engine.ECS().Registry.view<bee::Transform, FoliageComponent>();
        for (const auto& genPoint : generatedPoints)
        {
            if (view.size_hint() == 0)
                distributedPoints.push_back(genPoint);
            else
            {
                bool greenLight = true;
                for (auto [entity, transform, foliageComponent] : view.each())
                {
                    if (foliageComponent.name != foliage.first) continue;
                    if (!IsPointFarEnough(glm::vec2(transform.Translation.x, transform.Translation.y), genPoint,
                                          foliageTemplate.distanceRadius))
                    {
                        greenLight = false;
                        break;
                    }
                }
                if (greenLight) distributedPoints.push_back(genPoint);
            }
        }

        // 3) Get a color for the current brush stroke.
        if (bee::Engine.Input().GetMouseButtonOnce(bee::Input::MouseButton::Left))
        {
            if (!foliageTemplate.colorVariations.empty())
            {
                int colorIndex = bee::GetRandomNumberInt(0, foliageTemplate.colorVariations.size() - 1);
                foliageTemplate.currentBrushstrokeColor = foliageTemplate.colorVariations[colorIndex];
            }
        }

        // 4) Instance foliage entities
        for (const auto& dPoint : distributedPoints)
        {
            float height = 0.0f;
            bool hit = GetTerrainHeightAtPoint(dPoint.x, dPoint.y, height);
            if (hit)
            {
                bee::Transform testTransform;
                testTransform.Translation = glm::vec3(dPoint.x, dPoint.y, height);

                auto zOffset = RandomNumberGenerator(foliageTemplate.zOffsetMin, foliageTemplate.zOffsetMax);
                testTransform.Translation.z += zOffset;

                auto scaleOffset = RandomNumberGenerator(foliageTemplate.scaleRangeMin, foliageTemplate.scaleRangeMax);
                testTransform.Scale *= scaleOffset;

                glm::vec3 rotationOffset = glm::vec3(0.0f);
                rotationOffset.x = RandomNumberGenerator(-foliageTemplate.rotationRange.x, foliageTemplate.rotationRange.x);
                rotationOffset.y = RandomNumberGenerator(-foliageTemplate.rotationRange.y, foliageTemplate.rotationRange.y);
                rotationOffset.z = RandomNumberGenerator(-foliageTemplate.rotationRange.z, foliageTemplate.rotationRange.z);
                auto euler = glm::eulerAngles(testTransform.Rotation);
                euler += rotationOffset;
                testTransform.Rotation = glm::quat(euler);

                PlaceFoliageInstance(testTransform, foliageTemplate, foliageTemplate.currentBrushstrokeColor);
            }
        }
    }
}

void lvle::FoliageBrush::Erase()
{ 
    for (auto& foliage : m_foliageTypes)
    {
        auto& foliageTemplate = foliage.second;
        if (!foliageTemplate.active) continue;

        float radiusSqrd = static_cast<float>(m_radius * m_radius);
        auto view = bee::Engine.ECS().Registry.view<bee::Transform, FoliageComponent>();
        for (auto [entity, transform, foliageComponent] : view.each())
        {
            if (foliageComponent.name == foliage.first &&
                glm::distance2(glm::vec2(transform.Translation.x, transform.Translation.y),
                               glm::vec2(m_intersectionPoint.x, m_intersectionPoint.y)) < radiusSqrd)
            {
                bee::Engine.ECS().DeleteEntity(entity);
            }
        }
    }
}

void lvle::FoliageBrush::AddFoliageType(FoliageTemplate foliageTemplate)
{
    if (m_foliageTypes.find(foliageTemplate.name) != m_foliageTypes.end())
    {
        bee::Log::Warn("Prop Name " + foliageTemplate.name + " is already used. Try another name.");
        return;
    }
    foliageTemplate.model = bee::Engine.Resources().Load<bee::Model>(foliageTemplate.modelPath);

    if (foliageTemplate.materialPaths.empty() && foliageTemplate.materials.empty())
    {
        foliageTemplate.materialPaths.resize(foliageTemplate.model->GetMeshes().size());
        foliageTemplate.materials.resize(foliageTemplate.model->GetMeshes().size());

        for(int i=0; i<foliageTemplate.materialPaths.size();i++)
        {
            foliageTemplate.materialPaths[i] = "materials/Empty.pepimat";
            foliageTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(foliageTemplate.materialPaths[i]);
        }
    }
    else
    {
        foliageTemplate.materials.resize(foliageTemplate.model->GetMeshes().size());

        for (int i = 0; i < foliageTemplate.materialPaths.size(); i++)
        {
            foliageTemplate.materials[i] = bee::Engine.Resources().Load<bee::Material>(foliageTemplate.materialPaths[i]);
        }
    }
        
    //foliageTemplate.material = bee::Engine.Resources().Load<bee::Material>(foliageTemplate.materialPath);
    m_foliageTypes.insert(std::pair<std::string, FoliageTemplate>(foliageTemplate.name, foliageTemplate));
}

void lvle::FoliageBrush::RemoveFoliageType(const std::string& foliageHandle)
{
    if (m_foliageTypes.find(foliageHandle) == m_foliageTypes.end())
    {
        bee::Log::Warn("There is no unit type with the name " + foliageHandle + ". Try another name.");
        return;
    }
    m_foliageTypes.erase(foliageHandle);
}

void lvle::FoliageBrush::RemoveAllFoliageFromType(const std::string& foliageHandle)
{
    auto view = bee::Engine.ECS().Registry.view<FoliageComponent>();
    for (auto foliage : view)
    {
        auto [foliageComponent] = view.get(foliage);
        if (foliageComponent.name == foliageHandle)
        {
            bee::Engine.ECS().DeleteEntity(foliage);
        }
    }
}

FoliageTemplate& lvle::FoliageBrush::GetFoliageTemplate(const std::string& foliageHandle)
{
    auto foliage = m_foliageTypes.find(foliageHandle);
    if (foliage == m_foliageTypes.end())
    {
        //bee::Log::Warn("There is no foliage type with the name " + foliageHandle + ". Try another name.");
        // return {};
        return m_defaultFoliageTemplate;
    }
    return foliage->second;
}

void lvle::FoliageBrush::SaveFoliageTemplates(const std::string& filename)
{
    std::unordered_map<std::string, FoliageTemplate> templateData = m_foliageTypes;
    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, filename + "_FoliageTemplates.json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(templateData));
}

void lvle::FoliageBrush::SaveFoliageInstances(const std::string& filename)
{
    auto view = bee::Engine.ECS().Registry.view<FoliageComponent, bee::Transform>();

    std::ofstream os1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, filename + "_FoliageInstances.json"));
    cereal::JSONOutputArchive archive1(os1);

    std::vector<std::pair<bee::Transform, glm::vec3>> foliageInstances;

    for (auto& entity : view)
    {
        auto [foliageComponent, transform] = view.get(entity);
        bee::Transform newTransform;
        newTransform = transform;
        newTransform.Name = foliageComponent.name;
        glm::vec3 color = glm::vec3(1.0f);
        auto meshRendererView = bee::Engine.ECS().Registry.view<bee::MeshRenderer, bee::Transform>();
        for (auto [childEntity, meshRenderer, childTransform] : meshRendererView.each())
        {
            if (childTransform.GetParent() == entity)
                color = glm::vec3(meshRenderer.constant_data.red, meshRenderer.constant_data.green,
                                  meshRenderer.constant_data.blue);
        }
        std::pair<bee::Transform, glm::vec3> item = {newTransform, color};
        foliageInstances.push_back(item);
    }
    archive1(CEREAL_NVP(foliageInstances));
}

void lvle::FoliageBrush::LoadFoliageTemplates(const std::string& filename)
{
    m_foliageTypes.clear();
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, filename + "_FoliageTemplates.json")))
    {
        std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, filename + "_FoliageTemplates.json"));
        cereal::JSONInputArchive archive(is);

        std::unordered_map<std::string, FoliageTemplate> templateData;
        archive(CEREAL_NVP(templateData));
        for (auto it = templateData.begin(); it != templateData.end(); ++it)
        {
            AddFoliageType(it->second);

        }
    }
}

void lvle::FoliageBrush::LoadFoliageInstances(const std::string& filename)
{
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, filename + "_FoliageInstances.json")))
    {
        std::ifstream is1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, filename + "_FoliageInstances.json"));
        cereal::JSONInputArchive archive1(is1);

        std::vector<std::pair<bee::Transform, glm::vec3>> foliageInstances;

        archive1(CEREAL_NVP(foliageInstances));
        for (int i = 0; i < foliageInstances.size(); i++)
        {
            if (m_foliageTypes.find(foliageInstances[i].first.Name) == m_foliageTypes.end()) continue;
            PlaceFoliageInstance(foliageInstances[i].first, GetFoliageTemplate(foliageInstances[i].first.Name), foliageInstances[i].second, true);
        }
    }
}

void lvle::FoliageBrush::PlaceFoliageInstance(const bee::Transform& randomizedTransform, const FoliageTemplate& foliageTemplate,
                                              const glm::vec3& color, const bool fromFile)
{
    auto cMultiplier = SampleNoiseMask(randomizedTransform.Translation);
    // uncomment this if you want to see the noise more clearly
    /*if (cMultiplier.x < 0.35f)
        cMultiplier = glm::vec3(0.0f);
    else
        cMultiplier = glm::vec3(1.0f);*/

    auto entity = bee::Engine.ECS().CreateEntity();
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(entity);
    transform = randomizedTransform;
    transform.Name = "Foliage" + foliageTemplate.name;
    transform.Translation += foliageTemplate.modelOffset.Translation;
    auto modelEuler = glm::eulerAngles(transform.Rotation);
    auto modelOffsetEuler = glm::eulerAngles(foliageTemplate.modelOffset.Rotation);
    modelEuler += modelOffsetEuler;
    transform.Rotation = glm::quat(modelEuler);
    transform.Scale = foliageTemplate.modelOffset.Scale * randomizedTransform.Scale;
    auto& foliageTag = bee::Engine.ECS().CreateComponent<FoliageComponent>(entity);
    foliageTag.name = foliageTemplate.name;
    auto model = bee::Engine.Resources().Load<bee::Model>(foliageTemplate.modelPath);
    bee::ConstantBufferData constantData;
    if (!fromFile)
    {
        constantData.red = std::clamp(color.r + (1.0f - foliageTemplate.colorOverlayIntensity), 0.0f, 1.0f) * cMultiplier.r;
        constantData.green = std::clamp(color.g + (1.0f - foliageTemplate.colorOverlayIntensity), 0.0f, 1.0f) * cMultiplier.g;
        constantData.blue = std::clamp(color.b + (1.0f - foliageTemplate.colorOverlayIntensity), 0.0f, 1.0f) * cMultiplier.b;
        constantData.paddin1 = foliageTemplate.windMultiplier;
    }
    else
    {
        constantData.red = color.r;
        constantData.green = color.g;
        constantData.blue = color.b;
        constantData.paddin1 = foliageTemplate.windMultiplier;
    }
    //constantData.red = color.r * std::clamp(cMultiplier.r + (1.0f - foliageTemplate.colorOverlayIntensity), 0.0f, 1.0f);
    //constantData.green = color.g * std::clamp(cMultiplier.g + (1.0f - foliageTemplate.colorOverlayIntensity), 0.0f, 1.0f);
    //constantData.blue = color.b * std::clamp(cMultiplier.b + (1.0f - foliageTemplate.colorOverlayIntensity), 0.0f, 1.0f);
    model->Instantiate(entity, constantData);

    int index = 0;
    bee::UpdateMeshRenderer(entity, foliageTemplate.materials, index);
    /*auto& t = bee::Engine.ECS().Registry.get<bee::Transform>(entity);
    for (auto child = t.begin(); child != t.end(); ++child) {
        if(bee::Engine.ECS().Registry.valid(*child) &&  bee::Engine.ECS().Registry.all_of<bee::MeshRenderer>(*child))
        {
            auto& meshRenderer = bee::Engine.ECS().Registry.get<bee::MeshRenderer>(*child);
            meshRenderer.Material = foliageTemplate.material;
        }
    }*/
}

bool lvle::FoliageBrush::GenerateRandomPointsInCircle(const glm::vec2& center, const float radius, glm::vec2& result)
{
    // Random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);

    float x = dis(gen);
    float y = dis(gen);

    if (!IsPointInCircle(glm::vec2(x, y))) return false;

    // Scale the point to the desired radius
    x *= radius;
    y *= radius;
    // Translate the point to the desired position
    x += center.x;
    y += center.y;
    result = glm::vec2(x, y);

    return true;
}

bool lvle::FoliageBrush::IsPointInCircle(const glm::vec2& p) { return (p.x * p.x + p.y * p.y <= 1.0); }

bool lvle::FoliageBrush::IsPointFarEnough(const glm::vec2& genPoint, const glm::vec2& p, const float distance)
{
    float distanceSqrd = distance * distance;
    return (glm::distance2(genPoint, p) > distanceSqrd);
}

int lvle::FoliageBrush::FoliageInstancesOfTypeInBrushRadius(const std::string& foliageType)
{
    int cnt = 0;
    float radiusSqrd = static_cast<float>(m_radius * m_radius);
    auto view = bee::Engine.ECS().Registry.view<bee::Transform, FoliageComponent>();
    for (auto [entity, transform, foliageComponent] : view.each())
    {
        if (foliageComponent.name == foliageType && glm::distance2(glm::vec2(transform.Translation.x, transform.Translation.y),
            glm::vec2(m_intersectionPoint.x, m_intersectionPoint.y)) < radiusSqrd)
        {
            cnt++;
        }
    }
    return cnt;
}

glm::vec3 lvle::FoliageBrush::SampleNoiseMask(const glm::vec3& foliagePosition)
{
    auto terrainSystem = bee::Engine.ECS().GetSystem<TerrainSystem>();
    auto& terrainData = terrainSystem.m_data;

    glm::vec3 colorAtPosition = glm::vec3(1.0f);

    glm::vec3 translatedFoliagePosition = foliagePosition; // map from (0; 0) to (m_width; m_height)
    float terrainWidth = terrainData->m_width * terrainData->m_step;
    float terrainHeight = terrainData->m_height * terrainData->m_step;

    // 1) Translate point to [(0; 0) ; (terrainSize.x; terrainSize.y)]
    translatedFoliagePosition.x += terrainWidth / 2.0f;
    translatedFoliagePosition.y += terrainHeight / 2.0f;

    // 2) Translate point to [(0; 0) ; (terrainSize.x / noiseMaskSize; terrainSize.y / noiseMaskSize)]
    float upperRangeBorderX = terrainWidth / static_cast<float>(noiseMaskSize);
    float upperRangeBorderY = terrainHeight / static_cast<float>(noiseMaskSize);
    glm::vec2 temp;
    temp.x = bee::Remap(0.0f, terrainWidth, 0.0f, upperRangeBorderX, translatedFoliagePosition.x);
    temp.y = bee::Remap(0.0f, terrainHeight, 0.0f, upperRangeBorderY, translatedFoliagePosition.y);
    translatedFoliagePosition.x = temp.x;
    translatedFoliagePosition.y = temp.y;

    // 3) Take only the decimal part of the translated foliagePosition
    translatedFoliagePosition.x = translatedFoliagePosition.x - static_cast<float>(static_cast<int>(translatedFoliagePosition.x));
    translatedFoliagePosition.y = translatedFoliagePosition.y - static_cast<float>(static_cast<int>(translatedFoliagePosition.y));

    glm::vec2 noiseMaskCoords;
    noiseMaskCoords.x = static_cast<int>(translatedFoliagePosition.x * noiseImageWidth + 1); // add one so we get mathematical rounding
    noiseMaskCoords.y = static_cast<int>(translatedFoliagePosition.y * noiseImageHeight + 1);  // add one so we get mathematical rounding

    int index = (noiseMaskCoords.y * noiseImageWidth + noiseMaskCoords.x) * noiseImageChannels;
    if (index < 0 || index + 2 >= noiseImageWidth * noiseImageHeight * noiseImageChannels)
    {
        bee::Log::Warn("Index {} out of bounds", index);
        index = 0;
    }

    colorAtPosition = glm::vec3(m_noiseImage[index] / 255.0f, m_noiseImage[index + 1] / 255.0f, m_noiseImage[index + 2] / 255.0f);
   /* bee::Log::Info("Color at Pixel [{}, {}]: {}, {}, {}", noiseMaskCoords.x, noiseMaskCoords.y, colorAtPosition.r,
                   colorAtPosition.g,
                   colorAtPosition.b);
                   */
    //bee::Log::Info("Color at Pixel [{}, {}]: {}, {}, {}", noiseMaskCoords.x, noiseMaskCoords.y, colorAtPosition.r, colorAtPosition.g, colorAtPosition.b);

    return colorAtPosition;
}

float lvle::FoliageBrush::RandomNumberGenerator(const float min, const float max)
{
    // Random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);

    return dis(gen);
}

void lvle::FoliageBrush::LoadNoiseImage(const std::string& path)
{
    // Load the image
    m_noiseImage = stbi_load(path.c_str(), &noiseImageWidth, &noiseImageHeight, &noiseImageChannels, 0);

    if (m_noiseImage == NULL)
    {
        bee::Log::Warn("Image could not be loaded. Path: {}", path);
        return;
    }
}
