#pragma once

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "actors/attributes.hpp"
#include "ai/ai_behavior_selection_system.hpp"
#include "glm/glm.hpp"
#include "physics/physics_components.hpp"
#include "rendering/model.hpp"
#include "unit_order_type.hpp"

enum class UnitTemplatePresets
{
    None,
    AllyRangedUnit,
    AllyMeleeUnit,
    EnemyRangedUnit,
    EnemyMeleeUnit
};

enum class UnitSoundTypes
{
    None,
    Death,
    Attack,
    GotHit,
};

class UnitTemplate
{
public:
    UnitTemplate(UnitTemplatePresets preset = UnitTemplatePresets::None);
    void InitializeAttributes(UnitTemplatePresets preset = UnitTemplatePresets::None);
    bool HasAttribute(BaseAttributes type) { return m_baseAttributes.find(type) != m_baseAttributes.end(); }
    double GetAttribute(BaseAttributes type) { return m_baseAttributes.find(type)->second; }
    std::unordered_map<BaseAttributes, double> GetAttributes() { return m_baseAttributes; }

    void SetAttribute(BaseAttributes type, double value, bool checkIfExists = true)
    {
        if (checkIfExists)
        {
            if (m_baseAttributes.find(type) == m_baseAttributes.end()) return;
            m_baseAttributes.find(type)->second = value;
        }
        else
        {
            m_baseAttributes[type] = value;
        }
    }

    void RemoveAttribute(BaseAttributes attribute) { m_baseAttributes.erase(attribute); }

    std::string name = "";
    std::string fsmPath = "default_fsm.json";
    std::string animationControllerPath = "";
    std::string modelPath = "models/BoxAndCylinder.gltf";
    std::string corpsePath = "models/Skeleton.gltf";
    std::vector<std::string> materialPaths{} /* = {"materials/pop.pepimat"}*/;

    std::string iconPath = "textures/checkerboard.png";
    glm::vec4 iconTextureCoordinates = glm::vec4(0, 0, 1, 1);

    std::shared_ptr<bee::Model> model = nullptr;
    std::shared_ptr<bee::Model> corpseModel = nullptr;
    std::vector<std::shared_ptr<bee::Material>> materials{} /* = {nullptr}*/;
    // double scale = 1.0f;
    std::vector<OrderType> availableOrders{};
    std::unordered_map<UnitSoundTypes, std::string> unitSounds{};

    // temporary variable used for "Buildings" and "Props", will be split into its separate thing next sprint
    glm::vec2 tileDimensions{1.0f};

    // transforms the model relative to the entity's transform.
    bee::Transform modelOffset = bee::Transform(glm::vec3(0.0f), glm::vec3(0.0f), glm::identity<glm::quat>());

private:
    void AddAttribute(BaseAttributes attribute, double value);
    void NoPreset();
    void AllyRangedUnit();
    void AllyMeleeUnit();
    void EnemyRangedUnit();
    void EnemyMeleeUnit();

private:
    std::unordered_map<BaseAttributes, double> m_baseAttributes{};

    friend class cereal::access;

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(m_baseAttributes), CEREAL_NVP(fsmPath), CEREAL_NVP(modelPath),
                CEREAL_NVP(corpsePath), CEREAL_NVP(tileDimensions), CEREAL_NVP(modelOffset), CEREAL_NVP(availableOrders),
                CEREAL_NVP(iconPath), CEREAL_NVP(iconTextureCoordinates), CEREAL_NVP(animationControllerPath),
                CEREAL_NVP(materialPaths), CEREAL_NVP(unitSounds));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(m_baseAttributes), CEREAL_NVP(fsmPath), CEREAL_NVP(modelPath),
                CEREAL_NVP(corpsePath), CEREAL_NVP(tileDimensions), CEREAL_NVP(modelOffset), CEREAL_NVP(availableOrders),
                CEREAL_NVP(iconPath), CEREAL_NVP(iconTextureCoordinates), CEREAL_NVP(animationControllerPath),
                CEREAL_NVP(materialPaths)/*, CEREAL_NVP(unitSounds)*/);
    }
};