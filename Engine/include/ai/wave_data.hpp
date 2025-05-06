#pragma once
#include <string>
#include <vector>
#include <visit_struct/visit_struct.hpp>
#include "tools/serializable.hpp"


struct WaveData : public bee::Serializable
{
    std::vector<std::string> enemiesToSpawn{};
    std::vector<int> enemyWeights{};
    std::vector<int> numberOfEnemiesPerWave{};

    float minimalDistanceToBase = 5.0f;
    float ringThickness = 2.0f;
    float delayBetweenWaves = 10.0f;

    int GetTotalEnemyWeights() const
    {
        int totalWeight = 0;
        for (const int weight : enemyWeights)
        {
            totalWeight += weight;
        }
        return totalWeight;
    }
};
VISITABLE_STRUCT(WaveData, enemiesToSpawn, enemyWeights, numberOfEnemiesPerWave, minimalDistanceToBase, ringThickness,
                 delayBetweenWaves);