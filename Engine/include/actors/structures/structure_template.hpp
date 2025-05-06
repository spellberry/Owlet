#pragma once

#include <actors/units/unit_order_type.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "actors/actor_utils.hpp"
#include "actors/attributes.hpp"
#include "glm/glm.hpp"
#include "rendering/model.hpp"
#include "core/transform.hpp"

enum class StructureTemplatePresets
{
    None,
    ProductionStructure,
    AttackTower,
    Wall,
    BuffStructure
};


class StructureTemplate
{
public:
    StructureTemplate(StructureTemplatePresets preset = StructureTemplatePresets::None);
    void InitializeAttributes(StructureTemplatePresets preset = StructureTemplatePresets::None);
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
    std::string modelPath = "models/BoxAndCylinder.gltf";
    std::string corpsePath = "models/Skeleton.gltf";
    std::vector<std::string> materialPaths{} /* = {"materials/pop.pepimat"}*/;
    std::string buildingUpgradeHandle = "";
    StructureTypes structureType = StructureTypes::None;

    std::string iconPath = "textures/checkerboard.png";
    glm::vec4 iconTextureCoordinates = glm::vec4(0, 0, 1, 1);

    std::vector<OrderType> availableOrders{};
    std::shared_ptr<bee::Model> model = nullptr;
    std::shared_ptr<bee::Model> corpseModel = nullptr;
    std::vector<std::shared_ptr<bee::Material>> materials{} /* = {nullptr}*/;

    // temporary variable used for "Buildings" and "Props", will be split into its separate thing next sprint
    glm::vec2 tileDimensions{1.0f};

    // transforms the model relative to the entity's transform.
    bee::Transform modelOffset;
    BaseAttributes buffedAttribute;

private:
    void AddAttribute(BaseAttributes attribute, double value);
    void NoPreset();
    void ProductionStructure();
    void AttackTower();
    void Wall();
    void BuffTower();

private:
    std::unordered_map<BaseAttributes, double> m_baseAttributes{};

    friend class cereal::access;

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(m_baseAttributes), CEREAL_NVP(fsmPath), CEREAL_NVP(modelPath),
                CEREAL_NVP(corpsePath), CEREAL_NVP(materialPaths), CEREAL_NVP(buildingUpgradeHandle), CEREAL_NVP(tileDimensions),
                CEREAL_NVP(modelOffset),
                CEREAL_NVP(availableOrders), CEREAL_NVP(iconPath),
                CEREAL_NVP(iconTextureCoordinates),CEREAL_NVP(buffedAttribute), CEREAL_NVP(structureType));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(m_baseAttributes), CEREAL_NVP(fsmPath), CEREAL_NVP(modelPath),
                CEREAL_NVP(corpsePath), CEREAL_NVP(materialPaths), CEREAL_NVP(buildingUpgradeHandle), CEREAL_NVP(tileDimensions),
                CEREAL_NVP(modelOffset),
                CEREAL_NVP(availableOrders), CEREAL_NVP(iconPath), CEREAL_NVP(iconTextureCoordinates),
                CEREAL_NVP(buffedAttribute), CEREAL_NVP(structureType));
    }
};