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
#include "resource_type.hpp"

class PropTemplate
{
public:
    PropTemplate();
    void InitializeAttributes();
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
    std::string modelPath = "models/BoxAndCylinder.gltf";
    std::shared_ptr<bee::Model> model = nullptr;
    std::vector<std::string> materialPaths{} /* = {"materials/pop.pepimat"}*/;
    std::vector<std::shared_ptr<bee::Material>> materials{} /* = {nullptr}*/;
    GameResourceType resourceType = GameResourceType::None;

    // temporary variable used for "Buildings" and "Props", will be split into its separate thing next sprint
    glm::vec2 tileDimensions{1.0f};

    // Most props are small enough and their diskCollider might be enough.
    bool createCollider = false;

    // transforms the model relative to the entity's transform.
    bee::Transform modelOffset;

private:
    void AddAttribute(BaseAttributes attribute, double value);

private:
    std::unordered_map<BaseAttributes, double> m_baseAttributes{};

    friend class cereal::access;

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(m_baseAttributes), CEREAL_NVP(modelPath), CEREAL_NVP(materialPaths), CEREAL_NVP(tileDimensions),
                CEREAL_NVP(createCollider), CEREAL_NVP(modelOffset), CEREAL_NVP(resourceType));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(name), CEREAL_NVP(m_baseAttributes), CEREAL_NVP(modelPath), CEREAL_NVP(materialPaths), CEREAL_NVP(tileDimensions),
                CEREAL_NVP(createCollider), CEREAL_NVP(modelOffset), CEREAL_NVP(resourceType));
    }
};