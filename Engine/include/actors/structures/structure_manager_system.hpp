#pragma once
#include <core/fwd.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "actors/actor_utils.hpp"
#include "actors/structures/structure_template.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "glm/glm.hpp"

struct EnemyStructure
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};
struct AllyStructure
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};
struct NeutralStructure
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};

// Used for the transparent walls when dragging cursor
struct DragStructure
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};


// Assign this to the entity which is a parent to the model entities.
struct StructureModelTag
{
    std::string structureTemplate = "";
};

struct SpawningStructure
{
    glm::vec3 rallyPoint{};
    int spawnLimit = 6;
    bee::Entity flagEntity = entt::null;
};

struct BuffStructure
{
    std::vector<bee::Entity> buffedEntities{};
    BaseAttributes buffType;
    StatModifier buffModifier;
};

struct HurtBuildingVFX
{
    ~HurtBuildingVFX()
    {
        for (const auto particleEntity : vfxEntities)
        {
            if (!bee::Engine.ECS().Registry.valid(particleEntity)) continue;
            bee::Engine.ECS().DeleteEntity(particleEntity);
        }   
    }
    std::vector<bee::Entity> vfxEntities{};
};

class StructureManager : public bee::System
{
public:
    StructureManager() = default;

    const std::unordered_map<std::string, StructureTemplate>& GetStructures() const { return m_Structures; }

    void AddNewStructureTemplate(StructureTemplate structureTemplate);
    void RemoveStructureTemplate(const std::string& structureHandle);
    StructureTemplate& GetStructureTemplate(const std::string& structureHandle);
    std::pair<std::string, StructureTemplate> FindTemplateOfLevel(const double level, const StructureTypes type);

    void Update(float dt) override;

    // Functions that save and load the data of the untis that are already on the terrain.
    void SaveStructureData();
    void LoadStructureData(const std::string& fileName);

    // Functions that save and load the different types of structures (templates) that can be loaded in the level.
    void SaveStructureTemplates(const std::string&  fileName);
    void LoadStructureTemplates(const std::string& fileName);

    void ReloadStructuresFromTemplate(const std::string& structureHandle);

    //std::optional<bee::Entity> SpawnStructure(const std::string& structureTemplateHandle, const glm::vec3& position, const glm::quat& rotation, Team team = Team::Ally);
    std::optional<bee::Entity> SpawnStructure(const std::string& structureTemplateHandle, const glm::vec3& position, const int smallGridIndex, const bool flipped, Team team = Team::Ally);

#ifdef BEE_INSPECTOR
    void Inspect(bee::Entity e) override;
#endif

    void RemoveStructuresOfTemplate(std::string structureHandle);
    void RemoveStructure(bee::Entity structure);

private:
    StructureTemplate defaultStructureTemplate;
    std::unordered_map<std::string, StructureTemplate> m_Structures;
};