#include "tools/debug_metric.hpp"

#include "actors/attributes.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"

#include <fstream>
#include <cereal/archives/json.hpp>

#include "actors/structures/structure_manager_system.hpp"

using namespace bee;

void bee::DebugMetricData::Clear()
{
    timeInsideGame = 0.0f;
    diedOnWave = 0;
    woodDespawned = 0;
    stoneDespawned = 0;
    buildingSpawned.clear();
    upgradedBuildings.clear();
    allyUnitsSpawned.clear();
    allyUnitsKilled.clear();
    enemyUnitsSpawned.clear();
    enemyUnitsKilled.clear();

    auto structureManager = bee::Engine.ECS().GetSystem<StructureManager>();
    for (auto structureTemplate:structureManager.GetStructures())
    {

        if (structureTemplate.first.find("SwordsmenTowerLvl0") != std::string::npos ||
            structureTemplate.first.find("MageTowerLvl0") != std::string::npos ||
            structureTemplate.first.find("TownHallLvl0") != std::string::npos)
            continue;

        if (structureTemplate.first.find("SwordsmenTower") != std::string::npos ||
            structureTemplate.first.find("MageTower") != std::string::npos ||
            structureTemplate.first.find("TownHall") != std::string::npos)
        {
            upgradedBuildings.insert({structureTemplate.first, 0});
        if (structureTemplate.first.find("SwordsmenTowerLvl1") != std::string::npos ||
            structureTemplate.first.find("MageTowerLvl1") != std::string::npos)
                buildingSpawned.insert({structureTemplate.first, 0});
        }else
        {
            buildingSpawned.insert({structureTemplate.first, 0});
        }
    }
}

void bee::DebugMetricData::SaveData()
{
    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, savePath + ".json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(timeInsideGame), CEREAL_NVP(diedOnWave), CEREAL_NVP(woodDespawned), CEREAL_NVP(stoneDespawned),
            CEREAL_NVP(buildingSpawned), CEREAL_NVP(upgradedBuildings), CEREAL_NVP(allyUnitsSpawned),
            CEREAL_NVP(allyUnitsKilled),
            CEREAL_NVP(enemyUnitsSpawned), CEREAL_NVP(enemyUnitsKilled), CEREAL_NVP(resourcesSpentOn));
}
