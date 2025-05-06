#include "actors/actor_wrapper.hpp"

#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>
#include "tools/serialize_glm.h"

void bee::actors::CreateActorSystems()
{
    Engine.ECS().CreateSystem<UnitManager>();
    Engine.ECS().CreateSystem<StructureManager>();
    Engine.ECS().CreateSystem<PropManager>();
}

void bee::actors::LoadActorsData(const std::string& fileName)
{
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();
    for (auto entity : view)
    {
        bee::Engine.ECS().DeleteEntity(entity);
    }
    Engine.ECS().GetSystem<UnitManager>().LoadUnitData(fileName);
    Engine.ECS().GetSystem<StructureManager>().LoadStructureData(fileName);
    Engine.ECS().GetSystem<PropManager>().LoadPropData(fileName);
}

void bee::actors::LoadActorsTemplates(const std::string& fileName)
{
    Engine.ECS().GetSystem<UnitManager>().LoadUnitTemplates(fileName);
    Engine.ECS().GetSystem<StructureManager>().LoadStructureTemplates(fileName);
    Engine.ECS().GetSystem<PropManager>().LoadPropTemplates(fileName);
}

void bee::actors::SaveActorsData(const std::string& fileName)
{
    auto view = bee::Engine.ECS().Registry.view<AttributesComponent, bee::Transform>();

    std::ofstream os1(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Terrain, fileName + "_attributes.json"));
    cereal::JSONOutputArchive archive1(os1);

    std::vector<std::pair<AttributesComponent, bee::Transform>> attributes;

    for (auto& entity : view)
    {
        auto [actorAttributes, transform] = view.get(entity);
        attributes.push_back(std::pair<AttributesComponent, bee::Transform>(actorAttributes, transform));
    }
    archive1(CEREAL_NVP(attributes));
}

void bee::actors::SaveActorsTemplates(const std::string& fileName)
{
    Engine.ECS().GetSystem<UnitManager>().SaveUnitTemplates(fileName);
    Engine.ECS().GetSystem<StructureManager>().SaveStructureTemplates(fileName);
    Engine.ECS().GetSystem<PropManager>().SavePropTemplates(fileName);
}

void bee::actors::DeleteActorEntities()
{
    auto unit_view = Engine.ECS().Registry.view<AttributesComponent>();
    for (auto entity : unit_view)
    {
        auto [attributes] = unit_view.get(entity);
        Engine.ECS().DeleteEntity(entity);
    }
    auto structure_view = Engine.ECS().Registry.view<AttributesComponent>();
    for (auto entity : structure_view)
    {
        auto [attributes] = structure_view.get(entity);
        Engine.ECS().DeleteEntity(entity);
    }
    auto prop_view = Engine.ECS().Registry.view<AttributesComponent>();
    for (auto entity : prop_view)
    {
        auto [attributes] = prop_view.get(entity);
        Engine.ECS().DeleteEntity(entity);
    }
}

void bee::actors::CalculateSnappedObjectsSmallGridPointIndex() {}

bool bee::actors::IsUnit(const std::string& handle)
{
    auto unitManager = bee::Engine.ECS().GetSystem<UnitManager>();
    auto& unitTemplates = unitManager.GetUnits();
    auto unit = unitTemplates.find(handle);
    return unit != unitTemplates.end();
}

bool bee::actors::IsStructure(const std::string& handle)
{
    auto structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
    auto& structureTemplates = structureManager.GetStructures();
    auto structure = structureTemplates.find(handle);
    return structure != structureTemplates.end();
}

bool bee::actors::IsProp(const std::string& handle)
{
    auto propManager = bee::Engine.ECS().GetSystem<PropManager>();
    auto& propTemplates = propManager.GetProps();
    auto prop = propTemplates.find(handle);
    return prop != propTemplates.end();
}
