#pragma once
#include <string>
#include <unordered_map>
#include "cereal/cereal.hpp"
#include "cereal/types/unordered_map.hpp"

namespace bee
{

struct DebugMetricData
{
public:
    void Clear();
    void SaveData();
    float timeInsideGame = 0.0f;
    int diedOnWave = 0;
    int woodDespawned = 0;
    int stoneDespawned = 0;
    std::unordered_map<std::string, int> buildingSpawned = {};
    std::unordered_map<std::string, int> upgradedBuildings = {};
    std::unordered_map<std::string, int> allyUnitsSpawned = {};
    std::unordered_map<std::string, int> allyUnitsKilled = {};
    std::unordered_map<std::string, int> enemyUnitsSpawned = {};
    std::unordered_map<std::string, int> enemyUnitsKilled = {};
    std::unordered_map<std::string, int> resourcesSpentOn = {};
private:
    std::string savePath = "DebugMetricData";
    template <class Archive>
    void serialize(Archive& archive)
    {
        CEREAL_NVP(timeInsideGame), CEREAL_NVP(diedOnWave), CEREAL_NVP(woodDespawned), CEREAL_NVP(stoneDespawned),
            CEREAL_NVP(buildingSpawned), CEREAL_NVP(upgradedBuildings), CEREAL_NVP(allyUnitsSpawned),
            CEREAL_NVP(allyUnitsKilled),
            CEREAL_NVP(enemyUnitsSpawned), CEREAL_NVP(enemyUnitsKilled), CEREAL_NVP(resourcesSpentOn);
    }
};
}  // namespace bee