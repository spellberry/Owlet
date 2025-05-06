#pragma once
#include <core/fwd.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "actors/actor_utils.hpp"
#include "actors/units/unit_template.hpp"
#include "core/ecs.hpp"
#include "glm/glm.hpp"

struct EnemyUnit
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};
struct AllyUnit
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};
struct NeutralUnit
{
    char dummy = 'd';
};

// Assign this to the entity which is a parent to the model entities.
struct UnitModelTag
{
    std::string unitType = "";
};

// Assign this to units that have something in their hands
struct UnitPropTagR
{
    // Orientation left/right is from front of owl.
    bee::Entity propRight = entt::null;
    int jointRightArm = -1;  // Determines which bone they need to access
};

struct UnitPropTagL
{
    // Orientation left/right is from front of owl.
    bee::Entity propLeft = entt::null;
    int jointLeftArm = -1;  // Determines which bone they need to access
};

class UnitManager : public bee::System
{
public:
    UnitManager() = default;

    const std::unordered_map<std::string, UnitTemplate> GetUnits() const { return m_Units; }

    void AddNewUnitTemplate(UnitTemplate unitTemplate);
    void RemoveUnitTemplate(const std::string& unitHandle);
    UnitTemplate& GetUnitTemplate(const std::string& unitHandle);

    void Update(float dt) override;

    // Functions that save and load the data of the untis that are already on the terrain.
    void SaveUnitData();
    void LoadUnitData(const std::string& fileName);

    // Functions that save and load the different types of units (templates) that can be loaded in the level.
    void SaveUnitTemplates(const std::string& fileName);
    void LoadUnitTemplates(const std::string& fileName);

    void ReloadUnitsFromTemplate(const std::string& unitHandle);

    std::optional<bee::Entity> SpawnUnit(const std::string& unitTemplateHandle, const glm::vec3& position,
                                         Team team = Team::Ally);
    void InitAnimators(const std::string& unitTemplateHandle);
    std::vector<bee::Entity> SpawnUnits(std::vector<std::string> unitTemplateHandles, std::vector<glm::vec3> positions,
                                        std::vector<Team> teams);

#ifdef BEE_INSPECTOR
    void Inspect(bee::Entity e) override;
#endif

    void RemoveUnitsOfTemplate(const std::string& unitHandle);
    void RemoveUnit(bee::Entity unit);

private:

    /// <summary>
    /// Called inside the spawn unit function. Creates and attaches 
    /// </summary>
    void AttachProp(bee::Entity parentEntity, const bee::Transform& parentTransform);
    void ConfigureMage(bee::Entity parentEntity, const bee::Transform& parentTransform);
    void ConfigureWarrior(bee::Entity parentEntity, const bee::Transform& parentTransform);
    void InstantiateProp(bee::Entity parentEntity, const bee::Model& model, const std::string& fileName);

    /// <summary>
    /// Updates prop transform so that it follows the correct bone of the correct model
    /// </summary>
    void UpdateProps();

    UnitTemplate defaultUnitTemplate;
    std::unordered_map<std::string, UnitTemplate> m_Units;
};