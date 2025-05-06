#pragma once
#include <core/fwd.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>

#include "actors/actor_utils.hpp"
#include "actors/props/prop_template.hpp"
#include "core/ecs.hpp"
#include "glm/glm.hpp"

struct EnemyProp
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};
struct AllyProp
{
    char dummy = 'd';  // just a variable to make sure that the struct doesn't get optimized
};
struct NeutralProp
{
    char dummy = 'd';
};

// Assign this to the entity which is a parent to the model entities.
struct PropModelTag
{
    std::string propTemplate = "";
};

class PropManager : public bee::System
{
public:
    PropManager() = default;

    const std::unordered_map<std::string, PropTemplate>& GetProps() { return m_Props; }

    void AddNewPropTemplate(PropTemplate propTemplate);
    void RemovePropTemplate(std::string propHandle);
    // Note that this function returns a modifiable reference to the template. Any changes you make to it will be applied.
    PropTemplate& GetPropTemplate(std::string propHandle);

    void Update(float dt) override;

    // Functions that save and load the data of the untis that are already on the terrain.
    void SavePropData();
    void LoadPropData(const std::string& fileName);

    // Functions that save and load the different types of props (templates) that can be loaded in the level.
    void SavePropTemplates(const std::string& fileName);
    void LoadPropTemplates(const std::string& fileName);

    void ReloadPropsFromTemplate(std::string propHandle);

    std::optional<bee::Entity> SpawnProp(std::string propTemplateHandle, glm::vec3 position, const int smallGridIndex, const bool flipped);

#ifdef BEE_INSPECTOR
    void Inspect(bee::Entity e) override;
#endif

    void RemovePropsOfTemplate(std::string propHandle);
    void RemoveGameProp(bee::Entity prop);

private:
    

private:
    PropTemplate defaultPropTemplate;
    std::unordered_map<std::string, PropTemplate> m_Props;
};