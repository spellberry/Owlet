#pragma once
#include "props/prop_manager_system.hpp"
#include "structures/structure_manager_system.hpp"
#include "units/unit_manager_system.hpp"

// This class is provides some utility functions for when you need all actors at the same time.
namespace bee::actors
{
void CreateActorSystems();

void LoadActorsData(const std::string& fileName);

void LoadActorsTemplates(const std::string& fileName);

void SaveActorsData(const std::string& fileName);

void SaveActorsTemplates(const std::string& fileName);

void DeleteActorEntities();

void CalculateSnappedObjectsSmallGridPointIndex();

bool IsUnit(const std::string& handle);
bool IsStructure(const std::string& handle);
bool IsProp(const std::string& handle);

}  // namespace bee::actors